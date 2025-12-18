/**
 * RemoteControlPage.h
 * 
 * Fernsteuerungs-Seite mit Joystick-Visualisierung
 */

#ifndef REMOTECONTROLPAGE_H
#define REMOTECONTROLPAGE_H

#include "UIPage.h"
#include "UIManager.h"
#include "UILabel.h"
#include "UIProgressBar.h"
#include <TFT_eSPI.h>

class RemoteControlPage : public UIPage {
public:
    RemoteControlPage(UIManager* ui, TFT_eSPI* tft);
    
    void build() override;
    void update() override;
    
    void setJoystickPosition(int16_t x, int16_t y);
    
private:
    void drawJoystickPosition();
    
    int16_t joystickX, joystickY;
    int16_t joystickAreaX, joystickAreaY, joystickAreaSize;
    int16_t joystickCenterX, joystickCenterY;
    
    UILabel* labelConnectionStatus;
    UILabel* labelJoystickX;
    UILabel* labelJoystickY;
    UIProgressBar* barRemoteBattery;
    UILabel* labelRemoteBatteryValue;
};

#endif // REMOTECONTROLPAGE_H