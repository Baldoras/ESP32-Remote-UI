/**
 * SDCardHandler.h
 * 
 * SD-Karten Handler für ESP32-S3 mit JSON-Logging
 * 
 * Features:
 * - Mount/Unmount auf VSPI
 * - JSON-basiertes Logging
 * - Boot-Log, Battery-Log, ESP-NOW-Log, System-Log
 * - Automatisches Flush/Sync
 * - Thread-safe Operationen
 * 
 * REFACTORED: Config-Management entfernt - nur noch File I/O!
 */

#ifndef SD_CARD_HANDLER_H
#define SD_CARD_HANDLER_H

#include <Arduino.h>
#include <SD.h>
#include <SPI.h>
#include <FS.h>
#include <ArduinoJson.h>
#include "setupConf.h"

// Log-Dateinamen
#define LOG_FILE_BOOT       "/boot.log"
#define LOG_FILE_BATTERY    "/battery.log"
#define LOG_FILE_CONNECTION "/connection.log"
#define LOG_FILE_ERROR      "/error.log"

// Logging-Konfiguration
#define LOG_BUFFER_SIZE     512     // JSON-Buffer Größe
#define LOG_FLUSH_INTERVAL  5000    // Auto-Flush alle 5 Sekunden
#define LOG_MAX_FILE_SIZE   1048576 // 1 MB - dann rotieren

class SDCardHandler {
public:
    /**
     * Konstruktor
     */
    SDCardHandler();

    /**
     * Destruktor
     */
    ~SDCardHandler();

    /**
     * SD-Karte initialisieren und mounten
     * @return true bei Erfolg
     */
    bool begin();

    /**
     * SD-Karte unmounten
     */
    void end();

    /**
     * Ist SD-Karte verfügbar?
     */
    bool isAvailable() const { return mounted; }

    /**
     * Freier Speicher in Bytes
     */
    uint64_t getFreeSpace();

    /**
     * Gesamter Speicher in Bytes
     */
    uint64_t getTotalSpace();

    // ═══════════════════════════════════════════════════════════════════════
    // BOOT LOG (Setup-Methode)
    // ═══════════════════════════════════════════════════════════════════════

    /**
     * Boot-Start loggen
     * @param reason Reset-Grund (z.B. "PowerOn", "WatchdogReset")
     * @param freeHeap Freier Heap in Bytes
     * @param version Firmware-Version
     */
    bool logBootStart(const char* reason, uint32_t freeHeap, const char* version);

    /**
     * Setup-Step loggen (während der Initialisierung)
     * @param module Modul-Name (z.B. "Display", "Touch", "ESP-NOW")
     * @param success Erfolgreich?
     * @param message Optionale Nachricht
     */
    bool logSetupStep(const char* module, bool success, const char* message = nullptr);

    /**
     * Boot-Complete loggen
     * @param totalTimeMs Gesamt-Initialisierungszeit in ms
     * @param success Alle Module erfolgreich?
     */
    bool logBootComplete(uint32_t totalTimeMs, bool success);

    // ═══════════════════════════════════════════════════════════════════════
    // BATTERY LOG
    // ═══════════════════════════════════════════════════════════════════════

    /**
     * Battery-Status loggen
     * @param voltage Spannung in Volt
     * @param percent Ladezustand in %
     * @param isLow Low-Battery Status
     * @param isCritical Critical-Battery Status
     */
    bool logBattery(float voltage, uint8_t percent, bool isLow, bool isCritical);

    // ═══════════════════════════════════════════════════════════════════════
    // CONNECTION LOG (ESP-NOW)
    // ═══════════════════════════════════════════════════════════════════════

    /**
     * ESP-NOW Verbindungs-Event loggen
     * @param peerMac Peer MAC-Adresse als String
     * @param event Event-Typ ("paired", "connected", "disconnected", "timeout")
     * @param rssi Signalstärke in dBm
     */
    bool logConnection(const char* peerMac, const char* event, int8_t rssi = 0);

    /**
     * ESP-NOW Statistiken loggen
     * @param peerMac Peer MAC-Adresse als String
     * @param packetsSent Gesendete Pakete
     * @param packetsReceived Empfangene Pakete
     * @param packetsLost Verlorene Pakete
     * @param sendRate Sende-Rate (pkt/s)
     * @param receiveRate Empfangs-Rate (pkt/s)
     * @param avgRssi Durchschnittlicher RSSI
     */
    bool logConnectionStats(const char* peerMac, uint32_t packetsSent, 
                            uint32_t packetsReceived, uint32_t packetsLost,
                            uint16_t sendRate, uint16_t receiveRate, int8_t avgRssi);

    // ═══════════════════════════════════════════════════════════════════════
    // ERROR LOG (Exceptions & Errors)
    // ═══════════════════════════════════════════════════════════════════════

    /**
     * Error/Exception loggen
     * @param module Modul-Name (z.B. "Display", "Touch", "ESP-NOW")
     * @param errorCode Fehler-Code
     * @param message Fehlermeldung
     * @param freeHeap Aktueller freier Heap (optional)
     */
    bool logError(const char* module, int errorCode, const char* message, uint32_t freeHeap = 0);

    /**
     * Guru Meditation Error loggen (Crash)
     * @param pc Program Counter
     * @param excvaddr Exception Address
     * @param exccause Exception Cause
     * @param stackTrace Stack-Trace (optional)
     */
    bool logCrash(uint32_t pc, uint32_t excvaddr, uint32_t exccause, const char* stackTrace = nullptr);

    // ═══════════════════════════════════════════════════════════════════════
    // FILE OPERATIONEN (Generisch - für alle Zwecke)
    // ═══════════════════════════════════════════════════════════════════════

    /**
     * Datei schreiben (überschreiben)
     * @param path Dateipfad
     * @param data Daten
     * @return true bei Erfolg
     */
    bool writeFile(const char* path, const char* data);

    /**
     * Datei anhängen
     * @param path Dateipfad
     * @param data Daten
     * @return true bei Erfolg
     */
    bool appendFile(const char* path, const char* data);

    /**
     * Datei lesen (kompletter Inhalt)
     * @param path Dateipfad
     * @param buffer Buffer für Daten
     * @param maxLen Maximale Länge
     * @return Anzahl gelesener Bytes, -1 bei Fehler
     */
    int readFile(const char* path, char* buffer, size_t maxLen);

    /**
     * Datei lesen (als String)
     * @param path Dateipfad
     * @return Dateiinhalt als String (leer bei Fehler)
     */
    String readFileString(const char* path);

    /**
     * Datei löschen
     * @param path Dateipfad
     * @return true bei Erfolg
     */
    bool deleteFile(const char* path);

    /**
     * Datei existiert?
     * @param path Dateipfad
     * @return true wenn vorhanden
     */
    bool fileExists(const char* path);

    /**
     * Dateigröße abrufen
     * @param path Dateipfad
     * @return Größe in Bytes, 0 bei Fehler
     */
    size_t getFileSize(const char* path);

    /**
     * Datei umbenennen/verschieben
     * @param oldPath Alter Pfad
     * @param newPath Neuer Pfad
     * @return true bei Erfolg
     */
    bool renameFile(const char* oldPath, const char* newPath);

    /**
     * Alle Log-Dateien löschen
     */
    void clearAllLogs();

    /**
     * Manuelles Flush (alle offenen Dateien)
     */
    void flush();

    /**
     * Debug-Info ausgeben
     */
    void printInfo();

private:
    bool mounted;               // Mount-Status
    SPIClass* vspi;             // VSPI-Bus Pointer
    unsigned long lastFlush;    // Letzter Flush-Zeitpunkt
    
    /**
     * Zeitstempel generieren (millis)
     * @return Zeitstempel als String
     */
    String getTimestamp();
    
    /**
     * Datei rotieren wenn zu groß
     * @param path Dateipfad
     */
    void rotateLogIfNeeded(const char* path);
    
    /**
     * Auto-Flush prüfen
     */
    void checkAutoFlush();
};

#endif // SD_CARD_HANDLER_H