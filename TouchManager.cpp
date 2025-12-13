/**
 * TouchManager.cpp
 * 
 * Implementation des Touch-Managers mit UserConfig Integration
 */

#include "TouchManager.h"
#include "UserConfig.h"

TouchManager::TouchManager()
    : ts(nullptr)  // WICHTIG: Pointer auf nullptr initialisieren!
    , initialized(false)
    , lastTouchState(false)
    , currentTouchState(false)
    , calMinX(TOUCH_MIN_X)
    , calMaxX(TOUCH_MAX_X)
    , calMinY(TOUCH_MIN_Y)
    , calMaxY(TOUCH_MAX_Y)
    , calibrated(false)
    , pressureThreshold(TOUCH_THRESHOLD)
    , rotation(TOUCH_ROTATION)
    , displayWidth(DISPLAY_WIDTH)
    , displayHeight(DISPLAY_HEIGHT)
    , touchStartTime(0)
{
    // Aktuellen Touch-Punkt initialisieren
    currentPoint.x = 0;
    currentPoint.y = 0;
    currentPoint.rawX = 0;
    currentPoint.rawY = 0;
    currentPoint.z = 0;
    currentPoint.valid = false;
    currentPoint.timestamp = 0;
}

TouchManager::~TouchManager() {
    end();  // Speicher freigeben falls noch allokiert
}

bool TouchManager::begin(SPIClass* spi, UserConfig* config) {
    if (spi == nullptr) {
        DEBUG_PRINTLN("TouchManager: SPI-Bus ist nullptr!");
        return false;
    }
    
    DEBUG_PRINTLN("TouchManager: Initialisiere Touch...");
    
    // XPT2046 Objekt erstellen (dynamisch allokieren)
    ts = new XPT2046_Touchscreen(TOUCH_CS, TOUCH_IRQ);
    
    if (ts == nullptr) {
        DEBUG_PRINTLN("TouchManager: Konnte XPT2046 Objekt nicht erstellen!");
        return false;
    }
    
    // Touch initialisieren
    ts->begin(*spi);
    ts->setRotation(rotation);
    
    // Kalibrierung aus Config laden (falls verfügbar)
    if (config != nullptr) {
        loadCalibrationFromConfig(config);
    }
    
    initialized = true;
    
    DEBUG_PRINTLN("TouchManager: ✅ Touch initialisiert");
    DEBUG_PRINTF("TouchManager: CS=%d, IRQ=%d, Rotation=%d\n", TOUCH_CS, TOUCH_IRQ, rotation);
    DEBUG_PRINTF("TouchManager: Kalibrierung: X=%d-%d, Y=%d-%d, Threshold=%d\n",
                 calMinX, calMaxX, calMinY, calMaxY, pressureThreshold);
    
    return true;
}

void TouchManager::end() {
    if (ts != nullptr) {
        DEBUG_PRINTLN("TouchManager: Gebe Touch-Speicher frei...");
        delete ts;  // Speicher freigeben
        ts = nullptr;  // Auf nullptr setzen
        initialized = false;
        DEBUG_PRINTLN("TouchManager: ✅ Touch deaktiviert");
    }
}

bool TouchManager::isAvailable() {
    return (ts != nullptr && initialized);
}

bool TouchManager::update() {
    // Prüfen ob Touch verfügbar ist (nullptr-Check!)
    if (!isAvailable()) {
        return false;
    }
    
    // Letzten Status speichern
    lastTouchState = currentTouchState;
    
    // Aktuellen Touch-Status abrufen
    currentTouchState = ts->touched();
    
    if (currentTouchState) {
        // Touch-Punkt abrufen
        TS_Point p = ts->getPoint();
        
        // Nur gültig wenn Druck über Schwellwert
        if (p.z >= pressureThreshold) {
            // Touch gerade gestartet? Startzeit merken
            if (!lastTouchState) {
                touchStartTime = millis();
            }
            
            currentPoint.rawX = p.x;
            currentPoint.rawY = p.y;
            currentPoint.z = p.z;
            currentPoint.timestamp = millis();
            currentPoint.valid = true;
            
            // Auf Display-Koordinaten mappen
            mapCoordinates(p.x, p.y, &currentPoint.x, &currentPoint.y);
            
            return true;
        } else {
            currentPoint.valid = false;
            currentTouchState = false;
        }
    } else {
        currentPoint.valid = false;
    }
    
    return currentTouchState;
}

bool TouchManager::updateIfIRQ() {
    // HYBRID: Erst schneller GPIO-Check!
    if (!isIRQActive()) {
        // Kein Touch → Status aktualisieren
        lastTouchState = currentTouchState;
        currentTouchState = false;
        currentPoint.valid = false;
        return false;
    }
    
    // IRQ ist aktiv → normale update() Routine
    return update();
}

bool TouchManager::isIRQActive() {
    // Schneller GPIO-Check (keine SPI-Kommunikation!)
    // Touch IRQ ist LOW wenn Touch aktiv
    return (digitalRead(TOUCH_IRQ) == LOW);
}

bool TouchManager::isTouched() {
    if (!isAvailable()) return false;
    
    // Touch wurde gerade gedrückt (Flanke LOW → HIGH)
    return (currentTouchState && !lastTouchState);
}

bool TouchManager::isTouchActive() {
    if (!isAvailable()) return false;
    
    return currentTouchState;
}

bool TouchManager::isTouchReleased() {
    if (!isAvailable()) return false;
    
    // Touch wurde gerade losgelassen (Flanke HIGH → LOW)
    return (!currentTouchState && lastTouchState);
}

TouchPoint TouchManager::getTouchPoint() {
    return currentPoint;
}

bool TouchManager::getRawTouch(int16_t* x, int16_t* y, uint16_t* z) {
    if (!isAvailable()) return false;
    
    if (ts->touched()) {
        TS_Point p = ts->getPoint();
        
        if (x != nullptr) *x = p.x;
        if (y != nullptr) *y = p.y;
        if (z != nullptr) *z = p.z;
        
        return true;
    }
    
    return false;
}

void TouchManager::setRotation(uint8_t rot) {
    rotation = rot % 4;  // 0-3
    
    if (isAvailable()) {
        ts->setRotation(rotation);
        DEBUG_PRINTF("TouchManager: Rotation gesetzt: %d\n", rotation);
    }
    
    // Display-Größe anpassen basierend auf Rotation
    if (rotation == 0 || rotation == 2) {
        // Portrait
        displayWidth = DISPLAY_HEIGHT;
        displayHeight = DISPLAY_WIDTH;
    } else {
        // Landscape
        displayWidth = DISPLAY_WIDTH;
        displayHeight = DISPLAY_HEIGHT;
    }
}

// ═══════════════════════════════════════════════════════════════════════════
// CONFIG INTEGRATION
// ═══════════════════════════════════════════════════════════════════════════

void TouchManager::loadCalibrationFromConfig(UserConfig* config) {
    if (!config) {
        DEBUG_PRINTLN("TouchManager: ⚠️ Config ist nullptr");
        return;
    }
    
    DEBUG_PRINTLN("TouchManager: Lade Kalibrierung aus Config...");
    
    calMinX = config->getTouchMinX();
    calMaxX = config->getTouchMaxX();
    calMinY = config->getTouchMinY();
    calMaxY = config->getTouchMaxY();
    pressureThreshold = config->getTouchThreshold();
    
    calibrated = true;
    
    DEBUG_PRINTF("TouchManager: ✅ Kalibrierung geladen: X=%d-%d, Y=%d-%d, Threshold=%d\n",
                 calMinX, calMaxX, calMinY, calMaxY, pressureThreshold);
}

void TouchManager::saveCalibrationToConfig(UserConfig* config) {
    if (!config) {
        DEBUG_PRINTLN("TouchManager: ⚠️ Config ist nullptr");
        return;
    }
    
    DEBUG_PRINTLN("TouchManager: Speichere Kalibrierung in Config...");
    
    config->setTouchCalibration(calMinX, calMaxX, calMinY, calMaxY);
    config->setTouchThreshold(pressureThreshold);
    
    DEBUG_PRINTLN("TouchManager: ✅ Kalibrierung gespeichert");
}

void TouchManager::setCalibration(int16_t minX, int16_t maxX, int16_t minY, int16_t maxY) {
    calMinX = minX;
    calMaxX = maxX;
    calMinY = minY;
    calMaxY = maxY;
    calibrated = true;
    
    DEBUG_PRINTLN("TouchManager: Kalibrierung gesetzt:");
    DEBUG_PRINTF("  X: %d - %d\n", calMinX, calMaxX);
    DEBUG_PRINTF("  Y: %d - %d\n", calMinY, calMaxY);
}

void TouchManager::getCalibration(int16_t* minX, int16_t* maxX, int16_t* minY, int16_t* maxY) {
    if (minX != nullptr) *minX = calMinX;
    if (maxX != nullptr) *maxX = calMaxX;
    if (minY != nullptr) *minY = calMinY;
    if (maxY != nullptr) *maxY = calMaxY;
}

void TouchManager::setThreshold(uint16_t threshold) {
    pressureThreshold = constrain(threshold, 0, 4095);
    DEBUG_PRINTF("TouchManager: Schwellwert gesetzt: %d\n", pressureThreshold);
}

// ═══════════════════════════════════════════════════════════════════════════
// HELPER
// ═══════════════════════════════════════════════════════════════════════════

bool TouchManager::isPointInRect(int16_t x, int16_t y, int16_t rectX, int16_t rectY, int16_t rectW, int16_t rectH) {
    return (x >= rectX && x < rectX + rectW && 
            y >= rectY && y < rectY + rectH);
}

bool TouchManager::isTouchInRect(int16_t rectX, int16_t rectY, int16_t rectW, int16_t rectH) {
    if (!isAvailable() || !currentPoint.valid) {
        return false;
    }
    
    return isPointInRect(currentPoint.x, currentPoint.y, rectX, rectY, rectW, rectH);
}

unsigned long TouchManager::getTouchDuration() {
    if (!isAvailable() || !currentTouchState) {
        return 0;
    }
    
    return millis() - touchStartTime;
}

void TouchManager::printTouchInfo() {
    if (!isAvailable()) {
        DEBUG_PRINTLN("TouchManager: Touch nicht verfügbar");
        return;
    }
    
    DEBUG_PRINTLN("\n═══════════════════════════════════════");
    DEBUG_PRINTLN("TOUCH INFO");
    DEBUG_PRINTLN("═══════════════════════════════════════");
    
    DEBUG_PRINTF("Status: %s\n", currentTouchState ? "TOUCHED" : "not touched");
    
    if (currentPoint.valid) {
        DEBUG_PRINTF("Display: x=%d, y=%d\n", currentPoint.x, currentPoint.y);
        DEBUG_PRINTF("Raw:     x=%d, y=%d\n", currentPoint.rawX, currentPoint.rawY);
        DEBUG_PRINTF("Druck:   z=%d\n", currentPoint.z);
        DEBUG_PRINTF("Zeit:    %lu ms\n", currentPoint.timestamp);
    }
    
    DEBUG_PRINTF("\nRotation: %d\n", rotation);
    DEBUG_PRINTF("Display:  %dx%d\n", displayWidth, displayHeight);
    DEBUG_PRINTF("Schwelle: %d\n", pressureThreshold);
    DEBUG_PRINTF("Kalibrierung:\n");
    DEBUG_PRINTF("  X: %d - %d\n", calMinX, calMaxX);
    DEBUG_PRINTF("  Y: %d - %d\n", calMinY, calMaxY);
    
    DEBUG_PRINTLN("═══════════════════════════════════════\n");
}

void TouchManager::mapCoordinates(int16_t rawX, int16_t rawY, int16_t* mappedX, int16_t* mappedY) {
    // Auf Display-Koordinaten mappen
    int16_t x = map(rawX, calMinX, calMaxX, 0, displayWidth - 1);
    int16_t y = map(rawY, calMinY, calMaxY, 0, displayHeight - 1);
    
    // Auf gültigen Bereich begrenzen
    x = constrain(x, 0, displayWidth - 1);
    y = constrain(y, 0, displayHeight - 1);
    
    if (mappedX != nullptr) *mappedX = x;
    if (mappedY != nullptr) *mappedY = y;
}