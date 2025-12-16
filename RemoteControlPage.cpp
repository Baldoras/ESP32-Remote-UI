/**
 * RemoteControlPage.cpp
 * 
 * Fernsteuerungs-Seite mit Joystick-Visualisierung
 */

#include "RemoteControlPage.h"
#include "UIButton.h"
#include "UILabel.h"
#include "UIProgressBar.h"
#include "PageManager.h"
#include "ESPNowManager.h"
#include "Globals.h"

RemoteControlPage::RemoteControlPage(UIManager* ui, TFT_eSPI* tft)
    : UIPage("Remote Control", ui, tft)
    , joystickX(0)
    , joystickY(0)
    , labelConnectionStatus(nullptr)
    , labelJoystickX(nullptr)
    , labelJoystickY(nullptr)
    , barRemoteBattery(nullptr)
    , labelRemoteBatteryValue(nullptr)
{
    setBackButton(true, PAGE_HOME);
}

void RemoteControlPage::build() {
    Serial.println("Building RemoteControlPage...");
    
    // Status-Bar
    int16_t statusY = layout.contentY + 5;
    labelConnectionStatus = new UILabel(layout.contentX + 10, statusY, 150, 25, "DISCONNECTED");
    labelConnectionStatus->setAlignment(TextAlignment::LEFT);
    labelConnectionStatus->setFontSize(1);
    labelConnectionStatus->setTextColor(COLOR_RED);
    labelConnectionStatus->setTransparent(true);
    addContentElement(labelConnectionStatus);
    
    // Joystick-Bereich
    int16_t joyStartY = layout.contentY + 40;
    joystickAreaSize = 180;
    joystickAreaX = layout.contentX + 280;
    joystickAreaY = joyStartY;
    joystickCenterX = joystickAreaX + joystickAreaSize / 2;
    joystickCenterY = joystickAreaY + joystickAreaSize / 2;
    
    int16_t joyValuesY = joystickAreaY + joystickAreaSize + 10;
    
    labelJoystickX = new UILabel(joystickAreaX + (joystickAreaSize / 2), joyValuesY, joystickAreaSize / 2 - 5, 20, "X: 0");
    labelJoystickX->setAlignment(TextAlignment::LEFT);
    labelJoystickX->setFontSize(1);
    labelJoystickX->setTransparent(true);
    addContentElement(labelJoystickX);
    
    labelJoystickY = new UILabel(joystickAreaX, joyValuesY, joystickAreaSize / 2 - 5, 25, "Y: 0");
    labelJoystickY->setAlignment(TextAlignment::LEFT);
    labelJoystickY->setFontSize(1);
    labelJoystickY->setTransparent(true);
    addContentElement(labelJoystickY);
    
    // Battery-Anzeige
    int16_t batteryY = layout.contentY + layout.contentHeight - 60;
    UILabel* labelRemoteBattery = new UILabel(layout.contentX + 10, batteryY, 150, 25, "Remote Battery:");
    labelRemoteBattery->setAlignment(TextAlignment::LEFT);
    labelRemoteBattery->setFontSize(1);
    labelRemoteBattery->setTransparent(true);
    addContentElement(labelRemoteBattery);
    
    batteryY += 25;
    int16_t batteryWidth = (layout.contentWidth - 20) / 2;
    
    barRemoteBattery = new UIProgressBar(layout.contentX + 10, batteryY, batteryWidth - 80, 25);
    barRemoteBattery->setValue(0);
    barRemoteBattery->setBarColor(COLOR_RED);
    barRemoteBattery->setShowText(false);
    addContentElement(barRemoteBattery);
    
    labelRemoteBatteryValue = new UILabel(layout.contentX + 10 + batteryWidth - 70, batteryY, 70, 25, "0.0V");
    labelRemoteBatteryValue->setAlignment(TextAlignment::CENTER);
    labelRemoteBatteryValue->setFontSize(1);
    labelRemoteBatteryValue->setTransparent(true);
    addContentElement(labelRemoteBatteryValue);
    
    Serial.println("  ✅ RemoteControlPage build complete");
}

void RemoteControlPage::update() {
    
    if (!labelJoystickX || !labelJoystickY || !labelConnectionStatus) {
        Serial.println("  Labels nicht initialisiert");
        return;
    }
    
    // Connection Status
    static bool wasConnected = false;
    bool isConnected = espNow.isInitialized() && espNow.isConnected();
    
    if (isConnected != wasConnected) {
        if (isConnected) {
            labelConnectionStatus->setText("CONNECTED");
            labelConnectionStatus->setTextColor(COLOR_GREEN);
        } else {
            labelConnectionStatus->setText("DISCONNECTED");
            labelConnectionStatus->setTextColor(COLOR_RED);
        }
        wasConnected = isConnected;
    }
    
    // Joystick
    static int16_t lastJoyX = 0, lastJoyY = 0;
    if (joystickX != lastJoyX || joystickY != lastJoyY) {

        drawJoystickPosition();
                    
        lastJoyX = joystickX;
        lastJoyY = joystickY;
        
        char buffer[16];
        sprintf(buffer, "X: %d", joystickX);
        labelJoystickX->setText(buffer);
        sprintf(buffer, "Y: %d", joystickY);
        labelJoystickY->setText(buffer);
    }
}

void RemoteControlPage::setJoystickPosition(int16_t x, int16_t y) {
    joystickX = constrain(x, -100, 100);
    joystickY = constrain(y, -100, 100);
}

void RemoteControlPage::drawJoystickPosition() {
    
    if (!tft) {
        Serial.println("    ERROR: tft ist nullptr!");
        return;
    }

    tft->fillRect(joystickAreaX, joystickAreaY, joystickAreaSize, joystickAreaSize, COLOR_BLACK);

    int16_t radius = joystickAreaSize / 2 - 5;
    tft->drawCircle(joystickCenterX, joystickCenterY, radius, COLOR_WHITE);
    tft->drawLine(joystickCenterX - 10, joystickCenterY, joystickCenterX + 10, joystickCenterY, COLOR_GRAY);
    tft->drawLine(joystickCenterX, joystickCenterY - 10, joystickCenterX, joystickCenterY + 10, COLOR_GRAY);

    int16_t posX = joystickCenterX + (joystickX * radius) / 100;
    int16_t posY = joystickCenterY - (joystickY * radius) / 100;
    
    tft->fillCircle(posX + 1, posY + 1, 8, COLOR_DARKGRAY);
    tft->fillCircle(posX, posY, 8, COLOR_BLUE);
    tft->fillCircle(posX - 2, posY - 2, 3, COLOR_CYAN);
}