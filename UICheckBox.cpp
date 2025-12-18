/**
 * UICheckBox.cpp
 */

#include "include/UICheckBox.h"

UICheckBox::UICheckBox(int16_t x, int16_t y, int16_t size, const char* lbl)
    : UIElement(x, y, size + 5 + 150, size), checked(false), wasInside(false),
      checkColor(COLOR_GREEN), boxSize(size) {
    
    strncpy(label, lbl, sizeof(label) - 1);
    label[sizeof(label) - 1] = '\0';
    
    style.bgColor = COLOR_BLACK;
    style.borderColor = COLOR_WHITE;
    style.textColor = COLOR_WHITE;
    style.borderWidth = 2;
    style.cornerRadius = 3;
}

UICheckBox::~UICheckBox() {
}

void UICheckBox::draw(TFT_eSPI* tft) {
    if (!visible) return;
    
    drawCheckBox(tft);
    needsRedraw = false;
}

void UICheckBox::handleTouch(int16_t tx, int16_t ty, bool isPressed) {
    if (!visible || !enabled) return;
    
    bool inside = isPointInside(tx, ty);
    
    if (isPressed) {
        if (inside && !wasInside) {
            // Touch begonnen auf CheckBox
            EventData data = {tx, ty, 0, 0, nullptr};
            eventHandler.trigger(EventType::PRESS, &data);
        }
    } else {
        // Touch beendet
        if (inside && wasInside) {
            // Click nur wenn innerhalb begonnen und beendet
            toggle();
            
            EventData data = {tx, ty, checked ? 1 : 0, checked ? 0 : 1, nullptr};
            eventHandler.trigger(EventType::CLICK, &data);
            eventHandler.trigger(EventType::VALUE_CHANGED, &data);
        }
    }
    
    wasInside = inside && isPressed;
}

void UICheckBox::setChecked(bool chk) {
    if (checked != chk) {
        int oldValue = checked ? 1 : 0;
        checked = chk;
        needsRedraw = true;
        
        // onChange Event
        EventData data = {0, 0, checked ? 1 : 0, oldValue, nullptr};
        eventHandler.trigger(EventType::VALUE_CHANGED, &data);
    }
}

void UICheckBox::toggle() {
    setChecked(!checked);
}

void UICheckBox::setLabel(const char* lbl) {
    strncpy(label, lbl, sizeof(label) - 1);
    label[sizeof(label) - 1] = '\0';
    needsRedraw = true;
}

void UICheckBox::setCheckColor(uint16_t color) {
    checkColor = color;
    needsRedraw = true;
}

void UICheckBox::drawCheckBox(TFT_eSPI* tft) {
    // Box zeichnen
    int16_t boxX = x;
    int16_t boxY = y;
    
    // Hintergrund
    fillRoundRect(tft, boxX, boxY, boxSize, boxSize, style.cornerRadius, style.bgColor);
    
    // Rahmen (dicker wenn enabled)
    if (enabled) {
        drawRoundRect(tft, boxX, boxY, boxSize, boxSize, style.cornerRadius, style.borderColor);
    } else {
        drawRoundRect(tft, boxX, boxY, boxSize, boxSize, style.cornerRadius, COLOR_GRAY);
    }
    
    // Häkchen wenn checked
    if (checked) {
        drawCheckMark(tft, boxX, boxY, boxSize);
    }
    
    // Label rechts neben der Box
    if (strlen(label) > 0) {
        tft->setTextDatum(ML_DATUM);
        tft->setTextColor(enabled ? style.textColor : COLOR_GRAY);
        tft->setTextSize(2);
        tft->drawString(label, boxX + boxSize + 5, boxY + boxSize / 2);
    }
}

void UICheckBox::drawCheckMark(TFT_eSPI* tft, int16_t bx, int16_t by, int16_t size) {
    // Häkchen-Form zeichnen
    int16_t margin = size / 4;
    int16_t cx = bx + margin;
    int16_t cy = by + size / 2;
    
    // Kurzer Strich (unten links nach mitte)
    int16_t x1 = cx;
    int16_t y1 = cy;
    int16_t x2 = cx + size / 4;
    int16_t y2 = cy + size / 4;
    
    for (int i = 0; i < 3; i++) {
        tft->drawLine(x1, y1 + i, x2, y2 + i, checkColor);
    }
    
    // Langer Strich (mitte nach oben rechts)
    int16_t x3 = bx + size - margin;
    int16_t y3 = by + margin;
    
    for (int i = 0; i < 3; i++) {
        tft->drawLine(x2, y2 + i, x3, y3 + i, checkColor);
    }
}