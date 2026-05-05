#include "common.h"
#include "colors.h"

#include <WiFi.h>
#include <time.h>
#include <SPI.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <esp_system.h>
#include <Fonts/FreeSansBold9pt7b.h>

namespace {
	const int kBrightnessPwmChannel = 0;
	const int kBrightnessPwmFreq = 5000;
	const int kBrightnessPwmResolutionBits = 8;
	bool gBrightnessInitialized = false;
	bool gBrightnessInverted = false;
	Adafruit_ST7789* gBrightnessDisplay = nullptr;
}

void initPanelBrightnessControl(Adafruit_ST7789& display) {
	gBrightnessDisplay = &display;
}

void initBrightnessControl(int backlightPin, bool inverted) {
	if (backlightPin < 0) {
		gBrightnessInitialized = false;
		return;
	}

	ledcSetup(kBrightnessPwmChannel, kBrightnessPwmFreq, kBrightnessPwmResolutionBits);
	ledcAttachPin(backlightPin, kBrightnessPwmChannel);
	gBrightnessInverted = inverted;
	gBrightnessInitialized = true;
}

void applyBrightnessLevel(BrightnessLevel level) {
	int duty = 255;
	if (level == BRIGHTNESS_MEDIUM) {
		duty = 160;
	} else if (level == BRIGHTNESS_LOW) {
		duty = 70;
	}

	if (gBrightnessInitialized) {
		int pwmDuty = duty;
		if (gBrightnessInverted) {
			pwmDuty = 255 - pwmDuty;
		}
		ledcWrite(kBrightnessPwmChannel, pwmDuty);
	}

	if (gBrightnessDisplay != nullptr) {
		// Enable backlight/brightness control via display controller when supported.
		uint8_t ctrl = 0x2C;
		gBrightnessDisplay->sendCommand(0x53, &ctrl, 1);
		uint8_t panelDuty = static_cast<uint8_t>(duty);
		gBrightnessDisplay->sendCommand(0x51, &panelDuty, 1);
	}
}

bool isTouchTapped(uint8_t touchPin, bool activeLow) {
	static int lastReading = HIGH;
	static int stableState = HIGH;
	static unsigned long lastDebounceTime = 0;
	const unsigned long debounceMs = 30;

	int reading = digitalRead(touchPin);
	if (reading != lastReading) {
		lastDebounceTime = millis();
	}

	if ((millis() - lastDebounceTime) > debounceMs) {
		if (reading != stableState) {
			int prevState = stableState;
			stableState = reading;
			if (activeLow) {
				if (prevState == HIGH && stableState == LOW) {
					lastReading = reading;
					return true;
				}
			} else {
				if (prevState == LOW && stableState == HIGH) {
					lastReading = reading;
					return true;
				}
			}
		}
	}

	lastReading = reading;
	return false;
}

TouchEvent readTouchEvent(uint8_t touchPin, bool activeLow, unsigned long longPressMs) {
	static int lastReading = HIGH;
	static int stableState = HIGH;
	static unsigned long lastDebounceTime = 0;
	static unsigned long pressStartTime = 0;
	static bool longReported = false;
	static bool initialized = false;
	const unsigned long debounceMs = 20;

	int reading = digitalRead(touchPin);
	if (!initialized) {
		lastReading = reading;
		stableState = reading;
		initialized = true;
	}

	if (reading != lastReading) {
		lastDebounceTime = millis();
	}

	TouchEvent event = TOUCH_EVENT_NONE;

	if ((millis() - lastDebounceTime) > debounceMs) {
		if (reading != stableState) {
			int prevState = stableState;
			stableState = reading;

			bool prevPressed = activeLow ? (prevState == LOW) : (prevState == HIGH);
			bool nowPressed = activeLow ? (stableState == LOW) : (stableState == HIGH);

			if (!prevPressed && nowPressed) {
				pressStartTime = millis();
				longReported = false;
			} else if (prevPressed && !nowPressed) {
				if (!longReported) {
					event = TOUCH_EVENT_SHORT_TAP;
				}
				longReported = false;
			}
		}
	}

	bool currentlyPressed = activeLow ? (stableState == LOW) : (stableState == HIGH);
	if (currentlyPressed && !longReported && (millis() - pressStartTime >= longPressMs)) {
		longReported = true;
		event = TOUCH_EVENT_LONG_TAP;
	}

	lastReading = reading;
	return event;
}

void initDisplay(Adafruit_ST7789& display, int rstPin, int sclkPin, int mosiPin) {
	pinMode(rstPin, OUTPUT);
	digitalWrite(rstPin, HIGH);
	delay(50);
	digitalWrite(rstPin, LOW);
	delay(50);
	digitalWrite(rstPin, HIGH);
	delay(150);

	SPI.begin(sclkPin, -1, mosiPin, -1);
	display.init(240, 240, SPI_MODE3);
	display.setRotation(2);
	display.invertDisplay(true);
	display.fillScreen(COLOR_BLACK);
}

void connectWiFi(Adafruit_ST7789& display, const char* ssid, const char* password) {
	display.fillScreen(COLOR_BLACK);
	display.setTextColor(COLOR_CYAN);
	display.setTextSize(2);
	display.setCursor(30, 100);
	display.println("Connecting");

	WiFi.begin(ssid, password);

	int dots = 0;
	while (WiFi.status() != WL_CONNECTED) {
		delay(500);
		Serial.print(".");

		display.setCursor(30 + dots * 12, 130);
		display.setTextColor(COLOR_WHITE);
		display.print("-");
		dots = (dots + 1) % 10;
		if (dots == 0) {
			display.fillRect(30, 130, 120, 20, COLOR_BLACK);
		}
	}

	Serial.println("\nWiFi connected!");
	Serial.println(WiFi.localIP());

	display.fillScreen(COLOR_BLACK);
	display.setTextColor(COLOR_GREEN);
	display.setTextSize(2);
	display.setCursor(50, 100);
	display.println("Connected!");
	delay(1000);
}

void syncTime(Adafruit_ST7789& display, long gmtOffsetSec, int daylightOffsetSec, const char* ntpServer) {
	configTime(gmtOffsetSec, daylightOffsetSec, ntpServer);

	display.fillScreen(COLOR_BLACK);
	display.setTextColor(COLOR_YELLOW);
	display.setTextSize(2);
	display.setCursor(30, 100);
	display.println("Syncing time...");

	struct tm timeinfo;
	int retry = 0;
	while (!getLocalTime(&timeinfo) && retry < 10) {
		delay(500);
		retry++;
	}

	if (retry < 10) {
		Serial.println("Time synced!");
	}
}

void updateWeather(WeatherAPI& weatherApi, DeskPulseClock& clock) {
	if (weatherApi.update()) {
		WeatherData data = weatherApi.getData();
		clock.setWeather(data);
		Serial.println("Weather updated: " + data.city + " " + String(data.temperature) + "C");
	}
}

static const char* getWeekdayShortName(int wday) {
	static const char* kDays[] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
	return kDays[wday % 7];
}

bool fetchForecast(const char* apiKey, const char* city, const char* countryCode, ForecastDay outDays[3]) {
	if (WiFi.status() != WL_CONNECTED) {
		return false;
	}

	String cityQuery = city;
	if (countryCode != nullptr && strlen(countryCode) > 0) {
		cityQuery += ",";
		cityQuery += countryCode;
	}

	HTTPClient http;
	String url = "http://api.openweathermap.org/data/2.5/forecast?q=" + cityQuery + "&appid=" + String(apiKey) + "&units=metric";
	http.begin(url);
	int httpCode = http.GET();
	if (httpCode != 200) {
		Serial.println("Forecast fetch failed: " + String(httpCode));
		http.end();
		return false;
	}

	String payload = http.getString();
	http.end();

	DynamicJsonDocument doc(16384);
	DeserializationError error = deserializeJson(doc, payload);
	if (error) {
		Serial.println("Forecast JSON parse failed");
		return false;
	}

	struct tm timeinfo;
	int today = 0;
	if (getLocalTime(&timeinfo)) {
		today = timeinfo.tm_wday;
	}

	const int indices[3] = {7, 15, 23};
	for (int i = 0; i < 3; i++) {
		int idx = indices[i];
		outDays[i].temp = doc["list"][idx]["main"]["temp"].as<int>();
		outDays[i].condition = doc["list"][idx]["weather"][0]["main"].as<String>();
		outDays[i].dayName = getWeekdayShortName((today + i + 1) % 7);
	}

	return true;
}

void drawForecastPage(Adafruit_ST7789& display, const ForecastDay days[3], bool hasData) {
	drawWeatherForecastPage(display, days, hasData);
}

void drawAboutPage(Adafruit_ST7789& display) {
	display.fillScreen(COLOR_BLACK);
	display.fillRect(0, 0, 240, 28, COLOR_GREEN);
	display.setTextColor(COLOR_BLACK);
    display.setFont(&FreeSansBold9pt7b);
	display.setTextSize(1);
	display.setCursor(88, 20);
	display.print("ABOUT");

	esp_chip_info_t chip;
	esp_chip_info(&chip);

	uint32_t cpuFreq = getCpuFrequencyMhz();
	uint32_t freeHeap = esp_get_free_heap_size();
	uint32_t totalHeap = ESP.getHeapSize();
	uint32_t usedHeap = totalHeap - freeHeap;
	uint32_t flashTotal = ESP.getFlashChipSize() / 1024;
	uint32_t flashUsed = ESP.getSketchSize() / 1024;

	display.setTextColor(COLOR_WHITE);

	display.setTextSize(1);
	display.setCursor(10, 50);
	display.print("Board: ESP32-C3 SM");

	display.setCursor(10, 78);
	display.print("CPU: ");
	display.print(cpuFreq);
	display.print(" MHz");

	display.setCursor(10, 108);
	display.print("RAM: ");
	display.print(usedHeap / 1024);
	display.print("/");
	display.print(totalHeap / 1024);
	display.print(" KB");

	display.setCursor(10, 138);
	display.print("Flash: ");
	display.print(flashUsed);
	display.print("/");
	display.print(flashTotal);
	display.print(" KB");

	display.setCursor(10, 168);
	display.print("Cores: ");
	display.print(chip.cores);

	display.setCursor(10, 198);
	display.print("Chip Rev: ");
	display.print(chip.revision);
    display.setFont();
}

void drawBrightnessPopup(Adafruit_ST7789& display, BrightnessLevel level) {
	int levelIndex = 3;
	if (level == BRIGHTNESS_MEDIUM) levelIndex = 2;
	if (level == BRIGHTNESS_LOW) levelIndex = 1;

	const int boxX = 36;
	const int boxY = 150;
	const int boxW = 168;
	const int boxH = 72;

	display.fillRoundRect(boxX, boxY, boxW, boxH, 10, COLOR_BLACK);
	display.drawRoundRect(boxX, boxY, boxW, boxH, 10, COLOR_WHITE);
	display.setTextColor(COLOR_WHITE);
	display.setTextSize(2);
	display.setCursor(boxX + 16, boxY + 12);
	display.print("Brightness");

	display.drawRect(boxX + 16, boxY + 44, 136, 14, COLOR_WHITE);
	int fillW = (132 * levelIndex) / 3;
	display.fillRect(boxX + 18, boxY + 46, fillW, 10, COLOR_YELLOW);
}
