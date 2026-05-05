#ifndef MENU_H
#define MENU_H

#include <Arduino.h>
#include <Adafruit_GFX.h>

enum MenuAction {
  MENU_ACTION_NONE = 0,
  MENU_ACTION_SWITCH_CLOCK,
  MENU_ACTION_WEATHER_FORECAST,
  MENU_ACTION_WIFI_OPTION,
  MENU_ACTION_BRIGHTNESS_LEVEL,
  MENU_ACTION_ABOUT,
  MENU_ACTION_EXIT_TO_CLOCK
};

void menuInit();
bool menuIsOpen();
void menuOpen();
void menuClose();
void menuSetWifiEnabled(bool enabled);
void menuHandleSingleTap();
MenuAction menuHandleLongTap();
void menuDraw(Adafruit_GFX& display);

#endif
