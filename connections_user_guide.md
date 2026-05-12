# DeskBuddy Connection and User Guide

This guide explains how to wire, configure, and operate the DeskBuddy smart clock.

## 1. Wiring Guide

### TFT Display Wiring

| ST7789 Pin | ESP32-C3 Pin | Description |
|---|---:|---|
| VCC | 3.3V | Display power |
| GND | GND | Ground |
| SCL / SCLK | GPIO4 | SPI clock |
| SDA / MOSI | GPIO7 | SPI data |
| DC | GPIO1 | Data/command |
| RST | GPIO2 | Reset |
| CS | Not connected / tied active | Code uses `TFT_CS = -1` |
| BL / LED | GPIO5 | PWM backlight control |

### Touch Sensor Wiring

| Touch Module Pin | ESP32-C3 Pin | Description |
|---|---:|---|
| VCC | 3.3V | Power |
| GND | GND | Ground |
| OUT / SIG | GPIO3 | Touch input, active HIGH |

## 2. Power-On Flow

1. DeskBuddy shows boot animation.
2. It loads saved WiFi/config from Preferences.
3. If setup is missing, it starts `DeskBuddy-Setup` AP.
4. If setup exists, it connects to WiFi, syncs time, loads weather, loads photos, then starts clock.

## 3. Setup Portal

### Connect

1. Connect to WiFi network:

```txt
DeskBuddy-Setup
```

Password:

```txt
12345678
```

2. Open:

```txt
http://192.168.4.1/
```

Use HTTP, not HTTPS.

### Configure

In the portal:

- Select or type WiFi SSID.
- Enter WiFi password.
- Enter city.
- Enter country code.
- Upload up to 3 PhotoBooth images.
- Set slideshow interval; default is 10 seconds.
- Click **Save Configuration**.

During image upload, browser progress should show chunk status.

## 4. Device Controls

### Clock Screen

| Gesture | Action |
|---|---|
| Short tap | Open menu |
| Long press | Open PhotoBooth |

### PhotoBooth

| Gesture | Action |
|---|---|
| Long press | Exit PhotoBooth and return to clock |
| Short tap | No action |

### Menu

| Gesture | Action |
|---|---|
| Short tap | Move selection |
| Long press | Open selected option |

### Brightness Popup

| Gesture | Action |
|---|---|
| Short tap | Cycle brightness level |
| Long press | Close brightness popup and return to menu |

### Reconfigure WiFi

1. Open menu.
2. Select **Reconfigure WiFi**.
3. Long press to open setup portal.
4. Use web portal to save, or long press device touch sensor again to cancel and return to menu.

## 5. PhotoBooth Image Tips

For best stability:

- Use JPG or PNG source images.
- Let the setup portal compress images automatically.
- Re-upload images after flashing portal changes.
- Avoid manually placing large files into SPIFFS.

Expected serial logs during upload:

```txt
Photo 1 chunk 1/...
Photo 1 upload complete: ... chars
Photo 2 upload complete: ... chars
Photo 3 upload complete: ... chars
/save POST received: args=1, raw=... chars
```

## 6. Serial Monitor Checks

Useful boot/setup logs:

```txt
=== DeskBuddy Clock ===
SPIFFS mounted
Setup AP started, IP=192.168.4.1
Clock started!
```

Useful PhotoBooth logs:

```txt
[PhotoBooth] Photo 1: ... B base64 → ... B JPEG
[PhotoBooth] JPEG open OK, 240x240
[PhotoBooth] JPEG decode done, ok=1
[PhotoBooth] Displayed photo 1
```

## 7. Troubleshooting Quick Table

| Problem | Check |
|---|---|
| Portal not opening | Connect to `DeskBuddy-Setup`, use `http://192.168.4.1/` |
| Save button appears stuck | Watch browser progress panel and Serial Monitor |
| WiFi list empty | Use Refresh WiFi List or type SSID manually |
| Photo not showing | Re-upload image through portal; check size/decode logs |
| Brightness not changing | Open Brightness from menu, short tap to cycle |
| Setup portal stuck | Long press touch sensor to cancel if opened from menu |
