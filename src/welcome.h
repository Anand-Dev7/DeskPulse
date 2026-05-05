#ifndef WELCOME_H
#define WELCOME_H

#include <Arduino.h>
#include <Adafruit_ST7789.h>

namespace WelcomeUI {

void showBootAnimation(Adafruit_ST7789& display, const char* title);
void showWifiStatus(Adafruit_ST7789& display, const String& line1, const String& line2, const String& line3 = "");

}  // namespace WelcomeUI

#endif
