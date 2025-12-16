/**
 * HomePage.cpp
 * 
 * Startseite mit Navigation zu allen anderen Pages
 */

#include "HomePage.h"
#include "UIButton.h"
#include "UILabel.h"
#include "PageManager.h"
#include "BatteryMonitor.h"
#include "ESPNowManager.h"
#include "Globals.h"

HomePage::HomePage(UIManager* ui, TFT_eSPI* tft) 
    : UIPage("Home", ui, tft) {
}

void HomePage::build() {
    Serial.println("Building HomePage...");
    
    // Willkommens-Text
    UILabel* lblWelcome = new UILabel(
        layout.contentX + 20, 
        layout.contentY + 20, 
        layout.contentWidth - 40, 
        50, 
        "ESP32 Remote Control"
    );
    lblWelcome->setFontSize(3);
    lblWelcome->setAlignment(TextAlignment::CENTER);
    lblWelcome->setTransparent(true);
    addContentElement(lblWelcome);
    
    // Info-Text
    UILabel* lblInfo = new UILabel(
        layout.contentX + 20, 
        layout.contentY + 75, 
        layout.contentWidth - 40, 
        30, 
        "Choose a page:"
    );
    lblInfo->setFontSize(2);
    lblInfo->setAlignment(TextAlignment::CENTER);
    lblInfo->setTransparent(true);
    addContentElement(lblInfo);
    
    // Status-Übersicht
    int16_t statusY = layout.contentY + 115;
    
    // Battery Status
    UILabel* lblBattery = new UILabel(
        layout.contentX + 20, 
        statusY, 
        200, 
        20, 
        "Battery: --.-V (---%)"
    );
    lblBattery->setFontSize(1);
    lblBattery->setAlignment(TextAlignment::LEFT);
    lblBattery->setTextColor(COLOR_GREEN);
    lblBattery->setTransparent(true);
    addContentElement(lblBattery);
    
    // Connection Status
    UILabel* lblConnection = new UILabel(
        layout.contentX + 240, 
        statusY, 
        200, 
        20, 
        "ESP-NOW: Not Connected"
    );
    lblConnection->setFontSize(1);
    lblConnection->setAlignment(TextAlignment::LEFT);
    lblConnection->setTextColor(COLOR_RED);
    lblConnection->setTransparent(true);
    addContentElement(lblConnection);
    
    // Navigation Buttons (2x2 Grid)
    int16_t btnY = layout.contentY + 145;
    int16_t btnWidth = 200;
    int16_t btnHeight = 45;
    int16_t btnSpacing = 10;
    int16_t btnX1 = layout.contentX + 30;
    int16_t btnX2 = btnX1 + btnWidth + btnSpacing;
    
    // Remote Button
    UIButton* btnRemote = new UIButton(btnX1, btnY, btnWidth, btnHeight, "Remote Control");
    btnRemote->on(EventType::CLICK, [](EventData* data) {
        Serial.println("→ Remote Control");
        ::pageManager->showPageDeferred(PAGE_REMOTE);
    });
    addContentElement(btnRemote);
    
    // Connection Button
    UIButton* btnConnection = new UIButton(btnX2, btnY, btnWidth, btnHeight, "Connection");
    btnConnection->on(EventType::CLICK, [](EventData* data) {
        Serial.println("→ Connection");
        ::pageManager->showPageDeferred(PAGE_CONNECTION);
    });
    addContentElement(btnConnection);
    
    btnY += btnHeight + btnSpacing;
    
    // Settings Button
    UIButton* btnSettings = new UIButton(btnX1, btnY, btnWidth, btnHeight, "Settings");
    btnSettings->on(EventType::CLICK, [](EventData* data) {
        Serial.println("→ Settings");
        ::pageManager->showPageDeferred(PAGE_SETTINGS);
    });
    addContentElement(btnSettings);
    
    // Info Button
    UIButton* btnInfo = new UIButton(btnX2, btnY, btnWidth, btnHeight, "System Info");
    btnInfo->on(EventType::CLICK, [](EventData* data) {
        Serial.println("→ System Info");
        ::pageManager->showPageDeferred(PAGE_INFO);
    });
    addContentElement(btnInfo);
    
    Serial.println("  ✅ HomePage build complete");
}

void HomePage::update() {
    // Status-Labels aktualisieren (alle 2 Sekunden)
    static unsigned long lastUpdate = 0;
    if (millis() - lastUpdate > 2000) {
        updateStatus();
        lastUpdate = millis();
    }
}

void HomePage::updateStatus() {
    // Battery Status aktualisieren
    if (contentElements.size() > 2) {
        UILabel* lblBattery = static_cast<UILabel*>(contentElements[2]);
        if (lblBattery) {
            char buffer[64];
            sprintf(buffer, "Battery: %.1fV (%d%%)", 
                    battery.getVoltage(), battery.getPercent());
            lblBattery->setText(buffer);
            
            if (battery.isCritical()) {
                lblBattery->setTextColor(COLOR_RED);
            } else if (battery.isLow()) {
                lblBattery->setTextColor(COLOR_YELLOW);
            } else {
                lblBattery->setTextColor(COLOR_GREEN);
            }
        }
    }
    
    // Connection Status aktualisieren
    if (contentElements.size() > 3) {
        UILabel* lblConnection = static_cast<UILabel*>(contentElements[3]);
        if (lblConnection) {
            if (espNow.isInitialized() && espNow.isConnected()) {
                lblConnection->setText("ESP-NOW: Connected");
                lblConnection->setTextColor(COLOR_GREEN);
            } else {
                lblConnection->setText("ESP-NOW: Not Connected");
                lblConnection->setTextColor(COLOR_RED);
            }
        }
    }
}