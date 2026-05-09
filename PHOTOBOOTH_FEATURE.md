# DeskBuddy Photo Booth Feature

## Overview
The Photo Booth feature allows users to:
- Upload up to 2 photos through the web setup portal
- Configure custom slideshow interval (1-60 seconds)
- View photos in fullscreen slideshow mode
- Exit by tapping the screen

## How to Use

### Accessing Setup Portal
1. Power on the device
2. Connect to WiFi network: **DeskBuddy-Setup** (Password: 12345678)
3. Open browser and navigate to: **192.168.4.1**
4. Configure WiFi, location, and photo booth settings
5. Upload photos (JPEG/PNG, recommended 240x240px)
6. Set slideshow interval (default: 3 seconds)
7. Click "Save Configuration"

### Using Photo Booth
1. **Long press/tap** on the clock screen to start the photo slideshow
2. Photos will automatically rotate based on the configured interval
3. **Tap screen** to exit and return to clock view

## Features

### Configuration Saved in Device Memory
- WiFi credentials
- Location (city, country code)
- Device name
- Photo booth interval
- Photo data (base64 encoded)
- Setup completion flag (prevents re-showing setup on reboot)

### Photo Management
- Maximum 2 photos supported
- Stored as base64 strings in device preferences
- Recommended size: 240x240 pixels (device display is 240x240)
- Supported formats: JPEG, PNG

### Slideshow Interval
- Range: 1-60 seconds
- Configurable via web portal
- Applied immediately on device restart

## Technical Details

### Files Added
- `photobooth.h` - PhotoBooth class definition
- `photobooth.cpp` - PhotoBooth implementation

### Files Modified
- `config.h/cpp` - Added photo data and interval storage
- `setup_portal.cpp` - Added photo upload UI
- `main.cpp` - Integrated photo booth view and touch handling

### View Modes
- `VIEW_CLOCK` - Default clock display
- `VIEW_WEATHER` - 3-day weather forecast
- `VIEW_ABOUT` - Device information
- `VIEW_PHOTOBOOTH` - Photo slideshow (NEW)

## Touch Controls

### Clock Screen
- **Single tap**: Open menu
- **Long press** (800ms): Open photo booth slideshow

### Menu
- **Single tap**: Cycle through menu items
- **Long press**: Select menu item

### Photo Booth
- **Any tap**: Exit slideshow and return to clock

## Configuration Structure
```
Preferences (Device Storage):
├── WiFi Settings
│   ├── ssid
│   └── pass
├── Location
│   ├── city
│   ├── country
│   └── timezone (auto-calculated)
├── Photo Booth
│   ├── photo1 (base64 string)
│   ├── photo2 (base64 string)
│   └── pbInterval (milliseconds)
└── setupDone (flag)
```

## Limitations & Future Improvements

### Current Limitations
- Photos stored as base64 strings (memory dependent)
- Simple placeholder rendering (actual image decoding not yet implemented)
- File upload via web portal needs base64 conversion

### Future Enhancements
- Full image decoding (JPG, PNG)
- SD card storage for photos
- Photo library with browser interface
- Photo effects and transitions
- Additional slideshow modes

## Troubleshooting

### Setup Portal Not Showing
- The portal only appears on first boot or if configuration is deleted
- Press the reset button to clear saved config and show setup portal again

### Photos Not Showing
- Ensure photos are uploaded in the setup portal
- Check device has sufficient free memory (base64 strings are memory-intensive)
- Recommended: Use smaller images (max 240x240px)

### Interval Not Working
- Verify interval is saved (1-60 seconds)
- Device must be rebooted for changes to take effect
