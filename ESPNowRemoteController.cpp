/**
 * ESPNowRemoteController.cpp
 * 
 * Implementation der projekt-spezifischen ESP-NOW Controller-Klasse
 * Erweitert ESPNowManager um Remote-Control-spezifische Funktionalität
 * 
 * ROLLE: MASTER (Remote Control)
 * - Sendet: Joystick-Daten, Commands, PAIR_REQUEST, HEARTBEAT
 * - Empfängt: Telemetrie, Status, PAIR_RESPONSE, ACK vom Slave (Vehicle)
 * - ACK-Empfang aktualisiert lastSeen und verlängert Verbindungs-Timeout
 */

#include "include/ESPNowRemoteController.h"

// ═══════════════════════════════════════════════════════════════════════════
// REMOTE ESPNOW PACKET - ERWEITERTE BUILDER
// ═══════════════════════════════════════════════════════════════════════════

RemoteESPNowPacket& RemoteESPNowPacket::addJoystickX(int16_t x) {
    return static_cast<RemoteESPNowPacket&>(
        addInt16(DataCmd::JOYSTICK_X, x)
    );
}

RemoteESPNowPacket& RemoteESPNowPacket::addJoystickY(int16_t y) {
    return static_cast<RemoteESPNowPacket&>(
        addInt16(DataCmd::JOYSTICK_Y, y)
    );
}

RemoteESPNowPacket& RemoteESPNowPacket::addJoystickButton(bool pressed) {
    return static_cast<RemoteESPNowPacket&>(
        addByte(DataCmd::JOYSTICK_BTN, pressed ? 1 : 0)
    );
}

RemoteESPNowPacket& RemoteESPNowPacket::addJoystick(const JoystickData& data) {
    return static_cast<RemoteESPNowPacket&>(
        addStruct(DataCmd::JOYSTICK_ALL, data)
    );
}

RemoteESPNowPacket& RemoteESPNowPacket::addJoystick(int16_t x, int16_t y, bool button) {
    JoystickData data = {x, y, button ? (uint8_t)1 : (uint8_t)0};
    return addJoystick(data);
}

RemoteESPNowPacket& RemoteESPNowPacket::addMotorLeft(int16_t value) {
    return static_cast<RemoteESPNowPacket&>(
        addInt16(DataCmd::MOTOR_LEFT, value)
    );
}

RemoteESPNowPacket& RemoteESPNowPacket::addMotorRight(int16_t value) {
    return static_cast<RemoteESPNowPacket&>(
        addInt16(DataCmd::MOTOR_RIGHT, value)
    );
}

RemoteESPNowPacket& RemoteESPNowPacket::addMotors(const MotorData& data) {
    return static_cast<RemoteESPNowPacket&>(
        addStruct(DataCmd::MOTOR_ALL, data)
    );
}

RemoteESPNowPacket& RemoteESPNowPacket::addMotors(int16_t left, int16_t right) {
    MotorData data = {left, right};
    return addMotors(data);
}

RemoteESPNowPacket& RemoteESPNowPacket::addBatteryVoltage(uint16_t voltage) {
    return static_cast<RemoteESPNowPacket&>(
        addUInt16(DataCmd::BATTERY_VOLTAGE, voltage)
    );
}

RemoteESPNowPacket& RemoteESPNowPacket::addBatteryPercent(uint8_t percent) {
    return static_cast<RemoteESPNowPacket&>(
        addByte(DataCmd::BATTERY_PERCENT, percent)
    );
}

RemoteESPNowPacket& RemoteESPNowPacket::addTemperature(int16_t temp) {
    return static_cast<RemoteESPNowPacket&>(
        addInt16(DataCmd::TEMPERATURE, temp)
    );
}

RemoteESPNowPacket& RemoteESPNowPacket::addRSSI(int8_t rssi) {
    return static_cast<RemoteESPNowPacket&>(
        addInt8(DataCmd::RSSI, rssi)
    );
}

RemoteESPNowPacket& RemoteESPNowPacket::addTelemetry(const TelemetryData& data) {
    addBatteryVoltage(data.batteryVoltage);
    addBatteryPercent(data.batteryPercent);
    addTemperature(data.temperature);
    addRSSI(data.rssi);
    return *this;
}

RemoteESPNowPacket& RemoteESPNowPacket::addAcceleration(const AccelerationData& data) {
    return static_cast<RemoteESPNowPacket&>(
        addStruct(DataCmd::ACCELERATION, data)
    );
}

RemoteESPNowPacket& RemoteESPNowPacket::addGyroscope(const GyroscopeData& data) {
    return static_cast<RemoteESPNowPacket&>(
        addStruct(DataCmd::GYROSCOPE, data)
    );
}

// ═══════════════════════════════════════════════════════════════════════════
// REMOTE ESPNOW PACKET - ERWEITERTE PARSER
// ═══════════════════════════════════════════════════════════════════════════

bool RemoteESPNowPacket::getJoystickX(int16_t& outValue) const {
    return getInt16(DataCmd::JOYSTICK_X, outValue);
}

bool RemoteESPNowPacket::getJoystickY(int16_t& outValue) const {
    return getInt16(DataCmd::JOYSTICK_Y, outValue);
}

bool RemoteESPNowPacket::getJoystickButton(bool& outValue) const {
    uint8_t val;
    if (getByte(DataCmd::JOYSTICK_BTN, val)) {
        outValue = (val != 0);
        return true;
    }
    return false;
}

bool RemoteESPNowPacket::getJoystick(JoystickData& outData) const {
    const JoystickData* data = get<JoystickData>(DataCmd::JOYSTICK_ALL);
    if (data) {
        outData = *data;
        return true;
    }
    
    // Fallback: Einzelne Werte
    int16_t x, y;
    bool btn = false;
    if (getJoystickX(x) && getJoystickY(y)) {
        getJoystickButton(btn);
        outData.x = x;
        outData.y = y;
        outData.button = btn ? 1 : 0;
        return true;
    }
    
    return false;
}

bool RemoteESPNowPacket::getMotorLeft(int16_t& outValue) const {
    return getInt16(DataCmd::MOTOR_LEFT, outValue);
}

bool RemoteESPNowPacket::getMotorRight(int16_t& outValue) const {
    return getInt16(DataCmd::MOTOR_RIGHT, outValue);
}

bool RemoteESPNowPacket::getMotors(MotorData& outData) const {
    const MotorData* data = get<MotorData>(DataCmd::MOTOR_ALL);
    if (data) {
        outData = *data;
        return true;
    }
    
    // Fallback: Einzelne Werte
    int16_t left, right;
    if (getMotorLeft(left) && getMotorRight(right)) {
        outData.left = left;
        outData.right = right;
        return true;
    }
    
    return false;
}

bool RemoteESPNowPacket::getBatteryVoltage(uint16_t& outValue) const {
    return getUInt16(DataCmd::BATTERY_VOLTAGE, outValue);
}

bool RemoteESPNowPacket::getBatteryPercent(uint8_t& outValue) const {
    return getByte(DataCmd::BATTERY_PERCENT, outValue);
}

bool RemoteESPNowPacket::getTemperature(int16_t& outValue) const {
    return getInt16(DataCmd::TEMPERATURE, outValue);
}

bool RemoteESPNowPacket::getRSSI(int8_t& outValue) const {
    return getInt8(DataCmd::RSSI, outValue);
}

bool RemoteESPNowPacket::getTelemetry(TelemetryData& outData) const {
    uint16_t voltage;
    uint8_t percent;
    int16_t temp;
    int8_t rssi;
    
    bool hasVoltage = getBatteryVoltage(voltage);
    bool hasPercent = getBatteryPercent(percent);
    bool hasTemp = getTemperature(temp);
    bool hasRssi = getRSSI(rssi);
    
    if (hasVoltage) outData.batteryVoltage = voltage;
    if (hasPercent) outData.batteryPercent = percent;
    if (hasTemp) outData.temperature = temp;
    if (hasRssi) outData.rssi = rssi;
    
    return hasVoltage || hasPercent || hasTemp || hasRssi;
}

bool RemoteESPNowPacket::getAcceleration(AccelerationData& outData) const {
    const AccelerationData* data = get<AccelerationData>(DataCmd::ACCELERATION);
    if (data) {
        outData = *data;
        return true;
    }
    return false;
}

bool RemoteESPNowPacket::getGyroscope(GyroscopeData& outData) const {
    const GyroscopeData* data = get<GyroscopeData>(DataCmd::GYROSCOPE);
    if (data) {
        outData = *data;
        return true;
    }
    return false;
}

// ═══════════════════════════════════════════════════════════════════════════
// REMOTE ESP NOW CONTROLLER - HAUPTKLASSE
// ═══════════════════════════════════════════════════════════════════════════

ESPNowRemoteController::ESPNowRemoteController()
    : ESPNowManager()
    , joystickCallback(nullptr)
    , motorCallback(nullptr)
    , telemetryCallback(nullptr)
{
}

ESPNowRemoteController::~ESPNowRemoteController() {
}

// ═══════════════════════════════════════════════════════════════════════════
// HIGH-LEVEL SENDE-METHODEN
// ═══════════════════════════════════════════════════════════════════════════

bool ESPNowRemoteController::sendJoystick(const uint8_t* mac, int16_t x, int16_t y, bool button) {
    RemoteESPNowPacket packet;
    packet.begin(MainCmd::USER_START);
    packet.addJoystick(x, y, button);
    return send(mac, packet);
}

bool ESPNowRemoteController::sendJoystick(const uint8_t* mac, const JoystickData& data) {
    RemoteESPNowPacket packet;
    packet.begin(MainCmd::USER_START);
    packet.addJoystick(data);
    return send(mac, packet);
}

bool ESPNowRemoteController::sendMotorCommand(const uint8_t* mac, int16_t left, int16_t right) {
    RemoteESPNowPacket packet;
    packet.begin(MainCmd::USER_START);
    packet.addMotors(left, right);
    return send(mac, packet);
}

bool ESPNowRemoteController::sendMotorCommand(const uint8_t* mac, const MotorData& data) {
    RemoteESPNowPacket packet;
    packet.begin(MainCmd::USER_START);
    packet.addMotors(data);
    return send(mac, packet);
}

bool ESPNowRemoteController::sendTelemetry(const uint8_t* mac, const TelemetryData& data) {
    RemoteESPNowPacket packet;
    packet.begin(MainCmd::DATA_RESPONSE);
    packet.addTelemetry(data);
    return send(mac, packet);
}

bool ESPNowRemoteController::sendBatteryStatus(const uint8_t* mac, uint16_t voltage, uint8_t percent) {
    RemoteESPNowPacket packet;
    packet.begin(MainCmd::DATA_RESPONSE);
    packet.addBatteryVoltage(voltage);
    packet.addBatteryPercent(percent);
    return send(mac, packet);
}

bool ESPNowRemoteController::sendStatus(const uint8_t* mac, uint8_t status) {
    RemoteESPNowPacket packet;
    packet.begin(MainCmd::DATA_RESPONSE);
    packet.addByte(DataCmd::STATUS, status);
    return send(mac, packet);
}

bool ESPNowRemoteController::sendError(const uint8_t* mac, uint8_t errorCode) {
    RemoteESPNowPacket packet;
    packet.begin(MainCmd::ERROR);
    packet.addByte(DataCmd::ERROR_CODE, errorCode);
    return send(mac, packet);
}

// ═══════════════════════════════════════════════════════════════════════════
// PAIRING PROTOCOL
// ═══════════════════════════════════════════════════════════════════════════

bool ESPNowRemoteController::startPairing(const uint8_t* mac) {
    if (!initialized || !mac) {
        DEBUG_PRINTLN("ESPNowRemoteController: Pairing fehlgeschlagen - nicht initialisiert oder ungültige MAC");
        return false;
    }
    
    // Peer hinzufügen falls noch nicht vorhanden
    if (!hasPeer(mac)) {
        if (!addPeer(mac)) {
            DEBUG_PRINTLN("ESPNowRemoteController: Pairing fehlgeschlagen - Peer hinzufügen fehlgeschlagen");
            return false;
        }
    }
    
    // PAIR_REQUEST senden
    ESPNowPacket packet;
    packet.begin(MainCmd::PAIR_REQUEST);
    
    DEBUG_PRINTF("ESPNowRemoteController: Sende PAIR_REQUEST an %s\n", macToString(mac).c_str());
    
    return send(mac, packet);
}

// ═══════════════════════════════════════════════════════════════════════════
// CALLBACKS
// ═══════════════════════════════════════════════════════════════════════════

void ESPNowRemoteController::setJoystickCallback(JoystickCallback callback) {
    joystickCallback = callback;
}

void ESPNowRemoteController::setMotorCallback(MotorCallback callback) {
    motorCallback = callback;
}

void ESPNowRemoteController::setTelemetryCallback(TelemetryCallback callback) {
    telemetryCallback = callback;
}

// ═══════════════════════════════════════════════════════════════════════════
// RX-QUEUE VERARBEITUNG (spezialisiert)
// ═══════════════════════════════════════════════════════════════════════════

void ESPNowRemoteController::processRxQueue() {
    if (!rxQueue) {
        DEBUG_PRINTLN("ESPNowRemoteController: rxQueue ist NULL!");
        return;
    }
    
    RxQueueItem rxItem;
    int processed = 0;
    
    // Alle verfügbaren RX-Items verarbeiten
    while (xQueueReceive(rxQueue, &rxItem, 0) == pdTRUE) {
        processed++;
        
        DEBUG_PRINTF("\n[RX-Remote #%d] von %s\n", processed, macToString(rxItem.mac).c_str());
        
        // Paket als RemoteESPNowPacket parsen
        RemoteESPNowPacket packet;
        if (!packet.parse(rxItem.data, rxItem.length)) {
            DEBUG_PRINTLN("  Parse FAILED!");
            continue;
        }
        
        DEBUG_PRINTLN("  Parse SUCCESS");
        
        // Peer aktualisieren (mit Mutex)
        bool wasDisconnected = false;
        if (xSemaphoreTake(peersMutex, pdMS_TO_TICKS(10)) == pdTRUE) {
            int index = findPeerIndex(rxItem.mac);
            if (index >= 0) {
                wasDisconnected = !peers[index].connected;
                peers[index].connected = true;
                peers[index].lastSeen = rxItem.timestamp;
                peers[index].packetsReceived++;
            }
            xSemaphoreGive(peersMutex);
        }
        
        // Connected-Event triggern
        if (wasDisconnected) {
            DEBUG_PRINTF("  ✅ Peer verbunden\n");
            
            ESPNowEventData eventData = {};
            eventData.event = ESPNowEvent::PEER_CONNECTED;
            memcpy(eventData.mac, rxItem.mac, 6);
            triggerEvent(ESPNowEvent::PEER_CONNECTED, &eventData);
        }
        
        // Nach MainCmd verarbeiten
        MainCmd cmd = packet.getMainCmd();
        DEBUG_PRINTF("  MainCmd: 0x%02X\n", static_cast<uint8_t>(cmd));
        
        if (cmd == MainCmd::PAIR_RESPONSE) {
            DEBUG_PRINTLN("  → PAIR_RESPONSE empfangen - Pairing erfolgreich!");
            
            // Peer-Verbindung auf connected setzen (mit Mutex)
            if (xSemaphoreTake(peersMutex, pdMS_TO_TICKS(10)) == pdTRUE) {
                int index = findPeerIndex(rxItem.mac);
                if (index >= 0) {
                    peers[index].connected = true;
                    peers[index].lastSeen = rxItem.timestamp;
                    DEBUG_PRINTLN("    ✅ Peer als connected markiert");
                }
                xSemaphoreGive(peersMutex);
            }
            
            // PEER_CONNECTED Event triggern
            ESPNowEventData eventData = {};
            eventData.event = ESPNowEvent::PEER_CONNECTED;
            memcpy(eventData.mac, rxItem.mac, 6);
            triggerEvent(ESPNowEvent::PEER_CONNECTED, &eventData);
            continue;
        }
        
        if (cmd == MainCmd::ACK) {
            DEBUG_PRINTLN("  → ACK empfangen (Heartbeat-Bestätigung)");
            
            // lastSeen aktualisieren um Timeout zu verlängern (mit Mutex)
            if (xSemaphoreTake(peersMutex, pdMS_TO_TICKS(10)) == pdTRUE) {
                int index = findPeerIndex(rxItem.mac);
                if (index >= 0) {
                    peers[index].lastSeen = rxItem.timestamp;
                    DEBUG_PRINTF("    ✅ lastSeen aktualisiert (Timeout verlängert)\n");
                }
                xSemaphoreGive(peersMutex);
            }
            
            // ACK Event triggern (optional für UI-Feedback)
            ESPNowEventData eventData = {};
            eventData.event = ESPNowEvent::HEARTBEAT_RECEIVED;  // Nutze HEARTBEAT_RECEIVED für ACK
            memcpy(eventData.mac, rxItem.mac, 6);
            triggerEvent(ESPNowEvent::HEARTBEAT_RECEIVED, &eventData);
            continue;
        }
        
        // Projekt-spezifische Verarbeitung
        handleJoystickData(rxItem.mac, packet);
        handleMotorData(rxItem.mac, packet);
        handleTelemetryData(rxItem.mac, packet);
        
        // Basis-Callback (falls gesetzt)
        if (receiveCallback) {
            receiveCallback(rxItem.mac, packet);
        }
        
        // Data-Received Event
        ESPNowEventData eventData = {};
        eventData.event = ESPNowEvent::DATA_RECEIVED;
        memcpy(eventData.mac, rxItem.mac, 6);
        eventData.packet = &packet;
        triggerEvent(ESPNowEvent::DATA_RECEIVED, &eventData);
    }
    
    if (processed > 0) {
        DEBUG_PRINTF("[ESPNowRemoteController] %d Pakete verarbeitet\n", processed);
    }
}

// ═══════════════════════════════════════════════════════════════════════════
// INTERNE VERARBEITUNG
// ═══════════════════════════════════════════════════════════════════════════

void ESPNowRemoteController::handleJoystickData(const uint8_t* mac, RemoteESPNowPacket& packet) {
    if (!joystickCallback) return;
    
    JoystickData data;
    if (packet.getJoystick(data)) {
        DEBUG_PRINTF("  Joystick: x=%d, y=%d, btn=%d\n", data.x, data.y, data.button);
        joystickCallback(mac, data);
    }
}

void ESPNowRemoteController::handleMotorData(const uint8_t* mac, RemoteESPNowPacket& packet) {
    if (!motorCallback) return;
    
    MotorData data;
    if (packet.getMotors(data)) {
        DEBUG_PRINTF("  Motors: left=%d, right=%d\n", data.left, data.right);
        motorCallback(mac, data);
    }
}

void ESPNowRemoteController::handleTelemetryData(const uint8_t* mac, RemoteESPNowPacket& packet) {
    if (!telemetryCallback) return;
    
    TelemetryData data;
    if (packet.getTelemetry(data)) {
        DEBUG_PRINTF("  Telemetry: bat=%dmV/%d%%, temp=%d, rssi=%d\n", 
                     data.batteryVoltage, data.batteryPercent, data.temperature, data.rssi);
        telemetryCallback(mac, data);
    }
}

// ═══════════════════════════════════════════════════════════════════════════
// DEBUG
// ═══════════════════════════════════════════════════════════════════════════

void ESPNowRemoteController::printInfo() {
    DEBUG_PRINTLN("\n╔═══════════════════════════════════════════════╗");
    DEBUG_PRINTLN("║     REMOTE ESP-NOW CONTROLLER INFO           ║");
    DEBUG_PRINTLN("╚═══════════════════════════════════════════════╝");
    
    DEBUG_PRINTF("Status:     %s\n", initialized ? "✅ Initialisiert" : "❌ Nicht init");
    DEBUG_PRINTF("MAC:        %s\n", getOwnMacString().c_str());
    DEBUG_PRINTF("Kanal:      %d\n", wifiChannel);
    DEBUG_PRINTF("Heartbeat:  %s (%dms)\n", heartbeatEnabled ? "AN" : "AUS", heartbeatInterval);
    DEBUG_PRINTF("Timeout:    %dms\n", timeoutMs);
    DEBUG_PRINTLN("Typ:        Remote Control System");
    DEBUG_PRINTLN("Threading:  ❌ KEIN Worker-Thread (ESP-NOW ist async!)");
    
    // Callbacks
    DEBUG_PRINTLN("\n─── Callbacks ─────────────────────────────────");
    DEBUG_PRINTF("Joystick:   %s\n", joystickCallback ? "✅ Gesetzt" : "❌ Nicht gesetzt");
    DEBUG_PRINTF("Motor:      %s\n", motorCallback ? "✅ Gesetzt" : "❌ Nicht gesetzt");
    DEBUG_PRINTF("Telemetrie: %s\n", telemetryCallback ? "✅ Gesetzt" : "❌ Nicht gesetzt");
    
    // Queue-Statistiken
    DEBUG_PRINTLN("\n─── Queue ─────────────────────────────────────");
    DEBUG_PRINTF("RX-Queue:   %d / %d\n", getQueuePending(), ESPNOW_RX_QUEUE_SIZE);
    
    DEBUG_PRINTLN("\n─── Peers ─────────────────────────────────────");
    
    if (xSemaphoreTake(peersMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
        DEBUG_PRINTF("Anzahl: %d / %d\n", peers.size(), ESPNOW_MAX_PEERS_LIMIT);
        
        for (auto& peer : peers) {
            DEBUG_PRINTF("\n  MAC: %s\n", macToString(peer.mac).c_str());
            DEBUG_PRINTF("  Status:     %s\n", peer.connected ? "✅ Verbunden" : "❌ Getrennt");
            DEBUG_PRINTF("  LastSeen:   %lums ago\n", peer.lastSeen > 0 ? (millis() - peer.lastSeen) : 0);
            DEBUG_PRINTF("  RX/TX/Lost: %lu / %lu / %lu\n", 
                         peer.packetsReceived, peer.packetsSent, peer.packetsLost);
        }
        
        xSemaphoreGive(peersMutex);
    }
    
    DEBUG_PRINTLN("\n═══════════════════════════════════════════════\n");
}