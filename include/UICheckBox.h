/**
 * UICheckBox.h
 * 
 * CheckBox-Element (An/Aus Schalter)
 */

#ifndef UI_CHECKBOX_H
#define UI_CHECKBOX_H

#include "UIElement.h"

class UICheckBox : public UIElement {
public:
    UICheckBox(int16_t x, int16_t y, int16_t size, const char* label = "");
    ~UICheckBox();

    // Override virtuelle Funktionen
    void draw(TFT_eSPI* tft) override;
    void handleTouch(int16_t x, int16_t y, bool pressed) override;

    // CheckBox-spezifische Funktionen
    void setChecked(bool checked);
    bool isChecked() const { return checked; }
    void toggle();
    void setLabel(const char* label);
    const char* getLabel() const { return label; }
    void setCheckColor(uint16_t color);

private:
    char label[32];             // Label-Text
    bool checked;               // Checked-Status
    bool wasInside;             // War Touch beim letzten Mal innerhalb?
    uint16_t checkColor;        // Farbe des Häkchens
    int16_t boxSize;            // Größe der Box
    
    void drawCheckBox(TFT_eSPI* tft);
    void drawCheckMark(TFT_eSPI* tft, int16_t x, int16_t y, int16_t size);
};

#endif // UI_CHECKBOX_H