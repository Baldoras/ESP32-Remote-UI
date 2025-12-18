/**
 * UIProgressBar.h
 * 
 * ProgressBar-Element mit Fortschrittsanzeige (0-100%)
 */

#ifndef UI_PROGRESSBAR_H
#define UI_PROGRESSBAR_H

#include "UIElement.h"

class UIProgressBar : public UIElement {
public:
    UIProgressBar(int16_t x, int16_t y, int16_t w, int16_t h);
    ~UIProgressBar();

    // Override virtuelle Funktionen
    void draw(TFT_eSPI* tft) override;
    void handleTouch(int16_t x, int16_t y, bool pressed) override;

    // ProgressBar-spezifische Funktionen
    void setValue(int value);           // Wert setzen (0-100)
    int getValue() const { return value; }
    void setBarColor(uint16_t color);
    void setShowText(bool show);
    void setShowPercentage(bool show);

private:
    int value;                  // Aktueller Wert (0-100)
    uint16_t barColor;          // Farbe des Balkens
    bool showText;              // Text anzeigen?
    bool showPercentage;        // Prozent-Zeichen anzeigen?
    
    void drawProgressBar(TFT_eSPI* tft);
};

#endif // UI_PROGRESSBAR_H