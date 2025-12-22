/**
 * SettingsPage.cpp
 * 
 * Einstellungsseite für Helligkeit und Kalibrierung
 */

#include "include/SettingsPage.h"
#include "include/UIButton.h"
#include "include/UILabel.h"
#include "include/UISlider.h"
#include "include/UICheckBox.h"
#include "include/PageManager.h"
#include "include/DisplayHandler.h"
#include "include/UserConfig.h"
#include "include/JoystickHandler.h"
#include "include/Globals.h"

SettingsPage::SettingsPage(UIManager* ui, TFT_eSPI* tft) 
    : UIPage("Settings", ui, tft) {
    setBackButton(true, PAGE_HOME);
}

void SettingsPage::build() {
    Serial.println("Building SettingsPage...");
    
    UILabel* lblTitle = new UILabel(layout.contentX + 20, layout.contentY + 20, layout.contentWidth - 40, 30, "Brightness");
    lblTitle->setFontSize(2);
    lblTitle->setAlignment(TextAlignment::LEFT);
    lblTitle->setTransparent(true);
    addContentElement(lblTitle);
    
    UISlider* sldBrightness = new UISlider(layout.contentX + 20, layout.contentY + 45, 400, 40);
    sldBrightness->setValue(userConfig.getBacklightDefault());
    sldBrightness->setShowValue(true);
    sldBrightness->on(EventType::VALUE_CHANGED, [](EventData* data) {
        uint8_t brightness = map(data->value, 0, 100, BACKLIGHT_MIN, BACKLIGHT_MAX);
        display.setBacklight(brightness);
    });
    addContentElement(sldBrightness);

    UICheckBox* chkAutoShutdown = new UICheckBox(layout.contentX + 20, layout.contentY + 90, 30, "Auto shutdown");
    chkAutoShutdown->setChecked(userConfig.getAutoShutdownEnabled());
    chkAutoShutdown->on(EventType::VALUE_CHANGED, [this](EventData* data) {
        userConfig.setAutoShutdownEnabled(data->value);       
    });
    addContentElement(chkAutoShutdown);

    UILabel* lblInfo = new UILabel(layout.contentX + 20, layout.contentY + 120, layout.contentWidth - 40, 60, "Config via SD-Card config.json");
    lblInfo->setFontSize(1);
    lblInfo->setAlignment(TextAlignment::CENTER);
    lblInfo->setTransparent(true);
    addContentElement(lblInfo);

    UIButton* btnCalibrate = new UIButton(
        layout.contentX + 140,
        layout.contentY + 180,
        200,
        40,
        "Calibrate Center"
    );

    btnCalibrate->on(EventType::CLICK, [](EventData* data) {
        Serial.println("→ Kalibriere Joystick Center...");
        joystick.calibrateCenter();
        Serial.println("  Fertig! Joystick neutral halten!");
    });
    addContentElement(btnCalibrate);
    
    Serial.println("  ✅ SettingsPage build complete");
}