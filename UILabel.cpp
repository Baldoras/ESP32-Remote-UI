/**
 * UILabel.cpp
 */

#include "UILabel.h"

UILabel::UILabel(int16_t x, int16_t y, int16_t w, int16_t h, const char* txt)
    : UIElement(x, y, w, h), alignment(TextAlignment::CENTER), 
      fontSize(2), transparent(false) {
    
    strncpy(text, txt, sizeof(text) - 1);
    text[sizeof(text) - 1] = '\0';
    
    style.bgColor = COLOR_BLACK;
    style.borderWidth = 0;
}

UILabel::~UILabel() {
}

void UILabel::draw(TFT_eSPI* tft) {
    if (!visible) return;
    
    drawLabel(tft);
    needsRedraw = false;
}

void UILabel::handleTouch(int16_t tx, int16_t ty, bool isPressed) {
    // Label ist nicht interaktiv
    // Könnte aber onClick-Event unterstützen falls gewünscht
    if (!visible || !enabled) return;
    
    bool inside = isPointInside(tx, ty);
    
    if (isPressed && inside && eventHandler.hasHandler(EventType::CLICK)) {
        EventData data = {tx, ty, 0, 0, nullptr};
        eventHandler.trigger(EventType::CLICK, &data);
    }
}

void UILabel::setText(const char* txt) {
    strncpy(text, txt, sizeof(text) - 1);
    text[sizeof(text) - 1] = '\0';

    needsRedraw = true;
}

void UILabel::setTextColor(uint16_t color) {
    style.textColor = color;
    needsRedraw = true;
}

void UILabel::setAlignment(TextAlignment align) {
    alignment = align;
    needsRedraw = true;
}

void UILabel::setFontSize(uint8_t size) {
    if (size >= 1 && size <= 7) {
        fontSize = size;
        needsRedraw = true;
    }
}

void UILabel::setTransparent(bool trans) {
    transparent = trans;
    needsRedraw = true;
}

void UILabel::drawLabel(TFT_eSPI* tft) {
    // Hintergrund (falls nicht transparent)
    if (!transparent) {
        if (style.cornerRadius > 0) {
            fillRoundRect(tft, x, y, width, height, style.cornerRadius, style.bgColor);
        } else {
            tft->fillRect(x, y, width, height, style.bgColor);
        }
    }
    
    // Rahmen (falls borderWidth > 0)
    if (style.borderWidth > 0) {
        if (style.cornerRadius > 0) {
            drawRoundRect(tft, x, y, width, height, style.cornerRadius, style.borderColor);
        } else {
            tft->drawRect(x, y, width, height, style.borderColor);
        }
    }
    
    // Text
    tft->setTextSize(fontSize);
    tft->setTextDatum(getDatum());
    tft->setTextColor(style.textColor, transparent ? style.bgColor : style.bgColor);
    
    int16_t textX = x;
    int16_t textY = y + height / 2;
    
    switch (alignment) {
        case TextAlignment::LEFT:
            textX = x + 5;
            break;
        case TextAlignment::CENTER:
            textX = x + width / 2;
            break;
        case TextAlignment::RIGHT:
            textX = x + width - 5;
            break;
    }
    
    tft->drawString(text, textX, textY);
}

uint8_t UILabel::getDatum() {
    switch (alignment) {
        case TextAlignment::LEFT:
            return ML_DATUM;
        case TextAlignment::CENTER:
            return MC_DATUM;
        case TextAlignment::RIGHT:
            return MR_DATUM;
    }
    return MC_DATUM;
}