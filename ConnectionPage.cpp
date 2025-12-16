/**
 * ConnectionPage.cpp
 * 
 * Verbindungsseite für ESP-NOW Pairing
 */

#include "ConnectionPage.h"
#include "UIButton.h"
#include "UILabel.h"
#include "PageManager.h"
#include "ESPNowManager.h"
#include "SDCardHandler.h"
#include "Globals.h"

ConnectionPage::ConnectionPage(UIManager* ui, TFT_eSPI* tft)
    : UIPage("Connection", ui, tft)
    , isPaired(false)
    , isConnected(false)
    , labelStatusValue(nullptr)
    , labelOwnMacValue(nullptr)
    , labelPeerMacValue(nullptr)
    , btnPair(nullptr)
    , btnDisconnect(nullptr)
{
    strncpy(peerMacStr, "00:00:00:00:00:00", sizeof(peerMacStr));
    memset(peerMac, 0, 6);
    setBackButton(true, PAGE_HOME);
}

void ConnectionPage::build() {
    Serial.println("Building ConnectionPage...");
    
    int16_t contentX = layout.contentX;
    int16_t contentY = layout.contentY;
    int16_t contentW = layout.contentWidth;
    
    int16_t yPos = contentY + 10;
    int16_t labelWidth = 120;
    int16_t valueX = contentX + labelWidth + 10;
    int16_t valueWidth = contentW - labelWidth - 20;
    
    UILabel* labelStatus = new UILabel(contentX + 10, yPos, labelWidth, 30, "Status:");
    labelStatus->setAlignment(TextAlignment::LEFT);
    labelStatus->setFontSize(2);
    labelStatus->setTransparent(true);
    addContentElement(labelStatus);
    
    labelStatusValue = new UILabel(valueX, yPos, valueWidth, 30, "Disconnected");
    labelStatusValue->setAlignment(TextAlignment::LEFT);
    labelStatusValue->setFontSize(2);
    labelStatusValue->setTextColor(COLOR_RED);
    labelStatusValue->setTransparent(true);
    addContentElement(labelStatusValue);
    
    yPos += 40;
    
    UILabel* labelOwnMac = new UILabel(contentX + 10, yPos, labelWidth, 25, "Own MAC:");
    labelOwnMac->setAlignment(TextAlignment::LEFT);
    labelOwnMac->setFontSize(1);
    labelOwnMac->setTransparent(true);
    addContentElement(labelOwnMac);
    
    labelOwnMacValue = new UILabel(valueX, yPos, valueWidth, 25, "00:00:00:00:00:00");
    labelOwnMacValue->setAlignment(TextAlignment::LEFT);
    labelOwnMacValue->setFontSize(1);
    labelOwnMacValue->setTextColor(COLOR_CYAN);
    labelOwnMacValue->setTransparent(true);
    addContentElement(labelOwnMacValue);
    
    yPos += 30;
    
    UILabel* labelPeerMac = new UILabel(contentX + 10, yPos, labelWidth, 25, "Peer MAC:");
    labelPeerMac->setAlignment(TextAlignment::LEFT);
    labelPeerMac->setFontSize(1);
    labelPeerMac->setTransparent(true);
    addContentElement(labelPeerMac);
    
    labelPeerMacValue = new UILabel(valueX, yPos, valueWidth, 25, peerMacStr);
    labelPeerMacValue->setAlignment(TextAlignment::LEFT);
    labelPeerMacValue->setFontSize(1);
    labelPeerMacValue->setTextColor(COLOR_YELLOW);
    labelPeerMacValue->setTransparent(true);
    addContentElement(labelPeerMacValue);
    
    yPos += 50;
    
    int16_t btnWidth = 140;
    int16_t btnHeight = 40;
    int16_t btnSpacing = 10;
    int16_t btnX = contentX + 10;
    
    btnPair = new UIButton(btnX, yPos, btnWidth, btnHeight, "PAIR");
    btnPair->on(EventType::CLICK, [this](EventData* data) {
        this->onPairClicked();
    });
    addContentElement(btnPair);
    
    btnX += btnWidth + btnSpacing;
    
    btnDisconnect = new UIButton(btnX, yPos, btnWidth, btnHeight, "DISCONNECT");
    btnDisconnect->on(EventType::CLICK, [this](EventData* data) {
        this->onDisconnectClicked();
    });
    btnDisconnect->setEnabled(false);
    addContentElement(btnDisconnect);
    
    String ownMac = espNow.isInitialized() ? espNow.getOwnMacString() : "Not initialized";
    if (labelOwnMacValue) {
        labelOwnMacValue->setText(ownMac.c_str());
    }
    
    Serial.println("  ✅ ConnectionPage build complete");
}

void ConnectionPage::update() {
    updateConnectionStatus();
}

void ConnectionPage::setPeerMac(const char* macStr) {
    if (!macStr) return;
    
    // Nur Daten speichern - Label wird in build() initialisiert
    strncpy(peerMacStr, macStr, sizeof(peerMacStr) - 1);
    peerMacStr[sizeof(peerMacStr) - 1] = '\0';
    EspNowManager::stringToMac(macStr, peerMac);
    
    Serial.printf("ConnectionPage: Peer MAC gespeichert: %s\n", peerMacStr);
}

void ConnectionPage::updateConnectionStatus() {
    if (!labelStatusValue) return;
    
    bool wasConnected = isConnected;
    
    isConnected = espNow.isInitialized() && espNow.isPeerConnected(peerMac);
    isPaired = espNow.isInitialized() && espNow.hasPeer(peerMac);
    
    if (wasConnected != isConnected) {
        if (isConnected) {
            labelStatusValue->setText("Connected");
            labelStatusValue->setTextColor(COLOR_GREEN);
            if (btnPair) btnPair->setEnabled(false);
            if (btnDisconnect) btnDisconnect->setEnabled(true);
        } else if (isPaired) {
            labelStatusValue->setText("Paired (Waiting...)");
            labelStatusValue->setTextColor(COLOR_YELLOW);
            if (btnPair) btnPair->setEnabled(false);
            if (btnDisconnect) btnDisconnect->setEnabled(true);
        } else {
            labelStatusValue->setText("Disconnected");
            labelStatusValue->setTextColor(COLOR_RED);
            if (btnPair) btnPair->setEnabled(true);
            if (btnDisconnect) btnDisconnect->setEnabled(false);
        }
    }
}

void ConnectionPage::onPairClicked() {
    Serial.println("ConnectionPage: Pair clicked");
    
    if (espNow.isInitialized() && espNow.addPeer(peerMac)) {
        Serial.println("  Peer added");
        isPaired = true;
        updateConnectionStatus();
        
        sdCard.logConnection(peerMacStr, "paired");
    } else {
        Serial.println("  Add peer failed");
    }
}

void ConnectionPage::onDisconnectClicked() {
    Serial.println("ConnectionPage: Disconnect clicked");
    
    if (espNow.isInitialized() && espNow.removePeer(peerMac)) {
        Serial.println("  Peer removed");
        isPaired = false;
        isConnected = false;
        updateConnectionStatus();
    } else {
        Serial.println("  Remove peer failed");
    }
}