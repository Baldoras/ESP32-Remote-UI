/**
 * UITextBox.h
 * 
 * TextBox-Element für mehrzeilige Text-Anzeige
 * - Automatischer Zeilenumbruch
 * - Vertikales Scrolling
 * - Nur Anzeige (read-only)
 */

#ifndef UI_TEXTBOX_H
#define UI_TEXTBOX_H

#include "UIElement.h"
#include <vector>

class UITextBox : public UIElement {
public:
    UITextBox(int16_t x, int16_t y, int16_t w, int16_t h);
    ~UITextBox();

    // Override virtuelle Funktionen
    void draw(TFT_eSPI* tft) override;
    void handleTouch(int16_t x, int16_t y, bool pressed) override;

    // TextBox-spezifische Funktionen
    void setText(const char* text);             // Text setzen (mit Auto-Wrap)
    void appendLine(const char* line);          // Zeile hinzufügen
    void appendText(const char* text);          // Text anhängen (mit Auto-Wrap)
    void clear();                               // Text löschen
    void scrollToBottom();                      // Zum Ende scrollen
    void scrollToTop();                         // Zum Anfang scrollen
    void setWordWrap(bool wrap);                // Automatischer Umbruch
    void setLineHeight(int height);             // Zeilenhöhe setzen
    void setFontSize(uint8_t size);             // Font-Größe (1-4)
    void setPadding(int padding);               // Innenabstand
    
    // Scroll-Funktionen
    void scrollUp(int lines = 1);
    void scrollDown(int lines = 1);
    int getScrollPosition() const { return scrollY; }
    int getMaxScroll() const;
    bool canScrollUp() const { return scrollY > 0; }
    bool canScrollDown() const { return scrollY < getMaxScroll(); }

private:
    std::vector<String> lines;  // Text-Zeilen
    int scrollY;                // Scroll-Offset (in Zeilen)
    int lineHeight;             // Höhe einer Zeile in Pixeln
    uint8_t fontSize;           // Font-Größe
    bool wordWrap;              // Automatischer Umbruch?
    int padding;                // Innenabstand
    int maxVisibleLines;        // Maximale sichtbare Zeilen
    
    // Touch-Scrolling
    bool scrolling;             // Wird gescrollt?
    int16_t lastTouchY;         // Letzte Touch-Y-Position
    int16_t scrollStartY;       // Start-Y beim Scrolling
    
    void drawTextBox(TFT_eSPI* tft);
    void wrapText(const char* text, std::vector<String>& outLines);
    int calculateMaxVisibleLines();
    int getCharWidth(TFT_eSPI* tft);
    int getMaxCharsPerLine(TFT_eSPI* tft);
};

#endif // UI_TEXTBOX_H