/**
 * UIElement.cpp
 */

#include "UIElement.h"

UIElement::UIElement(int16_t x, int16_t y, int16_t w, int16_t h)
    : x(x), y(y), width(w), height(h),
      visible(true), enabled(true), touched(false), needsRedraw(true),
      ownerPage(nullptr),
      lastTouchX(0), lastTouchY(0) {
    
    // Standard-Style
    style.bgColor = COLOR_DARKGRAY;
    style.borderColor = COLOR_WHITE;
    style.textColor = COLOR_WHITE;
    style.borderWidth = 2;
    style.cornerRadius = 5;
}

UIElement::~UIElement() {
}

bool UIElement::isPointInside(int16_t px, int16_t py) {
    return (px >= x && px < (x + width) && 
            py >= y && py < (y + height));
}

void UIElement::setPosition(int16_t newX, int16_t newY) {
    if (x != newX || y != newY) {
        x = newX;
        y = newY;
        needsRedraw = true;
    }
}

void UIElement::setSize(int16_t w, int16_t h) {
    if (width != w || height != h) {
        width = w;
        height = h;
        needsRedraw = true;
    }
}

void UIElement::setBounds(int16_t newX, int16_t newY, int16_t w, int16_t h) {
    if (x != newX || y != newY || width != w || height != h) {
        x = newX;
        y = newY;
        width = w;
        height = h;
        needsRedraw = true;
    }
}

void UIElement::setVisible(bool vis) {
    if (visible != vis) {
        visible = vis;
        needsRedraw = true;
    }
}

void UIElement::setEnabled(bool en) {
    if (enabled != en) {
        enabled = en;
        needsRedraw = true;
    }
}

void UIElement::setStyle(ElementStyle newStyle) {
    style = newStyle;
    needsRedraw = true;
}

void UIElement::on(EventType type, EventCallback callback) {
    eventHandler.on(type, callback);
}

void UIElement::off(EventType type) {
    eventHandler.off(type);
}

void UIElement::getBounds(int16_t* outX, int16_t* outY, int16_t* outW, int16_t* outH) {
    if (outX) *outX = x;
    if (outY) *outY = y;
    if (outW) *outW = width;
    if (outH) *outH = height;
}

void UIElement::drawRoundRect(TFT_eSPI* tft, int16_t x, int16_t y, int16_t w, int16_t h, 
                              uint8_t r, uint16_t color) {
    if (!tft) {
        Serial.println("ERROR: UIElement::drawRoundRect() - tft is nullptr!");
        return;
    }
    tft->drawRoundRect(x, y, w, h, r, color);
}

void UIElement::fillRoundRect(TFT_eSPI* tft, int16_t x, int16_t y, int16_t w, int16_t h, 
                              uint8_t r, uint16_t color) {
    if (!tft) {
        Serial.println("ERROR: UIElement::fillRoundRect() - tft is nullptr!");
        return;
    }
    tft->fillRoundRect(x, y, w, h, r, color);
}