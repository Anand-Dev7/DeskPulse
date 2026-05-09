#include "photobooth.h"
#include "colors.h"
#include <Fonts/FreeSansBold9pt7b.h>

PhotoBooth::PhotoBooth(Adafruit_ST7789* display) {
    tft = display;
    slideInterval = 3000;  // Default 3 seconds
    lastPhotoTime = 0;
    currentPhotoIndex = 0;
    active = false;
    
    // Initialize photos as invalid
    photos[0].valid = false;
    photos[1].valid = false;
}

void PhotoBooth::begin() {
    active = true;
    currentPhotoIndex = 0;
    lastPhotoTime = millis();
    tft->fillScreen(COLOR_BLACK);
    
    if (photos[0].valid || photos[1].valid) {
        drawPhoto(0);
    } else {
        drawPlaceholder("No photos\nuploadedyet!");
    }
}

void PhotoBooth::update() {
    if (!active) return;
    
    // Check if we need to show next photo
    unsigned long now = millis();
    if (now - lastPhotoTime >= slideInterval) {
        // Find next valid photo
        int nextIndex = (currentPhotoIndex + 1) % 2;
        
        // If only one photo, just show it repeatedly
        if (!photos[nextIndex].valid && !photos[currentPhotoIndex].valid) {
            drawPlaceholder("No photos!");
            return;
        }
        
        // If next photo is invalid, wrap back to 0
        if (!photos[nextIndex].valid) {
            if (photos[currentPhotoIndex].valid) {
                // Keep showing current if next is invalid
                lastPhotoTime = now;
                return;
            } else {
                nextIndex = 0;
            }
        }
        
        currentPhotoIndex = nextIndex;
        lastPhotoTime = now;
        
        if (photos[currentPhotoIndex].valid) {
            drawPhoto(currentPhotoIndex);
        } else {
            drawPlaceholder("Photo unavailable");
        }
    }
}

void PhotoBooth::stop() {
    active = false;
}

bool PhotoBooth::isActive() const {
    return active;
}

void PhotoBooth::setPhotos(const PhotoData& photo1, const PhotoData& photo2) {
    photos[0] = photo1;
    photos[1] = photo2;
}

void PhotoBooth::setInterval(unsigned long intervalMs) {
    slideInterval = intervalMs;
}

void PhotoBooth::drawPhoto(int index) {
    if (index < 0 || index >= 2 || !photos[index].valid) {
        drawPlaceholder("Invalid index");
        return;
    }
    
    // Clear screen
    tft->fillScreen(COLOR_BLACK);
    
    // For now, display a colored background with photo indicator
    // In a production version, decode base64 and render actual image
    uint16_t photoColors[] = {COLOR_BLUE, COLOR_GREEN};
    tft->fillScreen(photoColors[index]);
    
    // Display photo number
    tft->setTextColor(COLOR_WHITE);
    tft->setFont(&FreeSansBold9pt7b);
    tft->setTextSize(2);
    tft->setCursor(80, 110);
    tft->print("Photo ");
    tft->print(index + 1);
    
    // Display interval info
    tft->setTextSize(1);
    tft->setCursor(50, 200);
    tft->print("Interval: ");
    tft->print(slideInterval / 1000);
    tft->print("s");
    
    tft->setFont(NULL);
}

void PhotoBooth::drawPlaceholder(const char* message) {
    tft->fillScreen(COLOR_BLACK);
    tft->fillRect(20, 60, 200, 120, COLOR_DARK_BLUE);
    
    tft->setTextColor(COLOR_WHITE);
    tft->setFont(&FreeSansBold9pt7b);
    tft->setTextSize(1);
    
    // Center text
    int cursorX = 40;
    int cursorY = 110;
    
    tft->setCursor(cursorX, cursorY);
    tft->print(message);
    
    tft->setFont(NULL);
}
