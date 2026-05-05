#ifndef COMMON_H
#define COMMON_H

#include <Arduino.h>
#include <Adafruit_ST7789.h>
#include "clock.h"
#include "weather.h"

enum TouchEvent {
	TOUCH_EVENT_NONE = 0,
	TOUCH_EVENT_SHORT_TAP,
	TOUCH_EVENT_LONG_TAP
};

enum BrightnessLevel {
	BRIGHTNESS_HIGH = 0,
	BRIGHTNESS_MEDIUM,
	BRIGHTNESS_LOW
};

bool isTouchTapped(uint8_t touchPin, bool activeLow = true);
TouchEvent readTouchEvent(uint8_t touchPin, bool activeLow = true, unsigned long longPressMs = 700);
void initDisplay(Adafruit_ST7789& display, int rstPin, int sclkPin, int mosiPin);
void connectWiFi(Adafruit_ST7789& display, const char* ssid, const char* password);
void syncTime(Adafruit_ST7789& display, long gmtOffsetSec, int daylightOffsetSec, const char* ntpServer);
void updateWeather(WeatherAPI& weatherApi, DeskPulseClock& clock);
bool fetchForecast(const char* apiKey, const char* city, const char* countryCode, ForecastDay outDays[3]);
void drawForecastPage(Adafruit_ST7789& display, const ForecastDay days[3], bool hasData);
void drawAboutPage(Adafruit_ST7789& display);
void drawBrightnessPopup(Adafruit_ST7789& display, BrightnessLevel level);
void initPanelBrightnessControl(Adafruit_ST7789& display);
void initBrightnessControl(int backlightPin, bool inverted = false);
void applyBrightnessLevel(BrightnessLevel level);

#endif
