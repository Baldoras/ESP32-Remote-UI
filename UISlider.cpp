/**
 * UISlider.cpp - FIXED VERSION
 * 
 * FIX: Slider löscht nur innerhalb eigener Bounds (kein Header-Überschreiben!)
 */

#include "UISlider.h"

UISlider::UISlider(int16_t x, int16_t y, int16_t w, int16_t h)
    : UIElement(x, y, w, h), value(50), dragging(false), 
      knobColor(COLOR_WHITE), barColor(COLOR_BLUE), showValue(true) {
    
    knobRadius = h / 2 - 2;
    if (knobRadius < 5) knobRadius = 5;
    if (knobRadius > 10) knobRadius = 10;
    
    style.bgColor = COLOR_BLACK;
    style.borderColor = COLOR_WHITE;
}

UISlider::~UISlider() {
}

void UISlider::draw(TFT_eSPI* tft) {
    if (!visible) return;
    
    drawSlider(tft);
    needsRedraw = false;
}

void UISlider::handleTouch(int16_t tx, int16_t ty, bool isPressed) {
    if (!visible || !enabled) return;
    
    bool inside = isPointInside(tx, ty);
    
    if (isPressed) {
        if (inside) {
            if (!dragging) {
                // Drag starten
                dragging = true;
                EventData data = {tx, ty, value, value, nullptr};
                eventHandler.trigger(EventType::DRAG_START, &data);
            }
            
            // Wert aktualisieren
            updateValueFromTouch(tx);
        }
    } else {
        // Touch beendet
        if (dragging) {
            dragging = false;
            EventData data = {tx, ty, value, value, nullptr};
            eventHandler.trigger(EventType::DRAG_END, &data);
            
            if (inside) {
                eventHandler.trigger(EventType::CLICK, &data);
            }
        }
    }
}

void UISlider::setValue(int val) {
    // Wert begrenzen auf 0-100
    if (val < 0) val = 0;
    if (val > 100) val = 100;
    
    if (value != val) {
        int oldValue = value;
        value = val;
        needsRedraw = true;
        
        // onChange Event
        EventData data = {0, 0, value, oldValue, nullptr};
        eventHandler.trigger(EventType::VALUE_CHANGED, &data);
    }
}

void UISlider::setKnobColor(uint16_t color) {
    knobColor = color;
    needsRedraw = true;
}

void UISlider::setBarColor(uint16_t color) {
    barColor = color;
    needsRedraw = true;
}

void UISlider::setShowValue(bool show) {
    showValue = show;
    needsRedraw = true;
}

void UISlider::drawSlider(TFT_eSPI* tft) {
    // Padding links/rechts für Knob-Radius
    int padding = knobRadius + 2;
    
    // Hintergrund-Schiene mit Padding
    int railX = x + padding;
    int railWidth = width - (2 * padding);
    int railY = y + height / 2 - 3;
    int railHeight = 6;
    
    // ✅ FIX: NUR innerhalb Widget-Bounds löschen (KEIN Überschreiben!)
    tft->fillRect(x, y, width, height, style.bgColor);
    
    // Hintergrund-Schiene zeichnen (mit Padding)
    fillRoundRect(tft, railX, railY, railWidth, railHeight, 3, COLOR_DARKGRAY);
    
    // Gefüllter Balken (bis zum Knob)
    int knobX = getKnobX();
    int filledWidth = knobX - railX;
    if (filledWidth > 0) {
        fillRoundRect(tft, railX, railY, filledWidth, railHeight, 3, barColor);
    }
    
    // Rahmen um Schiene
    drawRoundRect(tft, railX, railY, railWidth, railHeight, 3, style.borderColor);
    
    // Knob (Schieberegler)
    tft->fillCircle(knobX, y + height / 2, knobRadius, knobColor);
    tft->drawCircle(knobX, y + height / 2, knobRadius, style.borderColor);
    
    // Highlight auf Knob (für 3D-Effekt)
    if (!dragging) {
        tft->fillCircle(knobX - 2, y + height / 2 - 2, knobRadius / 3, COLOR_WHITE);
    }
    
    // ✅ FIX: Wert INNERHALB des Widgets anzeigen (rechts vom Slider)
    if (showValue) {
        char buffer[8];
        sprintf(buffer, "%d%%", value);
        
        tft->setTextDatum(MR_DATUM);  // Middle Right
        tft->setTextColor(style.textColor, COLOR_BLACK);  // Transparenter Hintergrund
        tft->setTextSize(2);
        
        // Wert rechts neben dem Slider (außerhalb Widget)
        tft->drawString(buffer, x + width + 40, y + height / 2);
    }
}

void UISlider::updateValueFromTouch(int16_t tx) {
    // Touch-X in Wert umrechnen (0-100)
    int localX = tx - x;
    int newValue = (localX * 100) / width;
    
    // Begrenzen
    if (newValue < 0) newValue = 0;
    if (newValue > 100) newValue = 100;
    
    setValue(newValue);
}

int UISlider::getKnobX() {
    // Padding links/rechts für Knob-Radius
    int padding = knobRadius + 2;
    
    // X-Position des Knobs berechnen (mit Padding)
    int railWidth = width - (2 * padding);
    return x + padding + (railWidth * value) / 100;
}