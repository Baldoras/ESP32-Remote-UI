/**
 * JoystickHandler.cpp
 * 
 * Implementation des Joystick-Handlers
 */

#include "JoystickHandler.h"

JoystickHandler::JoystickHandler()
    : pinX(JOY_PIN_Y)
    , pinY(JOY_PIN_X)
    , initialized(false)
    , rawX(0)
    , rawY(0)
    , valueX(0)
    , valueY(0)
    , deadzone(5)
    , updateInterval(20)
    , invertX(true)
    , invertY(true)
    , lastUpdateTime(0)
{
    // Standard-Kalibrierung (12-Bit ADC: 0-4095)
    calX.min = 100;
    calX.center = 2048;
    calX.max = 4000;
    
    calY.min = 100;
    calY.center = 2048;
    calY.max = 4000;
}

JoystickHandler::~JoystickHandler() {
}

bool JoystickHandler::begin() {
    DEBUG_PRINTLN("JoystickHandler: Initialisiere Joystick...");
    
    // ADC-Pins konfigurieren
    pinMode(pinX, INPUT);
    pinMode(pinY, INPUT);
    
    // ADC-Auflösung setzen (12-Bit)
    analogReadResolution(12);
    
    // Erste Messung
    rawX = readRaw(pinX);
    rawY = readRaw(pinY);
    
    valueX = mapValue(rawX, calX, invertX);
    valueY = mapValue(rawY, calY, invertY);
    
    initialized = true;
    
    DEBUG_PRINTLN("JoystickHandler: ✅ Initialisiert");
    DEBUG_PRINTF("JoystickHandler: X-Pin=%d, Y-Pin=%d\n", pinX, pinY);
    DEBUG_PRINTF("JoystickHandler: Start-Position: X=%d, Y=%d\n", valueX, valueY);
    
    return true;
}

bool JoystickHandler::update() {
    if (!initialized) return false;
    
    // Nur alle updateInterval ms aktualisieren
    unsigned long now = millis();
    if (now - lastUpdateTime < updateInterval) {
        return false;
    }
    lastUpdateTime = now;
    
    // ADC auslesen
    int16_t newRawX = readRaw(pinX);
    int16_t newRawY = readRaw(pinY);
    
    // Werte mappen
    int16_t newValueX = mapValue(newRawX, calX, invertX);
    int16_t newValueY = mapValue(newRawY, calY, invertY);
    
    // Deadzone anwenden
    newValueX = applyDeadzone(newValueX);
    newValueY = applyDeadzone(newValueY);
    
    // Prüfen ob sich Werte geändert haben
    bool changed = (newValueX != valueX || newValueY != valueY);
    
    rawX = newRawX;
    rawY = newRawY;
    valueX = newValueX;
    valueY = newValueY;
    
    if (changed) {
      DEBUG_PRINTF("Value X: %d\n", valueX);
      DEBUG_PRINTF("Value Y: %d\n", valueY);
    }
    return changed;
}

void JoystickHandler::setCalibration(uint8_t axis, int16_t min, int16_t center, int16_t max) {
    if (axis == 0) {
        // X-Achse
        calX.min = constrain(min, 0, 4095);
        calX.center = constrain(center, 0, 4095);
        calX.max = constrain(max, 0, 4095);
        DEBUG_PRINTF("JoystickHandler: X-Kalibrierung: %d / %d / %d\n", 
                     calX.min, calX.center, calX.max);
    } else if (axis == 1) {
        // Y-Achse
        calY.min = constrain(min, 0, 4095);
        calY.center = constrain(center, 0, 4095);
        calY.max = constrain(max, 0, 4095);
        DEBUG_PRINTF("JoystickHandler: Y-Kalibrierung: %d / %d / %d\n", 
                     calY.min, calY.center, calY.max);
    }
}

void JoystickHandler::getCalibration(uint8_t axis, int16_t* min, int16_t* center, int16_t* max) {
    if (axis == 0) {
        if (min) *min = calX.min;
        if (center) *center = calX.center;
        if (max) *max = calX.max;
    } else if (axis == 1) {
        if (min) *min = calY.min;
        if (center) *center = calY.center;
        if (max) *max = calY.max;
    }
}

void JoystickHandler::calibrateCenter() {
    DEBUG_PRINTLN("JoystickHandler: Auto-Kalibrierung Center...");
    
    // Mehrere Messungen für Durchschnitt
    int32_t sumX = 0;
    int32_t sumY = 0;
    const int samples = 10;
    
    for (int i = 0; i < samples; i++) {
        sumX += readRaw(pinX);
        sumY += readRaw(pinY);
        delay(10);
    }
    
    calX.center = sumX / samples;
    calY.center = sumY / samples;
    
    DEBUG_PRINTF("JoystickHandler: ✅ Center: X=%d, Y=%d\n", calX.center, calY.center);
}

void JoystickHandler::setDeadzone(uint8_t dz) {
    deadzone = constrain(dz, 0, 100);
    DEBUG_PRINTF("JoystickHandler: Deadzone: %d%%\n", deadzone);
}

void JoystickHandler::setUpdateInterval(uint16_t intervalMs) {
    updateInterval = intervalMs;
    DEBUG_PRINTF("JoystickHandler: Update-Intervall: %dms\n", updateInterval);
}

void JoystickHandler::printInfo() {
    DEBUG_PRINTLN("\n╔════════════════════════════════════════╗");
    DEBUG_PRINTLN("║       JOYSTICK HANDLER INFO            ║");
    DEBUG_PRINTLN("╚════════════════════════════════════════╝");
    DEBUG_PRINTF("Status:       %s\n", initialized ? "✅ OK" : "❌ Nicht init");
    DEBUG_PRINTF("Pins:         X=%d, Y=%d\n", pinX, pinY);
    DEBUG_PRINTLN("────────────────────────────────────────");
    DEBUG_PRINTF("Raw:          X=%d, Y=%d\n", rawX, rawY);
    DEBUG_PRINTF("Value:        X=%d, Y=%d\n", valueX, valueY);
    DEBUG_PRINTF("Neutral:      %s\n", isNeutral() ? "YES" : "NO");
    DEBUG_PRINTLN("────────────────────────────────────────");
    DEBUG_PRINTF("Kalibrierung X: %d / %d / %d\n", calX.min, calX.center, calX.max);
    DEBUG_PRINTF("Kalibrierung Y: %d / %d / %d\n", calY.min, calY.center, calY.max);
    DEBUG_PRINTLN("────────────────────────────────────────");
    DEBUG_PRINTF("Deadzone:     %d%%\n", deadzone);
    DEBUG_PRINTF("Interval:     %dms\n", updateInterval);
    DEBUG_PRINTF("Invert:       X=%s, Y=%s\n", 
                 invertX ? "YES" : "NO", 
                 invertY ? "YES" : "NO");
    DEBUG_PRINTLN("╚════════════════════════════════════════╝\n");
}

// ═══════════════════════════════════════════════════════════════════════════
// PRIVATE METHODEN
// ═══════════════════════════════════════════════════════════════════════════

int16_t JoystickHandler::readRaw(uint8_t pin) {
    // ADC auslesen (12-Bit: 0-4095)
    return analogRead(pin);
}

int16_t JoystickHandler::mapValue(int16_t raw, const AxisCalibration& cal, bool invert) {
    int16_t value;
    
    if (raw < cal.center) {
        // Untere Hälfte: min → center = -100 → 0
        value = map(raw, cal.min, cal.center, -100, 0);
    } else {
        // Obere Hälfte: center → max = 0 → +100
        value = map(raw, cal.center, cal.max, 0, 100);
    }
    
    // Begrenzen
    value = constrain(value, -100, 100);
    
    // Invertieren wenn gewünscht
    if (invert) {
        value = -value;
    }
    
    return value;
}

int16_t JoystickHandler::applyDeadzone(int16_t value) {
    // Deadzone-Bereich berechnen
    int16_t dzThreshold = deadzone;  // 0-100
    
    if (abs(value) <= dzThreshold) {
        return 0;
    }
    
    return value;
}