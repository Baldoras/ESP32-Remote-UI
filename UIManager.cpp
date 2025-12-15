/**
 * UIManager.cpp
 */

#include "UIManager.h"

UIManager::UIManager(TFT_eSPI* tft, TouchManager* touch)
    : tft(tft), touch(touch), lastTouchState(false), lastTouchX(0), lastTouchY(0), currentPage(nullptr) {
}

UIManager::~UIManager() {
    // Elemente werden nicht gelöscht (Ownership beim Aufrufer)
    elements.clear();
}

bool UIManager::add(UIElement* element) {
    if (!element) return false;
    
    elements.push_back(element);
    element->setNeedsRedraw(true);
    return true;
}

bool UIManager::remove(UIElement* element) {
    if (!element) return false;
    
    for (auto it = elements.begin(); it != elements.end(); ++it) {
        if (*it == element) {
            elements.erase(it);
            return true;
        }
    }
    return false;
}

void UIManager::clear() {
    elements.clear();
}

void UIManager::setCurrentPage(void* page) {
    currentPage = page;
    Serial.printf("UIManager: CurrentPage gesetzt auf %p\n", page);
}

void UIManager::update() {
    if (!touch) return;

    // KRITISCH: Erst IRQ prüfen (schneller GPIO-Check)
    // Nur wenn IRQ aktiv ist, Touch-Daten per SPI holen
    if (!touch->isIRQActive()) {
        // Kein Touch-IRQ → trotzdem Release-Events verarbeiten falls nötig
        if (lastTouchState) {
            // Touch war aktiv, ist jetzt aber beendet
            for (auto* element : elements) {
                if (element && element->isEnabled() && element->isVisible()) {
                    if (shouldProcessElement(element)) {
                        handleElementTouch(element, lastTouchX, lastTouchY, false);
                    }
                }
            }
            lastTouchState = false;
        }
        return;  // Kein Touch-IRQ → früh beenden
    }

    // Touch-Status abrufen (nur wenn IRQ aktiv war)
    bool touchActive = touch->isTouchActive();
    TouchPoint point = touch->getTouchPoint();
    
    if (touchActive && point.valid) {
        // Touch aktiv - an alle Elemente weiterleiten
        for (auto* element : elements) {
            if (element && element->isVisible() && element->isEnabled()) {
                // NUR verarbeiten wenn Element zur aktuellen Page gehört!
                if (shouldProcessElement(element)) {
                    handleElementTouch(element, point.x, point.y, true);
                }
            }
        }
        
        lastTouchX = point.x;
        lastTouchY = point.y;
    } else if (lastTouchState) {
        // Touch gerade beendet - Release-Event
        for (auto* element : elements) {
            if (element && element->isEnabled() && element->isVisible()) {
                // NUR verarbeiten wenn Element zur aktuellen Page gehört!
                if (shouldProcessElement(element)) {
                    handleElementTouch(element, lastTouchX, lastTouchY, false);
                }
            }
        }
    }
    
    lastTouchState = touchActive;
}

bool UIManager::shouldProcessElement(UIElement* element) {
    if (!element) return false;
    
    void* elementOwner = element->getOwnerPage();
    
    // Elemente ohne Owner (z.B. Header/Footer) immer verarbeiten
    if (!elementOwner) return true;
    
    // Element nur verarbeiten wenn es zur aktuellen Page gehört
    bool matches = (elementOwner == currentPage);
    
    return matches;
}

void UIManager::drawAll() {
    if (!tft) return;
    
    for (auto* element : elements) {
        if (element && element->isVisible()) {
            element->draw(tft);
        }
    }
}

void UIManager::drawUpdates() {
    if (!tft) return;
    
    for (int i = 0; i < elements.size(); i++) {
        auto* element = elements[i];

        if (element && element->isVisible() && element->getNeedsRedraw()) {
            
            // KRITISCH: Bounds ausgeben um Element zu identifizieren
            int16_t x, y, w, h;
            element->getBounds(&x, &y, &w, &h);
        
            element->draw(tft);
        }
    }
}

void UIManager::clearScreen(uint16_t color) {
    if (!tft) return;
    
    tft->fillScreen(color);
    
    // Alle Elemente als "needs redraw" markieren
    for (auto* element : elements) {
        if (element) {
            element->setNeedsRedraw(true);
        }
    }
}

UIElement* UIManager::getElement(int index) {
    if (index >= 0 && index < elements.size()) {
        return elements[index];
    }
    return nullptr;
}

void UIManager::printDebugInfo() {
    Serial.println("\n=== UI Manager Debug Info ===");
    Serial.printf("Elements: %d\n", elements.size());
    Serial.printf("Current Page: %p\n", currentPage);
    Serial.printf("Last Touch: %s at (%d, %d)\n", 
                  lastTouchState ? "ACTIVE" : "INACTIVE", 
                  lastTouchX, lastTouchY);
    
    for (int i = 0; i < elements.size(); i++) {
        auto* el = elements[i];
        if (!el) {
            Serial.printf("  [%d] NULL\n", i);
            continue;
        }
        
        int16_t x, y, w, h;
        el->getBounds(&x, &y, &w, &h);
        
        Serial.printf("  [%d] Pos(%d,%d) Size(%dx%d) Visible:%s Enabled:%s Owner:%p\n",
                      i, x, y, w, h,
                      el->isVisible() ? "YES" : "NO",
                      el->isEnabled() ? "YES" : "NO",
                      el->getOwnerPage());
    }
    Serial.println("============================\n");
}

void UIManager::handleElementTouch(UIElement* element, int16_t x, int16_t y, bool pressed) {
    if (!element) return;
    
    element->handleTouch(x, y, pressed);
}