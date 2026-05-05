#ifndef CLOCK_H
#define CLOCK_H

#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>
#include "weather.h"
#include "colors.h"

class DeskPulseClock {
public:
    enum ClockStyle {
        CLOCK_STYLE_WEATHER = 0,
        CLOCK_STYLE_SIMPLE = 1
    };

    DeskPulseClock(Adafruit_ST7789* display);
    
    void begin();
    void setTime(int hour, int minute, int second);
    void setDate(int day, int month, int year, int weekday);
    void setWeather(WeatherData data);
    void update();
    void setClockStyle(ClockStyle style);
    void toggleClockStyle();
    ClockStyle getClockStyle() const;
    
private:
    Adafruit_ST7789* tft;
    
    // Time
    int hour = 0, minute = 0, second = 0;
    int prevHour = -1, prevMinute = -1, prevSecond = -1;
    
    // Date
    int day = 1, month = 1, year = 2024, weekday = 0;
    int prevDay = -1;
    
    // Weather
    WeatherData weather;
    bool weatherUpdated = false;
    ClockStyle clockStyle = CLOCK_STYLE_WEATHER;
    
    // UI Drawing
    void drawHeader();
    void drawTime();
    void drawSeconds();
    void drawDate();
    void drawWeatherInfo();
    void drawWeatherIcon(int x, int y, String icon);
    void drawTemperatureBar(int x, int y, float temp);
    void drawHumidity(int x, int y, int humidity);
    void drawBadge(int x, int y, int w, int h, uint16_t bgColor, uint16_t textColor, const char* text);
    void drawBadge(int x, int y, int w, int h, uint16_t bgColor, uint16_t textColor, const char* text, uint8_t textSize);
    void simpleClock(bool forceRedraw);
    void smartClock(bool forceRedraw);
    
    // Helpers
    const char* getWeekdayName(int wday);
    void drawThermometerIcon(int x, int y);
    void drawHumidityIcon(int x, int y);
};

#endif

