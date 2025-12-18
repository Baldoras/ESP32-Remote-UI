/**
 * UILabel.h
 * 
 * Label-Element für Text-Anzeige (nur Anzeige, keine Interaktion)
 */

#ifndef UI_LABEL_H
#define UI_LABEL_H

#include "UIElement.h"

enum class TextAlignment {
    LEFT,
    CENTER,
    RIGHT
};

class UILabel : public UIElement {
public:
    UILabel(int16_t x, int16_t y, int16_t w, int16_t h, const char* text = "");
    ~UILabel();

    // Override virtuelle Funktionen
    void draw(TFT_eSPI* tft) override;
    void handleTouch(int16_t x, int16_t y, bool pressed) override;

    // Label-spezifische Funktionen
    void setText(const char* text);
    const char* getText() const { return text; }
    void setTextColor(uint16_t color);
    void setAlignment(TextAlignment align);
    void setFontSize(uint8_t size);
    void setTransparent(bool transparent);

private:
    char text[64];              // Label-Text
    TextAlignment alignment;    // Text-Ausrichtung
    uint8_t fontSize;           // Font-Größe (1-7)
    bool transparent;           // Transparenter Hintergrund?
    
    void drawLabel(TFT_eSPI* tft);
    uint8_t getDatum();         // TFT_eSPI Datum für Alignment
};

#endif // UI_LABEL_H