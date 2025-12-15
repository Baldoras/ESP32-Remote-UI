/**
 * DisplayHandler.cpp
 * 
 * Implementation des Display-Handlers (nur Hardware)
 */

#include "DisplayHandler.h"
#include "UserConfig.h"
#include <stdarg.h>

DisplayHandler::DisplayHandler() 
    : initialized(false)
    , currentBrightness(128)
{
}

DisplayHandler::~DisplayHandler() {
}

bool DisplayHandler::begin(UserConfig* config) {
    DEBUG_PRINTLN("DisplayHandler: Initialisiere Display...");
    
    // ═══════════════════════════════════════════════════════════════
    // KRITISCH: Touch CS auf HIGH (inaktiv!)
    // ═══════════════════════════════════════════════════════════════
    disableTouch();
    delay(100);
    
    // ═══════════════════════════════════════════════════════════════
    // Backlight initialisieren und einschalten
    // ═══════════════════════════════════════════════════════════════
    uint8_t brightness = 128;  // Fallback
    if (config != nullptr) {
        brightness = config->getBacklightDefault();
        DEBUG_PRINTF("DisplayHandler: Lade Helligkeit aus Config: %d\n", brightness);
    }

    initBacklight(brightness);
    delay(100);
    
    // ═══════════════════════════════════════════════════════════════
    // TFT_eSPI initialisieren
    // ═══════════════════════════════════════════════════════════════
    tft.init();
    
    // WICHTIG: Rotation 3 = Landscape richtig rum
    tft.setRotation(DISPLAY_ROTATION);
    
    // Display löschen
    tft.fillScreen(TFT_BLACK);
    
    initialized = true;
    
    DEBUG_PRINTLN("DisplayHandler: ✅ Display initialisiert");
    DEBUG_PRINTF("DisplayHandler: Auflösung: %d x %d\n", tft.width(), tft.height());
    
    return true;
}

void DisplayHandler::disableTouch() {
    DEBUG_PRINTLN("DisplayHandler: Deaktiviere Touch (CS auf HIGH)...");
    pinMode(TOUCH_CS, OUTPUT);
    //digitalWrite(TOUCH_CS, HIGH);  // Touch inaktiv
    DEBUG_PRINTLN("DisplayHandler: ✅ Touch CS inaktiv");
}

void DisplayHandler::initBacklight(uint8_t brightness) {
    DEBUG_PRINTLN("DisplayHandler: Initialisiere Backlight...");
    
    // GPIO als Output
    pinMode(TFT_BL, OUTPUT);
    
    // PWM konfigurieren (ESP32 Core 3.x API)
    ledcAttach(TFT_BL, BACKLIGHT_PWM_FREQ, BACKLIGHT_PWM_RES);
    
    // Helligkeit setzen
    currentBrightness = constrain(brightness, BACKLIGHT_MIN, BACKLIGHT_MAX);
    setBacklight(currentBrightness);
    
    DEBUG_PRINTF("DisplayHandler: ✅ Backlight initialisiert (Helligkeit: %d)\n", currentBrightness);
}

void DisplayHandler::clear(uint16_t color) {
    if (!initialized) return;
    tft.fillScreen(color);
}

void DisplayHandler::setBacklight(uint8_t brightness) {
    currentBrightness = constrain(brightness, BACKLIGHT_MIN, BACKLIGHT_MAX);
    ledcWrite(TFT_BL, currentBrightness);
    DEBUG_PRINTF("DisplayHandler: Backlight-Helligkeit: %d\n", currentBrightness);
}

void DisplayHandler::setBacklightOn(bool on) {
    if (on) {
        setBacklight(currentBrightness);
    } else {
        ledcWrite(TFT_BL, 0);
    }
}

void DisplayHandler::drawText(const char* text, int16_t x, int16_t y, uint16_t color, uint8_t size) {
    if (!initialized) return;
    
    tft.setTextColor(color);
    tft.setTextSize(size);
    tft.setCursor(x, y);
    tft.print(text);
}

void DisplayHandler::drawTextF(int16_t x, int16_t y, uint16_t color, uint8_t size, const char* format, ...) {
    if (!initialized) return;
    
    char buffer[256];
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);
    
    drawText(buffer, x, y, color, size);
}

void DisplayHandler::fillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color) {
    if (!initialized) return;
    tft.fillRect(x, y, w, h, color);
}

void DisplayHandler::drawRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color) {
    if (!initialized) return;
    tft.drawRect(x, y, w, h, color);
}

void DisplayHandler::fillCircle(int16_t x, int16_t y, int16_t r, uint16_t color) {
    if (!initialized) return;
    tft.fillCircle(x, y, r, color);
}

void DisplayHandler::drawCircle(int16_t x, int16_t y, int16_t r, uint16_t color) {
    if (!initialized) return;
    tft.drawCircle(x, y, r, color);
}

void DisplayHandler::drawLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint16_t color) {
    if (!initialized) return;
    tft.drawLine(x0, y0, x1, y1, color);
}

int16_t DisplayHandler::width() {
    return tft.width();
}

int16_t DisplayHandler::height() {
    return tft.height();
}

TFT_eSPI& DisplayHandler::getTft() {
    return tft;
}

uint16_t DisplayHandler::rgb565(uint8_t r, uint8_t g, uint8_t b) {
    return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
}