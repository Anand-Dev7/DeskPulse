#ifndef PHOTOBOOTH_H
#define PHOTOBOOTH_H

#include <Arduino.h>
#include <Adafruit_ST7789.h>

struct PhotoData {
    String base64Data;      // Base64 encoded image data
    bool valid;
};

class PhotoBooth {
public:
    PhotoBooth(Adafruit_ST7789* display);
    
    void begin();
    void update();
    void stop();
    bool isActive() const;
    
    void setPhotos(const PhotoData& photo1, const PhotoData& photo2);
    void setInterval(unsigned long intervalMs);
    
private:
    Adafruit_ST7789* tft;
    PhotoData photos[2];
    unsigned long slideInterval;
    unsigned long lastPhotoTime;
    int currentPhotoIndex;
    bool active;
    
    void drawPhoto(int index);
    void drawPlaceholder(const char* message);
};

#endif
