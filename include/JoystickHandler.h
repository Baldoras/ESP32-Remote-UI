/**
 * JoystickHandler.h
 * 
 * Joystick-Handler für analoge 2-Achsen Joysticks
 * 
 * Features:
 * - ADC-Auslesen für X/Y-Achsen
 * - Kalibrierung (Min/Max/Center)
 * - Deadzone (Mittelbereich)
 * - Mapping auf -100 bis +100
 * - Achsen-Invertierung
 * - Update-Intervall (Debouncing)
 */

#ifndef JOYSTICK_HANDLER_H
#define JOYSTICK_HANDLER_H

#include <Arduino.h>
#include "setupConf.h"

class JoystickHandler {
public:
    /**
     * Konstruktor
     */
    JoystickHandler();
    
    /**
     * Destruktor
     */
    ~JoystickHandler();

    /**
     * Joystick initialisieren
     * @return true bei Erfolg
     */
    bool begin();

    /**
     * Joystick-Werte aktualisieren (in loop() aufrufen!)
     * @return true wenn neue Werte verfügbar
     */
    bool update();

    /**
     * X-Wert abrufen (-100 bis +100)
     */
    int16_t getX() const { return valueX; }

    /**
     * Y-Wert abrufen (-100 bis +100)
     */
    int16_t getY() const { return valueY; }

    /**
     * Rohe ADC-Werte abrufen
     */
    int16_t getRawX() const { return rawX; }
    int16_t getRawY() const { return rawY; }

    /**
     * Ist Joystick in Neutralposition?
     */
    bool isNeutral() const { return (valueX == 0 && valueY == 0); }

    /**
     * Kalibrierung manuell setzen
     * @param axis 0=X, 1=Y
     * @param min Minimum (0-4095)
     * @param center Center (0-4095)
     * @param max Maximum (0-4095)
     */
    void setCalibration(uint8_t axis, int16_t min, int16_t center, int16_t max);

    /**
     * Kalibrierung abrufen
     */
    void getCalibration(uint8_t axis, int16_t* min, int16_t* center, int16_t* max);

    /**
     * Auto-Kalibrierung starten (Center-Position)
     * Joystick muss in Neutralstellung sein!
     */
    void calibrateCenter();

    /**
     * Deadzone setzen (0-100, Standard: 5)
     * Werte innerhalb Deadzone werden als 0 interpretiert
     */
    void setDeadzone(uint8_t deadzone);

    /**
     * Update-Intervall setzen (ms, Standard: 20)
     */
    void setUpdateInterval(uint16_t intervalMs);

    /**
     * Achse invertieren
     */
    void setInvertX(bool invert) { invertX = invert; }
    void setInvertY(bool invert) { invertY = invert; }

    /**
     * Debug-Informationen ausgeben
     */
    void printInfo();

private:
    // Hardware
    uint8_t pinX;
    uint8_t pinY;
    bool initialized;

    // Kalibrierung
    struct AxisCalibration {
        int16_t min;        // ADC-Minimum (z.B. 100)
        int16_t center;     // ADC-Center (z.B. 2048)
        int16_t max;        // ADC-Maximum (z.B. 4000)
    };
    
    AxisCalibration calX;
    AxisCalibration calY;

    // Rohe ADC-Werte (0-4095)
    int16_t rawX;
    int16_t rawY;

    // Gemappte Werte (-100 bis +100)
    int16_t valueX;
    int16_t valueY;

    // Einstellungen
    uint8_t deadzone;           // Prozent (0-100)
    uint16_t updateInterval;    // Millisekunden
    bool invertX;
    bool invertY;

    // Timing
    unsigned long lastUpdateTime;

    /**
     * Rohen ADC-Wert auslesen
     */
    int16_t readRaw(uint8_t pin);

    /**
     * ADC-Wert auf -100 bis +100 mappen
     */
    int16_t mapValue(int16_t raw, const AxisCalibration& cal, bool invert);

    /**
     * Deadzone anwenden
     */
    int16_t applyDeadzone(int16_t value);
};

#endif // JOYSTICK_HANDLER_H