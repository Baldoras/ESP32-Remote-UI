/**
 * UserConfig.h
 * 
 * User-Konfiguration mit Scheme-basiertem Config-Management
 * Erbt von ConfigManager und definiert die Struktur der User-Settings
 * 
 * Verwendung:
 *   UserConfig uConf;
 *   uConf.init("/config.json", &sdCard);
 *   uConf.load();
 *   
 *   uint8_t brightness = uConf.getBacklightDefault();
 *   uConf.setBacklightDefault(128);
 *   uConf.save();
 */

#ifndef USER_CONFIG_H
#define USER_CONFIG_H

#include "ConfigManager.h"

/**
 * UserConfigStruct - alle editierbaren User-Einstellungen
 */
struct UserConfigStruct {
    // Display
    uint8_t backlightDefault;
    
    // Touch
    int16_t touchMinX;
    int16_t touchMaxX;
    int16_t touchMinY;
    int16_t touchMaxY;
    uint16_t touchThreshold;
    uint8_t touchRotation;
    
    // ESP-NOW
    uint8_t espnowChannel;
    uint8_t espnowMaxPeers;
    uint32_t espnowHeartbeat;
    uint32_t espnowTimeout;
    char espnowPeerMac[18];  // "XX:XX:XX:XX:XX:XX"
    
    // Joystick
    uint8_t joyDeadzone;
    uint16_t joyUpdateInterval;
    bool joyInvertX;
    bool joyInvertY;
    
    // Joystick Kalibrierung
    int16_t joyCalXMin;
    int16_t joyCalXCenter;
    int16_t joyCalXMax;
    int16_t joyCalYMin;
    int16_t joyCalYCenter;
    int16_t joyCalYMax;
    
    // Power
    bool autoShutdownEnabled;
    
    // Debug
    bool debugSerialEnabled;
};

class UserConfig : public ConfigManager {
public:
    /**
     * Konstruktor
     */
    UserConfig();
    
    /**
     * Destruktor
     */
    ~UserConfig();

    // ═══════════════════════════════════════════════════════════════════════
    // PUBLIC INTERFACE
    // ═══════════════════════════════════════════════════════════════════════
    
    /**
     * UserConfig initialisieren
     * @param configPath Pfad zur Config-Datei auf SD (z.B. "/config.json")
     * @param sdHandler Pointer zum SDCardHandler
     * @return true bei Erfolg
     */
    bool init(const char* configPath, SDCardHandler* sdHandler);
    
    /**
     * Config laden (aus Storage)
     * @return true bei Erfolg
     */
    bool load();
    
    /**
     * Config speichern (zu Storage)
     * @return true bei Erfolg
     */
    bool save();
    
    /**
     * Config validieren
     * Prüft alle Werte auf Gültigkeit und korrigiert falls nötig
     * @return true wenn gültig (oder korrigiert)
     */
    bool validate();
    
    /**
     * Config auf Defaults zurücksetzen
     */
    void reset();
    
    /**
     * Debug-Info ausgeben
     */
    void printInfo() const;

    ConfigScheme getConfigScheme() const;

    // ═══════════════════════════════════════════════════════════════════════
    // GETTER (Lesezugriff)
    // ═══════════════════════════════════════════════════════════════════════
    
    // Display
    uint8_t getBacklightDefault() const { return config.backlightDefault; }
    
    // Touch
    int16_t getTouchMinX() const { return config.touchMinX; }
    int16_t getTouchMaxX() const { return config.touchMaxX; }
    int16_t getTouchMinY() const { return config.touchMinY; }
    int16_t getTouchMaxY() const { return config.touchMaxY; }
    uint16_t getTouchThreshold() const { return config.touchThreshold; }
    uint8_t getTouchRotation() const { return config.touchRotation; }
    
    // ESP-NOW
    uint8_t getEspnowChannel() const { return config.espnowChannel; }
    uint8_t getEspnowMaxPeers() const { return config.espnowMaxPeers; }
    uint32_t getEspnowHeartbeat() const { return config.espnowHeartbeat; }
    uint32_t getEspnowTimeout() const { return config.espnowTimeout; }
    const char* getEspnowPeerMac() const { return config.espnowPeerMac; }
    
    // Joystick
    uint8_t getJoyDeadzone() const { return config.joyDeadzone; }
    uint16_t getJoyUpdateInterval() const { return config.joyUpdateInterval; }
    bool getJoyInvertX() const { return config.joyInvertX; }
    bool getJoyInvertY() const { return config.joyInvertY; }
    
    // Joystick Kalibrierung
    int16_t getJoyCalXMin() const { return config.joyCalXMin; }
    int16_t getJoyCalXCenter() const { return config.joyCalXCenter; }
    int16_t getJoyCalXMax() const { return config.joyCalXMax; }
    int16_t getJoyCalYMin() const { return config.joyCalYMin; }
    int16_t getJoyCalYCenter() const { return config.joyCalYCenter; }
    int16_t getJoyCalYMax() const { return config.joyCalYMax; }
    
    // Power
    bool getAutoShutdownEnabled() const { return config.autoShutdownEnabled; }
    
    // Debug
    bool getDebugSerialEnabled() const { return config.debugSerialEnabled; }

    // ═══════════════════════════════════════════════════════════════════════
    // SETTER (Schreibzugriff mit Dirty-Tracking)
    // ═══════════════════════════════════════════════════════════════════════
    
    // Display
    void setBacklightDefault(uint8_t value);
    
    // Touch
    void setTouchCalibration(int16_t minX, int16_t maxX, int16_t minY, int16_t maxY);
    void setTouchThreshold(uint16_t value);
    void setTouchRotation(uint8_t value);
    
    // ESP-NOW
    void setEspnowChannel(uint8_t value);
    void setEspnowMaxPeers(uint8_t value);
    void setEspnowHeartbeat(uint32_t value);
    void setEspnowTimeout(uint32_t value);
    void setEspnowPeerMac(const char* mac);
    
    // Joystick
    void setJoyDeadzone(uint8_t value);
    void setJoyUpdateInterval(uint16_t value);
    void setJoyInvertX(bool value);
    void setJoyInvertY(bool value);
    
    // Joystick Kalibrierung
    void setJoyCalibration(uint8_t axis, int16_t min, int16_t center, int16_t max);
    
    // Power
    void setAutoShutdownEnabled(bool value);
    
    // Debug
    void setDebugSerialEnabled(bool value);

    ConfigScheme getConfigScheme();
    
private:
    UserConfigStruct config;          // Aktuelle Config-Werte
    UserConfigStruct defaults;        // Default-Werte
    
    /**
     * Config-Scheme aufbauen
     * Definiert Struktur und Metadaten aller Config-Items
     */
    ConfigScheme buildScheme();
    
    /**
     * Default-Werte aus userConf.h initialisieren
     */
    void initDefaults();
};

#endif // USER_CONFIG_H