/**
 * UIButton.h
 * 
 * Button-Element mit Click-Feedback
 */

#ifndef UI_BUTTON_H
#define UI_BUTTON_H

#include "UIElement.h"

class UIButton : public UIElement {
public:
    UIButton(int16_t x, int16_t y, int16_t w, int16_t h, const char* text = "");
    ~UIButton();

    // Override virtuelle Funktionen
    void draw(TFT_eSPI* tft) override;
    void handleTouch(int16_t x, int16_t y, bool pressed) override;

    // Button-spezifische Funktionen
    void setText(const char* text);
    const char* getText() const { return text; }
    void setTextColor(uint16_t color);
    void setPressedColor(uint16_t color);
    bool isPressed() const { return pressed; }

private:
    char text[32];              // Button-Text
    bool pressed;               // Button gedrückt?
    bool wasInside;             // War Touch beim letzten Mal innerhalb?
    uint16_t pressedColor;      // Farbe wenn gedrückt
    uint16_t normalColor;       // Normale Farbe
    
    void drawButton(TFT_eSPI* tft, bool isPressed);
};

#endif // UI_BUTTON_H