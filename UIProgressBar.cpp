/**
 * UIProgressBar.cpp
 */

#include "include/UIProgressBar.h"

UIProgressBar::UIProgressBar(int16_t x, int16_t y, int16_t w, int16_t h)
    : UIElement(x, y, w, h), value(0), barColor(COLOR_GREEN), 
      showText(true), showPercentage(true) {
    
    style.bgColor = COLOR_DARKGRAY;
    style.borderColor = COLOR_WHITE;
}

UIProgressBar::~UIProgressBar() {
}

void UIProgressBar::draw(TFT_eSPI* tft) {
    if (!visible) return;
    
    drawProgressBar(tft);
    needsRedraw = false;
}

void UIProgressBar::handleTouch(int16_t tx, int16_t ty, bool isPressed) {
    // ProgressBar ist normalerweise nicht interaktiv
    // Könnte aber onClick für Details unterstützen
    if (!visible || !enabled) return;
    
    bool inside = isPointInside(tx, ty);
    
    if (isPressed && inside && eventHandler.hasHandler(EventType::CLICK)) {
        EventData data = {tx, ty, value, value, nullptr};
        eventHandler.trigger(EventType::CLICK, &data);
    }
}

void UIProgressBar::setValue(int val) {
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

void UIProgressBar::setBarColor(uint16_t color) {
    barColor = color;
    needsRedraw = true;
}

void UIProgressBar::setShowText(bool show) {
    showText = show;
    needsRedraw = true;
}

void UIProgressBar::setShowPercentage(bool show) {
    showPercentage = show;
    needsRedraw = true;
}

void UIProgressBar::drawProgressBar(TFT_eSPI* tft) {
    // Hintergrund
    fillRoundRect(tft, x, y, width, height, style.cornerRadius, style.bgColor);
    
    // Fortschritts-Balken
    int barWidth = (width - 4) * value / 100;
    if (barWidth > 0) {
        fillRoundRect(tft, x + 2, y + 2, barWidth, height - 4, 
                     style.cornerRadius - 1, barColor);
    }
    
    // Rahmen
    drawRoundRect(tft, x, y, width, height, style.cornerRadius, style.borderColor);
    
    // Text (Prozent-Anzeige)
    if (showText) {
        char buffer[8];
        if (showPercentage) {
            sprintf(buffer, "%d%%", value);
        } else {
            sprintf(buffer, "%d", value);
        }
        
        tft->setTextDatum(MC_DATUM);
        tft->setTextColor(style.textColor);
        tft->drawString(buffer, x + width / 2, y + height / 2);
    }
}