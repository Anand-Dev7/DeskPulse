#include "welcome.h"
#include "colors.h"

#include <Fonts/FreeSansBold12pt7b.h>

namespace WelcomeUI {

void showBootAnimation(Adafruit_ST7789& display, const char* title) {
	const unsigned long kProgressFillMs = 7000;
	const unsigned long kBootScreenMs = 10000;
	const unsigned long startMs = millis();

	display.fillScreen(COLOR_BLACK);
	display.setTextColor(COLOR_CYAN);
	display.setFont(&FreeSansBold12pt7b);
	int16_t tx, ty;
	uint16_t tw, th;
	display.getTextBounds(title, 0, 0, &tx, &ty, &tw, &th);
	display.setCursor((display.width() - tw) / 2, 82);
	display.print(title);
	display.setFont(NULL);

	const int barX = 40;
	const int barY = 112;
	const int barW = 160;
	const int barH = 12;
	display.drawRoundRect(barX, barY, barW, barH, 6, COLOR_WHITE);

	int lastPercent = -1;
	while (true) {
		unsigned long elapsed = millis() - startMs;
		if (elapsed > kProgressFillMs) {
			elapsed = kProgressFillMs;
		}

		int p = (elapsed * 100) / kProgressFillMs;
		if (p != lastPercent) {
			lastPercent = p;

			int fill = (barW - 4) * p / 100;
			display.fillRect(barX + 2, barY + 2, barW - 4, barH - 4, COLOR_BLACK);
			display.fillRect(barX + 2, barY + 2, fill, barH - 4, COLOR_GREEN);

			const int labelX = 96;
			const int labelY = barY + 22;
			const int labelW = 48;
			const int labelH = 18;
			String percentText = String(p) + "%";
			int textW = percentText.length() * 6;
			int textX = labelX + (labelW - textW) / 2;

			display.fillRect(labelX, labelY, labelW, labelH, COLOR_BLACK);
			display.setTextColor(COLOR_WHITE);
			display.setTextSize(2);
			display.setCursor(textX, labelY + 6);
			display.print(percentText);
		}

		if (elapsed >= kProgressFillMs) {
			break;
		}

		delay(10);
	}

	unsigned long elapsedMs = millis() - startMs;
	if (elapsedMs < kBootScreenMs) {
		delay(kBootScreenMs - elapsedMs);
	}
}

void showWifiStatus(Adafruit_ST7789& display, const String& line1, const String& line2, const String& line3) {
	display.fillScreen(COLOR_BLACK);
	display.fillRect(0, 0, 240, 28, COLOR_CYAN);
	display.setTextColor(COLOR_BLACK);
	display.setTextSize(2);
	display.setCursor(58, 7);
	display.print("DeskPulse");

	display.setTextColor(COLOR_WHITE);
	display.setTextSize(2);
	display.setCursor(10, 62);
	display.print(line1);
	display.setCursor(10, 94);
	display.print(line2);

	if (line3.length() > 0) {
		display.setCursor(10, 126);
		display.print(line3);
	}
}

}  // namespace WelcomeUI
