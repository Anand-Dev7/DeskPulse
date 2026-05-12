#include "clock.h"
#include <U8g2_for_Adafruit_GFX.h>
#include <Fonts/FreeMonoBold9pt7b.h>
#include <math.h>  // For sun rays (cos, sin)

// U8g2 font wrapper - gives access to 100+ fonts!
U8G2_FOR_ADAFRUIT_GFX u8g2Fonts;

// Available font styles (uncomment the one you want):
// DIGITAL/7-SEGMENT:
//   u8g2_font_7Segments_26x42_mn - Classic LCD clock
//   u8g2_font_logisoso42_tn - Modern digital
//   u8g2_font_inb42_mn - Bold digital
// MODERN/CLEAN:
//   u8g2_font_helvB24_tf - Helvetica Bold
//   u8g2_font_fur42_tn - Futuristic
// PIXEL/RETRO:
//   u8g2_font_profont22_mn - Retro pixel
//   u8g2_font_ncenB24_tf - New Century Bold



DeskPulseClock::DeskPulseClock(Adafruit_ST7789* display) {
    tft = display;
}

void DeskPulseClock::begin() {
    // Initialize U8g2 fonts
    u8g2Fonts.begin(*tft);
    
    tft->fillScreen(COLOR_BG);
    if (clockStyle == CLOCK_STYLE_SIMPLE) {
        simpleClock(true);
    } else {
        smartClock(true);
    }
}

void DeskPulseClock::setTime(int h, int m, int s) {
    hour = h;
    minute = m;
    second = s;
}

void DeskPulseClock::setDate(int d, int m, int y, int wd) {
    day = d;
    month = m;
    year = y;
    weekday = wd;
}

void DeskPulseClock::setWeather(WeatherData data) {
    weather = data;
    weatherUpdated = true;
}

void DeskPulseClock::setClockStyle(ClockStyle style) {
    if (clockStyle != style) {
        clockStyle = style;
        prevHour = -1;
        prevMinute = -1;
        prevSecond = -1;
        prevDay = -1;
        weatherUpdated = true;
    }
}

void DeskPulseClock::toggleClockStyle() {
    setClockStyle(clockStyle == CLOCK_STYLE_WEATHER ? CLOCK_STYLE_SIMPLE : CLOCK_STYLE_WEATHER);
}

DeskPulseClock::ClockStyle DeskPulseClock::getClockStyle() const {
    return clockStyle;
}

const char* DeskPulseClock::getWeekdayName(int wday) {
    const char* days[] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
    return days[wday % 7];
}

void DeskPulseClock::drawBadge(int x, int y, int w, int h, uint16_t bgColor, uint16_t textColor, const char* text, uint8_t textSize) {
    tft->fillRoundRect(x, y, w, h, 8, bgColor);
    // For weather condition badge, use slightly larger bold black text for max contrast
    bool isWeatherLabel = (textSize == 1 && bgColor == COLOR_WHITE);
    uint8_t actualTextSize = isWeatherLabel ? 2 : textSize;
    uint16_t actualTextColor = isWeatherLabel ? COLOR_BLACK : textColor;
    tft->setTextColor(actualTextColor);
    tft->setTextSize(actualTextSize);
    int textWidth = strlen(text) * (actualTextSize == 2 ? 12 : 6);
    int textHeight = (actualTextSize == 2 ? 16 : 8);
    int tx = x + (w - textWidth) / 2;
    int ty = y + (h - textHeight) / 2 + 2;
    // Bold effect for weather label
    if (isWeatherLabel) {
        tft->setCursor(tx, ty);
        tft->print(text);
        tft->setCursor(tx+1, ty);
        tft->print(text);
        tft->setCursor(tx, ty+1);
        tft->print(text);
        tft->setCursor(tx+1, ty+1);
        tft->print(text);
    } else {
        tft->setCursor(tx, ty);
        tft->print(text);
    }
}

void DeskPulseClock::drawHeader() {
    // Set U8g2 font settings
    u8g2Fonts.setFontMode(1); // Transparent
    u8g2Fonts.setFontDirection(0);
    u8g2Fonts.setBackgroundColor(COLOR_BG);

    // ---- CITY NAME (was TextSize 2) ----
    u8g2Fonts.setForegroundColor(COLOR_CYAN);
    u8g2Fonts.setFont(u8g2_font_helvB14_tf); // ~same as size 2

    if (weather.valid) {
        u8g2Fonts.setCursor(10, 20); // Y adjusted for baseline
        u8g2Fonts.print(weather.city.c_str());

        // Calculate width properly (IMPORTANT)
        int cityWidth = u8g2Fonts.getUTF8Width(weather.city.c_str());

        // ---- COUNTRY BADGE ----
        drawBadge(30 + cityWidth, 2, 48, 28, COLOR_GREEN, COLOR_BG, weather.country.c_str(), 2);

        tft->setFont(&FreeMonoBold9pt7b);
        tft->setTextColor(COLOR_WHITE); 
        tft->setTextSize(1); 
        tft->setCursor(10, 42); 
        tft->print("Feels "); 
        tft->print((int)weather.feelsLike); 
        // Draw degree symbol as small circle
        int16_t cx, cy;
        uint16_t cw, ch;
        tft->getTextBounds("0", 0, 0, &cx, &cy, &cw, &ch);
        int degX = tft->getCursorX() + 2;
        int degY = 37 - 10;  // near top of text
        tft->drawCircle(degX + 2, degY, 3, COLOR_WHITE);
        tft->setCursor(degX + 8, 37);
        tft->print("C"); 
        tft->setFont();
    } else { 
        tft->setCursor(60, 8); 
        tft->print("Loading..."); 
    }
}

void DeskPulseClock::drawWeatherIcon(int x, int y, String icon) {
    // Improved weather icons based on OpenWeatherMap icon codes
    if (icon.startsWith("01")) {
        // ☀️ CLEAR SKY - Beautiful sun with rays
        tft->fillCircle(x + 25, y + 25, 16, COLOR_ORANGE);
        tft->fillCircle(x + 25, y + 25, 12, COLOR_YELLOW);
        tft->fillCircle(x + 25, y + 25, 8, 0xFFE0);  // Lighter yellow center
        for (int i = 0; i < 8; i++) {
            float angle = i * 0.785;  // 45 degrees in radians
            int x1 = x + 25 + cos(angle) * 20;
            int y1 = y + 25 + sin(angle) * 20;
            int x2 = x + 25 + cos(angle) * 24;
            int y2 = y + 25 + sin(angle) * 24;
            tft->drawLine(x1, y1, x2, y2, COLOR_YELLOW);
        }
    } else if (icon.startsWith("02") || icon.startsWith("03")) {
        // ⛅ PARTLY CLOUDY - Sun peeking behind cloud
        tft->fillCircle(x + 35, y + 15, 12, COLOR_ORANGE);
        tft->fillCircle(x + 35, y + 15, 8, COLOR_YELLOW);
        tft->fillCircle(x + 12, y + 32, 10, COLOR_WHITE);
        tft->fillCircle(x + 25, y + 28, 12, COLOR_WHITE);
        tft->fillCircle(x + 38, y + 32, 10, COLOR_WHITE);
        tft->fillCircle(x + 18, y + 38, 8, COLOR_WHITE);
        tft->fillCircle(x + 32, y + 38, 8, COLOR_WHITE);
        tft->fillRoundRect(x + 8, y + 32, 34, 12, 4, COLOR_WHITE);
    } else if (icon.startsWith("04")) {
        // ☁️ CLOUDY - Dense clouds
        tft->fillCircle(x + 35, y + 18, 10, COLOR_GRAY);
        tft->fillCircle(x + 42, y + 24, 8, COLOR_GRAY);
        tft->fillRoundRect(x + 28, y + 20, 20, 10, 4, COLOR_GRAY);
        tft->fillCircle(x + 12, y + 30, 11, COLOR_WHITE);
        tft->fillCircle(x + 26, y + 26, 13, COLOR_WHITE);
        tft->fillCircle(x + 40, y + 30, 11, COLOR_WHITE);
        tft->fillRoundRect(x + 8, y + 30, 36, 14, 5, COLOR_WHITE);
    } else if (icon.startsWith("09") || icon.startsWith("10")) {
        // 🌧️ RAIN - Cloud with rain drops
        tft->fillCircle(x + 12, y + 16, 9, COLOR_GRAY);
        tft->fillCircle(x + 25, y + 12, 11, COLOR_GRAY);
        tft->fillCircle(x + 38, y + 16, 9, COLOR_GRAY);
        tft->fillRoundRect(x + 8, y + 16, 34, 10, 4, COLOR_GRAY);
        uint16_t rainColor = 0x04DF;  // Light blue
        tft->fillCircle(x + 14, y + 32, 2, rainColor);
        tft->drawLine(x + 14, y + 30, x + 12, y + 36, rainColor);
        tft->fillCircle(x + 26, y + 34, 2, rainColor);
        tft->drawLine(x + 26, y + 32, x + 24, y + 38, rainColor);
        tft->fillCircle(x + 38, y + 32, 2, rainColor);
        tft->drawLine(x + 38, y + 30, x + 36, y + 36, rainColor);
        tft->fillCircle(x + 20, y + 40, 2, rainColor);
        tft->drawLine(x + 20, y + 38, x + 18, y + 44, rainColor);
        tft->fillCircle(x + 32, y + 42, 2, rainColor);
        tft->drawLine(x + 32, y + 40, x + 30, y + 46, rainColor);
    } else if (icon.startsWith("11")) {
        // ⛈️ THUNDERSTORM - Cloud with lightning
        tft->fillCircle(x + 12, y + 14, 9, 0x4208);  // Dark gray
        tft->fillCircle(x + 25, y + 10, 11, 0x4208);
        tft->fillCircle(x + 38, y + 14, 9, 0x4208);
        tft->fillRoundRect(x + 8, y + 14, 34, 10, 4, 0x4208);
        tft->fillTriangle(x + 28, y + 24, x + 22, y + 34, x + 26, y + 32, COLOR_YELLOW);
        tft->fillTriangle(x + 24, y + 32, x + 18, y + 44, x + 26, y + 36, COLOR_YELLOW);
        tft->fillTriangle(x + 26, y + 32, x + 30, y + 32, x + 24, y + 38, COLOR_YELLOW);
        tft->fillCircle(x + 12, y + 38, 2, COLOR_CYAN);
        tft->fillCircle(x + 40, y + 36, 2, COLOR_CYAN);
    } else if (icon.startsWith("13")) {
        // ❄️ SNOW - Cloud with snowflakes
        tft->fillCircle(x + 12, y + 14, 9, 0xC618);
        tft->fillCircle(x + 25, y + 10, 11, 0xC618);
        tft->fillCircle(x + 38, y + 14, 9, 0xC618);
        tft->fillRoundRect(x + 8, y + 14, 34, 10, 4, 0xC618);
        int snowY1 = y + 32;
        int snowY2 = y + 42;
        tft->drawLine(x + 14 - 4, snowY1, x + 14 + 4, snowY1, COLOR_WHITE);
        tft->drawLine(x + 14, snowY1 - 4, x + 14, snowY1 + 4, COLOR_WHITE);
        tft->drawLine(x + 14 - 3, snowY1 - 3, x + 14 + 3, snowY1 + 3, COLOR_WHITE);
        tft->drawLine(x + 14 - 3, snowY1 + 3, x + 14 + 3, snowY1 - 3, COLOR_WHITE);
        tft->drawLine(x + 36 - 4, snowY1, x + 36 + 4, snowY1, COLOR_WHITE);
        tft->drawLine(x + 36, snowY1 - 4, x + 36, snowY1 + 4, COLOR_WHITE);
        tft->drawLine(x + 36 - 3, snowY1 - 3, x + 36 + 3, snowY1 + 3, COLOR_WHITE);
        tft->drawLine(x + 36 - 3, snowY1 + 3, x + 36 + 3, snowY1 - 3, COLOR_WHITE);
        tft->drawLine(x + 25 - 3, snowY2, x + 25 + 3, snowY2, COLOR_WHITE);
        tft->drawLine(x + 25, snowY2 - 3, x + 25, snowY2 + 3, COLOR_WHITE);
        tft->drawPixel(x + 25 - 2, snowY2 - 2, COLOR_WHITE);
        tft->drawPixel(x + 25 + 2, snowY2 + 2, COLOR_WHITE);
    } else if (icon.startsWith("50")) {
        // 🌫️ MIST/FOG - Horizontal lines
        tft->drawFastHLine(x + 5, y + 15, 40, COLOR_GRAY);
        tft->drawFastHLine(x + 8, y + 22, 34, COLOR_WHITE);
        tft->drawFastHLine(x + 5, y + 29, 40, COLOR_GRAY);
        tft->drawFastHLine(x + 8, y + 36, 34, COLOR_WHITE);
        tft->drawFastHLine(x + 5, y + 43, 40, COLOR_GRAY);
    } else {
        // Default - sun
        tft->fillCircle(x + 25, y + 25, 16, COLOR_ORANGE);
        tft->fillCircle(x + 25, y + 25, 12, COLOR_YELLOW);
    }
}

void DeskPulseClock::drawTime() {
    if (clockStyle == CLOCK_STYLE_SIMPLE) {
        // Clear the whole time band before drawing. U8g2 text is transparent,
        // so old digits can otherwise remain and overlap on minute changes.
        tft->fillRect(0, 18, 240, 74, COLOR_BG);
        int y = 74;
        u8g2Fonts.setFontMode(1);
        u8g2Fonts.setFontDirection(0);
        u8g2Fonts.setBackgroundColor(COLOR_BG);
        u8g2Fonts.setFont(u8g2_font_logisoso42_tn);

        char hh[3];
        char mm[3];
        sprintf(hh, "%02d", hour);
        sprintf(mm, "%02d", minute);

        int hourWidth = u8g2Fonts.getUTF8Width(hh);
        int colonWidth = u8g2Fonts.getUTF8Width(":");
        int minuteWidth = u8g2Fonts.getUTF8Width(mm);
        int totalWidth = hourWidth + colonWidth + minuteWidth;
        int x = (240 - totalWidth) / 2;

        u8g2Fonts.setForegroundColor(COLOR_WHITE);
        u8g2Fonts.setCursor(x, y);
        u8g2Fonts.print(hh);

        u8g2Fonts.setForegroundColor(COLOR_WHITE);
        u8g2Fonts.setCursor(x + hourWidth, y);
        u8g2Fonts.print(":");

        u8g2Fonts.setForegroundColor(COLOR_YELLOW);
        u8g2Fonts.setCursor(x + hourWidth + colonWidth, y);
        u8g2Fonts.print(mm);
        return;
    }

    // Weather clock time is dynamically drawn in smartClock() instead
}

void DeskPulseClock::drawSeconds() {
    // This is a legacy function for the simple fixed-coordinate seconds rendering.
    // It is no longer called because smartClock() handles seconds placement dynamically.
    if (clockStyle == CLOCK_STYLE_SIMPLE) {
        return;
    }

    int x = 135;
    int y = 86; //95
    char secStr[3];
    sprintf(secStr, "%02d", second);

    u8g2Fonts.setFontMode(1);
    u8g2Fonts.setFontDirection(0);
    u8g2Fonts.setForegroundColor(COLOR_ORANGE1);
    u8g2Fonts.setBackgroundColor(COLOR_BLUE1);
    u8g2Fonts.setFont(u8g2_font_logisoso24_tn);

    int textWidth = u8g2Fonts.getUTF8Width(secStr);
    int boxWidth = textWidth + 12;

    tft->fillRoundRect(x, y, boxWidth, 35, 5, COLOR_DARK_BLUE);
    u8g2Fonts.setCursor(x + (boxWidth - textWidth) / 2, y + 26);
    u8g2Fonts.print(secStr);
}

void DeskPulseClock::drawDate() {
    if (clockStyle == CLOCK_STYLE_SIMPLE) {
        // Clear date/day band so old text never overlaps after date/style changes.
        tft->fillRect(0, 132, 240, 72, COLOR_BG);
        const char* weekdayStr = getWeekdayName(this->weekday);
        int dayBadgeW = 44;
        int dayBadgeX = (240 - dayBadgeW) / 2;
        int dayBadgeY = 142;
        drawBadge(dayBadgeX, dayBadgeY, dayBadgeW, 22, COLOR_GREEN, COLOR_BG, weekdayStr, 2);

        char dateStr[12];
        sprintf(dateStr, "%04d/%02d/%02d", year, month, day);
        tft->setTextColor(COLOR_WHITE);
        tft->setTextSize(2);
        int textW = strlen(dateStr) * 12;
        tft->setCursor((240 - textW) / 2, 176);
        tft->print(dateStr);
        return;
    }

    int y = 140;
    tft->setTextColor(COLOR_WHITE);
    tft->setTextSize(2);
    tft->setCursor(10, y);
    
    char dateStr[12];
    sprintf(dateStr, "%02d/%02d/%04d", day, month, year);
    tft->print(dateStr);
    
    tft->setTextColor(COLOR_YELLOW);
    tft->setCursor(145, y);
    tft->print(getWeekdayName(weekday));
}

void DeskPulseClock::drawThermometerIcon(int x, int y) {
    tft->fillRoundRect(x + 4, y, 6, 20, 3, COLOR_WHITE);
    tft->fillCircle(x + 7, y + 22, 6, COLOR_RED);
    tft->fillRect(x + 5, y + 10, 4, 12, COLOR_RED);
}

void DeskPulseClock::drawHumidityIcon(int x, int y) {
    tft->fillCircle(x + 7, y + 12, 7, COLOR_GREEN);
    tft->fillTriangle(x + 7, y, x + 1, y + 10, x + 13, y + 10, COLOR_GREEN);
}

void DeskPulseClock::drawTemperatureBar(int x, int y, float temp) {
    drawThermometerIcon(x, y);
    
    tft->fillRoundRect(x + 25, y + 8, 60, 8, 4, COLOR_GRAY);
    
    int barWidth = constrain(map((int)temp, -10, 40, 0, 56), 0, 56);
    uint16_t barColor = temp < 10 ? COLOR_BLUE : (temp < 25 ? COLOR_GREEN : COLOR_RED);
    tft->fillRoundRect(x + 27, y + 10, barWidth, 4, 2, barColor);
    
    tft->setTextColor(COLOR_WHITE);
    tft->setTextSize(2);
    tft->setCursor(x + 90, y + 6);
    tft->print((int)temp);
    
    int degX = tft->getCursorX() + 2;
    int degY = y + 6 + 2;
    tft->drawCircle(degX + 3, degY, 4, COLOR_WHITE);
    tft->setCursor(degX + 12, y + 6);
    tft->print("C");
}

void DeskPulseClock::drawHumidity(int x, int y, int humidity) {
    drawHumidityIcon(x, y + 5);
    
    tft->setTextColor(COLOR_WHITE);
    tft->setTextSize(2);
    tft->setCursor(x + 25, y + 8);
    tft->print(humidity);
    tft->print("%");
}

void DeskPulseClock::drawWeatherInfo() {
    if (!weather.valid) return;
    
    tft->fillRoundRect(170, 66, 70, 28, 6, COLOR_WHITE);
    drawWeatherIcon(185, 10, weather.icon);
    
    drawBadge(200, 85, 38, 20, COLOR_WHITE, COLOR_BG, weather.condition.c_str(), 2);
    
    drawTemperatureBar(10, 175, weather.temperature);
    drawHumidity(10, 205, weather.humidity);
    
    tft->setFont(&FreeMonoBold9pt7b);
    tft->setTextColor(COLOR_GREEN);
    tft->setTextSize(1);
    tft->setCursor(127, 225);
    tft->print("Wind:");
    tft->print((int)weather.windSpeed);
    tft->print("m/s");
    tft->setFont();
}

void DeskPulseClock::simpleClock(bool forceRedraw) {
    if (forceRedraw) {
        tft->fillScreen(COLOR_BG);
        // Force all simple-clock regions to redraw cleanly after style/view changes.
        prevHour = -1;
        prevMinute = -1;
        prevDay = -1;
    }
    if (forceRedraw || hour != prevHour || minute != prevMinute) {
        drawTime();
        prevHour = hour;
        prevMinute = minute;
    }
    if (forceRedraw || day != prevDay) {
        drawDate();
        prevDay = day;
    }
}

void DeskPulseClock::smartClock(bool forceRedraw) {
    static int lastHour = -1, lastMinute = -1, lastDay = -1;
    bool layoutRedraw = forceRedraw || lastDay < 0 || day != lastDay || weatherUpdated;
    bool timeChanged = layoutRedraw || hour != lastHour || minute != lastMinute;

    // Full screen only for first draw/date/weather/style changes.
    // Normal minute changes update only the time band to reduce flicker.
    if (layoutRedraw) {
        tft->fillScreen(COLOR_BG);
        drawHeader();
        if (weather.valid) {
            drawWeatherIcon(180, 10, weather.icon);
            drawBadge(185, 60, 48, 22, COLOR_WHITE, COLOR_BG, weather.condition.c_str(), 2);
        }

        int dateY = 140;
        tft->setTextColor(COLOR_WHITE);
        tft->setTextSize(2);
        char dateStr[12];
        sprintf(dateStr, "%02d/%02d/%04d", day, month, year);
        tft->setCursor(10, dateY);
        tft->print(dateStr);
        tft->setTextColor(COLOR_YELLOW);
        tft->setCursor(145, dateY);
        tft->print(getWeekdayName(weekday));

        if (weather.valid) {
            drawTemperatureBar(10, 175, weather.temperature);
            drawHumidity(10, 205, weather.humidity);
            tft->setFont(&FreeMonoBold9pt7b);
            tft->setTextColor(COLOR_GREEN);
            tft->setTextSize(1);
            tft->setCursor(127, 225);
            tft->print("Wind:");
            tft->print((int)weather.windSpeed);
            tft->print("m/s");
            tft->setFont();
        }
    }

    if (timeChanged) {
        tft->fillRect(0, 48, 178, 54, COLOR_BG);
        u8g2Fonts.setFontMode(1);
        u8g2Fonts.setFontDirection(0);
        u8g2Fonts.setBackgroundColor(COLOR_BG);
        u8g2Fonts.setFont(u8g2_font_logisoso42_tn);
        char hmStr[6];
        sprintf(hmStr, "%02d:%02d", hour, minute);
        u8g2Fonts.setForegroundColor(COLOR_WHITE);
        u8g2Fonts.setCursor(16, 90);
        u8g2Fonts.print(hmStr);
        lastHour = hour;
        lastMinute = minute;
    }

    if (layoutRedraw) {
        lastDay = day;
        weatherUpdated = false;
        prevSecond = -1;
    }

    u8g2Fonts.setFontMode(1);
    u8g2Fonts.setFontDirection(0);
    u8g2Fonts.setFont(u8g2_font_logisoso42_tn);
    char timeStr[6];
    sprintf(timeStr, "%02d:%02d", hour, minute);
    int timeWidth = u8g2Fonts.getUTF8Width(timeStr);
    int timeX = 16;
    int timeY = 90;

    if (layoutRedraw || second != prevSecond) {
        char secStr[3];
        sprintf(secStr, "%02d", second);
        u8g2Fonts.setFont(u8g2_font_logisoso24_tn);
        int secWidth = u8g2Fonts.getUTF8Width(secStr) + 12;
        int secX = timeX + timeWidth + 10;
        int secY = timeY - 32;
        tft->fillRoundRect(secX, secY, secWidth, 35, 5, COLOR_BG);
        tft->fillRoundRect(secX, secY, secWidth, 35, 5, COLOR_DARK_BLUE);
        u8g2Fonts.setForegroundColor(COLOR_ORANGE1);
        u8g2Fonts.setBackgroundColor(COLOR_DARK_BLUE);
        u8g2Fonts.setCursor(secX + (secWidth - u8g2Fonts.getUTF8Width(secStr)) / 2, secY + 26);
        u8g2Fonts.print(secStr);
        u8g2Fonts.setBackgroundColor(COLOR_BG);
        prevSecond = second;
    }
}

// SIMPLIFIED AND FIXED UPDATE FUNCTION
void DeskPulseClock::update() {
    // Rely entirely on simpleClock and smartClock to natively track changes 
    // and draw what needs to be redrawn. They already do this perfectly.
    if (clockStyle == CLOCK_STYLE_SIMPLE) {
        simpleClock(false);
    } else {
        smartClock(false);
    }
}