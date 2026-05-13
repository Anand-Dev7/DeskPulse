#include "weather.h"
#include "colors.h"
#include <Adafruit_ST7789.h>
#include <Fonts/FreeMono9pt7b.h>
#include <Fonts/FreeSans9pt7b.h>
#include <Fonts/FreeSansBold9pt7b.h>

namespace
{

    const char *getWeekdayShortName(int wday)
    {
        static const char *kDays[] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
        if (wday < 0)
            wday = 0;
        return kDays[wday % 7];
    }

    int weekdayFromEpoch(long epochSec, long tzOffsetSec)
    {
        time_t localEpoch = static_cast<time_t>(epochSec + tzOffsetSec);
        struct tm tmInfo;
        gmtime_r(&localEpoch, &tmInfo);
        return tmInfo.tm_wday;
    }

    String urlEncode(const String &in)
    {
        String out;
        out.reserve(in.length() * 3);
        const char *hex = "0123456789ABCDEF";
        for (size_t i = 0; i < in.length(); i++)
        {
            uint8_t c = static_cast<uint8_t>(in[i]);
            if ((c >= 'a' && c <= 'z') ||
                (c >= 'A' && c <= 'Z') ||
                (c >= '0' && c <= '9') ||
                c == '-' || c == '_' || c == '.' || c == '~')
            {
                out += static_cast<char>(c);
            }
            else
            {
                out += '%';
                out += hex[(c >> 4) & 0x0F];
                out += hex[c & 0x0F];
            }
        }
        return out;
    }

    String buildLocationQuery(const String &city, const String &countryCode)
    {
        String query = city;
        if (countryCode.length() > 0)
        {
            query += ",";
            query += countryCode;
        }
        return urlEncode(query);
    }

    bool containsIgnoreCase(const String &text, const char *token)
    {
        String lhs = text;
        lhs.toLowerCase();
        String rhs = token;
        rhs.toLowerCase();
        return lhs.indexOf(rhs) >= 0;
    }

    // --- HELPER TO CENTER TEXT ---
    void drawCenteredText(Adafruit_ST7789 &display, const String &text, int boxX, int boxW, int y)
    {
        int16_t x1, y1;
        uint16_t w, h;
        display.getTextBounds(text, 0, 0, &x1, &y1, &w, &h);
        display.setCursor(boxX + (boxW - w) / 2, y);
        display.print(text);
    }

    void drawSunIcon(Adafruit_ST7789 &display, int cx, int cy)
    {
        display.fillCircle(cx, cy, 8, COLOR_YELLOW);
        display.drawCircle(cx, cy, 10, COLOR_ORANGE);
        for (int i = -1; i <= 1; i++)
        {
            display.drawFastVLine(cx - 13 + (i * 13), cy - 2, 5, COLOR_YELLOW);
            display.drawFastHLine(cx - 2, cy - 13 + (i * 13), 5, COLOR_YELLOW);
        }
    }

    void drawCloudIcon(Adafruit_ST7789 &display, int cx, int cy, uint16_t color)
    {
        display.fillCircle(cx - 8, cy, 7, color);
        display.fillCircle(cx, cy - 4, 9, color);
        display.fillCircle(cx + 10, cy, 7, color);
        display.fillRoundRect(cx - 16, cy + 2, 34, 10, 5, color);
    }

    void drawRainIcon(Adafruit_ST7789 &display, int cx, int cy)
    {
        drawCloudIcon(display, cx, cy - 3, COLOR_WHITE);
        display.drawLine(cx - 10, cy + 10, cx - 14, cy + 16, COLOR_CYAN);
        display.drawLine(cx - 2, cy + 10, cx - 6, cy + 16, COLOR_CYAN);
        display.drawLine(cx + 6, cy + 10, cx + 2, cy + 16, COLOR_CYAN);
    }

    void drawStormIcon(Adafruit_ST7789 &display, int cx, int cy)
    {
        drawCloudIcon(display, cx, cy - 3, COLOR_WHITE);
        display.fillTriangle(cx - 2, cy + 8, cx + 5, cy + 8, cx + 1, cy + 16, COLOR_YELLOW);
        display.fillTriangle(cx + 1, cy + 14, cx + 8, cy + 14, cx + 3, cy + 22, COLOR_YELLOW);
    }

    void drawSnowIcon(Adafruit_ST7789 &display, int cx, int cy)
    {
        drawCloudIcon(display, cx, cy - 3, COLOR_WHITE);
        display.drawLine(cx - 8, cy + 12, cx - 2, cy + 18, COLOR_CYAN);
        display.drawLine(cx - 2, cy + 12, cx - 8, cy + 18, COLOR_CYAN);
        display.drawLine(cx + 4, cy + 12, cx + 10, cy + 18, COLOR_CYAN);
        display.drawLine(cx + 10, cy + 12, cx + 4, cy + 18, COLOR_CYAN);
    }

    void drawFogIcon(Adafruit_ST7789 &display, int cx, int cy)
    {
        drawCloudIcon(display, cx, cy - 5, COLOR_WHITE);
        display.drawFastHLine(cx - 16, cy + 8, 32, COLOR_CYAN);
        display.drawFastHLine(cx - 12, cy + 13, 24, COLOR_CYAN);
    }

    void drawHotIcon(Adafruit_ST7789 &display, int cx, int cy)
    {
        drawSunIcon(display, cx, cy);
        display.fillTriangle(cx + 9, cy + 7, cx + 15, cy + 1, cx + 16, cy + 12, COLOR_RED);
    }

    void drawColdIcon(Adafruit_ST7789 &display, int cx, int cy)
    {
        drawCloudIcon(display, cx, cy - 3, COLOR_WHITE);
        display.drawFastVLine(cx + 12, cy + 7, 10, COLOR_CYAN);
        display.fillCircle(cx + 12, cy + 19, 3, COLOR_CYAN);
    }

    void drawDynamicWeatherIcon(Adafruit_ST7789 &display, const String &condition, int tempC, int cx, int cy)
    {
        if (containsIgnoreCase(condition, "thunder"))
        {
            drawStormIcon(display, cx, cy);
            return;
        }
        if (containsIgnoreCase(condition, "rain") || containsIgnoreCase(condition, "drizzle"))
        {
            drawRainIcon(display, cx, cy);
            return;
        }
        if (containsIgnoreCase(condition, "snow"))
        {
            drawSnowIcon(display, cx, cy);
            return;
        }
        if (containsIgnoreCase(condition, "mist") || containsIgnoreCase(condition, "fog") || containsIgnoreCase(condition, "haze"))
        {
            drawFogIcon(display, cx, cy);
            return;
        }
        if (containsIgnoreCase(condition, "cloud"))
        {
            drawCloudIcon(display, cx, cy, COLOR_WHITE);
            return;
        }

        if (tempC >= 34)
        {
            drawHotIcon(display, cx, cy);
        }
        else if (tempC <= 10)
        {
            drawColdIcon(display, cx, cy);
        }
        else
        {
            drawSunIcon(display, cx, cy);
        }
    }

} // end anonymous namespace

WeatherAPI::WeatherAPI(const char *apiKey, const char *city, const char *countryCode)
{
    _apiKey = apiKey;
    _city = city;
    _countryCode = countryCode;
    _lastUpdate = 0;
    _data.valid = false;
}

void WeatherAPI::configure(const String &apiKey, const String &city, const String &countryCode)
{
    _apiKey = apiKey;
    _city = city;
    _countryCode = countryCode;
    _data.valid = false;
    _lastUpdate = 0;
}

bool WeatherAPI::update()
{
    if (WiFi.status() != WL_CONNECTED)
    {
        return false;
    }

    if (_apiKey.isEmpty() || _city.isEmpty())
    {
        Serial.println("Weather fetch skipped: missing api key or city");
        return false;
    }

    // Check if we need to update
    if (_data.valid && (millis() - _lastUpdate < UPDATE_INTERVAL))
    {
        return true;
    }

    String queries[2];
    int queryCount = 1;
    queries[0] = buildLocationQuery(_city, _countryCode);
    if (_countryCode.length() > 0)
    {
        queries[1] = urlEncode(_city);
        queryCount = 2;
    }

    for (int attempt = 0; attempt < queryCount; attempt++)
    {
        HTTPClient http;
        String url = "http://api.openweathermap.org/data/2.5/weather?q=" + queries[attempt];
        url += "&appid=" + _apiKey;
        url += "&units=metric";

        if (attempt == 0)
        {
            Serial.println("Fetching weather: " + url);
        }
        else
        {
            Serial.println("Retrying weather with city-only query: " + url);
        }

        http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
        http.begin(url);
        int httpCode = http.GET();

        if (httpCode == 200)
        {
            String payload = http.getString();

            DynamicJsonDocument doc(1536);
            DeserializationError error = deserializeJson(doc, payload);

            if (!error)
            {
                _data.city = doc["name"].as<String>();
                _data.country = doc["sys"]["country"].as<String>();
                _data.condition = doc["weather"][0]["main"].as<String>();
                _data.icon = doc["weather"][0]["icon"].as<String>();
                _data.temperature = doc["main"]["temp"].as<float>();
                _data.feelsLike = doc["main"]["feels_like"].as<float>();
                _data.humidity = doc["main"]["humidity"].as<int>();
                _data.windSpeed = doc["wind"]["speed"].as<float>();
                _data.valid = true;
                _lastUpdate = millis();

                Serial.println("Weather updated: " + _data.city + " " + String(_data.temperature) + "°C");
                http.end();
                return true;
            }
        }

        Serial.println("Weather fetch failed: " + String(httpCode));
        http.end();
    }

    return false;
}

bool WeatherAPI::weatherForecast(ForecastDay outDays[3])
{
    if (WiFi.status() != WL_CONNECTED)
    {
        return false;
    }

    if (_apiKey.isEmpty() || _city.isEmpty())
    {
        Serial.println("Forecast fetch skipped: missing api key or city");
        return false;
    }

    for (int i = 0; i < 3; i++)
    {
        outDays[i].dayName = "";
        outDays[i].condition = "";
        outDays[i].temp = 0;
    }

    String queries[2];
    int queryCount = 1;
    queries[0] = buildLocationQuery(_city, _countryCode);
    if (_countryCode.length() > 0)
    {
        queries[1] = urlEncode(_city);
        queryCount = 2;
    }

    for (int attempt = 0; attempt < queryCount; attempt++)
    {
        HTTPClient http;
        String url = "http://api.openweathermap.org/data/2.5/forecast?q=" + queries[attempt];
        url += "&appid=" + _apiKey + "&units=metric";

        if (attempt == 0)
        {
            Serial.println("Fetching forecast: " + url);
        }
        else
        {
            Serial.println("Retrying forecast with city-only query: " + url);
        }

        http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
        http.begin(url);
        int httpCode = http.GET();
        if (httpCode != 200)
        {
            Serial.println("Forecast fetch failed: " + String(httpCode));
            http.end();
            continue;
        }

        String payload = http.getString();
        http.end();

        DynamicJsonDocument doc(32768);
        DeserializationError error = deserializeJson(doc, payload);
        if (error)
        {
            Serial.println("Forecast JSON parse failed: " + String(error.c_str()));
            continue;
        }

        String cod = doc["cod"] | "";
        if (cod.length() > 0 && cod != "200")
        {
            String message = doc["message"] | "unknown";
            Serial.println("Forecast API error: cod=" + cod + " message=" + message);
            continue;
        }

        JsonArray list = doc["list"].as<JsonArray>();
        if (list.isNull() || list.size() < 3)
        {
            Serial.println("Forecast list missing or too short, size=" + String(list.size()));
            continue;
        }

        long tzOffsetSec = doc["city"]["timezone"] | 0;
        int listSize = static_cast<int>(list.size());
        int indices[3];
        if (listSize >= 24)
        {
            indices[0] = 7;
            indices[1] = 15;
            indices[2] = 23;
        }
        else
        {
            indices[0] = listSize / 4;
            indices[1] = listSize / 2;
            indices[2] = (listSize * 3) / 4;
        }

        bool ok = true;
        for (int i = 0; i < 3; i++)
        {
            int idx = indices[i];
            if (idx < 0 || idx >= listSize)
            {
                ok = false;
                break;
            }

            JsonObject entry = list[idx].as<JsonObject>();
            if (entry.isNull())
            {
                ok = false;
                break;
            }

            outDays[i].temp = entry["main"]["temp"].as<int>();
            outDays[i].condition = entry["weather"][0]["main"].as<String>();

            long entryEpoch = entry["dt"] | 0;
            int wday = weekdayFromEpoch(entryEpoch, tzOffsetSec);
            outDays[i].dayName = getWeekdayShortName(wday);

            if (outDays[i].dayName.length() == 0)
            {
                outDays[i].dayName = "Day";
            }
            if (outDays[i].condition.length() == 0)
            {
                outDays[i].condition = "N/A";
            }
        }

        if (ok)
        {
            return true;
        }
    }

    return false;
}

WeatherData WeatherAPI::getData()
{
    return _data;
}

// -------------------------------------------------------------
// NEW DASHBOARD UI FOR 3-DAY FORECAST
// -------------------------------------------------------------
void drawWeatherForecastPage(Adafruit_ST7789 &display, const ForecastDay days[3], bool hasData)
{
    // Premium Modern Colors (RGB565 format)
    const uint16_t THEME_BG = 0x0825;    // Dark Navy Blue background
    const uint16_t CARD_1_BG = 0x2129;   // Deep Purple/Indigo
    const uint16_t CARD_2_BG = 0x18C8;   // Deep Blue
    const uint16_t CARD_3_BG = 0x0887;   // Dark Ocean Blue
    const uint16_t TEXT_WHITE = 0xFFFF;  // Pure White
    const uint16_t TEXT_ACCENT = 0x07FF; // Cyan (For the days)
    const uint16_t TEXT_MUTED = 0xBDD7;  // Light Gray/Blue for conditions

    display.fillScreen(THEME_BG);

    // Modern centered header
    display.setTextColor(TEXT_WHITE);
    display.setTextSize(1);
    display.setFont(&FreeSansBold9pt7b);
    drawCenteredText(display, "3-DAY FORECAST", 0, 240, 24);

    // Tiny elegant separator line under header
    display.drawFastHLine(80, 32, 80, TEXT_ACCENT);

    if (!hasData)
    {
        display.setTextColor(TEXT_MUTED);
        display.setFont(&FreeSans9pt7b);
        drawCenteredText(display, "Loading data...", 0, 240, 120);
        display.setFont(NULL);
        return;
    }

    // Card Dimensions
    const int cardY = 45;
    const int cardW = 70;
    const int cardH = 175;
    const int spacing = 8;
    const int startX = 7; // (240 - (3*70 + 2*8)) / 2 = 7

    uint16_t cardColors[3] = {CARD_1_BG, CARD_2_BG, CARD_3_BG};

    for (int i = 0; i < 3; i++)
    {
        int cX = startX + i * (cardW + spacing);

        // Draw Card Background with rounded corners
        display.fillRoundRect(cX, cardY, cardW, cardH, 8, cardColors[i]);

        // 1. DAY NAME (Top)
        String dayName = days[i].dayName;
        dayName.toUpperCase(); // Looks better in uppercase on dashboards
        display.setTextColor(TEXT_ACCENT);
        display.setFont(&FreeSansBold9pt7b);
        drawCenteredText(display, dayName, cX, cardW, cardY + 22);

        // 2. WEATHER ICON (Center)
        // Icon takes up about 30x30, so place it nicely in the middle
        int iconCenterY = cardY + 65;
        int iconCenterX = cX + (cardW / 2);
        drawDynamicWeatherIcon(display, days[i].condition, days[i].temp, iconCenterX, iconCenterY);

        // 3. TEMPERATURE (Lower Middle)
        String tempStr = String(days[i].temp) + "C";
        display.setTextColor(TEXT_WHITE);
        display.setFont(&FreeSansBold9pt7b);
        drawCenteredText(display, tempStr, cX, cardW, cardY + 120);

        // 4. CONDITION (Bottom)
        String condition = days[i].condition;
        // Truncate to fit card width perfectly
        if (condition.length() > 9)
        {
            condition = condition.substring(0, 8) + ".";
        }
        display.setTextColor(TEXT_MUTED);

        // Use a smaller standard font for the condition so it fits nicely
        display.setFont();
        display.setTextSize(1);

        // Using standard font means no getTextBounds easily,
        // standard font characters are 6 pixels wide.
        int condWidth = condition.length() * 6;
        int condX = cX + (cardW - condWidth) / 2;
        display.setCursor(condX, cardY + 145);
        display.print(condition);
    }

    // Reset font to default when done
    display.setFont(NULL);
}