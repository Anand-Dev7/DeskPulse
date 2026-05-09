#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>
#include <WiFi.h>
#include <time.h>
#include "clock.h"
#include "weather.h"
#include "menu.h"
#include "common.h"
#include "config.h"
#include "setup_portal.h"
#include "welcome.h"
#include "photobooth.h"

// ===== ESP32-C3 Super Mini pins =====
#define TFT_CS   -1
#define TFT_DC    1
#define TFT_RST   2
#define TFT_MOSI  7
#define TFT_SCLK  4
#define TOUCH_PIN 3   //Touch
#define TFT_BL    5   // Backlight PWM pin (set to -1 if your display BL is not connected)

const unsigned long TOUCH_LONG_PRESS_MS = 800;
const unsigned long BRIGHTNESS_POPUP_MS = 5000;
const unsigned long FORECAST_RETRY_INTERVAL = 15000;

enum ViewMode {
  VIEW_CLOCK = 0,
  VIEW_WEATHER,
  VIEW_ABOUT,
  VIEW_PHOTOBOOTH
};

Adafruit_ST7789 tft = Adafruit_ST7789(TFT_CS, TFT_DC, TFT_RST);
DeskPulseClock gmClock(&tft);
WeatherAPI weatherApi("", "", "");
PhotoBooth photoBooth(&tft);

unsigned long lastWeatherUpdate = 0;
unsigned long lastForecastUpdate = 0;
unsigned long brightnessPopupUntil = 0;
BrightnessLevel brightnessLevel = BRIGHTNESS_HIGH;
const unsigned long WEATHER_UPDATE_INTERVAL = 600000;  // 10 minutes
ViewMode currentView = VIEW_CLOCK;
ForecastDay forecastDays[3];
bool forecastValid = false;
bool brightnessAdjustMode = false;

void startSetupPortalUntilSaved() {
  SetupPortal portal(tft);
  portal.begin();
  while (!portal.isCompleted()) {
    portal.loop();
    delay(10);
  }
  portal.stop();
}

bool connectConfiguredWiFi() {
  if (!Config::hasWifiConfig()) {
    return false;
  }

  WelcomeUI::showWifiStatus(tft, "Connecting WiFi", Config::wifiSsid, "Please wait...");

  WiFi.mode(WIFI_STA);
  WiFi.begin(Config::wifiSsid.c_str(), Config::wifiPass.c_str());

  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 40) {
    delay(500);
    attempts++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    WelcomeUI::showWifiStatus(tft, "WiFi Connected", WiFi.localIP().toString(), Config::city + "," + Config::countryCode);
    delay(800);
    return true;
  }

  WelcomeUI::showWifiStatus(tft, "WiFi Failed", "Open setup portal", "192.168.4.1");
  delay(1000);
  return false;
}

bool syncTimeFromConfig() {
  WelcomeUI::showWifiStatus(tft, "Syncing Time", Config::timezone, Config::ntpServer);

  configTzTime(Config::timezone.c_str(), Config::ntpServer.c_str(), "time.nist.gov");
  struct tm timeinfo;
  int tries = 0;
  while (!getLocalTime(&timeinfo) && tries < 30) {
    delay(300);
    tries++;
  }

  bool synced = getLocalTime(&timeinfo);
  if (!synced) {
    WelcomeUI::showWifiStatus(tft, "Time Sync Failed", "Using default clock", "");
    delay(900);
  }
  return synced;
}

bool refreshForecastIfNeeded(bool forceNow) {
  if (WiFi.status() != WL_CONNECTED) {
    return false;
  }

  unsigned long interval = forecastValid ? WEATHER_UPDATE_INTERVAL : FORECAST_RETRY_INTERVAL;
  if (!forceNow && (millis() - lastForecastUpdate <= interval)) {
    return false;
  }

  bool ok = weatherApi.weatherForecast(forecastDays);
  lastForecastUpdate = millis();
  if (ok) {
    forecastValid = true;
    return true;
  }

  return false;
}

void cycleBrightnessLevel() {
  if (brightnessLevel == BRIGHTNESS_HIGH) {
    brightnessLevel = BRIGHTNESS_MEDIUM;
  } else if (brightnessLevel == BRIGHTNESS_MEDIUM) {
    brightnessLevel = BRIGHTNESS_LOW;
  } else {
    brightnessLevel = BRIGHTNESS_HIGH;
  }
  applyBrightnessLevel(brightnessLevel);
  brightnessPopupUntil = millis() + BRIGHTNESS_POPUP_MS;
}

void reverseBrightnessLevel() {
  if (brightnessLevel == BRIGHTNESS_HIGH) {
    brightnessLevel = BRIGHTNESS_LOW;
  } else if (brightnessLevel == BRIGHTNESS_MEDIUM) {
    brightnessLevel = BRIGHTNESS_HIGH;
  } else {
    brightnessLevel = BRIGHTNESS_MEDIUM;
  }
  applyBrightnessLevel(brightnessLevel);
  brightnessPopupUntil = millis() + BRIGHTNESS_POPUP_MS;
}

void setup() {
  Serial.begin(115200);
  delay(2000);
  Serial.println("=== DeskBuddy Clock ===");

  pinMode(TOUCH_PIN, INPUT);
  menuInit();
  
  initDisplay(tft, TFT_RST, TFT_SCLK, TFT_MOSI);
  initPanelBrightnessControl(tft);
  initBrightnessControl(TFT_BL, false);
  applyBrightnessLevel(brightnessLevel);
  WelcomeUI::showBootAnimation(tft, "DeskBuddy");

  Config::init();
  
  // Only show setup portal if setup hasn't been completed
  bool needsSetup = !Config::setupCompleted || !Config::hasWifiConfig() || !Config::hasLocationConfig();
  
  if (needsSetup) {
    WelcomeUI::showWifiStatus(tft, "No WiFi Saved", "Starting setup AP", "192.168.4.1");
    startSetupPortalUntilSaved();
    Config::load();
    Config::markSetupCompleted();  // Mark as completed after first setup
  }

  while (!connectConfiguredWiFi()) {
    startSetupPortalUntilSaved();
    Config::load();
  }

  syncTimeFromConfig();
  weatherApi.configure(Config::apiKey, Config::city, Config::countryCode);

  WelcomeUI::showWifiStatus(tft, "Loading Weather", Config::getCityForApi(), "");

  // Get initial weather data
  updateWeather(weatherApi, gmClock);
  
  // Get initial forecast data
  if (WiFi.status() == WL_CONNECTED) {
    refreshForecastIfNeeded(true);
  }

  // Initialize photo booth with saved photos and interval
  PhotoData photo1, photo2;
  photo1.base64Data = Config::photoData1;
  photo1.valid = !Config::photoData1.isEmpty();
  photo2.base64Data = Config::photoData2;
  photo2.valid = !Config::photoData2.isEmpty();
  photoBooth.setPhotos(photo1, photo2);
  photoBooth.setInterval(Config::photoBoothInterval);

  // Initialize clock display
  gmClock.begin();
  
  Serial.println("Clock started!");
}

void loop() {
  static bool menuPreviouslyOpen = false;
  static int lastMenuWifiStatus = -1;
  static bool forceMenuRedraw = false;
  static bool forceViewRedraw = false;
  static bool weatherDrawn = false;
  static bool aboutDrawn = false;
  static bool popupNeedsRedraw = true;

  TouchEvent touchEvent = readTouchEvent(TOUCH_PIN, false, TOUCH_LONG_PRESS_MS);

  if (brightnessAdjustMode) {
    if (touchEvent == TOUCH_EVENT_SHORT_TAP) {
      cycleBrightnessLevel();
      forceViewRedraw = true;
      popupNeedsRedraw = true;
    } else if (touchEvent == TOUCH_EVENT_LONG_TAP) {
      // Long tap: close brightness adjustment and open menu
      brightnessAdjustMode = false;
      brightnessPopupUntil = 0;
      menuOpen();
      forceMenuRedraw = true;
      forceViewRedraw = false;
      popupNeedsRedraw = false;
    }

    if (millis() > brightnessPopupUntil) {
      brightnessAdjustMode = false;
      forceViewRedraw = true;
      popupNeedsRedraw = true;
    }
  }

  if (!brightnessAdjustMode && !menuIsOpen()) {
    if (touchEvent == TOUCH_EVENT_SHORT_TAP) {
      menuOpen();
      forceMenuRedraw = true;
    } else if (touchEvent == TOUCH_EVENT_LONG_TAP && currentView == VIEW_CLOCK) {
      // Long tap on clock: open photo booth
      currentView = VIEW_PHOTOBOOTH;
      photoBooth.begin();
      forceViewRedraw = true;
    }
  } else if (!brightnessAdjustMode) {
    if (touchEvent == TOUCH_EVENT_SHORT_TAP) {
      // Short tap: move to next menu option.
      menuHandleSingleTap();
      forceMenuRedraw = true;
    } else if (touchEvent == TOUCH_EVENT_LONG_TAP) {
      // Long tap: run selected menu action.
      MenuAction action = menuHandleLongTap();
      switch (action) {
        case MENU_ACTION_SWITCH_CLOCK:
          gmClock.toggleClockStyle();
          currentView = VIEW_CLOCK;
          menuClose();
          gmClock.begin();
          forceViewRedraw = true;
          break;
        case MENU_ACTION_WEATHER_FORECAST:
          currentView = VIEW_WEATHER;
          menuClose();
          refreshForecastIfNeeded(true);
          forceViewRedraw = true;
          break;
        case MENU_ACTION_WIFI_OPTION:
          if (WiFi.status() == WL_CONNECTED) {
            WiFi.disconnect(true, true);
            WiFi.mode(WIFI_OFF);
            menuSetWifiEnabled(false);
          } else {
            if (!Config::hasWifiConfig() || !Config::hasLocationConfig()) {
              startSetupPortalUntilSaved();
              Config::load();
            }
            connectConfiguredWiFi();
            syncTimeFromConfig();
            weatherApi.configure(Config::apiKey, Config::city, Config::countryCode);
            updateWeather(weatherApi, gmClock);
            refreshForecastIfNeeded(true);
            menuSetWifiEnabled(WiFi.status() == WL_CONNECTED);
          }
          forceMenuRedraw = true;
          break;
        case MENU_ACTION_BRIGHTNESS_LEVEL:
          menuClose();
          brightnessAdjustMode = true;
          brightnessPopupUntil = millis() + BRIGHTNESS_POPUP_MS;
          forceViewRedraw = true;
          popupNeedsRedraw = true;
          break;
        case MENU_ACTION_ABOUT:
          currentView = VIEW_ABOUT;
          menuClose();
          forceViewRedraw = true;
          break;
        case MENU_ACTION_EXIT_TO_CLOCK:
          currentView = VIEW_CLOCK;
          menuClose();
          gmClock.begin();
          forceViewRedraw = true;
          break;
        default:
          Serial.println("Menu action: none");
          forceMenuRedraw = true;
          break;
      }
    }
  }

  if (menuIsOpen()) {
    int wifiStatus = WiFi.status();
    if (!menuPreviouslyOpen || wifiStatus != lastMenuWifiStatus || forceMenuRedraw) {
      menuSetWifiEnabled(wifiStatus == WL_CONNECTED);
      menuDraw(tft);
      lastMenuWifiStatus = wifiStatus;
      menuPreviouslyOpen = true;
      forceMenuRedraw = false;
    }
    delay(20);
    return;
  }

  if (menuPreviouslyOpen) {
    menuPreviouslyOpen = false;
    lastMenuWifiStatus = -1;
    forceViewRedraw = true;
  }

  if (brightnessAdjustMode && currentView == VIEW_CLOCK) {
    // Keep the base clock frame stable while adjusting brightness.
    if (forceViewRedraw) {
      gmClock.begin();
      forceViewRedraw = false;
      popupNeedsRedraw = true;
    }
    if (popupNeedsRedraw) {
      drawBrightnessPopup(tft, brightnessLevel);
      popupNeedsRedraw = false;
    }
    delay(20);
    return;
  }

  if (currentView == VIEW_PHOTOBOOTH) {
    // Handle touch events in photo booth
    if (touchEvent == TOUCH_EVENT_SHORT_TAP || touchEvent == TOUCH_EVENT_LONG_TAP) {
      // Any touch exits photo booth
      currentView = VIEW_CLOCK;
      photoBooth.stop();
      gmClock.begin();
      forceViewRedraw = true;
    } else {
      // Update photo booth slideshow
      photoBooth.update();
    }
    delay(20);
    return;
  }

  if (currentView == VIEW_WEATHER) {
    if (!weatherDrawn) {
      forceViewRedraw = true;
      weatherDrawn = true;
      aboutDrawn = false;
    }

    if (refreshForecastIfNeeded(false)) {
      forceViewRedraw = true;
    }
    if (forceViewRedraw) {
      drawForecastPage(tft, forecastDays, forecastValid);
      forceViewRedraw = false;
      popupNeedsRedraw = true;
    }
    // No brightness popup on weather view
    delay(20);
    return;
  }

  if (currentView == VIEW_ABOUT) {
    if (!aboutDrawn) {
      forceViewRedraw = true;
      aboutDrawn = true;
      weatherDrawn = false;
    }

    if (forceViewRedraw) {
      drawAboutPage(tft);
      forceViewRedraw = false;
      popupNeedsRedraw = true;
    }
    // No brightness popup on about view
    delay(20);
    return;
  }

  weatherDrawn = false;
  aboutDrawn = false;

  struct tm timeinfo;
  if (getLocalTime(&timeinfo)) {
    gmClock.setTime(timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);
    gmClock.setDate(timeinfo.tm_mday, timeinfo.tm_mon + 1, timeinfo.tm_year + 1900, timeinfo.tm_wday);
    gmClock.update();
  }

  // Update weather every 10 minutes
  if (millis() - lastWeatherUpdate > WEATHER_UPDATE_INTERVAL) {
    updateWeather(weatherApi, gmClock);
    lastWeatherUpdate = millis();
  }

  if (brightnessAdjustMode || millis() < brightnessPopupUntil) {
    if (popupNeedsRedraw) {
      drawBrightnessPopup(tft, brightnessLevel);
      popupNeedsRedraw = false;
    }
  }

  delay(20);
}