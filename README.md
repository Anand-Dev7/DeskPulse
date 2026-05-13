# DeskBuddy Clock

DeskBuddy is an ESP32-C3 based smart desk clock with a 240x240 ST7789 TFT display, touch control, weather/forecast pages, configurable WiFi setup portal, adjustable brightness, and a 3-image PhotoBooth slideshow.

## Features

- Smart and simple clock views
- OpenWeather current weather and forecast
- Captive WiFi setup portal at `http://192.168.4.1`
- 3-photo PhotoBooth slideshow stored in SPIFFS
- Touch-based menu navigation
- Brightness popup/control
- Reconfigure WiFi from device menu
- Lightweight animated welcome screen

## Hardware

Tested target: **ESP32-C3 DevKitM-1** or compatible ESP32-C3 board.

Required modules:

- ESP32-C3 development board
- ST7789 240x240 SPI TFT display
- Touch sensor/button module
- Optional PWM-controllable TFT backlight pin
- USB cable/power supply

## Pin Connections

| Function | ESP32-C3 GPIO | Notes |
|---|---:|---|
| TFT SCLK | GPIO4 | SPI clock |
| TFT MOSI/SDA | GPIO7 | SPI data out |
| TFT DC | GPIO1 | Data/command |
| TFT RST | GPIO2 | Display reset |
| TFT CS | -1 | Not connected / tied active, as configured |
| TFT BL | GPIO5 | Backlight PWM control |
| Touch signal | GPIO3 | Active HIGH in current code |
| TFT VCC | 3.3V | Use display-compatible voltage |
| TFT GND | GND | Common ground |
| Touch VCC | 3.3V | Match touch module requirement |
| Touch GND | GND | Common ground |

> Important: keep all grounds common. If your display has CS connected, update `TFT_CS` in `main.cpp`.

## Software Dependencies

Install through PlatformIO/Arduino libraries as needed:

- Arduino framework for ESP32
- Adafruit GFX
- Adafruit ST7789
- U8g2 for Adafruit GFX
- JPEGDEC
- ArduinoJson
- WebServer / DNSServer / WiFi, included with ESP32 Arduino core
- SPIFFS support, included with ESP32 Arduino core

## Build and Upload

1. Open the project in PlatformIO.
2. Select your ESP32-C3 environment, for example `esp32-c3-devkitm-1`.
3. Clean and upload:

```bash
pio run -t clean
pio run -t upload
```

4. Open Serial Monitor at the configured baud rate.

## First-Time Setup

When no WiFi/location config exists, DeskBuddy starts a setup access point.

| Item | Value |
|---|---|
| AP SSID | `DeskBuddy-Setup` |
| AP Password | `12345678` |
| Portal URL | `http://192.168.4.1/` |

Steps:

1. Connect phone/laptop to `DeskBuddy-Setup`.
2. Open `http://192.168.4.1/` using HTTP, not HTTPS.
3. Select or type WiFi SSID.
4. Enter WiFi password.
5. Enter city and country code, for example `Hyderabad`, `IN`.
6. Upload up to 3 PhotoBooth images if desired.
7. Set slideshow interval; default is 10 seconds.
8. Press **Save Configuration**.

The portal uploads images in chunks and shows progress in the browser.

## Touch Controls

| Action | Behavior |
|---|---|
| Short tap on clock | Open menu / move through menu options |
| Long press on clock | Open PhotoBooth |
| Long press in PhotoBooth | Close PhotoBooth and return to clock |
| Short tap in PhotoBooth | Ignored |
| Long press on menu option | Select/open highlighted option |
| Brightness option long press | Open brightness popup |
| Short tap in brightness popup | Cycle brightness |
| Long press in brightness popup | Close popup and return to menu |
| Reconfigure WiFi option long press | Open setup portal |
| Long press during setup portal | Cancel setup portal and return to menu |

## Menu Options

- Clock
- Show Weather
- Brightness
- Reconfigure WiFi
- About
- Exit

## PhotoBooth

- Supports 3 images.
- Images are uploaded through the setup portal.
- Images are stored in SPIFFS as:
  - `/photo1.b64`
  - `/photo2.b64`
  - `/photo3.b64`
- The browser compresses/crops images to 240x240 before upload.
- Safety limits reject images that are too large for stable decoding.

## Troubleshooting

### Portal does not open

- Confirm you are connected to `DeskBuddy-Setup`.
- Open `http://192.168.4.1/`, not HTTPS.
- Check Serial Monitor for:

```txt
Setup AP started, IP=192.168.4.1
```

### Images do not update

- Re-upload images after flashing new portal code.
- Watch Serial Monitor for `Photo N chunk...` and `Photo N upload complete...`.
- If only `/save` appears and no chunk logs appear, refresh the portal page.

### PhotoBooth shows placeholder

- Image may be too large or decode failed.
- Re-upload through the portal so it is compressed/chunked safely.

### Weather not loading

- Confirm WiFi credentials, city, and country code.
- Confirm OpenWeather API key in `config.h`.

## Project Structure

| File | Purpose |
|---|---|
| `main.cpp` | Main lifecycle, touch handling, view routing |
| `clock.cpp/.h` | Clock rendering |
| `weather.cpp/.h` | Weather API logic |
| `setup_portal.cpp/.h` | WiFi/config/photo upload portal |
| `photobooth.cpp/.h` | JPEG decode and slideshow rendering |
| `menu.cpp/.h` | Device menu |
| `common.cpp/.h` | Shared display/touch/brightness helpers |
| `config.cpp/.h` | Preferences/config defaults |
| `welcome.cpp/.h` | Boot/status UI |
| `colors.h` | Color constants |
