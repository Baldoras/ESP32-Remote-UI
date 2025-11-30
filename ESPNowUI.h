/**
 * ESPNowUI.h
 * 
 * Helper-Klasse für ESP-NOW UI-Integration
 * 
 * Features:
 * - Thread-safe Callbacks von ESP-NOW Worker zu UI
 * - Automatische Updates von ConnectionPage und RemoteControlPage
 * - Event-Handling für Verbindungsstatus, RSSI, etc.
 * - Puffer für Telemetrie-Daten
 */

#ifndef ESPNOW_UI_H
#define ESPNOW_UI_H

#include <Arduino.h>
#include "ESPNowManager.h"
#include "ConnectionPage.h"
#include "RemoteControlPage.h"

class ESPNowUI {
public:
    /**
     * Konstruktor
     */
    ESPNowUI();
    
    /**
     * Destruktor
     */
    ~ESPNowUI();

    /**
     * Initialisieren
     * @param connectionPage Pointer zur ConnectionPage (optional)
     * @param remotePage Pointer zur RemoteControlPage (optional)
     */
    void begin(ConnectionPage* connectionPage = nullptr, 
               RemoteControlPage* remotePage = nullptr);

    /**
     * Update-Schleife (in loop() aufrufen)
     * Verarbeitet ESP-NOW Events und aktualisiert UI
     */
    void update();

    /**
     * ConnectionPage setzen
     */
    void setConnectionPage(ConnectionPage* page) { connPage = page; }

    /**
     * RemoteControlPage setzen
     */
    void setRemotePage(RemoteControlPage* page) { remotePage = page; }

    /**
     * Joystick-Daten senden (für Remote)
     * @param x X-Achse (-100 bis +100)
     * @param y Y-Achse (-100 bis +100)
     * @param btn Button-Status (0/1)
     * @return true wenn gesendet
     */
    bool sendJoystick(int16_t x, int16_t y, uint8_t btn = 0);

    /**
     * Motor-Daten senden (für Fahrwerk)
     * @param left Links (-100 bis +100)
     * @param right Rechts (-100 bis +100)
     * @return true wenn gesendet
     */
    bool sendMotor(int16_t left, int16_t right);

    /**
     * Batterie-Daten senden
     * @param voltage Spannung in mV (z.B. 8400 für 8.4V)
     * @param percent Prozent (0-100)
     * @return true wenn gesendet
     */
    bool sendBattery(uint16_t voltage, uint8_t percent);

    /**
     * Peer-MAC setzen (für Verbindung)
     */
    void setPeerMac(const uint8_t* mac);

    /**
     * Peer-MAC setzen (String)
     */
    void setPeerMac(const char* macStr);

    /**
     * Sende-Rate abrufen (Pakete/Sekunde)
     */
    uint16_t getSendRate() const { return sendRate; }

    /**
     * Empfangs-Rate abrufen (Pakete/Sekunde)
     */
    uint16_t getReceiveRate() const { return receiveRate; }

private:
    // ESP-NOW Manager Referenz
    EspNowManager& espnow;
    
    // UI-Seiten
    ConnectionPage* connPage;
    RemoteControlPage* remotePage;
    
    // Peer-MAC
    uint8_t peerMac[6];
    bool hasPeer;
    
    // Telemetrie-Puffer (vom Peer empfangen)
    struct {
        int16_t motorLeft;
        int16_t motorRight;
        bool hasMotor;
        
        uint16_t batteryVoltage;  // in mV
        uint8_t batteryPercent;
        bool hasBattery;
        
        int8_t rssi;
        bool hasRSSI;
        
        unsigned long lastUpdate;
    } telemetry;
    
    // Statistiken
    uint16_t sendRate;          // Pakete/Sekunde
    uint16_t receiveRate;       // Pakete/Sekunde
    unsigned long lastSendTime;
    unsigned long lastReceiveTime;
    uint16_t sendCounter;
    uint16_t receiveCounter;
    unsigned long lastStatsUpdate;
    
    // Interne Methoden
    void processReceivedData();
    void updateStatistics();
    void updateConnectionPage();
    void updateRemotePage();
    
    // RSSI aus letztem Paket extrahieren (vereinfacht)
    int8_t estimateRSSI();
};

#endif // ESPNOW_UI_H
