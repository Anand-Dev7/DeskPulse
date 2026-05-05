#ifndef SETUP_PORTAL_H
#define SETUP_PORTAL_H

#include <Adafruit_ST7789.h>

class SetupPortal {
public:
    explicit SetupPortal(Adafruit_ST7789& display);
    void begin();
    void loop();
    void stop();
    bool isCompleted() const;

private:
    Adafruit_ST7789& tft;
    bool completed = false;
};

#endif
