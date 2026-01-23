/**
 * ESPNowRemoteController.h
 * 
 * Projekt-spezifische ESP-NOW Controller-Klasse für Remote Control System
 * Erweitert ESPNowManager um anwendungsspezifische Funktionalität
 * 
 * ROLLE: MASTER (Remote Control)
 * - Sendet: Joystick-Daten, Motor-Commands, PAIR_REQUEST, HEARTBEAT
 * - Empfängt: Telemetrie, Status-Updates, PAIR_RESPONSE, ACK vom Slave (Vehicle)
 * - ACK dient als Heartbeat-Bestätigung und verlängert Verbindungs-Timeout
 * 
 * Features:
 * - Joystick-Datenstrukturen und Commands
 * - Motor/Antrieb Commands
 * - Telemetrie (Batterie, Sensoren)
 * - High-Level Sende-/Empfangsmethoden
 * - Pairing-Protocol: Sendet PAIR_REQUEST, empfängt PAIR_RESPONSE
 * - Heartbeat mit ACK-Bestätigung für Timeout-Management
 * - Projekt-spezifische Datentypen
 */

#ifndef ESP_NOW_REMOTE_CONTROLLER_H
#define ESP_NOW_REMOTE_CONTROLLER_H

#include "ESPNowManager.h"
#include "ESPNowPacket.h"

// DataCmd ist bereits in ESPNowPacket.h definiert - keine eigene Enum nötig!

// ═══════════════════════════════════════════════════════════════════════════
// PROJEKT-SPEZIFISCHE DATENSTRUKTUREN
// ═══════════════════════════════════════════════════════════════════════════

/**
 * Joystick-Daten (kombiniert)
 */
struct JoystickData {
    int16_t x;          // X-Achse (-32768 bis +32767)
    int16_t y;          // Y-Achse (-32768 bis +32767)
    uint8_t button;     // Button-Status (0/1)
} __attribute__((packed));

/**
 * Motor-Daten (kombiniert)
 */
struct MotorData {
    int16_t left;       // Linker Motor (-100 bis +100)
    int16_t right;      // Rechter Motor (-100 bis +100)
} __attribute__((packed));

/**
 * Beschleunigungsdaten (3-Achsen)
 */
struct AccelerationData {
    int16_t x;          // X-Achse
    int16_t y;          // Y-Achse
    int16_t z;          // Z-Achse
} __attribute__((packed));

/**
 * Gyroskop-Daten (3-Achsen)
 */
struct GyroscopeData {
    int16_t x;          // X-Achse
    int16_t y;          // Y-Achse
    int16_t z;          // Z-Achse
} __attribute__((packed));

/**
 * Telemetrie-Daten (kombiniert)
 */
struct TelemetryData {
    uint16_t batteryVoltage;    // Batteriespannung (mV)
    uint8_t batteryPercent;     // Batterieladung (0-100%)
    int16_t temperature;        // Temperatur (°C * 10)
    int8_t rssi;                // Signalstärke (dBm)
} __attribute__((packed));

// ═══════════════════════════════════════════════════════════════════════════
// ERWEITERTE PAKET-KLASSE
// ═══════════════════════════════════════════════════════════════════════════

/**
 * Erweitertes Paket mit projekt-spezifischen Helper-Methoden
 */
class RemoteESPNowPacket : public ESPNowPacket {
public:
    // ═══════════════════════════════════════════════════════════════════════
    // BUILDER-ERWEITERUNGEN (RemoteDataCmd)
    // ═══════════════════════════════════════════════════════════════════════
    
    /**
     * Joystick-Daten hinzufügen (einzeln)
     */
    RemoteESPNowPacket& addJoystickX(int16_t x);
    RemoteESPNowPacket& addJoystickY(int16_t y);
    RemoteESPNowPacket& addJoystickButton(bool pressed);
    
    /**
     * Joystick-Daten hinzufügen (kombiniert)
     */
    RemoteESPNowPacket& addJoystick(const JoystickData& data);
    RemoteESPNowPacket& addJoystick(int16_t x, int16_t y, bool button);
    
    /**
     * Motor-Daten hinzufügen
     */
    RemoteESPNowPacket& addMotorLeft(int16_t value);
    RemoteESPNowPacket& addMotorRight(int16_t value);
    RemoteESPNowPacket& addMotors(const MotorData& data);
    RemoteESPNowPacket& addMotors(int16_t left, int16_t right);
    
    /**
     * Telemetrie-Daten hinzufügen
     */
    RemoteESPNowPacket& addBatteryVoltage(uint16_t voltage);
    RemoteESPNowPacket& addBatteryPercent(uint8_t percent);
    RemoteESPNowPacket& addTemperature(int16_t temp);
    RemoteESPNowPacket& addRSSI(int8_t rssi);
    RemoteESPNowPacket& addTelemetry(const TelemetryData& data);
    
    /**
     * Sensor-Daten hinzufügen
     */
    RemoteESPNowPacket& addAcceleration(const AccelerationData& data);
    RemoteESPNowPacket& addGyroscope(const GyroscopeData& data);
    
    // ═══════════════════════════════════════════════════════════════════════
    // PARSER-ERWEITERUNGEN
    // ═══════════════════════════════════════════════════════════════════════
    
    /**
     * Joystick-Daten abrufen
     */
    bool getJoystickX(int16_t& outValue) const;
    bool getJoystickY(int16_t& outValue) const;
    bool getJoystickButton(bool& outValue) const;
    bool getJoystick(JoystickData& outData) const;
    
    /**
     * Motor-Daten abrufen
     */
    bool getMotorLeft(int16_t& outValue) const;
    bool getMotorRight(int16_t& outValue) const;
    bool getMotors(MotorData& outData) const;
    
    /**
     * Telemetrie-Daten abrufen
     */
    bool getBatteryVoltage(uint16_t& outValue) const;
    bool getBatteryPercent(uint8_t& outValue) const;
    bool getTemperature(int16_t& outValue) const;
    bool getRSSI(int8_t& outValue) const;
    bool getTelemetry(TelemetryData& outData) const;
    
    /**
     * Sensor-Daten abrufen
     */
    bool getAcceleration(AccelerationData& outData) const;
    bool getGyroscope(GyroscopeData& outData) const;
};

// ═══════════════════════════════════════════════════════════════════════════
// HAUPT-CONTROLLER-KLASSE
// ═══════════════════════════════════════════════════════════════════════════

/**
 * Remote Control ESP-NOW Controller
 * Erweitert ESPNowManager um projekt-spezifische Funktionalität
 */
class ESPNowRemoteController : public ESPNowManager {
public:
    
    /**
     * Konstruktor
     */
    ESPNowRemoteController();
    
    /**
     * Destruktor
     */
    ~ESPNowRemoteController() override;
    
    // ═══════════════════════════════════════════════════════════════════════
    // HIGH-LEVEL SENDE-METHODEN
    // ═══════════════════════════════════════════════════════════════════════
    
    /**
     * Joystick-Daten senden
     */
    bool sendJoystick(const uint8_t* mac, int16_t x, int16_t y, bool button);
    bool sendJoystick(const uint8_t* mac, const JoystickData& data);
    
    /**
     * Motor-Commands senden
     */
    bool sendMotorCommand(const uint8_t* mac, int16_t left, int16_t right);
    bool sendMotorCommand(const uint8_t* mac, const MotorData& data);
    
    /**
     * Telemetrie senden
     */
    bool sendTelemetry(const uint8_t* mac, const TelemetryData& data);
    bool sendBatteryStatus(const uint8_t* mac, uint16_t voltage, uint8_t percent);
    
    /**
     * Status-Updates senden
     */
    bool sendStatus(const uint8_t* mac, uint8_t status);
    bool sendError(const uint8_t* mac, uint8_t errorCode);
    
    // ═══════════════════════════════════════════════════════════════════════
    // PAIRING PROTOCOL
    // ═══════════════════════════════════════════════════════════════════════
    
    /**
     * Pairing initiieren (sendet PAIR_REQUEST)
     * @param mac Ziel-MAC-Adresse
     * @return true bei erfolgreichem Senden
     */
    bool startPairing(const uint8_t* mac);
    
    // ═══════════════════════════════════════════════════════════════════════
    // SPEZIFISCHE CALLBACKS
    // ═══════════════════════════════════════════════════════════════════════
    
    /**
     * Callback für Joystick-Daten
     */
    typedef std::function<void(const uint8_t* mac, const JoystickData& data)> JoystickCallback;
    void setJoystickCallback(JoystickCallback callback);
    
    /**
     * Callback für Motor-Commands
     */
    typedef std::function<void(const uint8_t* mac, const MotorData& data)> MotorCallback;
    void setMotorCallback(MotorCallback callback);
    
    /**
     * Callback für Telemetrie
     */
    typedef std::function<void(const uint8_t* mac, const TelemetryData& data)> TelemetryCallback;
    void setTelemetryCallback(TelemetryCallback callback);
    
    // ═══════════════════════════════════════════════════════════════════════
    // OVERRIDE: Spezialisierte RX-Verarbeitung
    // ═══════════════════════════════════════════════════════════════════════
    
    /**
     * Überschriebene processRxQueue mit projekt-spezifischer Verarbeitung
     */
    void processRxQueue() override;
    
    /**
     * Debug-Info mit projekt-spezifischen Daten
     */
    void printInfo() override;

protected:
    // Projekt-spezifische Callbacks
    JoystickCallback joystickCallback;
    MotorCallback motorCallback;
    TelemetryCallback telemetryCallback;
    
    // Interne Verarbeitungsmethoden
    void handleJoystickData(const uint8_t* mac, RemoteESPNowPacket& packet);
    void handleMotorData(const uint8_t* mac, RemoteESPNowPacket& packet);
    void handleTelemetryData(const uint8_t* mac, RemoteESPNowPacket& packet);
};

#endif // ESP_NOW_REMOTE_CONTROLLER_H