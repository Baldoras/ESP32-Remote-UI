/**
 * ConnectionPage.cpp
 * 
 * Implementation der Verbindungs-Seite
 */

#include "ConnectionPage.h"

ConnectionPage::ConnectionPage(UIManager* ui, TFT_eSPI* tft)
    : UIPage("Connection", ui, tft)
    , espnow(EspNowManager::getInstance())
    , isPaired(false)
    , isConnected(false)
    , currentRSSI(-100)
    , lastPacketTime(0)
    , heartbeatState(false)
    , lastHeartbeat(0)
{
    strncpy(peerMacStr, "00:00:00:00:00:00", sizeof(peerMacStr));
    memset(peerMac, 0, 6);
    
    // Back-Button wird in initUI() erstellt
}

ConnectionPage::~ConnectionPage() {
    // UI-Elemente werden von UIPage aufgeräumt
}

void ConnectionPage::build() {
    DEBUG_PRINTLN("ConnectionPage: Building UI...");
    
    initUI();
    
    // Eigene MAC-Adresse anzeigen
    String ownMac = espnow.getOwnMacString();
    labelOwnMacValue->setText(ownMac.c_str());
    
    updateUI();
    
    DEBUG_PRINTLN("ConnectionPage: ✅ UI erstellt");
}

void ConnectionPage::update() {
    // Verbindungsstatus prüfen
    updateConnectionStatus();
    
    // Heartbeat-Animation
    updateHeartbeatLED();
    
    // Letztes Paket Zeit aktualisieren
    updateLastPacketTime();
    
    // RSSI-Balken aktualisieren (wenn verbunden)
    if (isConnected) {
        updateRSSIBar();
    }
}

void ConnectionPage::setPeerMac(const char* macStr) {
    strncpy(peerMacStr, macStr, sizeof(peerMacStr) - 1);
    peerMacStr[sizeof(peerMacStr) - 1] = '\0';
    
    // String zu Binär konvertieren
    EspNowManager::stringToMac(macStr, peerMac);
    
    // UI aktualisieren
    if (labelPeerMacValue) {
        labelPeerMacValue->setText(peerMacStr);
    }
    
    DEBUG_PRINTF("ConnectionPage: Peer MAC gesetzt: %s\n", peerMacStr);
}

void ConnectionPage::updateConnectionStatus() {
    bool wasConnected = isConnected;
    
    // Status vom ESP-NOW Manager abrufen
    isConnected = espnow.isPeerConnected(peerMac);
    isPaired = espnow.hasPeer(peerMac);
    
    // UI aktualisieren wenn Status geändert
    if (wasConnected != isConnected) {
        updateUI();
    }
}

void ConnectionPage::updateRSSI(int8_t rssi) {
    currentRSSI = rssi;
    lastPacketTime = millis();
    updateRSSIBar();
}

void ConnectionPage::updateHeartbeat(bool received) {
    if (received) {
        lastHeartbeat = millis();
        heartbeatState = true;
        updateHeartbeatLED();
    }
}

// ═══════════════════════════════════════════════════════════════════════════
// PRIVATE METHODEN
// ═══════════════════════════════════════════════════════════════════════════

void ConnectionPage::initUI() {
    int16_t contentX = layout.contentX;
    int16_t contentY = layout.contentY;
    int16_t contentW = layout.contentWidth;
    
    int16_t yPos = contentY + 10;
    int16_t labelWidth = 120;
    int16_t valueX = contentX + labelWidth + 10;
    int16_t valueWidth = contentW - labelWidth - 20;
    
    // ═══════════════════════════════════════════════════════════════════════
    // Status
    // ═══════════════════════════════════════════════════════════════════════
    
    labelStatus = new UILabel(contentX + 10, yPos, labelWidth, 30, "Status:");
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
    
    // ═══════════════════════════════════════════════════════════════════════
    // Eigene MAC
    // ═══════════════════════════════════════════════════════════════════════
    
    labelOwnMac = new UILabel(contentX + 10, yPos, labelWidth, 25, "Own MAC:");
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
    
    // ═══════════════════════════════════════════════════════════════════════
    // Peer MAC
    // ═══════════════════════════════════════════════════════════════════════
    
    labelPeerMac = new UILabel(contentX + 10, yPos, labelWidth, 25, "Peer MAC:");
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
    
    yPos += 40;
    
    // ═══════════════════════════════════════════════════════════════════════
    // RSSI (Signalstärke)
    // ═══════════════════════════════════════════════════════════════════════
    
    labelRSSI = new UILabel(contentX + 10, yPos, labelWidth, 25, "Signal:");
    labelRSSI->setAlignment(TextAlignment::LEFT);
    labelRSSI->setFontSize(2);
    labelRSSI->setTransparent(true);
    addContentElement(labelRSSI);
    
    barRSSI = new UIProgressBar(valueX, yPos, 200, 25);
    barRSSI->setValue(0);
    barRSSI->setBarColor(COLOR_RED);
    barRSSI->setShowText(false);
    addContentElement(barRSSI);
    
    labelRSSIValue = new UILabel(valueX + 210, yPos, 100, 25, "-100 dBm");
    labelRSSIValue->setAlignment(TextAlignment::LEFT);
    labelRSSIValue->setFontSize(1);
    labelRSSIValue->setTransparent(true);
    addContentElement(labelRSSIValue);
    
    yPos += 35;
    
    // ═══════════════════════════════════════════════════════════════════════
    // Heartbeat-Indikator
    // ═══════════════════════════════════════════════════════════════════════
    
    labelHeartbeat = new UILabel(contentX + 10, yPos, labelWidth, 25, "Heartbeat:");
    labelHeartbeat->setAlignment(TextAlignment::LEFT);
    labelHeartbeat->setFontSize(1);
    labelHeartbeat->setTransparent(true);
    addContentElement(labelHeartbeat);
    
    labelHeartbeatLED = new UILabel(valueX, yPos, 30, 25, "●");
    labelHeartbeatLED->setAlignment(TextAlignment::CENTER);
    labelHeartbeatLED->setFontSize(2);
    labelHeartbeatLED->setTextColor(COLOR_DARKGRAY);
    labelHeartbeatLED->setTransparent(true);
    addContentElement(labelHeartbeatLED);
    
    yPos += 35;
    
    // ═══════════════════════════════════════════════════════════════════════
    // Letztes Paket
    // ═══════════════════════════════════════════════════════════════════════
    
    labelLastPacket = new UILabel(contentX + 10, yPos, labelWidth, 25, "Last Packet:");
    labelLastPacket->setAlignment(TextAlignment::LEFT);
    labelLastPacket->setFontSize(1);
    labelLastPacket->setTransparent(true);
    addContentElement(labelLastPacket);
    
    labelLastPacketValue = new UILabel(valueX, yPos, valueWidth, 25, "Never");
    labelLastPacketValue->setAlignment(TextAlignment::LEFT);
    labelLastPacketValue->setFontSize(1);
    labelLastPacketValue->setTransparent(true);
    addContentElement(labelLastPacketValue);
    
    yPos += 40;
    
    // ═══════════════════════════════════════════════════════════════════════
    // Buttons
    // ═══════════════════════════════════════════════════════════════════════
    
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
    
    btnX += btnWidth + btnSpacing;
    
    btnRefresh = new UIButton(btnX, yPos, btnWidth, btnHeight, "REFRESH");
    btnRefresh->on(EventType::CLICK, [this](EventData* data) {
        this->onRefreshClicked();
    });
    addContentElement(btnRefresh);
}

void ConnectionPage::updateUI() {
    // Status-Text + Farbe
    if (isConnected) {
        labelStatusValue->setText("Connected");
        labelStatusValue->setTextColor(COLOR_GREEN);
        btnPair->setEnabled(false);
        btnDisconnect->setEnabled(true);
    } else if (isPaired) {
        labelStatusValue->setText("Paired (Waiting...)");
        labelStatusValue->setTextColor(COLOR_YELLOW);
        btnPair->setEnabled(false);
        btnDisconnect->setEnabled(true);
    } else {
        labelStatusValue->setText("Disconnected");
        labelStatusValue->setTextColor(COLOR_RED);
        btnPair->setEnabled(true);
        btnDisconnect->setEnabled(false);
    }
}

void ConnectionPage::updateRSSIBar() {
    if (!barRSSI || !labelRSSIValue) return;
    
    // RSSI in Prozent umrechnen
    uint8_t percent = rssiToPercent(currentRSSI);
    barRSSI->setValue(percent);
    
    // Farbe basierend auf Signal
    uint16_t color = rssiToColor(currentRSSI);
    barRSSI->setBarColor(color);
    
    // Text aktualisieren
    char buffer[16];
    sprintf(buffer, "%d dBm", currentRSSI);
    labelRSSIValue->setText(buffer);
}

void ConnectionPage::updateHeartbeatLED() {
    if (!labelHeartbeatLED) return;
    
    unsigned long now = millis();
    
    // LED ausschalten nach 200ms
    if (heartbeatState && (now - lastHeartbeat > 200)) {
        heartbeatState = false;
        labelHeartbeatLED->setTextColor(COLOR_DARKGRAY);
    }
    
    // LED einschalten bei Heartbeat
    if (heartbeatState) {
        labelHeartbeatLED->setTextColor(COLOR_GREEN);
    }
}

void ConnectionPage::updateLastPacketTime() {
    if (!labelLastPacketValue) return;
    
    if (lastPacketTime == 0) {
        labelLastPacketValue->setText("Never");
        return;
    }
    
    unsigned long now = millis();
    unsigned long elapsed = now - lastPacketTime;
    
    char buffer[32];
    if (elapsed < 1000) {
        sprintf(buffer, "%lu ms ago", elapsed);
    } else if (elapsed < 60000) {
        sprintf(buffer, "%lu s ago", elapsed / 1000);
    } else {
        sprintf(buffer, "%lu min ago", elapsed / 60000);
    }
    
    labelLastPacketValue->setText(buffer);
}

// ═══════════════════════════════════════════════════════════════════════════
// BUTTON CALLBACKS
// ═══════════════════════════════════════════════════════════════════════════

void ConnectionPage::onPairClicked() {
    DEBUG_PRINTLN("ConnectionPage: Pair clicked");
    
    // Peer hinzufügen
    if (espnow.addPeer(peerMac)) {
        DEBUG_PRINTLN("ConnectionPage: ✅ Peer hinzugefügt");
        isPaired = true;
        updateUI();
    } else {
        DEBUG_PRINTLN("ConnectionPage: ❌ Peer hinzufügen fehlgeschlagen");
    }
}

void ConnectionPage::onDisconnectClicked() {
    DEBUG_PRINTLN("ConnectionPage: Disconnect clicked");
    
    // Peer entfernen
    if (espnow.removePeer(peerMac)) {
        DEBUG_PRINTLN("ConnectionPage: ✅ Peer entfernt");
        isPaired = false;
        isConnected = false;
        updateUI();
    } else {
        DEBUG_PRINTLN("ConnectionPage: ❌ Peer entfernen fehlgeschlagen");
    }
}

void ConnectionPage::onRefreshClicked() {
    DEBUG_PRINTLN("ConnectionPage: Refresh clicked");
    
    // Status neu prüfen
    updateConnectionStatus();
    updateUI();
}

// ═══════════════════════════════════════════════════════════════════════════
// HELPER FUNKTIONEN
// ═══════════════════════════════════════════════════════════════════════════

uint8_t ConnectionPage::rssiToPercent(int8_t rssi) {
    // -100 dBm = 0%, -50 dBm = 100%
    if (rssi >= -50) return 100;
    if (rssi <= -100) return 0;
    
    return (rssi + 100) * 2;  // Linear mapping
}

uint16_t ConnectionPage::rssiToColor(int8_t rssi) {
    if (rssi >= -60) return COLOR_GREEN;   // Sehr gut
    if (rssi >= -70) return COLOR_YELLOW;  // Gut
    if (rssi >= -80) return COLOR_ORANGE;  // Mittel
    return COLOR_RED;                      // Schlecht
}
