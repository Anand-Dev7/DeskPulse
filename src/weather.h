#ifndef WEATHER_H
#define WEATHER_H

#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

class Adafruit_ST7789;

struct WeatherData {
    String city;
    String country;
    String condition;      // "Clear", "Clouds", "Rain", etc.
    String icon;           // "01d", "02n", etc.
    float temperature;
    float feelsLike;
    int humidity;
    float windSpeed;
    bool valid;
};

struct ForecastDay {
    String dayName;
    int temp;
    String condition;
};

class WeatherAPI {
public:
    WeatherAPI(const char* apiKey, const char* city, const char* countryCode = "");
    void configure(const String& apiKey, const String& city, const String& countryCode);
    
    bool update();
    bool weatherForecast(ForecastDay outDays[3]);
    WeatherData getData();
    
private:
    String _apiKey;
    String _city;
    String _countryCode;
    WeatherData _data;
    unsigned long _lastUpdate;
    const unsigned long UPDATE_INTERVAL = 600000;  // 10 minutes
};

void drawWeatherForecastPage(Adafruit_ST7789& display, const ForecastDay days[3], bool hasData);

#endif
