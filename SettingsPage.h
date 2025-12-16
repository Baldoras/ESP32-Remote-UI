/**
 * SettingsPage.h
 * 
 * Einstellungsseite für Helligkeit und Kalibrierung
 */

#ifndef SETTINGSPAGE_H
#define SETTINGSPAGE_H

#include "UIPage.h"
#include "UIManager.h"
#include <TFT_eSPI.h>

class SettingsPage : public UIPage {
public:
    SettingsPage(UIManager* ui, TFT_eSPI* tft);
    
    void build() override;
};

#endif // SETTINGSPAGE_H