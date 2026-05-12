#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>
#include <WiFi.h>
#include <time.h>
#include <esp_task_wdt.h>
#include "clock.h"
#include "weather.h"
#include "menu.h"
#include "common.h"
#include "config.h"
#include "setup_portal.h"
#include "welcome.h"
#include "photobooth.h"
#include <SPIFFS.H>
#include <esp_system.h>

// ===== ESP32-C3 Super Mini pins =====
#define TFT_CS   -1
#define TFT_DC    1
#define TFT_RST   2
#define TFT_MOSI  7
#define TFT_SCLK  4
#define TOUCH_PIN 3   //Touch
#define TFT_BL    5   // Backlight PWM pin (set to -1 if your display BL is not connected)

// --- Captive Portal Reboot Flags ---
bool shouldReboot = false;
unsigned long rebootTime = 0;

// Add these as global variables in main.cpp
bool isPhotoMode = false;
bool lockToggle = false; 
unsigned long touchStartTime = 0;

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

// void reloadPhotosFromConfig() {
//   // Reload photo data from config into PhotoBooth
//   PhotoData photo1, photo2;
//   photo1.base64Data = Config::photoData1;
//   photo1.valid = !Config::photoData1.isEmpty();
//   photo2.base64Data = Config::photoData2;
//   photo2.valid = !Config::photoData2.isEmpty();
//   photoBooth.setPhotos(photo1, photo2);
//   photoBooth.setInterval(Config::photoBoothInterval);
// }

void reloadPhotosFromConfig() {
  // PhotoData photo1, photo2;
  // Reload photo data from SPIFFS into PhotoBooth (photos are NOT stored in NVS)
  // File f1 = SPIFFS.open("/photo1.b64", FILE_READ);
  // if (f1) {
  //   photo1.base64Data = f1.readString();
  //   photo1.valid = true;
  //   f1.close();
  // } else {
  //   photo1.valid = false;
  // }


  
  PhotoData photo1, photo2, photo3;
  photo1.valid = false;
  photo2.valid = false;
  photo3.valid = false;

  // SPIFFS is mounted in setup() via SPIFFS.begin(true)

  
if (!SPIFFS.begin(false)) {
    Serial.println("SPIFFS not mounted; no photos loaded");
  } else {
    File f1 = SPIFFS.open("/photo1.b64", FILE_READ);
    if (f1) {
      photo1.base64Data = f1.readString();
      photo1.valid = !photo1.base64Data.isEmpty();
      f1.close();
      Serial.println("Loaded photo1 from SPIFFS: " + String(photo1.base64Data.length()) + " chars");
    }

    File f2 = SPIFFS.open("/photo2.b64", FILE_READ);
    if (f2) {
      photo2.base64Data = f2.readString();
      photo2.valid = !photo2.base64Data.isEmpty();
      f2.close();
      Serial.println("Loaded photo2 from SPIFFS: " + String(photo2.base64Data.length()) + " chars");
    }

    File f3 = SPIFFS.open("/photo3.b64", FILE_READ);
    if (f3) {
      photo3.base64Data = f3.readString();
      photo3.valid = !photo3.base64Data.isEmpty();
      f3.close();
      Serial.println("Loaded photo3 from SPIFFS: " + String(photo3.base64Data.length()) + " chars");
    }
  }


  photoBooth.setPhotos(photo1, photo2, photo3);
  photoBooth.setInterval(Config::photoBoothInterval);
}

bool startSetupPortalUntilSaved(bool allowCancel = false) {
  SetupPortal portal(tft);
  portal.begin();
  unsigned long completedTime = 0;
  const unsigned long COMPLETION_WAIT_MS = 2500;  // Give browser time to receive /save response
  bool canceled = false;
  
  Serial.println(allowCancel ? "Setup portal started; long press touch to cancel..." : "Setup portal started; waiting for explicit Save button...");

  while (true) {
    portal.loop();
    esp_task_wdt_reset();

    if (allowCancel && readTouchEvent(TOUCH_PIN, false, TOUCH_LONG_PRESS_MS) == TOUCH_EVENT_LONG_TAP) {
      canceled = true;
      Serial.println("Setup portal canceled by device long press");
      break;
    }

    // IMPORTANT: No idle timeout here. The portal must stay open until /save succeeds or long-press cancels.
    if (portal.isCompleted() && completedTime == 0) {
      completedTime = millis();
      Serial.println("Portal reports saved; closing setup portal shortly...");
    }

    if (completedTime > 0 && (millis() - completedTime >= COMPLETION_WAIT_MS)) {
      break;
    }

    delay(10);
  }
  
  portal.stop();
  delay(300);
  WiFi.mode(WIFI_STA);
  Serial.println(canceled ? "Setup portal stopped after cancel; STA mode restored" : "Setup portal stopped after explicit save; STA mode restored");
  
  if (!canceled && completedTime > 0) {
    Config::load();
    reloadPhotosFromConfig();
    return true;
  }

  return false;
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
    esp_task_wdt_reset();  // Feed watchdog during WiFi connection wait
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
    
    esp_reset_reason_t r = esp_reset_reason();
    Serial.printf("Reset reason = %d\n", (int)r);

    Serial.println("=== DeskBuddy Clock ===");

    // Mount SPIFFS for photo storage (photos are stored here, not in NVS
    if (!SPIFFS.begin(true)) {
      Serial.println("SPIFFS mount failed");
    }else {
      
    Serial.println("SPIFFS mounted");

  } 


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
  reloadPhotosFromConfig();
  // PhotoData photo1, photo2;
  // photo1.base64Data = Config::photoData1;
  // photo1.valid = !Config::photoData1.isEmpty();
  // photo2.base64Data = Config::photoData2;
  // photo2.valid = !Config::photoData2.isEmpty();
  // photoBooth.setPhotos(photo1, photo2);
  // photoBooth.setInterval(Config::photoBoothInterval);

  // Initialize clock display
  gmClock.begin();
  
  Serial.println("Clock started!");
}

void loop() {
  static bool menuPreviouslyOpen = false;
  static bool forceMenuRedraw = false;
  static bool forceViewRedraw = false;
  static bool weatherDrawn = false;
  static bool aboutDrawn = false;
  static bool popupNeedsRedraw = true;

  struct tm timeinfo;
  #if 0

  // 1. CUSTOM MANUAL TOUCH LOGIC (PhotoBooth 3-sec toggle)
  int touchValue = digitalRead(TOUCH_PIN); 

  if (touchValue == HIGH) { 
    if (touchStartTime == 0) touchStartTime = millis();
    
    // Check if held for 3 seconds
    if (millis() - touchStartTime > 3000 && !lockToggle) {
      lockToggle = true; 

      // TOGGLE LOGIC: Switch between Clock and PhotoBooth
      if (currentView != VIEW_PHOTOBOOTH) {
          Serial.println("Switching to Photo Mode");
          currentView = VIEW_PHOTOBOOTH;
          photoBooth.begin(); 
      } else {
          Serial.println("Returning to Clock Mode");
          currentView = VIEW_CLOCK;
          photoBooth.stop();  
          gmClock.begin();    
      }
    }
  } else {
    touchStartTime = 0;
    lockToggle = false; 
  }
  #endif

  // 2. STANDARD TOUCH EVENT PROCESSING (Read touches FIRST)
  TouchEvent touchEvent = readTouchEvent(TOUCH_PIN, false, TOUCH_LONG_PRESS_MS);

  if (brightnessAdjustMode) {
    if (touchEvent == TOUCH_EVENT_SHORT_TAP) {
      cycleBrightnessLevel();
      forceViewRedraw = true;
      popupNeedsRedraw = true;
    } else if (touchEvent == TOUCH_EVENT_LONG_TAP) {
      brightnessAdjustMode = false;
      brightnessPopupUntil = 0;
      menuOpen();
      forceMenuRedraw = true;
      forceViewRedraw = false;
      popupNeedsRedraw = false;
    }
    // Brightness popup stays open until long press closes it.
  } 
  else if (currentView == VIEW_PHOTOBOOTH) {
    // If in Photobooth, any tap exits back to clock
    if (touchEvent == TOUCH_EVENT_SHORT_TAP || touchEvent == TOUCH_EVENT_LONG_TAP) {
      currentView = VIEW_CLOCK;
      photoBooth.stop();
      gmClock.begin();
      forceViewRedraw = true;
    }
  }
  else if (!menuIsOpen()) {
    // Standard touches when menu is CLOSED
    if (touchEvent == TOUCH_EVENT_SHORT_TAP) {
      menuOpen();
      forceMenuRedraw = true;
    } else if (touchEvent == TOUCH_EVENT_LONG_TAP && currentView == VIEW_CLOCK) {
      // currentView = VIEW_PHOTOBOOTH;
      // photoBooth.begin();
      // forceViewRedraw = true;
      
      static unsigned long lastEnter = 0;
      if (millis() - lastEnter > 1500) {   // ignore repeated triggers for 1.5s
        lastEnter = millis();
        currentView = VIEW_PHOTOBOOTH;
        photoBooth.begin();
        forceViewRedraw = true;
      }

    }
  } 
  else {
    // Menu is OPEN - Handle Menu Toggles
    if (touchEvent == TOUCH_EVENT_SHORT_TAP) {
      menuHandleSingleTap();
      forceMenuRedraw = true;
    } else if (touchEvent == TOUCH_EVENT_LONG_TAP) {
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
        case MENU_ACTION_BRIGHTNESS_LEVEL:
          // Keep the menu open and draw the brightness popup on top of it.
          brightnessAdjustMode = true;
          brightnessPopupUntil = 0;
          forceMenuRedraw = true;
          forceViewRedraw = false;
          popupNeedsRedraw = true;
          break;
        case MENU_ACTION_RECONFIGURE_WIFI:
          menuClose();
          {
            bool saved = startSetupPortalUntilSaved(true);
            if (saved) {
              Config::load();
            }
          }
          // Return to menu options whether the portal was saved or canceled.
          menuOpen();
          menuPreviouslyOpen = false;
          forceMenuRedraw = true;
          forceViewRedraw = false;
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

  // 3. DRAWING & VIEW MANAGEMENT (Now it's safe to return early)
  
  // -- Menu View --
  if (brightnessAdjustMode) {
    if (!menuIsOpen()) {
      menuOpen();
      forceMenuRedraw = true;
    }
    if (!menuPreviouslyOpen || forceMenuRedraw || popupNeedsRedraw) {
      menuDraw(tft);
      drawBrightnessPopup(tft, brightnessLevel);
      menuPreviouslyOpen = true;
      forceMenuRedraw = false;
      popupNeedsRedraw = false;
    }
    delay(20);
    return;
  }

  if (menuIsOpen()) {
    if (!menuPreviouslyOpen || forceMenuRedraw) {
      menuDraw(tft);
      menuPreviouslyOpen = true;
      forceMenuRedraw = false;
    }
    delay(20);
    return;
  }

  if (menuPreviouslyOpen) {
    menuPreviouslyOpen = false;
    forceViewRedraw = true;
  }


  // -- Photobooth View --
  if (currentView == VIEW_PHOTOBOOTH) {
    photoBooth.update(); 
    delay(20);
    return;
  }

  // -- Weather Forecast View --
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
    delay(20);
    return; 
  }

  // -- About View --
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
    delay(20);
    return;
  }

  // -- DEFAULT VIEW: CLOCK --
  weatherDrawn = false;
  aboutDrawn = false;

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


  delay(20);
}