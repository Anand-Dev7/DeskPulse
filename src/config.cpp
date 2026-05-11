#include "config.h"
#include <esp_task_wdt.h>

namespace Config {

String wifiSsid = "";
String wifiPass = "";
String city = "";
String countryCode = "";
String apiKey = "";
String timezone = "";
String ntpServer = "";
String deviceName = "";
String photoData1 = "";
String photoData2 = "";
unsigned long photoBoothInterval = 3000;  // Default 3 seconds
bool setupCompleted = false;

static Preferences prefs;
static const char* PREF_NAMESPACE = "DeskBuddy";

namespace {

String normalizeUpper(const String& input) {
    String out = input;
    out.trim();
    out.toUpperCase();
    return out;
}

bool cityContains(const String& cityName, const char* key) {
    String cityUpper = normalizeUpper(cityName);
    String keyUpper = normalizeUpper(String(key));
    return cityUpper.indexOf(keyUpper) >= 0;
}

}  // namespace

void init() {
    load();
}

void load() {
    prefs.begin(PREF_NAMESPACE, true);
    wifiSsid = prefs.getString("ssid", DEFAULT_WIFI_SSID);
    wifiPass = prefs.getString("pass", DEFAULT_WIFI_PASS);
    city = prefs.getString("city", DEFAULT_CITY);
    countryCode = prefs.getString("country", DEFAULT_COUNTRY);
    timezone = prefs.getString("tz", "");
    deviceName = prefs.getString("name", DEFAULT_DEVICE_NAME);
    photoData1 = prefs.getString("photo1", "");
    photoData2 = prefs.getString("photo2", "");
    photoBoothInterval = prefs.getULong("pbInterval", 3000);
    setupCompleted = prefs.getBool("setupDone", false);
    prefs.end();

    apiKey = DEFAULT_API_KEY;
    ntpServer = DEFAULT_NTP_SERVER;

    if (timezone.isEmpty()) {
        timezone = inferTimezone(city, countryCode);
    }
    if (ntpServer.isEmpty()) ntpServer = DEFAULT_NTP_SERVER;
    if (deviceName.isEmpty()) deviceName = DEFAULT_DEVICE_NAME;
}

// void save() {
//     refreshTimezoneFromLocation();

//     prefs.begin(PREF_NAMESPACE, false);
    
//     prefs.putString("ssid", wifiSsid);
//     esp_task_wdt_reset();
//     delay(50);
    
//     prefs.putString("pass", wifiPass);
//     esp_task_wdt_reset();
//     delay(50);
    
//     prefs.putString("city", city);
//     prefs.putString("country", countryCode);
//     prefs.putString("tz", timezone);
//     prefs.putString("name", deviceName);
//     esp_task_wdt_reset();
//     delay(50);
    
//     // Save photo data with extra watchdog feeding and delays
//     if (!photoData1.isEmpty()) {
//         Serial.println("Saving photo1 (" + String(photoData1.length()) + " bytes)...");
//         esp_task_wdt_reset();
//         delay(100);
//         prefs.putString("photo1", photoData1);
//         esp_task_wdt_reset();
//         delay(100);
//         Serial.println("Photo1 saved");
//     }
    
//     if (!photoData2.isEmpty()) {
//         Serial.println("Saving photo2 (" + String(photoData2.length()) + " bytes)...");
//         esp_task_wdt_reset();
//         delay(100);
//         prefs.putString("photo2", photoData2);
//         esp_task_wdt_reset();
//         delay(100);
//         Serial.println("Photo2 saved");
//     }
    
//     prefs.putULong("pbInterval", photoBoothInterval);
//     prefs.putBool("setupDone", setupCompleted);
//     esp_task_wdt_reset();
//     delay(50);
    
//     Serial.println("Closing preferences...");
//     prefs.end();
//     esp_task_wdt_reset();
// }

void save() {
    refreshTimezoneFromLocation();

    prefs.begin(PREF_NAMESPACE, false);
    
    // 1. SAVE CRITICAL DATA FIRST
    prefs.putString("ssid", wifiSsid);
    prefs.putString("pass", wifiPass);
    prefs.putString("city", city);
    prefs.putString("country", countryCode);
    prefs.putString("tz", timezone);
    prefs.putString("name", deviceName);
    prefs.putULong("pbInterval", photoBoothInterval);
    
    // 2. SAVE THE SETUP FLAG NOW 
    // Even if photos fail, the device will know setup is done!
    prefs.putBool("setupDone", true); 
    
    esp_task_wdt_reset();
    delay(100);

    // 3. TRY TO SAVE PHOTOS (This might still fail if images are too large)
    if (!photoData1.isEmpty() && photoData1.length() < 4000) {
        prefs.putString("photo1", photoData1);
    } else if (photoData1.length() >= 4000) {
        Serial.println("ERROR: Photo 1 too large for NVS!");
    }
    
    if (!photoData2.isEmpty() && photoData2.length() < 4000) {
        prefs.putString("photo2", photoData2);
    }

    prefs.end();
    Serial.println("Configuration Saved!");
}

bool hasWifiConfig() {
    return !wifiSsid.isEmpty() && !wifiPass.isEmpty();
}

bool hasLocationConfig() {
    return !city.isEmpty() && !countryCode.isEmpty();
}

String getCityForApi() {
    return city + "," + countryCode;
}

String inferTimezone(const String& cityName, const String& country) {
    String cc = normalizeUpper(country);

    if (cc == "IN") return "IST-5:30";
    if (cc == "GB" || cc == "UK") return "GMT0BST,M3.5.0/1,M10.5.0";
    if (cc == "DE" || cc == "FR" || cc == "ES" || cc == "IT" || cc == "NL") return "CET-1CEST,M3.5.0,M10.5.0/3";
    if (cc == "AE" || cc == "OM") return "GST-4";
    if (cc == "SG") return "SGT-8";
    if (cc == "JP") return "JST-9";
    if (cc == "KR") return "KST-9";
    if (cc == "CN") return "CST-8";
    if (cc == "RU") return "MSK-3";
    if (cc == "BR") return "BRT3BRST,M11.1.0/0,M2.3.0/0";

    if (cc == "AU") {
        if (cityContains(cityName, "PERTH")) return "AWST-8";
        if (cityContains(cityName, "BRISBANE")) return "AEST-10";
        return "AEST-10AEDT,M10.1.0,M4.1.0/3";
    }

    if (cc == "US" || cc == "USA") {
        if (cityContains(cityName, "CHICAGO") || cityContains(cityName, "DALLAS") || cityContains(cityName, "HOUSTON")) {
            return "CST6CDT,M3.2.0,M11.1.0";
        }
        if (cityContains(cityName, "DENVER") || cityContains(cityName, "PHOENIX")) {
            return "MST7MDT,M3.2.0,M11.1.0";
        }
        if (cityContains(cityName, "LOS ANGELES") || cityContains(cityName, "SAN FRANCISCO") || cityContains(cityName, "SEATTLE")) {
            return "PST8PDT,M3.2.0,M11.1.0";
        }
        return "EST5EDT,M3.2.0,M11.1.0";
    }

    return FALLBACK_TIMEZONE;
}

void refreshTimezoneFromLocation() {
    timezone = inferTimezone(city, countryCode);
}

bool hasPhotos() {
    return !photoData1.isEmpty() || !photoData2.isEmpty();
}

void markSetupCompleted() {
    setupCompleted = true;
    save();
}

}  // namespace Config
