/**
 * ConnectionPage.h
 * 
 * Verbindungsseite für ESP-NOW Pairing
 */

#ifndef CONNECTIONPAGE_H
#define CONNECTIONPAGE_H

#include "UIPage.h"
#include "UIManager.h"
#include "UILabel.h"
#include "UIButton.h"
#include <TFT_eSPI.h>

class ConnectionPage : public UIPage {
public:
    ConnectionPage(UIManager* ui, TFT_eSPI* tft);
    
    void build() override;
    void update() override;
    
    void setPeerMac(const char* macStr);
    
private:
    void updateConnectionStatus();
    void onPairClicked();
    void onDisconnectClicked();
    
    char peerMacStr[18];
    uint8_t peerMac[6];
    bool isPaired;
    bool isConnected;
    
    UILabel* labelStatusValue;
    UILabel* labelOwnMacValue;
    UILabel* labelPeerMacValue;
    UIButton* btnPair;
    UIButton* btnDisconnect;
};

#endif // CONNECTIONPAGE_H