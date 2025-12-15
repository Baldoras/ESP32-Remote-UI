/**
 * UIManager.h
 * 
 * UI-Manager - Verwaltet alle UI-Elemente
 */

#ifndef UI_MANAGER_H
#define UI_MANAGER_H

#include <Arduino.h>
#include <TFT_eSPI.h>
#include <vector>
#include "UIElement.h"
#include "TouchManager.h"

class UIManager {
public:
    UIManager(TFT_eSPI* tft, TouchManager* touch);
    ~UIManager();

    bool add(UIElement* element);
    bool remove(UIElement* element);
    void clear();
    void update();
    void drawAll();
    void drawUpdates();
    void clearScreen(uint16_t color = COLOR_BLACK);
    UIElement* getElement(int index);
    int getElementCount() const { return elements.size(); }
    void printDebugInfo();
    
    // Aktuelle Page setzen (für Touch-Event-Filterung)
    void setCurrentPage(void* page);

private:
    TFT_eSPI* tft;
    TouchManager* touch;
    std::vector<UIElement*> elements;
    bool lastTouchState;
    int16_t lastTouchX;
    int16_t lastTouchY;
    void* currentPage;  // Aktuelle sichtbare Page
    
    void handleElementTouch(UIElement* element, int16_t x, int16_t y, bool pressed);
    bool shouldProcessElement(UIElement* element);
};

#endif // UI_MANAGER_H