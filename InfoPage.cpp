/**
 * InfoPage.cpp
 * 
 * System-Informationsseite mit Hardware- und Software-Status
 */

#include "InfoPage.h"
#include "UIButton.h"
#include "UITextBox.h"
#include "PageManager.h"
#include "BatteryMonitor.h"
#include "TouchManager.h"
#include "SDCardHandler.h"
#include "ESPNowManager.h"
#include "JoystickHandler.h"
#include "Globals.h"
#include <ESP.h>

InfoPage::InfoPage(UIManager* ui, TFT_eSPI* tft) 
    : UIPage("System Info", ui, tft)
    , txtInfo(nullptr)
    , lastUpdate(0) {
    setBackButton(true, PAGE_HOME);
}

void InfoPage::build() {
    Serial.println("Building InfoPage...");
    
    int16_t tbY = layout.contentY + 10;
    int16_t tbHeight = layout.contentHeight - 60;
    
    txtInfo = new UITextBox(layout.contentX + 10, tbY, layout.contentWidth - 20, tbHeight);
    txtInfo->setFontSize(1);
    txtInfo->setLineHeight(14);
    addContentElement(txtInfo);
    
    updateInfo();
    
    int16_t buttonHeight = 40;
    UIButton* btnRefresh = new UIButton(
        layout.contentX + (layout.contentWidth - 150) / 2,
        layout.contentY + layout.contentHeight - buttonHeight - 10, 
        150, buttonHeight, "Refresh"
    );
    btnRefresh->on(EventType::CLICK, [this](EventData* data) { updateInfo(); });
    addContentElement(btnRefresh);
    
    Serial.println("  ✅ InfoPage build complete");
}

void InfoPage::update() {
    if (millis() - lastUpdate > 10000) {
        updateInfo();
    }
}

void InfoPage::updateInfo() {
    if (!txtInfo) return;
    
    txtInfo->clear();
    txtInfo->appendLine("=== System Information ===");
    txtInfo->appendLine("");
    txtInfo->appendLine("Hardware:");
    txtInfo->appendLine("  ESP32-S3-N16R8");
    txtInfo->appendLine("  Flash: 16MB / PSRAM: 8MB");
    txtInfo->appendLine("");
    
    txtInfo->appendLine("Display:");
    txtInfo->appendLine("  ST7796 480x320");
    char buffer[64];
    sprintf(buffer, "  Touch: %s", touch.isAvailable() ? "OK" : "N/A");
    txtInfo->appendLine(buffer);
    txtInfo->appendLine("");
    
    txtInfo->appendLine("Battery:");
    sprintf(buffer, "  Voltage: %.2fV", battery.getVoltage());
    txtInfo->appendLine(buffer);
    sprintf(buffer, "  Charge: %d%%", battery.getPercent());
    txtInfo->appendLine(buffer);
    txtInfo->appendLine("");
    
    txtInfo->appendLine("SD-Card:");
    if (sdCard.isAvailable()) {
        sprintf(buffer, "  Free: %.1f MB", sdCard.getFreeSpace() / 1024.0 / 1024.0);
        txtInfo->appendLine(buffer);
    } else {
        txtInfo->appendLine("  Not available");
    }
    txtInfo->appendLine("");
    
    txtInfo->appendLine("ESP-NOW:");
    sprintf(buffer, "  Init: %s", espNow.isInitialized() ? "Yes" : "No");
    txtInfo->appendLine(buffer);
    sprintf(buffer, "  Connected: %s", (espNow.isInitialized() && espNow.isConnected()) ? "Yes" : "No");
    txtInfo->appendLine(buffer);
    txtInfo->appendLine("");
    
    txtInfo->appendLine("Joystick:");
    sprintf(buffer, "  X-Achse: %d (raw: %d)", joystick.getX(), joystick.getRawX());
    txtInfo->appendLine(buffer);
    sprintf(buffer, "  Y-Achse: %d (raw: %d)", joystick.getY(), joystick.getRawY());
    txtInfo->appendLine(buffer);
    sprintf(buffer, "  Neutral: %s", joystick.isNeutral() ? "Yes" : "No");
    txtInfo->appendLine(buffer);
    txtInfo->appendLine("");

    txtInfo->appendLine("System:");
    sprintf(buffer, "  Free Heap: %d bytes", ESP.getFreeHeap());
    txtInfo->appendLine(buffer);
    sprintf(buffer, "  Uptime: %lu sec", millis() / 1000);
    txtInfo->appendLine(buffer);
    
    txtInfo->scrollToTop();
    lastUpdate = millis();
}