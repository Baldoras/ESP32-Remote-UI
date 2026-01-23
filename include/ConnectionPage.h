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
    void checkPairingTimeout();
    void checkEventHandler();    // Registriert Event-Handler
    void onPairClicked();
    void sendPairRequest();  // Sendet PAIR_REQUEST (mit Retry-Logik)
    void onDisconnectClicked();
    
    char peerMacStr[18];
    uint8_t peerMac[6];
    bool isPaired;
    bool isConnected;
    
    // Pairing-Timeout
    unsigned long pairingTimestamp;
    const unsigned long pairingTimeout;  // 30 Sekunden
    
    // Pairing-Retry Mechanismus
    unsigned long lastPairRequestTime;   // Letzte PAIR_REQUEST Sendung
    uint8_t pairRequestCount;            // Anzahl gesendeter Requests
    const unsigned long pairRequestInterval = 5000;  // 5 Sekunden
    const uint8_t maxPairRequests = 5;   // Maximal 5 Versuche
    bool isPairing;                      // Pairing aktiv?
    
    UILabel* labelStatusValue;
    UILabel* labelOwnMacValue;
    UILabel* labelPeerMacValue;
    UIButton* btnPair;
    UIButton* btnDisconnect;
};

#endif // CONNECTIONPAGE_H