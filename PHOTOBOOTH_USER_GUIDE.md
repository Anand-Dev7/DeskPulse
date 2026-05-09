# How to Use Photo Booth - User Guide

## Quick Start

### Step 1: Access Setup Portal
1. **Power on your DeskBuddy device**
2. **Look for WiFi network:** `DeskBuddy-Setup`
3. **Connect to it** (Password: `12345678`)
4. **Open browser** and go to: `192.168.4.1`

### Step 2: Configure Your Device
The setup page has two sections:

#### WiFi & Location Section
- **WiFi SSID:** Select from available networks or enter manually
- **WiFi Password:** Your WiFi password
- **City:** Your city name (e.g., "New York", "London")
- **Country Code:** 2-letter country code (e.g., "US", "GB", "IN")
- **Device Name:** Optional - name for your device

#### Photo Booth Section (NEW!)
- **Photo 1:** Click "Choose File" and select your first photo
- **Photo 2:** Click "Choose File" and select your second photo
- **Slideshow Interval:** Set duration between photos (1-60 seconds)
  - Default: 3 seconds
  - Recommended: 3-5 seconds for viewing

### Step 3: Upload Photos
1. **Click on "Photo 1"** button
2. **Select an image** from your computer
   - Supported formats: JPEG, PNG
   - Recommended size: 240×240 pixels
   - Larger images will be resized
3. **See preview** appear below the upload
4. **Repeat for Photo 2** (optional)

### Step 4: Save Configuration
1. **Click "Save Configuration"** button
2. **Wait for confirmation** message
3. **Device will connect** to your WiFi
4. **Setup complete!** Portal closes automatically

### Important Notes
✅ **Setup portal appears only ONCE** - on first boot
✅ **After saving, setup never shows again** (unless you reset the device)
✅ **All settings are saved** in the device permanently
✅ **Photos are stored** as encoded data in device memory

---

## Using Photo Booth on Device

### Starting Photo Booth
1. **Look at the clock display**
2. **Press and hold** (long tap) for about 1 second
   - The display will change to show "Photo 1"

### Viewing Photos
- **Photos automatically rotate** at your set interval
  - Example: If you set 3 seconds, each photo displays for 3 seconds
- **Photo counter** shows which photo is displayed
- **Interval display** shows the current slideshow duration

### Exiting Photo Booth
- **Tap the screen** (any tap duration)
- **Returns to clock display** automatically

### Changing Settings Later
1. **You cannot edit photos via device**
2. **To change photos or interval:**
   - **Option 1:** Reset device and setup again
   - **Option 2:** Use web interface if future updates enable it

---

## Photo Requirements

### Recommended Specifications
```
Format:     JPEG or PNG
Size:       240 × 240 pixels (perfect fit for screen)
File Size:  < 50 KB each (to save device memory)
Color:      RGB or Grayscale
```

### Preparing Your Photos
1. **Use any image editing tool** (Paint, Photoshop, Canva, etc.)
2. **Resize to 240×240 pixels**
3. **Export as JPEG** (best for small file size)
4. **Test the file size** - keep under 50 KB

### Example Tools
- **Free Online:** Pixlr, Canva, Online-Convert.com
- **Desktop:** Photoshop, GIMP, Paint.NET
- **Command Line:** ImageMagick
  ```bash
  convert input.jpg -resize 240x240 output.jpg
  ```

---

## Troubleshooting

### Q: Setup portal not showing?
**A:** Setup only shows on first boot. After saving once, it won't appear again.
- **To reset:** Use device reset button to factory reset

### Q: Can't connect to DeskBuddy-Setup WiFi?
**A:** 
- Check if device is powered on
- Look for "DeskBuddy-Setup" in available networks
- Try refreshing your WiFi list
- Try connecting again with password: `12345678`

### Q: Photos not showing in slideshow?
**A:**
- Make sure you uploaded photos in the setup
- Photos need to be in supported format (JPEG/PNG)
- Check if device has been rebooted after saving

### Q: Slideshow interval too fast/slow?
**A:**
- Go back to setup portal and adjust interval (1-60 seconds)
- Requires device reboot for changes to take effect
- Note: Currently this requires manual reset

### Q: How do I change WiFi?
**A:**
- Currently, you need to factory reset and setup again
- Future updates may allow WiFi changes without reset

### Q: Photos look wrong/distorted?
**A:**
- Current version uses placeholder colors (image decoding in progress)
- Actual photo display coming in future update

### Q: Device memory full?
**A:**
- If photos are too large, there may be memory issues
- Reduce image file size by:
  - Compressing JPEG quality
  - Using smaller dimensions
  - Using PNG instead (if JPEG is larger)

---

## Tips & Tricks

### Best Slideshow Duration
- **1-2 seconds:** Fast paced, good for quick viewing
- **3-5 seconds:** Balanced, recommended default
- **10+ seconds:** Slow, good for detailed photos

### Photo Ideas
- Family photos
- Travel memories
- Artwork or designs
- Inspirational images
- Custom graphics with text

### Organizing Photos
1. Create a folder: "DeskBuddy_Photos"
2. Keep resized versions (240×240)
3. Keep originals for reference
4. Date them: Photo_1.jpg, Photo_2.jpg

---

## Advanced Information

### Storage Details
- **Device Type:** ESP32-C3
- **Storage:** Internal EEPROM/NVS (non-volatile)
- **Capacity:** Limited (photos stored as base64)
- **Persistence:** Survives power loss and reboots

### Technical Specs
- **Display:** 240×240 pixels, ST7789 TFT
- **Rotation Speed:** Smooth, no flicker
- **Max Photos:** 2 (due to memory constraints)
- **Max Storage:** ~200KB for both photos combined

### Touch Sensitivity
- **Short Tap:** < 800ms duration
- **Long Press:** ≥ 800ms duration
- Required for entering photo booth

---

## Support & Help

### Check Device Status
The device shows status on screen during:
1. **Boot animation** - DeskBuddy startup
2. **WiFi connection** - Connecting to your network
3. **Time sync** - Synchronizing system time
4. **Weather loading** - Fetching weather data

### Available Views
1. **Clock** - Main display with time, date, weather
2. **Menu** - Options menu (single tap to open)
3. **Weather** - 3-day forecast (from menu)
4. **About** - Device information (from menu)
5. **Photo Booth** - Slideshow (long press on clock)

### Reset Device
To factory reset and start setup again:
1. Locate reset button on device
2. Press and hold for 3+ seconds
3. Device will restart and show setup portal
4. Reconfigure all settings

---

## FAQ

**Q: Can I have more than 2 photos?**
A: Not currently, due to device memory limits. Future update planned.

**Q: Do photos survive power loss?**
A: Yes! Photos are stored in non-volatile memory.

**Q: Can I update photos without resetting?**
A: Not currently. Future update will enable remote configuration.

**Q: What formats are supported?**
A: JPEG and PNG. Other formats will not work.

**Q: How much memory do photos use?**
A: Approximately 15-30 KB per photo depending on size/compression.

**Q: Can slideshow run in background?**
A: No, it's a dedicated view. Returns to clock when you exit.

---

**Version:** 1.0
**Last Updated:** May 2026
**Device:** DeskBuddy ESP32-C3
