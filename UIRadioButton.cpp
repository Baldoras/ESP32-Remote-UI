/**
 * UIRadioButton.cpp
 */

#include "include/UIRadioButton.h"

// ═══════════════════════════════════════════════════════════════════════════
// UIRadioButton
// ═══════════════════════════════════════════════════════════════════════════

UIRadioButton::UIRadioButton(int16_t x, int16_t y, int16_t size, const char* lbl, int val)
    : UIElement(x, y, size + 5 + 150, size), selected(false), wasInside(false),
      dotColor(COLOR_BLUE), circleSize(size), value(val), group(nullptr) {
    
    strncpy(label, lbl, sizeof(label) - 1);
    label[sizeof(label) - 1] = '\0';
    
    style.bgColor = COLOR_BLACK;
    style.borderColor = COLOR_WHITE;
    style.textColor = COLOR_WHITE;
    style.borderWidth = 2;
}

UIRadioButton::~UIRadioButton() {
}

void UIRadioButton::draw(TFT_eSPI* tft) {
    if (!visible) return;
    
    drawRadioButton(tft);
    needsRedraw = false;
}

void UIRadioButton::handleTouch(int16_t tx, int16_t ty, bool isPressed) {
    if (!visible || !enabled) return;
    
    bool inside = isPointInside(tx, ty);
    
    if (isPressed) {
        if (inside && !wasInside) {
            // Touch begonnen auf RadioButton
            EventData data = {tx, ty, value, value, nullptr};
            eventHandler.trigger(EventType::PRESS, &data);
        }
    } else {
        // Touch beendet
        if (inside && wasInside && !selected) {
            // Click nur wenn innerhalb begonnen/beendet und noch nicht selected
            if (group != nullptr) {
                group->select(this);
            } else {
                setSelected(true);
            }
            
            EventData data = {tx, ty, value, value, nullptr};
            eventHandler.trigger(EventType::CLICK, &data);
            eventHandler.trigger(EventType::VALUE_CHANGED, &data);
        }
    }
    
    wasInside = inside && isPressed;
}

void UIRadioButton::setSelected(bool sel) {
    if (selected != sel) {
        selected = sel;
        needsRedraw = true;
        
        // onChange Event
        EventData data = {0, 0, value, value, nullptr};
        eventHandler.trigger(EventType::VALUE_CHANGED, &data);
    }
}

void UIRadioButton::setLabel(const char* lbl) {
    strncpy(label, lbl, sizeof(label) - 1);
    label[sizeof(label) - 1] = '\0';
    needsRedraw = true;
}

void UIRadioButton::setDotColor(uint16_t color) {
    dotColor = color;
    needsRedraw = true;
}

void UIRadioButton::drawRadioButton(TFT_eSPI* tft) {
    // Kreis zeichnen
    int16_t circleX = x + circleSize / 2;
    int16_t circleY = y + circleSize / 2;
    int16_t radius = circleSize / 2;
    
    // Hintergrund
    tft->fillCircle(circleX, circleY, radius, style.bgColor);
    
    // Rahmen
    if (enabled) {
        tft->drawCircle(circleX, circleY, radius, style.borderColor);
        tft->drawCircle(circleX, circleY, radius - 1, style.borderColor);
    } else {
        tft->drawCircle(circleX, circleY, radius, COLOR_GRAY);
    }
    
    // Punkt wenn selected
    if (selected) {
        int16_t dotRadius = radius - 4;
        tft->fillCircle(circleX, circleY, dotRadius, dotColor);
    }
    
    // Label rechts neben dem Kreis
    if (strlen(label) > 0) {
        tft->setTextDatum(ML_DATUM);
        tft->setTextColor(enabled ? style.textColor : COLOR_GRAY);
        tft->setTextSize(2);
        tft->drawString(label, x + circleSize + 5, y + circleSize / 2);
    }
}

// ═══════════════════════════════════════════════════════════════════════════
// UIRadioGroup
// ═══════════════════════════════════════════════════════════════════════════

UIRadioGroup::UIRadioGroup()
    : selectedButton(nullptr) {
}

UIRadioGroup::~UIRadioGroup() {
    // Buttons werden nicht gelöscht (Ownership beim Aufrufer)
    buttons.clear();
}

void UIRadioGroup::add(UIRadioButton* radio) {
    if (!radio) return;
    
    buttons.push_back(radio);
    radio->setGroup(this);
    
    // Ersten Button automatisch auswählen
    if (buttons.size() == 1) {
        select(radio);
    }
}

void UIRadioGroup::select(UIRadioButton* radio) {
    if (!radio) return;
    
    // Alle anderen deselektieren
    for (auto* btn : buttons) {
        if (btn != radio && btn->isSelected()) {
            btn->setSelected(false);
        }
    }
    
    // Diesen Button selektieren
    if (!radio->isSelected()) {
        radio->setSelected(true);
    }
    
    selectedButton = radio;
}

int UIRadioGroup::getSelectedValue() const {
    if (selectedButton != nullptr) {
        return selectedButton->getValue();
    }
    return -1;
}

void UIRadioGroup::selectByValue(int value) {
    for (auto* btn : buttons) {
        if (btn->getValue() == value) {
            select(btn);
            return;
        }
    }
}