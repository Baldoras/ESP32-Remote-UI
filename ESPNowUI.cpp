/**
 * ESPNowUI.cpp
 * 
 * Implementation der ESP-NOW UI-Integration
 */

#include "ESPNowUI.h"

ESPNowUI::ESPNowUI()
    : espnow(EspNowManager::getInstance())
    , connPage(nullptr)
    , remotePage(nullptr)
    , hasPeer(false)
    , sendRate(0)
    , receiveRate(0)
    , lastSendTime(0)
    , lastReceiveTime(0)
    , sendCounter(0)
    , receiveCounter(0)
    , lastStatsUpdate(0)
{
    memset(peerMac, 0, 6);
    
    // Telemetrie initialisieren
    telemetry.motorLeft = 0;
    telemetry.motorRight = 0;
    telemetry.hasMotor = false;
    
    telemetry.batteryVoltage = 0;
    telemetry.batteryPercent = 0;
    telemetry.hasBattery = false;
    
    telemetry.rssi = -100;
    telemetry.hasRSSI = false;
    
    telemetry.lastUpdate = 0;
}

ESPNowUI::~ESPNowUI() {
}

void ESPNowUI::begin(ConnectionPage* connectionPage, RemoteControlPage* remotePage) {
    connPage = connectionPage;
    this->remotePage = remotePage;
    
    DEBUG_PRINTLN("ESPNowUI: ✅ Initialisiert");
}

void ESPNowUI::update() {
    // ESP-NOW empfangene Daten verarbeiten
    processReceivedData();
    
    // Statistiken aktualisieren
    updateStatistics();
    
    // UI-Seiten aktualisieren
    if (connPage) {
        updateConnectionPage();
    }
    
    if (remotePage) {
        updateRemotePage();
    }
}

bool ESPNowUI::sendJoystick(int16_t x, int16_t y, uint8_t btn) {
    if (!hasPeer) {
        DEBUG_PRINTLN("ESPNowUI: ❌ Kein Peer gesetzt!");
        return false;
    }
    
    // Paket erstellen
    EspNowPacket packet;
    packet.begin(MainCmd::DATA_REQUEST)
          .addInt16(DataCmd::JOYSTICK_X, x)
          .addInt16(DataCmd::JOYSTICK_Y, y)
          .addByte(DataCmd::JOYSTICK_BTN, btn);
    
    // Senden
    bool success = espnow.send(peerMac, packet);
    
    if (success) {
        sendCounter++;
        lastSendTime = millis();
        
        // UI aktualisieren (lokale Anzeige)
        if (remotePage) {
            remotePage->setJoystickPosition(x, y);
        }
    }
    
    return success;
}

bool ESPNowUI::sendMotor(int16_t left, int16_t right) {
    if (!hasPeer) {
        DEBUG_PRINTLN("ESPNowUI: ❌ Kein Peer gesetzt!");
        return false;
    }
    
    // Paket erstellen
    EspNowPacket packet;
    packet.begin(MainCmd::DATA_RESPONSE)
          .addInt16(DataCmd::MOTOR_LEFT, left)
          .addInt16(DataCmd::MOTOR_RIGHT, right);
    
    // Senden
    bool success = espnow.send(peerMac, packet);
    
    if (success) {
        sendCounter++;
        lastSendTime = millis();
    }
    
    return success;
}

bool ESPNowUI::sendBattery(uint16_t voltage, uint8_t percent) {
    if (!hasPeer) {
        return false;
    }
    
    // Paket erstellen
    EspNowPacket packet;
    packet.begin(MainCmd::DATA_RESPONSE)
          .addUInt16(DataCmd::BATTERY_VOLTAGE, voltage)
          .addByte(DataCmd::BATTERY_PERCENT, percent);
    
    // Senden
    bool success = espnow.send(peerMac, packet);
    
    if (success) {
        sendCounter++;
        lastSendTime = millis();
    }
    
    return success;
}

void ESPNowUI::setPeerMac(const uint8_t* mac) {
    if (!mac) return;
    
    memcpy(peerMac, mac, 6);
    hasPeer = true;
    
    // ConnectionPage aktualisieren
    if (connPage) {
        String macStr = EspNowManager::macToString(mac);
        connPage->setPeerMac(macStr.c_str());
    }
    
    DEBUG_PRINTF("ESPNowUI: Peer MAC gesetzt: %s\n", 
                 EspNowManager::macToString(mac).c_str());
}

void ESPNowUI::setPeerMac(const char* macStr) {
    if (!macStr) return;
    
    uint8_t mac[6];
    if (EspNowManager::stringToMac(macStr, mac)) {
        setPeerMac(mac);
    } else {
        DEBUG_PRINTLN("ESPNowUI: ❌ Ungültige MAC-Adresse!");
    }
}

// ═══════════════════════════════════════════════════════════════════════════
// PRIVATE METHODEN
// ═══════════════════════════════════════════════════════════════════════════

void ESPNowUI::processReceivedData() {
    // Alle verfügbaren Pakete verarbeiten
    ResultQueueItem result;
    
    while (espnow.getData(&result)) {
        receiveCounter++;
        lastReceiveTime = millis();
        telemetry.lastUpdate = millis();
        
        // Motor-Daten
        if (result.data.hasMotor) {
            telemetry.motorLeft = result.data.motorLeft;
            telemetry.motorRight = result.data.motorRight;
            telemetry.hasMotor = true;
        }
        
        // Batterie-Daten
        if (result.data.hasBattery) {
            telemetry.batteryVoltage = result.data.batteryVoltage;
            telemetry.batteryPercent = result.data.batteryPercent;
            telemetry.hasBattery = true;
        }
        
        // RSSI schätzen (vereinfacht, da nicht direkt verfügbar)
        telemetry.rssi = estimateRSSI();
        telemetry.hasRSSI = true;
        
        // Heartbeat erhalten
        if (result.mainCmd == MainCmd::HEARTBEAT) {
            if (connPage) {
                connPage->updateHeartbeat(true);
            }
        }
        
        DEBUG_PRINTF("ESPNowUI: Paket empfangen - Motor: %d/%d, Battery: %dmV/%d%%\n",
                     telemetry.motorLeft, telemetry.motorRight,
                     telemetry.batteryVoltage, telemetry.batteryPercent);
    }
}

void ESPNowUI::updateStatistics() {
    unsigned long now = millis();
    
    // Alle 1000ms Raten berechnen
    if (now - lastStatsUpdate >= 1000) {
        sendRate = sendCounter;
        receiveRate = receiveCounter;
        
        sendCounter = 0;
        receiveCounter = 0;
        lastStatsUpdate = now;
    }
}

void ESPNowUI::updateConnectionPage() {
    if (!connPage) return;
    
    // Verbindungsstatus
    connPage->updateConnectionStatus();
    
    // RSSI
    if (telemetry.hasRSSI) {
        connPage->updateRSSI(telemetry.rssi);
    }
}

void ESPNowUI::updateRemotePage() {
    if (!remotePage) return;
    
    // Motor-Output
    if (telemetry.hasMotor) {
        remotePage->setMotorOutput(telemetry.motorLeft, telemetry.motorRight);
    }
    
    // Vehicle-Batterie
    if (telemetry.hasBattery) {
        float voltage = telemetry.batteryVoltage / 1000.0f;  // mV → V
        remotePage->setVehicleBattery(voltage, telemetry.batteryPercent);
    }
    
    // Verbindungsstatus
    bool connected = espnow.isPeerConnected(peerMac);
    remotePage->setConnectionStatus(connected);
    
    // RSSI
    if (telemetry.hasRSSI) {
        remotePage->setRSSI(telemetry.rssi);
    }
    
    // Sende-Rate
    remotePage->setSendRate(sendRate);
}

int8_t ESPNowUI::estimateRSSI() {
    // VEREINFACHT: ESP-NOW hat keinen direkten RSSI-Zugriff
    // In echter Implementierung würde man:
    // 1. RSSI als Telemetrie-Daten mitschicken
    // 2. WiFi.RSSI() verwenden (aber nur im AP/STA Mode)
    // 3. esp_wifi_get_ap_list() für gescannte APs
    
    // Für Demo: Basierend auf Verbindungsstatus schätzen
    if (!hasPeer) return -100;
    
    bool connected = espnow.isPeerConnected(peerMac);
    
    if (connected) {
        // Verbunden → Annehmen dass Signal OK ist
        unsigned long timeSinceLastPacket = millis() - telemetry.lastUpdate;
        
        if (timeSinceLastPacket < 500) {
            return -50;  // Sehr gut
        } else if (timeSinceLastPacket < 1000) {
            return -65;  // Gut
        } else if (timeSinceLastPacket < 2000) {
            return -75;  // Mittel
        } else {
            return -85;  // Schwach
        }
    } else {
        return -100;  // Keine Verbindung
    }
}