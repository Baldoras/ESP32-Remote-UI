/**
 * UISlider.h
 * 
 * Slider-Element mit Drag-Funktionalität (0-100)
 */

#ifndef UI_SLIDER_H
#define UI_SLIDER_H

#include "UIElement.h"

class UISlider : public UIElement {
public:
    UISlider(int16_t x, int16_t y, int16_t w, int16_t h);
    ~UISlider();

    // Override virtuelle Funktionen
    void draw(TFT_eSPI* tft) override;
    void handleTouch(int16_t x, int16_t y, bool pressed) override;

    // Slider-spezifische Funktionen
    void setValue(int value);           // Wert setzen (0-100)
    int getValue() const { return value; }
    void setKnobColor(uint16_t color);
    void setBarColor(uint16_t color);
    void setShowValue(bool show);
    bool isDragging() const { return dragging; }

private:
    int value;                  // Aktueller Wert (0-100)
    bool dragging;              // Wird gerade gezogen?
    uint16_t knobColor;         // Farbe des Schiebereglers
    uint16_t barColor;          // Farbe des gefüllten Balkens
    bool showValue;             // Wert anzeigen?
    int knobRadius;             // Radius des Knobs
    
    void drawSlider(TFT_eSPI* tft);
    void updateValueFromTouch(int16_t tx);
    int getKnobX();             // X-Position des Knobs berechnen
};

#endif // UI_SLIDER_H