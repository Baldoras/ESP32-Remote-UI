/**
 * UIRadioButton.h
 * 
 * RadioButton-Element (Gruppen-basiert, nur einer aktiv)
 */

#ifndef UI_RADIOBUTTON_H
#define UI_RADIOBUTTON_H

#include "UIElement.h"
#include <vector>

// Forward declaration für Group
class UIRadioGroup;

class UIRadioButton : public UIElement {
public:
    UIRadioButton(int16_t x, int16_t y, int16_t size, const char* label = "", int value = 0);
    ~UIRadioButton();

    // Override virtuelle Funktionen
    void draw(TFT_eSPI* tft) override;
    void handleTouch(int16_t x, int16_t y, bool pressed) override;

    // RadioButton-spezifische Funktionen
    void setSelected(bool selected);
    bool isSelected() const { return selected; }
    void setLabel(const char* label);
    const char* getLabel() const { return label; }
    void setDotColor(uint16_t color);
    int getValue() const { return value; }
    void setValue(int val) { value = val; }
    
    // Gruppe setzen (intern verwendet)
    void setGroup(UIRadioGroup* grp) { group = grp; }

private:
    char label[32];             // Label-Text
    bool selected;              // Selected-Status
    bool wasInside;             // War Touch beim letzten Mal innerhalb?
    uint16_t dotColor;          // Farbe des Punktes
    int16_t circleSize;         // Größe des Kreises
    int value;                  // Wert (für Identifikation)
    UIRadioGroup* group;        // Gruppe (für Deselection)
    
    void drawRadioButton(TFT_eSPI* tft);
};

/**
 * UIRadioGroup - Verwaltet RadioButton-Gruppe
 */
class UIRadioGroup {
public:
    UIRadioGroup();
    ~UIRadioGroup();

    /**
     * RadioButton zur Gruppe hinzufügen
     */
    void add(UIRadioButton* radio);

    /**
     * RadioButton auswählen (andere werden deselected)
     */
    void select(UIRadioButton* radio);

    /**
     * Aktuell ausgewählten Button abrufen
     */
    UIRadioButton* getSelected() const { return selectedButton; }

    /**
     * Aktuellen Wert abrufen
     */
    int getSelectedValue() const;

    /**
     * Button nach Wert auswählen
     */
    void selectByValue(int value);

private:
    std::vector<UIRadioButton*> buttons;
    UIRadioButton* selectedButton;
};

#endif // UI_RADIOBUTTON_H