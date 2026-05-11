#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>
#include <Preferences.h>

namespace Config {

constexpr const char* DEFAULT_CITY = "";
constexpr const char* DEFAULT_COUNTRY = "";
constexpr const char* DEFAULT_API_KEY = "51b0d52ef797830fc47dcc2d47eff0ff";
constexpr const char* DEFAULT_NTP_SERVER = "pool.ntp.org";
constexpr const char* DEFAULT_WIFI_SSID = "";
constexpr const char* DEFAULT_WIFI_PASS = "";
constexpr const char* DEFAULT_DEVICE_NAME = "DeskBuddy";
constexpr const char* FALLBACK_TIMEZONE = "IST-5:30";

extern String wifiSsid;
extern String wifiPass;
extern String city;
extern String countryCode;
extern String apiKey;
extern String timezone;
extern String ntpServer;
extern String deviceName;
// extern String photoData1;
// extern String photoData2;
extern unsigned long photoBoothInterval;
extern bool setupCompleted;

void init();
void load();
void save();
bool hasWifiConfig();
bool hasLocationConfig();
// Photos are stored in SPIFFS (NOT Preferences/NVS) to avoid ESP32 resets.
bool hasPhotos();
String getCityForApi();
String inferTimezone(const String& cityName, const String& country);
void refreshTimezoneFromLocation();
void markSetupCompleted();

}  // namespace Config

#endif
