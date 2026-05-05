#include "weather.h"
#include "colors.h"

#include <Adafruit_ST7789.h>
#include <Fonts/FreeMono9pt7b.h>
#include <Fonts/FreeSans9pt7b.h>
#include <Fonts/FreeSansBold9pt7b.h>

namespace {

const char* getWeekdayShortName(int wday) {
    static const char* kDays[] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
    if (wday < 0) wday = 0;
    return kDays[wday % 7];
}

int weekdayFromEpoch(long epochSec, long tzOffsetSec) {
    time_t localEpoch = static_cast<time_t>(epochSec + tzOffsetSec);
    struct tm tmInfo;
    gmtime_r(&localEpoch, &tmInfo);
    return tmInfo.tm_wday;
}

String urlEncode(const String& in) {
    String out;
    out.reserve(in.length() * 3);
    const char* hex = "0123456789ABCDEF";
    for (size_t i = 0; i < in.length(); i++) {
        uint8_t c = static_cast<uint8_t>(in[i]);
        if ((c >= 'a' && c <= 'z') ||
            (c >= 'A' && c <= 'Z') ||
            (c >= '0' && c <= '9') ||
            c == '-' || c == '_' || c == '.' || c == '~') {
            out += static_cast<char>(c);
        } else {
            out += '%';
            out += hex[(c >> 4) & 0x0F];
            out += hex[c & 0x0F];
        }
    }
    return out;
}

String buildLocationQuery(const String& city, const String& countryCode) {
    String query = city;
    if (countryCode.length() > 0) {
        query += ",";
        query += countryCode;
    }
    return urlEncode(query);
}

bool containsIgnoreCase(const String& text, const char* token) {
    String lhs = text;
    lhs.toLowerCase();
    String rhs = token;
    rhs.toLowerCase();
    return lhs.indexOf(rhs) >= 0;
}

void drawSunIcon(Adafruit_ST7789& display, int cx, int cy) {
    display.fillCircle(cx, cy, 8, COLOR_YELLOW);
    display.drawCircle(cx, cy, 10, COLOR_ORANGE);
    for (int i = -1; i <= 1; i++) {
        display.drawFastVLine(cx - 13 + (i * 13), cy - 2, 5, COLOR_YELLOW);
        display.drawFastHLine(cx - 2, cy - 13 + (i * 13), 5, COLOR_YELLOW);
    }
}

void drawCloudIcon(Adafruit_ST7789& display, int cx, int cy, uint16_t color) {
    display.fillCircle(cx - 8, cy, 7, color);
    display.fillCircle(cx, cy - 4, 9, color);
    display.fillCircle(cx + 10, cy, 7, color);
    display.fillRoundRect(cx - 16, cy + 2, 34, 10, 5, color);
}

void drawRainIcon(Adafruit_ST7789& display, int cx, int cy) {
    drawCloudIcon(display, cx, cy - 3, COLOR_WHITE);
    display.drawLine(cx - 10, cy + 10, cx - 14, cy + 16, COLOR_CYAN);
    display.drawLine(cx - 2, cy + 10, cx - 6, cy + 16, COLOR_CYAN);
    display.drawLine(cx + 6, cy + 10, cx + 2, cy + 16, COLOR_CYAN);
}

void drawStormIcon(Adafruit_ST7789& display, int cx, int cy) {
    drawCloudIcon(display, cx, cy - 3, COLOR_WHITE);
    display.fillTriangle(cx - 2, cy + 8, cx + 5, cy + 8, cx + 1, cy + 16, COLOR_YELLOW);
    display.fillTriangle(cx + 1, cy + 14, cx + 8, cy + 14, cx + 3, cy + 22, COLOR_YELLOW);
}

void drawSnowIcon(Adafruit_ST7789& display, int cx, int cy) {
    drawCloudIcon(display, cx, cy - 3, COLOR_WHITE);
    display.drawLine(cx - 8, cy + 12, cx - 2, cy + 18, COLOR_CYAN);
    display.drawLine(cx - 2, cy + 12, cx - 8, cy + 18, COLOR_CYAN);
    display.drawLine(cx + 4, cy + 12, cx + 10, cy + 18, COLOR_CYAN);
    display.drawLine(cx + 10, cy + 12, cx + 4, cy + 18, COLOR_CYAN);
}

void drawFogIcon(Adafruit_ST7789& display, int cx, int cy) {
    drawCloudIcon(display, cx, cy - 5, COLOR_WHITE);
    display.drawFastHLine(cx - 16, cy + 8, 32, COLOR_CYAN);
    display.drawFastHLine(cx - 12, cy + 13, 24, COLOR_CYAN);
}

void drawHotIcon(Adafruit_ST7789& display, int cx, int cy) {
    drawSunIcon(display, cx, cy);
    display.fillTriangle(cx + 9, cy + 7, cx + 15, cy + 1, cx + 16, cy + 12, COLOR_RED);
}

void drawColdIcon(Adafruit_ST7789& display, int cx, int cy) {
    drawCloudIcon(display, cx, cy - 3, COLOR_WHITE);
    display.drawFastVLine(cx + 12, cy + 7, 10, COLOR_CYAN);
    display.fillCircle(cx + 12, cy + 19, 3, COLOR_CYAN);
}

void drawDynamicWeatherIcon(Adafruit_ST7789& display, const String& condition, int tempC, int cx, int cy) {
    if (containsIgnoreCase(condition, "thunder")) {
        drawStormIcon(display, cx, cy);
        return;
    }
    if (containsIgnoreCase(condition, "rain") || containsIgnoreCase(condition, "drizzle")) {
        drawRainIcon(display, cx, cy);
        return;
    }
    if (containsIgnoreCase(condition, "snow")) {
        drawSnowIcon(display, cx, cy);
        return;
    }
    if (containsIgnoreCase(condition, "mist") || containsIgnoreCase(condition, "fog") || containsIgnoreCase(condition, "haze")) {
        drawFogIcon(display, cx, cy);
        return;
    }
    if (containsIgnoreCase(condition, "cloud")) {
        drawCloudIcon(display, cx, cy, COLOR_WHITE);
        return;
    }

    if (tempC >= 34) {
        drawHotIcon(display, cx, cy);
    } else if (tempC <= 10) {
        drawColdIcon(display, cx, cy);
    } else {
        drawSunIcon(display, cx, cy);
    }
}

}

WeatherAPI::WeatherAPI(const char* apiKey, const char* city, const char* countryCode) {
    _apiKey = apiKey;
    _city = city;
    _countryCode = countryCode;
    _lastUpdate = 0;
    _data.valid = false;
}

void WeatherAPI::configure(const String& apiKey, const String& city, const String& countryCode) {
    _apiKey = apiKey;
    _city = city;
    _countryCode = countryCode;
    _data.valid = false;
    _lastUpdate = 0;
}

bool WeatherAPI::update() {
    if (WiFi.status() != WL_CONNECTED) {
        return false;
    }

    if (_apiKey.isEmpty() || _city.isEmpty()) {
        Serial.println("Weather fetch skipped: missing api key or city");
        return false;
    }
    
    // Check if we need to update
    if (_data.valid && (millis() - _lastUpdate < UPDATE_INTERVAL)) {
        return true;
    }
    
    String queries[2];
    int queryCount = 1;
    queries[0] = buildLocationQuery(_city, _countryCode);
    if (_countryCode.length() > 0) {
        queries[1] = urlEncode(_city);
        queryCount = 2;
    }

    for (int attempt = 0; attempt < queryCount; attempt++) {
        HTTPClient http;
        String url = "http://api.openweathermap.org/data/2.5/weather?q=" + queries[attempt];
        url += "&appid=" + _apiKey;
        url += "&units=metric";

        if (attempt == 0) {
            Serial.println("Fetching weather: " + url);
        } else {
            Serial.println("Retrying weather with city-only query: " + url);
        }

        http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
        http.begin(url);
        int httpCode = http.GET();

        if (httpCode == 200) {
            String payload = http.getString();

            DynamicJsonDocument doc(1536);
            DeserializationError error = deserializeJson(doc, payload);

            if (!error) {
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

bool WeatherAPI::weatherForecast(ForecastDay outDays[3]) {
    if (WiFi.status() != WL_CONNECTED) {
        return false;
    }

    if (_apiKey.isEmpty() || _city.isEmpty()) {
        Serial.println("Forecast fetch skipped: missing api key or city");
        return false;
    }

    for (int i = 0; i < 3; i++) {
        outDays[i].dayName = "";
        outDays[i].condition = "";
        outDays[i].temp = 0;
    }

    String queries[2];
    int queryCount = 1;
    queries[0] = buildLocationQuery(_city, _countryCode);
    if (_countryCode.length() > 0) {
        queries[1] = urlEncode(_city);
        queryCount = 2;
    }

    for (int attempt = 0; attempt < queryCount; attempt++) {
        HTTPClient http;
        String url = "http://api.openweathermap.org/data/2.5/forecast?q=" + queries[attempt];
        url += "&appid=" + _apiKey + "&units=metric";

        if (attempt == 0) {
            Serial.println("Fetching forecast: " + url);
        } else {
            Serial.println("Retrying forecast with city-only query: " + url);
        }

        http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
        http.begin(url);
        int httpCode = http.GET();
        if (httpCode != 200) {
            Serial.println("Forecast fetch failed: " + String(httpCode));
            http.end();
            continue;
        }

        String payload = http.getString();
        http.end();

        DynamicJsonDocument doc(32768);
        DeserializationError error = deserializeJson(doc, payload);
        if (error) {
            Serial.println("Forecast JSON parse failed: " + String(error.c_str()));
            continue;
        }

        String cod = doc["cod"] | "";
        if (cod.length() > 0 && cod != "200") {
            String message = doc["message"] | "unknown";
            Serial.println("Forecast API error: cod=" + cod + " message=" + message);
            continue;
        }

        JsonArray list = doc["list"].as<JsonArray>();
        if (list.isNull() || list.size() < 3) {
            Serial.println("Forecast list missing or too short, size=" + String(list.size()));
            continue;
        }

        long tzOffsetSec = doc["city"]["timezone"] | 0;
        int listSize = static_cast<int>(list.size());
        int indices[3];
        if (listSize >= 24) {
            indices[0] = 7;
            indices[1] = 15;
            indices[2] = 23;
        } else {
            indices[0] = listSize / 4;
            indices[1] = listSize / 2;
            indices[2] = (listSize * 3) / 4;
        }

        bool ok = true;
        for (int i = 0; i < 3; i++) {
            int idx = indices[i];
            if (idx < 0 || idx >= listSize) {
                ok = false;
                break;
            }

            JsonObject entry = list[idx].as<JsonObject>();
            if (entry.isNull()) {
                ok = false;
                break;
            }

            outDays[i].temp = entry["main"]["temp"].as<int>();
            outDays[i].condition = entry["weather"][0]["main"].as<String>();

            long entryEpoch = entry["dt"] | 0;
            int wday = weekdayFromEpoch(entryEpoch, tzOffsetSec);
            outDays[i].dayName = getWeekdayShortName(wday);

            if (outDays[i].dayName.length() == 0) {
                outDays[i].dayName = "Day";
            }
            if (outDays[i].condition.length() == 0) {
                outDays[i].condition = "N/A";
            }
        }

        if (ok) {
            return true;
        }
    }

    return false;
}

WeatherData WeatherAPI::getData() {
    return _data;
}

void drawWeatherForecastPage(Adafruit_ST7789& display, const ForecastDay days[3], bool hasData) {
    display.fillScreen(COLOR_BG_DARK);
    display.fillRect(0, 0, 240, 28, COLOR_BG_HEADER);
    display.drawFastHLine(0, 28, 240, COLOR_WHITE);
    display.setTextColor(COLOR_BLACK);
    display.setTextSize(1);
    display.setFont(&FreeSansBold9pt7b);
    display.setCursor(22, 20);
    display.print("3-DAY FORECAST");

    if (!hasData) {
        display.setTextColor(COLOR_WHITE);
        display.setTextSize(1);
        display.setFont(&FreeSans9pt7b);
        display.setCursor(60, 120);
        display.print("No data");
        display.setFont(NULL);
        return;
    }

    const int rowTopStart = 38;
    const int rowHeight = 62;
    const uint16_t dayLabelColor = COLOR_FORECAST_DAY;
    const uint16_t dayDataColor = COLOR_FORECAST_DATA;
    for (int i = 0; i < 3; i++) {
        int y = rowTopStart + (i * rowHeight);

        String condition = days[i].condition;
        if (condition.length() > 12) {
            condition = condition.substring(0, 12);
        }
        String dataLine = condition + "  " + String(days[i].temp) + "C";

        uint16_t rowTint = (i % 2 == 0) ? COLOR_ROW_EVEN : COLOR_ROW_ODD;
        display.fillRoundRect(6, y - 6, 228, rowHeight - 6, 6, rowTint);

        display.setTextColor(dayLabelColor);
        display.setTextSize(1);
        display.setFont(&FreeSansBold9pt7b);
        display.setCursor(14, y + 14);
        display.print(days[i].dayName);

        display.setTextColor(dayDataColor);
        display.setTextSize(1);
        display.setFont(&FreeMono9pt7b);
        display.setCursor(14, y + 38);
        display.print(dataLine);

        drawDynamicWeatherIcon(display, days[i].condition, days[i].temp, 198, y + 18);
        display.drawFastHLine(12, y + rowHeight - 8, 216, COLOR_FORECAST_SEP);
    }

    display.setFont(NULL);
}
