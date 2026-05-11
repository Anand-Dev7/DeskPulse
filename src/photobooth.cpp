// ============================================================
//  photobooth.cpp  —  Fixed: actually decodes & renders images
//
//  Requires the JPEGDEC library (install via Arduino Library Manager:
//  "JPEGDEC" by Larry Bank).
//
//  Image pipeline:
//    NVS string  →  base64-decode (mbedtls, built-in)
//                →  JPEG decode   (JPEGDEC, row-by-row)
//                →  drawRGBBitmap (Adafruit_ST7789)
//
//  The web portal stores images as base64-encoded JPEG (either a raw
//  base64 string or a data-URL like "data:image/jpeg;base64,…").
//  Both are handled automatically.
// ============================================================

#include "photobooth.h"
#include "colors.h"

#include <Fonts/FreeSansBold9pt7b.h>
#include <mbedtls/base64.h>   // built into ESP32 Arduino core
#include <JPEGDEC.h>          // install: Arduino Library Manager → "JPEGDEC"
#include <esp_task_wdt.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

// ---------------------------------------------------------------------------
// JPEGDEC requires a plain C callback, so we keep a module-level pointer to
// the active display. This is safe because decoding is synchronous and
// single-threaded.
// ---------------------------------------------------------------------------
static Adafruit_ST7789* _jpegTft = nullptr;

static int jpegDrawCallback(JPEGDRAW* pDraw) {
    if (!_jpegTft) return 0;

    // JPEG rendering can take long enough to trip the ESP32-C3 task WDT.
    // Feed/yield from every MCU row callback so entering PhotoBooth does not reset.
    esp_task_wdt_reset();
    delay(0);

    int drawX = pDraw->x;
    int drawY = pDraw->y;
    int drawW = pDraw->iWidth;
    int drawH = pDraw->iHeight;
    if (drawX >= 240 || drawY >= 240) return 1;
    if (drawX + drawW > 240) drawW = 240 - drawX;
    if (drawY + drawH > 240) drawH = 240 - drawY;
    if (drawW <= 0 || drawH <= 0) return 1;

    // pDraw->pPixels is already RGB565 big-endian, which ST7789 expects.
    _jpegTft->drawRGBBitmap(drawX, drawY, pDraw->pPixels, drawW, drawH);
    return 1;  // non-zero = keep decoding
}

// ===========================================================================
//  Constructor
// ===========================================================================
PhotoBooth::PhotoBooth(Adafruit_ST7789* display) {
    tft              = display;
    slideInterval    = 3000;
    lastPhotoTime    = 0;
    currentPhotoIndex = 0;
    active           = false;

    photos[0].valid  = false;
    photos[1].valid  = false;
}

// ===========================================================================
//  Public interface
// ===========================================================================

void PhotoBooth::begin() {
    active            = true;
    currentPhotoIndex = 0;
    lastPhotoTime     = millis();
    tft->fillScreen(COLOR_BLACK);

    if (photos[0].valid) {
        drawPhoto(0);
    } else if (photos[1].valid) {
        currentPhotoIndex = 1;
        drawPhoto(1);
    } else {
        drawPlaceholder("No photos uploaded.\n\nGo to menu >\nReconfigure WiFi\nto upload photos.");
    }
}

void PhotoBooth::update() {
    if (!active) return;

    unsigned long now = millis();
    if (now - lastPhotoTime < slideInterval) return;

    // Advance to the next valid photo
    int nextIndex = (currentPhotoIndex + 1) % 2;

    if (!photos[0].valid && !photos[1].valid) {
        drawPlaceholder("No photos!");
        lastPhotoTime = now;
        return;
    }

    // If the next slot is empty but the current one is valid, stay on it
    if (!photos[nextIndex].valid) {
        lastPhotoTime = now;
        return;
    }

    currentPhotoIndex = nextIndex;
    lastPhotoTime     = now;
    drawPhoto(currentPhotoIndex);
}

void PhotoBooth::stop()             { active = false; }
bool PhotoBooth::isActive() const   { return active; }

void PhotoBooth::setPhotos(const PhotoData& photo1, const PhotoData& photo2) {
    photos[0] = photo1;
    photos[1] = photo2;
}

void PhotoBooth::setInterval(unsigned long intervalMs) {
    slideInterval = intervalMs;
}

// ===========================================================================
//  drawPhoto  —  the real implementation
// ===========================================================================
void PhotoBooth::drawPhoto(int index) {

    Serial.printf("[PhotoBooth] drawPhoto(%d), stack HWM=%u words, heap=%u\n",
                  index, (unsigned)uxTaskGetStackHighWaterMark(NULL),
                  (unsigned)ESP.getFreeHeap());

    // ── Guard: valid index and non-empty data ───────────────────────────────
    if (index < 0 || index >= 2
            || !photos[index].valid
            || photos[index].base64Data.isEmpty()) {
        drawPlaceholder("No image data");
        return;
    }

    // ── Step 1: strip the data-URL prefix if the portal added one ───────────
    //   e.g. "data:image/jpeg;base64,/9j/4AAQ…"
    String b64 = photos[index].base64Data;
    int commaIdx = b64.indexOf(',');
    if (commaIdx >= 0) {
        b64 = b64.substring(commaIdx + 1);
    }
    b64.trim();

    if (b64.isEmpty()) {
        drawPlaceholder("Empty image data");
        return;
    }

    // ── Step 2: base64 → raw bytes ──────────────────────────────────────────
    size_t inputLen = b64.length();
    //  base64 decode is at most 3/4 the input length
    size_t bufSize  = ((inputLen / 4) + 1) * 3 + 8;

    uint8_t* imgBuf = (uint8_t*)malloc(bufSize);
    if (!imgBuf) {
        drawPlaceholder("Out of memory");
        return;
    }

    size_t decodedLen = 0;
    int ret = mbedtls_base64_decode(
        imgBuf, bufSize, &decodedLen,
        (const unsigned char*)b64.c_str(), inputLen
    );
    esp_task_wdt_reset();
    delay(0);

    if (ret != 0 || decodedLen == 0) {
        free(imgBuf);
        // ret codes: MBEDTLS_ERR_BASE64_BUFFER_TOO_SMALL = -0x002A
        //            MBEDTLS_ERR_BASE64_INVALID_CHARACTER = -0x002C
        char msg[40];
        snprintf(msg, sizeof(msg), "Base64 error\nCode: 0x%04X", -ret);
        drawPlaceholder(msg);
        return;
    }

    Serial.printf("[PhotoBooth] Photo %d: %u B base64 → %u B JPEG\n",
                  index + 1, (unsigned)inputLen, (unsigned)decodedLen);

    // ── Step 3: JPEG decode → ST7789 ────────────────────────────────────────
    tft->fillScreen(COLOR_BLACK);

    // JPEGDEC is large enough to overflow the default ESP32-C3 loopTask stack
    // when allocated as a local variable. Keep it on the heap instead.
    JPEGDEC* jpeg = new JPEGDEC();
    if (!jpeg) {
        free(imgBuf);
        drawPlaceholder("Out of memory\nJPEG decoder");
        return;
    }

    _jpegTft = tft;

    if (!jpeg->openRAM(imgBuf, (int)decodedLen, jpegDrawCallback)) {
        _jpegTft = nullptr;
        delete jpeg;
        free(imgBuf);
        drawPlaceholder("Not a valid JPEG.\nUpload a JPEG image.");
        return;
    }

    // Adafruit_GFX::drawRGBBitmap expects native uint16_t RGB565 values.
    // On ESP32-C3, RGB565_BIG_ENDIAN corrupts colors/renders as rainbow noise.
    jpeg->setPixelType(RGB565_LITTLE_ENDIAN);

    int imgW = jpeg->getWidth();
    int imgH = jpeg->getHeight();

    Serial.printf("[PhotoBooth] JPEG open OK, %dx%d, stack HWM=%u words, heap=%u\n",
                  imgW, imgH, (unsigned)uxTaskGetStackHighWaterMark(NULL),
                  (unsigned)ESP.getFreeHeap());

    // Pick the largest JPEGDEC scale that still fits in 240×240
    // int options = JPEG_SCALE_FULL;   // 0 = 1:1
    int options = 0;   // ✅ full resolution in JPEGDEC = integer 0
    int scaledW = imgW;
    int scaledH = imgH;

    if (imgW > 240 || imgH > 240) {
        if (imgW > 480 || imgH > 480) {
            options = JPEG_SCALE_QUARTER;   // 1/4
            scaledW = imgW / 4;
            scaledH = imgH / 4;
        } else {
            options = JPEG_SCALE_HALF;      // 1/2
            scaledW = imgW / 2;
            scaledH = imgH / 2;
        }
    }

    // Centre the image on the 240×240 screen
    int x = max(0, (240 - scaledW) / 2);
    int y = max(0, (240 - scaledH) / 2);

    esp_task_wdt_reset();
    delay(0);
    int decodeOk = jpeg->decode(x, y, options);
    jpeg->close();
    delete jpeg;
    jpeg = nullptr;
    _jpegTft = nullptr;
    esp_task_wdt_reset();
    delay(0);

    free(imgBuf);
    imgBuf = nullptr;

    Serial.printf("[PhotoBooth] JPEG decode done, ok=%d, stack HWM=%u words, heap=%u\n",
                  decodeOk, (unsigned)uxTaskGetStackHighWaterMark(NULL),
                  (unsigned)ESP.getFreeHeap());

    if (!decodeOk) return;

    // ── Step 4: subtle overlay badge ────────────────────────────────────────
    // Thin strip at the very bottom so the image isn't fully hidden
    tft->fillRect(0, 226, 240, 14, 0x0000);        // black strip
    tft->setTextColor(COLOR_LIGHT_GRAY);
    tft->setFont(NULL);
    tft->setTextSize(1);
    char badge[32];
    snprintf(badge, sizeof(badge), "  Photo %d of 2   %dx%d px",
             index + 1, imgW, imgH);
    tft->setCursor(0, 228);
    tft->print(badge);

    Serial.printf("[PhotoBooth] Displayed photo %d (%dx%d)\n", index + 1, imgW, imgH);
}

// ===========================================================================
//  drawPlaceholder  — shown when there's nothing to render
// ===========================================================================
void PhotoBooth::drawPlaceholder(const char* message) {
    tft->fillScreen(COLOR_BLACK);
    tft->fillRect(20, 50, 200, 150, COLOR_DARK_BLUE);
    tft->fillRect(22, 52, 196, 146, 0x0821);   // slightly lighter inner

    tft->setTextColor(COLOR_WHITE);
    tft->setFont(&FreeSansBold9pt7b);
    tft->setTextSize(1);
    tft->setCursor(30, 90);
    tft->print("Photo Booth");

    tft->setFont(NULL);
    tft->setTextSize(1);
    tft->setTextColor(COLOR_LIGHT_GRAY);
    tft->setCursor(28, 118);
    tft->print(message);

    tft->setFont(NULL);
}