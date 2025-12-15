/**
 * UIButton.cpp
 */

#include "UIButton.h"

UIButton::UIButton(int16_t x, int16_t y, int16_t w, int16_t h, const char* txt)
    : UIElement(x, y, w, h), pressed(false), wasInside(false) {
    
    strncpy(text, txt, sizeof(text) - 1);
    text[sizeof(text) - 1] = '\0';
    
    normalColor = COLOR_BLUE;
    pressedColor = COLOR_DARKGRAY;
    style.bgColor = normalColor;
}

UIButton::~UIButton() {
}

void UIButton::draw(TFT_eSPI* tft) {
    if (!visible) {
        return;
    }

    drawButton(tft, pressed);

    needsRedraw = false;
}

void UIButton::handleTouch(int16_t tx, int16_t ty, bool isPressed) {
    if (!visible || !enabled) return;
    
    bool inside = isPointInside(tx, ty);
    
    if (isPressed) {
        // Touch aktiv
        if (inside) {
            if (!pressed) {
                // Touch begonnen auf Button
                pressed = true;
                needsRedraw = true;
                EventData data = {tx, ty, 0, 0, nullptr};
                eventHandler.trigger(EventType::PRESS, &data);
            } else if (!wasInside) {
                // Touch wieder in Button-Bereich
                EventData data = {tx, ty, 0, 0, nullptr};
                eventHandler.trigger(EventType::HOVER, &data);
            }
        } else if (pressed && wasInside) {
            // Touch verlässt Button-Bereich
            EventData data = {tx, ty, 0, 0, nullptr};
            eventHandler.trigger(EventType::LEAVE, &data);
        }
    } else {
        // Touch beendet
        if (pressed) {
            pressed = false;
            needsRedraw = true;
            
            EventData data = {tx, ty, 0, 0, nullptr};
            eventHandler.trigger(EventType::RELEASE, &data);
            
            // Click nur wenn innerhalb beendet
            if (inside) {
                eventHandler.trigger(EventType::CLICK, &data);
            }
        }
    }
    
    wasInside = inside;
}

void UIButton::setText(const char* txt) {
    strncpy(text, txt, sizeof(text) - 1);
    text[sizeof(text) - 1] = '\0';
    needsRedraw = true;
}

void UIButton::setTextColor(uint16_t color) {
    style.textColor = color;
    needsRedraw = true;
}

void UIButton::setPressedColor(uint16_t color) {
    pressedColor = color;
    needsRedraw = true;
}

void UIButton::drawButton(TFT_eSPI* tft, bool isPressed) {
    // Hintergrund
    uint16_t bgColor = isPressed ? pressedColor : normalColor;

    fillRoundRect(tft, x, y, width, height, style.cornerRadius, bgColor);
    
    // Rahmen
    if (!enabled) {
        drawRoundRect(tft, x, y, width, height, style.cornerRadius, COLOR_GRAY);
    } else {
        drawRoundRect(tft, x, y, width, height, style.cornerRadius, style.borderColor);
    }
    
    // Text zentriert
    tft->setTextDatum(MC_DATUM);
    tft->setTextColor(enabled ? style.textColor : COLOR_GRAY);
    tft->drawString(text, x + width / 2, y + height / 2);
}