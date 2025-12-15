/**
 * UIElement.h
 * 
 * Abstrakte Basisklasse für alle UI-Elemente
 */

#ifndef UI_ELEMENT_H
#define UI_ELEMENT_H

#include <Arduino.h>
#include <TFT_eSPI.h>
#include "UIEventHandler.h"
#include "setupConf.h"

// Element-Style Struktur
struct ElementStyle {
    uint16_t bgColor;
    uint16_t borderColor;
    uint16_t textColor;
    uint8_t borderWidth;
    uint8_t cornerRadius;
};

class UIElement {
public:
    UIElement(int16_t x, int16_t y, int16_t w, int16_t h);
    virtual ~UIElement();

    // Rein virtuelle Funktionen (müssen implementiert werden)
    virtual void draw(TFT_eSPI* tft) = 0;
    virtual void handleTouch(int16_t x, int16_t y, bool pressed) = 0;

    // Gemeinsame Funktionen
    bool isPointInside(int16_t px, int16_t py);
    void setPosition(int16_t x, int16_t y);
    void setSize(int16_t w, int16_t h);
    void setBounds(int16_t x, int16_t y, int16_t w, int16_t h);
    void setVisible(bool visible);
    void setEnabled(bool enabled);
    void setStyle(ElementStyle style);
    
    // Owner-Page Verwaltung
    void setOwnerPage(void* page) { ownerPage = page; }
    void* getOwnerPage() const { return ownerPage; }
    
    // Event-Handler
    void on(EventType type, EventCallback callback);
    void off(EventType type);
    
    // Getter
    bool isVisible() const { return visible; }
    bool isEnabled() const { return enabled; }
    bool isTouched() const { return touched; }
    void getBounds(int16_t* outX, int16_t* outY, int16_t* outW, int16_t* outH);
    
    // Redraw-Flag
    void setNeedsRedraw(bool redraw) { needsRedraw = redraw; }
    bool getNeedsRedraw() const { return needsRedraw; }

protected:
    // Position und Größe
    int16_t x, y;
    int16_t width, height;
    
    // Status
    bool visible;
    bool enabled;
    bool touched;
    bool needsRedraw;
    
    // Style
    ElementStyle style;
    
    // Event-Handler
    UIEventHandler eventHandler;
    
    // Owner-Page (void* um Zirkelbezug zu vermeiden)
    void* ownerPage;
    
    // Letzte Touch-Position
    int16_t lastTouchX, lastTouchY;
    
    // Helper-Funktionen
    void drawRoundRect(TFT_eSPI* tft, int16_t x, int16_t y, int16_t w, int16_t h, 
                       uint8_t r, uint16_t color);
    void fillRoundRect(TFT_eSPI* tft, int16_t x, int16_t y, int16_t w, int16_t h, 
                       uint8_t r, uint16_t color);
};

#endif // UI_ELEMENT_H