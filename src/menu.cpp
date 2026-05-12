#include "menu.h"
#include "colors.h"

#include <Fonts/FreeSansBold9pt7b.h>
#include <Fonts/FreeSans9pt7b.h>

namespace {

const char* kMenuItems[] = {
  "Clock",
  "Show Weather",
  "Brightness",
  "Reconfigure WiFi",
  "About",
  "Exit"
};

const int kMenuItemCount = sizeof(kMenuItems) / sizeof(kMenuItems[0]);

bool gMenuOpen = false;
int gSelectedIndex = 0;
int gPrevSelectedIndex = -1;
int gPrevStartIndex = -1;

void drawMenuHeader(Adafruit_GFX& display) {
  const int width = display.width();
  const int headerH = 34;
  display.fillRect(0, 0, width, headerH, COLOR_MENU_HEADER_BG);
  display.setTextColor(COLOR_MENU_HEADER_TXT);
  display.setFont(&FreeSansBold9pt7b);
  display.setTextSize(1);
  display.setCursor((width / 2) - 32, 24);
  display.print("MENU");
  display.setFont(&FreeSans9pt7b);
}

void drawMenuRow(Adafruit_GFX& display, int itemIndex, int startIndex, int visibleItems) {
  const int width = display.width();
  const int headerH = 34;
  const int rowHeight = 30;
  const int listTop = headerH + 8;
  const int row = itemIndex - startIndex;
  if (itemIndex < 0 || itemIndex >= kMenuItemCount || row < 0 || row >= visibleItems) return;

  const int rowTop = listTop + (row * rowHeight);
  const int baselineY = rowTop + 20;
  display.fillRect(0, rowTop, width, rowHeight, COLOR_MENU_BG);

  const char* menuText = kMenuItems[itemIndex];
  if (itemIndex == gSelectedIndex) {
    display.fillRoundRect(6, rowTop + 2, width - 12, rowHeight - 4, 5, COLOR_MENU_SEL_BG);
    display.setTextColor(COLOR_MENU_SEL_TEXT);
    display.setFont(&FreeSansBold9pt7b);
    display.setCursor(12, baselineY);
    display.print(" * ");
    display.print(menuText);
    display.setTextColor(COLOR_MENU_TEXT);
    display.setFont(&FreeSans9pt7b);
  } else {
    display.setTextColor(COLOR_MENU_TEXT);
    display.setFont(&FreeSans9pt7b);
    display.setCursor(10, baselineY);
    display.print("  ");
    display.print(menuText);
  }
}

}  // namespace

void menuInit() {
  gMenuOpen = false;
  gSelectedIndex = 0;
  menuInvalidate();
}

bool menuIsOpen() {
  return gMenuOpen;
}

void menuOpen() {
  gMenuOpen = true;
  menuInvalidate();
}

void menuClose() {
  gMenuOpen = false;
  menuInvalidate();
}

void menuInvalidate() {
  gPrevSelectedIndex = -1;
  gPrevStartIndex = -1;
}

void menuHandleSingleTap() {
  if (!gMenuOpen) {
    gMenuOpen = true;
    menuInvalidate();
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
      return MENU_ACTION_BRIGHTNESS_LEVEL;
    case 3:
      return MENU_ACTION_RECONFIGURE_WIFI;
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

  const bool fullRedraw = (gPrevSelectedIndex < 0 || gPrevStartIndex != startIndex);
  if (fullRedraw) {
    display.fillScreen(COLOR_MENU_BG);
    drawMenuHeader(display);
    for (int i = startIndex; i < endIndex; i++) {
      drawMenuRow(display, i, startIndex, visibleItems);
    }
  } else if (gPrevSelectedIndex != gSelectedIndex) {
    drawMenuRow(display, gPrevSelectedIndex, startIndex, visibleItems);
    drawMenuRow(display, gSelectedIndex, startIndex, visibleItems);
  }

  gPrevSelectedIndex = gSelectedIndex;
  gPrevStartIndex = startIndex;
  display.setFont(NULL);
}
