/**
 * TouchManager.h
 * 
 * Touch-Manager für XPT2046 Touch Controller
 * 
 * Features:
 * - Optional (Pointer-basiert)
 * - Touch-Erkennung mit Zustandsverwaltung
 * - Koordinaten-Mapping auf Display
 * - Kalibrierung (via UserConfig)
 * - Rotation-Unterstützung
 * 
 * WICHTIG: Touch ist OPTIONAL!
 * - nullptr = Touch nicht verfügbar/deaktiviert
 * - Alle Methoden prüfen auf nullptr
 */

#ifndef TOUCH_MANAGER_H
#define TOUCH_MANAGER_H

#include <Arduino.h>
#include <SPI.h>
#include <XPT2046_Touchscreen.h>
#include "setupConf.h"

// Forward declaration
class UserConfig;

// Touch-Punkt Struktur
struct TouchPoint {
    int16_t x;              // X-Koordinate (Display-Koordinaten)
    int16_t y;              // Y-Koordinate (Display-Koordinaten)
    int16_t rawX;           // Rohe X-Koordinate
    int16_t rawY;           // Rohe Y-Koordinate
    uint16_t z;             // Druck
    bool valid;             // Punkt gültig?
    unsigned long timestamp; // Zeitstempel
};

class TouchManager {
public:
    /**
     * Konstruktor
     */
    TouchManager();

    /**
     * Destruktor
     * Räumt XPT2046 Objekt auf falls vorhanden
     */
    ~TouchManager();

    /**
     * Touch initialisieren
     * @param spi SPI-Bus Pointer (HSPI)
     * @param config UserConfig Pointer (optional für Kalibrierung)
     * @return true bei Erfolg, false bei Fehler
     */
    bool begin(SPIClass* spi, UserConfig* config = nullptr);

    /**
     * Touch deaktivieren und Speicher freigeben
     */
    void end();

    /**
     * Ist Touch verfügbar?
     * @return true wenn Touch initialisiert ist
     */
    bool isAvailable();

    /**
     * Touch-Status aktualisieren (in loop() aufrufen!)
     * @return true wenn Touch erkannt
     */
    bool update();

    /**
     * Touch-Status aktualisieren NUR wenn IRQ aktiv (HYBRID!)
     * Prüft zuerst GPIO, dann SPI-Abfrage
     * @return true wenn Touch erkannt
     */
    bool updateIfIRQ();

    /**
     * Prüfen ob Touch IRQ aktiv ist (schneller GPIO-Check!)
     * @return true wenn Touch möglicherweise aktiv (IRQ LOW)
     */
    bool isIRQActive();

    /**
     * Wurde Touch gerade gedrückt?
     * @return true wenn Touch gerade begonnen
     */
    bool isTouched();

    /**
     * Ist Touch aktuell gedrückt?
     * @return true wenn Touch aktiv
     */
    bool isTouchActive();

    /**
     * Wurde Touch gerade losgelassen?
     * @return true wenn Touch gerade beendet
     */
    bool isTouchReleased();

    /**
     * Touch-Punkt abrufen (gemappte Display-Koordinaten)
     * @return Aktueller Touch-Punkt
     */
    TouchPoint getTouchPoint();

    /**
     * Rohe Touch-Koordinaten abrufen (nicht gemappt)
     * @param x Pointer für X-Koordinate
     * @param y Pointer für Y-Koordinate
     * @param z Pointer für Druck
     * @return true bei Erfolg
     */
    bool getRawTouch(int16_t* x, int16_t* y, uint16_t* z);

    /**
     * Touch-Rotation setzen (muss zu Display-Rotation passen!)
     * @param rotation Rotation (0-3)
     */
    void setRotation(uint8_t rotation);

    /**
     * Kalibrierung aus UserConfig laden
     * @param config UserConfig Pointer
     */
    void loadCalibrationFromConfig(UserConfig* config);

    /**
     * Aktuelle Kalibrierung in UserConfig speichern
     * @param config UserConfig Pointer
     */
    void saveCalibrationToConfig(UserConfig* config);

    /**
     * Kalibrier-Werte manuell setzen
     * @param minX Minimum X
     * @param maxX Maximum X
     * @param minY Minimum Y
     * @param maxY Maximum Y
     */
    void setCalibration(int16_t minX, int16_t maxX, int16_t minY, int16_t maxY);

    /**
     * Kalibrier-Werte abrufen
     * @param minX Pointer für Minimum X
     * @param maxX Pointer für Maximum X
     * @param minY Pointer für Minimum Y
     * @param maxY Pointer für Maximum Y
     */
    void getCalibration(int16_t* minX, int16_t* maxX, int16_t* minY, int16_t* maxY);

    /**
     * Touch-Schwellwert setzen
     * @param threshold Mindestdruck (0-4095)
     */
    void setThreshold(uint16_t threshold);

    /**
     * Prüfen ob Punkt innerhalb Rechteck
     * @param x X-Koordinate
     * @param y Y-Koordinate
     * @param rectX Rechteck X
     * @param rectY Rechteck Y
     * @param rectW Rechteck Breite
     * @param rectH Rechteck Höhe
     * @return true wenn Punkt innerhalb
     */
    static bool isPointInRect(int16_t x, int16_t y, int16_t rectX, int16_t rectY, int16_t rectW, int16_t rectH);

    /**
     * Prüfen ob aktueller Touch in Rechteck
     * @param rectX Rechteck X
     * @param rectY Rechteck Y
     * @param rectW Rechteck Breite
     * @param rectH Rechteck Höhe
     * @return true wenn Touch in Rechteck
     */
    bool isTouchInRect(int16_t rectX, int16_t rectY, int16_t rectW, int16_t rectH);

    /**
     * Touch-Dauer abrufen (wie lange gedrückt)
     * @return Dauer in ms, 0 wenn nicht gedrückt
     */
    unsigned long getTouchDuration();

    /**
     * Touch-Info ausgeben (Debug)
     */
    void printTouchInfo();

private:
    XPT2046_Touchscreen* ts;  // ⭐ POINTER (kann nullptr sein!)
    bool initialized;          // Initialisierungs-Flag
    
    // Touch-Status
    bool lastTouchState;       // Letzter Touch-Status
    bool currentTouchState;    // Aktueller Touch-Status
    TouchPoint currentPoint;   // Aktueller Touch-Punkt
    
    // Kalibrierung
    int16_t calMinX;
    int16_t calMaxX;
    int16_t calMinY;
    int16_t calMaxY;
    bool calibrated;
    
    // Schwellwert
    uint16_t pressureThreshold;
    
    // Rotation
    uint8_t rotation;
    
    // Display-Größe (für Mapping)
    int16_t displayWidth;
    int16_t displayHeight;
    
    // Touch-Zeitstempel
    unsigned long touchStartTime;  // Wann wurde Touch gedrückt
    
    /**
     * Rohe Koordinaten auf Display mappen
     * @param rawX Rohe X-Koordinate
     * @param rawY Rohe Y-Koordinate
     * @param mappedX Pointer für gemappte X-Koordinate
     * @param mappedY Pointer für gemappte Y-Koordinate
     */
    void mapCoordinates(int16_t rawX, int16_t rawY, int16_t* mappedX, int16_t* mappedY);
};

#endif // TOUCH_MANAGER_H