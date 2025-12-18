/**
 * InfoPage.h
 * 
 * System-Informationsseite mit Hardware- und Software-Status
 */

#ifndef INFOPAGE_H
#define INFOPAGE_H

#include "UIPage.h"
#include "UIManager.h"
#include "UITextBox.h"
#include <TFT_eSPI.h>

class InfoPage : public UIPage {
public:
    InfoPage(UIManager* ui, TFT_eSPI* tft);
    
    void build() override;
    void update() override;
    
private:
    void updateInfo();
    
    UITextBox* txtInfo;
    unsigned long lastUpdate;
};

#endif // INFOPAGE_H