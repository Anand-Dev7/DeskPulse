#ifndef COLORS_H
#define COLORS_H

#include <stdint.h>

// ===== Unified Color Palette for DeskPulse =====
// All UI components should use these constants instead of
// raw hex values or ST77XX_* library constants.

// --- Base Colors ---
#define COLOR_BLACK       0x0000
#define COLOR_WHITE       0xFFFF
#define COLOR_RED         0xF800
#define COLOR_GREEN       0x07E0
#define COLOR_BLUE        0x001F
#define COLOR_CYAN        0x07FF
#define COLOR_YELLOW      0xFFE0
#define COLOR_ORANGE      0xFD20
#define COLOR_GRAY        0x7BEF

// --- Extended Palette ---
#define COLOR_DARK_BLUE   0x000F
#define COLOR_BLUE_ALT    0x2005
#define COLOR_PURPLE      0xA01D
#define COLOR_ORANGE_ALT  0xF60F
#define COLOR_OLIVE_GREEN 0x9586

// Legacy aliases (for clock.cpp compatibility)
#define COLOR_BLUE1       COLOR_BLUE_ALT
#define COLOR_PURPLE1     COLOR_PURPLE
#define COLOR_ORANGE1     COLOR_ORANGE_ALT
#define COLOR_olive_green COLOR_OLIVE_GREEN
#define COLOR_LIGHT_GRAY  0xC618
#define COLOR_DARK_GRAY   0x4208
#define COLOR_LIGHT_BLUE  0x04DF

// --- UI Backgrounds ---
#define COLOR_BG          0x0000  // Main background (black)
#define COLOR_BG_DARK     0x0821  // Slightly lighter dark bg
#define COLOR_BG_HEADER   0x04B8  // Header band background
#define COLOR_ROW_EVEN    0x1062  // Alternating row tint (even)
#define COLOR_ROW_ODD     0x18A3  // Alternating row tint (odd)

// --- Menu Colors ---
#define COLOR_MENU_BG         0x0000
#define COLOR_MENU_TEXT       0xFFFF
#define COLOR_MENU_HEADER_BG  0x07FF
#define COLOR_MENU_HEADER_TXT 0x0000
#define COLOR_MENU_SEL_BG     0xA015
#define COLOR_MENU_SEL_TEXT   0xFFFF

// --- Forecast Page ---
#define COLOR_FORECAST_DAY    0x9586  // Day label color
#define COLOR_FORECAST_DATA   0xE71C  // Data line color
#define COLOR_FORECAST_SEP    0xC618  // Separator line

#endif
