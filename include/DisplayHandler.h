/**
 * DisplayHandler.h
 * 
 * Display-Manager für ST7796 mit TFT_eSPI
 * 
 * NUR HARDWARE-VERWALTUNG:
 * - Display-Initialisierung
 * - Backlight-Steuerung (PWM)
 * - Touch CS deaktivieren
 * - Rotation
 * 
 * KEIN UI-SYSTEM mehr! (PageManager verwaltet UI)
 */

#ifndef DISPLAY_HANDLER_H
#define DISPLAY_HANDLER_H

#include <Arduino.h>
#include <TFT_eSPI.h>
#include "setupConf.h"

class UserConfig;

class DisplayHandler {
public:
    /**
     * Konstruktor
     */
    DisplayHandler();

    /**
     * Destruktor
     */
    ~DisplayHandler();

    /**
     * Display initialisieren (nur Hardware!)
     * @param config Optional: UserConfig für Helligkeit
     * @return true bei Erfolg
     */
    bool begin(UserConfig* config = nullptr);

    /**
     * Display löschen
     * @param color Farbe (Standard: Schwarz)
     */
    void clear(uint16_t color = TFT_BLACK);

    /**
     * Backlight-Helligkeit setzen
     * @param brightness Helligkeit 0-255
     */
    void setBacklight(uint8_t brightness);

    /**
     * Backlight ein/aus
     * @param on true = an, false = aus
     */
    void setBacklightOn(bool on);

    /**
     * Text zeichnen (Basis-Funktion)
     * @param text Text
     * @param x X-Position
     * @param y Y-Position
     * @param color Textfarbe
     * @param size Textgröße (1-7)
     */
    void drawText(const char* text, int16_t x, int16_t y, uint16_t color = TFT_WHITE, uint8_t size = 2);

    /**
     * Formatierter Text (wie printf)
     */
    void drawTextF(int16_t x, int16_t y, uint16_t color, uint8_t size, const char* format, ...);

    /**
     * Rechteck zeichnen (gefüllt)
     */
    void fillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color);

    /**
     * Rechteck-Rahmen zeichnen
     */
    void drawRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color);

    /**
     * Kreis zeichnen (gefüllt)
     */
    void fillCircle(int16_t x, int16_t y, int16_t r, uint16_t color);

    /**
     * Kreis-Umriss zeichnen
     */
    void drawCircle(int16_t x, int16_t y, int16_t r, uint16_t color);

    /**
     * Linie zeichnen
     */
    void drawLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint16_t color);

    /**
     * Display-Breite abrufen
     */
    int16_t width();

    /**
     * Display-Höhe abrufen
     */
    int16_t height();

    /**
     * TFT_eSPI Objekt direkt zugreifen
     */
    TFT_eSPI& getTft();

    /**
     * RGB888 zu RGB565 konvertieren
     */
    static uint16_t rgb565(uint8_t r, uint8_t g, uint8_t b);

private:
    TFT_eSPI tft;              // TFT_eSPI Objekt
    bool initialized;          // Initialisierungs-Flag
    uint8_t currentBrightness; // Aktuelle Helligkeit
    
    /**
     * Touch CS inaktiv setzen (wichtig!)
     */
    void disableTouch();
    
    /**
     * Backlight initialisieren
     * @param brightness Start-Helligkeit (0-255)
     */
    void initBacklight(uint8_t brightness);
};

#endif // DISPLAY_HANDLER_H