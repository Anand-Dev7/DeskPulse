#include "menu.h"
#include "colors.h"

#include <Fonts/FreeSansBold9pt7b.h>
#include <Fonts/FreeSans9pt7b.h>

namespace {

const char* kMenuItems[] = {
  "Clock",
  "Show Weather",
  "WiFi Option",
  "Brightness",
  "About",
  "Exit"
};

const int kMenuItemCount = sizeof(kMenuItems) / sizeof(kMenuItems[0]);

bool gMenuOpen = false;
int gSelectedIndex = 0;
bool gWifiEnabled = true;

}  // namespace

void menuInit() {
  gMenuOpen = false;
  gSelectedIndex = 0;
}

bool menuIsOpen() {
  return gMenuOpen;
}

void menuOpen() {
  gMenuOpen = true;
}

void menuClose() {
  gMenuOpen = false;
}

void menuSetWifiEnabled(bool enabled) {
  gWifiEnabled = enabled;
}

void menuHandleSingleTap() {
  if (!gMenuOpen) {
    gMenuOpen = true;
    return;
  }

  gSelectedIndex++;
  if (gSelectedIndex >= kMenuItemCount) {
    gSelectedIndex = 0;
  }
}

MenuAction menuHandleLongTap() {
  if (!gMenuOpen) {
    return MENU_ACTION_NONE;
  }

  switch (gSelectedIndex) {
    case 0:
      return MENU_ACTION_SWITCH_CLOCK;
    case 1:
      return MENU_ACTION_WEATHER_FORECAST;
    case 2:
      return MENU_ACTION_WIFI_OPTION;
    case 3:
      return MENU_ACTION_BRIGHTNESS_LEVEL;
    case 4:
      return MENU_ACTION_ABOUT;
    case 5:
      return MENU_ACTION_EXIT_TO_CLOCK;
    default:
      return MENU_ACTION_NONE;
  }
}

void menuDraw(Adafruit_GFX& display) {
  const int width = display.width();
  const int height = display.height();
  const int headerH = 34;
  const int rowHeight = 30;
  const int listTop = headerH + 8;
  const int listBottom = height - 8;
  int visibleItems = (listBottom - listTop) / rowHeight;
  if (visibleItems < 1) visibleItems = 1;

  display.fillScreen(COLOR_MENU_BG);

  display.fillRect(0, 0, width, headerH, COLOR_MENU_HEADER_BG);
  display.setTextColor(COLOR_MENU_HEADER_TXT);
  display.setFont(&FreeSansBold9pt7b);
  display.setTextSize(1);
  display.setCursor((width / 2) - 32, 24);
  display.print("MENU");
  display.setFont(&FreeSans9pt7b);

  int startIndex = 0;
  if (gSelectedIndex >= visibleItems) {
    startIndex = gSelectedIndex - visibleItems + 1;
  }

  int endIndex = startIndex + visibleItems;
  if (endIndex > kMenuItemCount) {
    endIndex = kMenuItemCount;
    startIndex = endIndex - visibleItems;
    if (startIndex < 0) startIndex = 0;
  }

  display.setTextColor(COLOR_MENU_TEXT);
  for (int i = startIndex; i < endIndex; i++) {
    int row = i - startIndex;
    int rowTop = listTop + (row * rowHeight);
    int baselineY = rowTop + 20;
    const char* menuText = nullptr;
    String wifiText;
    if (i == 2) {
      wifiText = gWifiEnabled ? "WIFI : ON" : "WIFI : OFF";
      menuText = wifiText.c_str();
    } else {
      menuText = kMenuItems[i];
    }

    if (i == gSelectedIndex) {
      display.fillRoundRect(6, rowTop + 2, width - 12, rowHeight - 4, 5, COLOR_MENU_SEL_BG);
      display.setTextColor(COLOR_MENU_SEL_TEXT);
      display.setFont(&FreeSansBold9pt7b);
      display.setCursor(12, baselineY);
      display.print(" * ");
      display.print(menuText);
      display.setTextColor(COLOR_MENU_TEXT);
      display.setFont(&FreeSans9pt7b);
    } else {
      display.setCursor(10, baselineY);
      display.print("  ");
      display.print(menuText);
    }
  }

  display.setFont(NULL);
}
