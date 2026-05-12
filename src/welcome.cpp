#include "welcome.h"
#include "colors.h"

#include <Fonts/FreeSansBold12pt7b.h>

namespace WelcomeUI {

void showBootAnimation(Adafruit_ST7789& display, const char* title) {
	const unsigned long kBootMs = 5200;
	const unsigned long kFinalHoldMs = 800;
	const unsigned long startMs = millis();
	const int faceX = 48, faceY = 24, faceW = 144, faceH = 88;
	const int cx = faceX + faceW / 2;
	const int eyeY = faceY + 36;
	const int leftEyeX = faceX + 42;
	const int rightEyeX = faceX + faceW - 42;

	display.fillScreen(COLOR_BLACK);

	int16_t tx, ty;
	uint16_t tw, th;
	display.setFont(&FreeSansBold12pt7b);
	display.getTextBounds(title, 0, 0, &tx, &ty, &tw, &th);
	display.setFont(NULL);

	// Static colorful background accents. Draw once to avoid full-screen flicker.
	for (int i = 0; i < 18; i++) {
		int x = (i * 29) % 220 + 10;
		int y = 8 + ((i * 37) % 135);
		uint16_t c = (i % 3 == 0) ? COLOR_DARK_BLUE : ((i % 3 == 1) ? COLOR_PURPLE : COLOR_BLUE_ALT);
		display.drawPixel(x, y, c);
	}

	// Static robot body.
	display.drawLine(cx - 34, faceY, cx - 34, faceY - 10, COLOR_CYAN);
	display.drawLine(cx + 34, faceY, cx + 34, faceY - 10, COLOR_CYAN);
	display.fillCircle(cx - 34, faceY - 12, 4, COLOR_ORANGE1);
	display.fillCircle(cx + 34, faceY - 12, 4, COLOR_GREEN);
	display.fillRoundRect(faceX - 10, faceY + 28, 10, 30, 5, COLOR_PURPLE);
	display.fillRoundRect(faceX + faceW, faceY + 28, 10, 30, 5, COLOR_PURPLE);
	display.fillRoundRect(faceX, faceY, faceW, faceH, 16, COLOR_DARK_BLUE);
	display.drawRoundRect(faceX, faceY, faceW, faceH, 16, COLOR_CYAN);
	display.drawRoundRect(faceX + 3, faceY + 3, faceW - 6, faceH - 6, 13, COLOR_PURPLE);

	// Name below animation, drawn once and never cleared during animation.
	display.setFont(&FreeSansBold12pt7b);
	display.setTextColor(COLOR_CYAN);
	display.setCursor((display.width() - tw) / 2, 168);
	display.print(title);
	display.setFont(NULL);

	int lastFrame = -1;
	int lastSmileShift = -1;
	bool lastEyesOpen = true;
	int lastDotCount = -1;
	while (millis() - startMs < kBootMs) {
		unsigned long elapsed = millis() - startMs;
		int frame = elapsed / 80;  // lower frame rate = smoother ST7789 updates
		if (frame != lastFrame) {
			lastFrame = frame;
			bool eyesOpen = (frame % 55) < 50;
			int smileShift = (frame / 4) % 5;
			if (smileShift > 2) smileShift = 4 - smileShift;
			int dotCount = (frame / 5) % 4;
			uint16_t eyeColor = (frame % 18 < 9) ? COLOR_GREEN : COLOR_CYAN;

			// Clear and redraw only the small face interior regions that animate.
			if (eyesOpen != lastEyesOpen || frame == 0) {
				display.fillRoundRect(faceX + 18, faceY + 18, faceW - 36, 38, 8, COLOR_DARK_BLUE);
				if (eyesOpen) {
					display.drawCircle(leftEyeX, eyeY, 13, COLOR_WHITE);
					display.fillCircle(leftEyeX, eyeY, 6, eyeColor);
					display.fillCircle(leftEyeX + 2, eyeY - 2, 2, COLOR_WHITE);
					display.drawCircle(rightEyeX, eyeY, 13, COLOR_WHITE);
					display.fillCircle(rightEyeX, eyeY, 6, eyeColor);
					display.fillCircle(rightEyeX + 2, eyeY - 2, 2, COLOR_WHITE);
				} else {
					display.drawLine(leftEyeX - 13, eyeY, leftEyeX + 13, eyeY, COLOR_WHITE);
					display.drawLine(rightEyeX - 13, eyeY, rightEyeX + 13, eyeY, COLOR_WHITE);
				}
				lastEyesOpen = eyesOpen;
			} else if (eyesOpen) {
				// Small eye-color pulse without clearing the whole screen.
				display.fillCircle(leftEyeX, eyeY, 6, eyeColor);
				display.fillCircle(leftEyeX + 2, eyeY - 2, 2, COLOR_WHITE);
				display.fillCircle(rightEyeX, eyeY, 6, eyeColor);
				display.fillCircle(rightEyeX + 2, eyeY - 2, 2, COLOR_WHITE);
			}

			if (smileShift != lastSmileShift || frame == 0) {
				display.fillRoundRect(faceX + 42, faceY + faceH - 35, 60, 24, 8, COLOR_DARK_BLUE);
				int mouthCenterY = faceY + faceH - 28 + smileShift;
				for (int angle = 25; angle <= 155; angle += 4) {
					float rad = angle * 3.14159f / 180.0f;
					int x = cx + (int)(20 * cos(rad));
					int y = mouthCenterY + (int)(12 * sin(rad));
					display.fillCircle(x, y, 1, COLOR_ORANGE1);
				}
				lastSmileShift = smileShift;
			}

			// Small pulse bar only; no full-screen clearing.
			display.fillRect(44, 122, 152, 16, COLOR_BLACK);
			int pulseW = 50 + ((frame * 5) % 100);
			uint16_t accent = (frame % 24 < 8) ? COLOR_CYAN : ((frame % 24 < 16) ? COLOR_PURPLE : COLOR_ORANGE1);
			display.drawRoundRect((240 - pulseW) / 2, 124, pulseW, 10, 5, accent);
			display.fillRoundRect((240 - pulseW) / 2 + 2, 127, pulseW - 4, 4, 2, COLOR_BLUE_ALT);

			if (dotCount != lastDotCount || frame == 0) {
				display.fillRect(70, 186, 110, 18, COLOR_BLACK);
				display.setTextSize(1);
				display.setTextColor(COLOR_WHITE);
				display.setCursor(78, 194);
				display.print("Booting");
				for (int d = 0; d < dotCount; d++) display.print('.');
				lastDotCount = dotCount;
			}
		}
		delay(12);
	}
	delay(kFinalHoldMs);
}

void showWifiStatus(Adafruit_ST7789& display, const String& line1, const String& line2, const String& line3) {
	display.fillScreen(COLOR_BLACK);
	display.fillRect(0, 0, 240, 28, COLOR_CYAN);
	display.setTextColor(COLOR_BLACK);
	display.setTextSize(2);
	display.setCursor(58, 7);
	display.print("DeskBuddy");

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
