/**
 * UserConfig.cpp
 * 
 * Implementation der UserConfig Interface-Klasse
 */

#include "include/UserConfig.h"
#include "include/userConf.h"
#include "include/setupConf.h"

UserConfig::UserConfig()
    : ConfigManager()
{
    // Defaults initialisieren
    initDefaults();
    
    // Config auf Defaults setzen
    memcpy(&config, &defaults, sizeof(UserConfigStruct));
}

UserConfig::~UserConfig() {
}

// ═══════════════════════════════════════════════════════════════════════════
// PUBLIC INTERFACE
// ═══════════════════════════════════════════════════════════════════════════

bool UserConfig::init(const char* configPath, SDCardHandler* sdHandler) {
    DEBUG_PRINTLN("UserConfig: Initialisiere...");
    
    // SDCardHandler setzen
    setSDCardHandler(sdHandler);
    
    // Config-Pfad setzen
    setConfigPath(configPath);
    
    DEBUG_PRINTLN("UserConfig: ✅ Initialisiert");
    return true;
}

bool UserConfig::load() {
    DEBUG_PRINTLN("UserConfig: Lade Config...");
    
    // 1. Aus Storage laden
    String content;
    if (!loadFromStorage(content)) {
        DEBUG_PRINTLN("UserConfig: ⚠️ Keine Config gefunden, verwende Defaults");
        reset();
        return false;
    }
    
    // 2. Scheme aufbauen
    ConfigScheme scheme = buildScheme();
    
    // 3. JSON deserialisieren
    if (!deserializeFromJson(content, scheme)) {
        DEBUG_PRINTLN("UserConfig: ❌ JSON-Deserialisierung fehlgeschlagen");
        return false;
    }
    
    // 4. Validieren
    if (!validate()) {
        DEBUG_PRINTLN("UserConfig: ⚠️ Werte korrigiert");
    }
    
    setDirty(false);
    
    DEBUG_PRINTLN("UserConfig: ✅ Config geladen");
    return true;
}

bool UserConfig::save() {
    DEBUG_PRINTLN("UserConfig: Speichere Config...");
    
    // 1. Validieren vor dem Speichern
    validate();
    
    // 2. Scheme aufbauen
    ConfigScheme scheme = buildScheme();
    
    // 3. Zu JSON serialisieren
    String content;
    if (!serializeToJson(content, scheme)) {
        DEBUG_PRINTLN("UserConfig: ❌ JSON-Serialisierung fehlgeschlagen");
        return false;
    }
    
    // 4. Zu Storage speichern
    if (!saveToStorage(content)) {
        DEBUG_PRINTLN("UserConfig: ❌ Speichern fehlgeschlagen");
        return false;
    }
    
    setDirty(false);
    
    DEBUG_PRINTLN("UserConfig: ✅ Config gespeichert");
    return true;
}

bool UserConfig::validate() {
    // Scheme aufbauen
    ConfigScheme scheme = buildScheme();
    
    // Generische Validierung der Basis-Klasse verwenden
    return ConfigManager::validate(scheme);
}

void UserConfig::reset() {
    DEBUG_PRINTLN("UserConfig: Setze auf Defaults zurück...");
    
    // Scheme aufbauen
    ConfigScheme scheme = buildScheme();
    
    // Defaults laden (aus Basis-Klasse)
    loadDefaults(scheme);
    
    setDirty(true);
    
    DEBUG_PRINTLN("UserConfig: ✅ Defaults geladen");
}

void UserConfig::printInfo() const {
    DEBUG_PRINTLN("═══════════════════════════════════════════════════════");
    DEBUG_PRINTLN("UserConfig - Aktuelle Werte:");
    DEBUG_PRINTLN("═══════════════════════════════════════════════════════");
    
    // Display
    DEBUG_PRINTLN("[Display]");
    DEBUG_PRINTF("  backlightDefault: %d\n", config.backlightDefault);
    
    // Touch
    DEBUG_PRINTLN("[Touch]");
    DEBUG_PRINTF("  touchMinX: %d\n", config.touchMinX);
    DEBUG_PRINTF("  touchMaxX: %d\n", config.touchMaxX);
    DEBUG_PRINTF("  touchMinY: %d\n", config.touchMinY);
    DEBUG_PRINTF("  touchMaxY: %d\n", config.touchMaxY);
    DEBUG_PRINTF("  touchThreshold: %d\n", config.touchThreshold);
    DEBUG_PRINTF("  touchRotation: %d\n", config.touchRotation);
    
    // ESP-NOW
    DEBUG_PRINTLN("[ESP-NOW]");
    DEBUG_PRINTF("  espnowChannel: %d\n", config.espnowChannel);
    DEBUG_PRINTF("  espnowMaxPeers: %d\n", config.espnowMaxPeers);
    DEBUG_PRINTF("  espnowHeartbeat: %lu ms\n", config.espnowHeartbeat);
    DEBUG_PRINTF("  espnowTimeout: %lu ms\n", config.espnowTimeout);
    DEBUG_PRINTF("  espnowPeerMac: %s\n", config.espnowPeerMac);
    
    // Joystick
    DEBUG_PRINTLN("[Joystick]");
    DEBUG_PRINTF("  joyDeadzone: %d %%\n", config.joyDeadzone);
    DEBUG_PRINTF("  joyUpdateInterval: %d ms\n", config.joyUpdateInterval);
    DEBUG_PRINTF("  joyInvertX: %s\n", config.joyInvertX ? "true" : "false");
    DEBUG_PRINTF("  joyInvertY: %s\n", config.joyInvertY ? "true" : "false");
    
    // Joystick Kalibrierung
    DEBUG_PRINTLN("[Joystick Kalibrierung]");
    DEBUG_PRINTF("  X-Axis: min=%d, center=%d, max=%d\n", 
                 config.joyCalXMin, config.joyCalXCenter, config.joyCalXMax);
    DEBUG_PRINTF("  Y-Axis: min=%d, center=%d, max=%d\n",
                 config.joyCalYMin, config.joyCalYCenter, config.joyCalYMax);
    
    // Power
    DEBUG_PRINTLN("[Power]");
    DEBUG_PRINTF("  autoShutdownEnabled: %s\n", config.autoShutdownEnabled ? "true" : "false");
    
    // Debug
    DEBUG_PRINTLN("[Debug]");
    DEBUG_PRINTF("  debugSerialEnabled: %s\n", config.debugSerialEnabled ? "true" : "false");
    
    DEBUG_PRINTLN("═══════════════════════════════════════════════════════");
}

ConfigScheme UserConfig::getConfigScheme() {
    return buildScheme();
}

// ═══════════════════════════════════════════════════════════════════════════
// SETTER (mit Dirty-Tracking)
// ═══════════════════════════════════════════════════════════════════════════

void UserConfig::setBacklightDefault(uint8_t value) {
    config.backlightDefault = value;
    setDirty(true);
}

void UserConfig::setTouchCalibration(int16_t minX, int16_t maxX, int16_t minY, int16_t maxY) {
    config.touchMinX = minX;
    config.touchMaxX = maxX;
    config.touchMinY = minY;
    config.touchMaxY = maxY;
    setDirty(true);
}

void UserConfig::setTouchThreshold(uint16_t value) {
    config.touchThreshold = value;
    setDirty(true);
}

void UserConfig::setTouchRotation(uint8_t value) {
    config.touchRotation = value;
    setDirty(true);
}

void UserConfig::setEspnowChannel(uint8_t value) {
    config.espnowChannel = value;
    setDirty(true);
}

void UserConfig::setEspnowMaxPeers(uint8_t value) {
    config.espnowMaxPeers = value;
    setDirty(true);
}

void UserConfig::setEspnowHeartbeat(uint32_t value) {
    config.espnowHeartbeat = value;
    setDirty(true);
}

void UserConfig::setEspnowTimeout(uint32_t value) {
    config.espnowTimeout = value;
    setDirty(true);
}

void UserConfig::setEspnowPeerMac(const char* mac) {
    if (mac) {
        strncpy(config.espnowPeerMac, mac, sizeof(config.espnowPeerMac) - 1);
        config.espnowPeerMac[sizeof(config.espnowPeerMac) - 1] = '\0';
        setDirty(true);
    }
}

void UserConfig::setJoyDeadzone(uint8_t value) {
    config.joyDeadzone = value;
    setDirty(true);
}

void UserConfig::setJoyUpdateInterval(uint16_t value) {
    config.joyUpdateInterval = value;
    setDirty(true);
}

void UserConfig::setJoyInvertX(bool value) {
    config.joyInvertX = value;
    setDirty(true);
}

void UserConfig::setJoyInvertY(bool value) {
    config.joyInvertY = value;
    setDirty(true);
}

void UserConfig::setJoyCalibration(uint8_t axis, int16_t min, int16_t center, int16_t max) {
    if (axis == 0) {
        // X-Achse
        config.joyCalXMin = min;
        config.joyCalXCenter = center;
        config.joyCalXMax = max;
    } else {
        // Y-Achse
        config.joyCalYMin = min;
        config.joyCalYCenter = center;
        config.joyCalYMax = max;
    }
    setDirty(true);
}

void UserConfig::setAutoShutdownEnabled(bool value) {
    config.autoShutdownEnabled = value;
    setDirty(true);
}

void UserConfig::setDebugSerialEnabled(bool value) {
    config.debugSerialEnabled = value;
    setDirty(true);
}

// ═══════════════════════════════════════════════════════════════════════════
// PRIVATE - Scheme Definition
// ═══════════════════════════════════════════════════════════════════════════

ConfigScheme UserConfig::buildScheme() {
    // Statisches Array mit allen Config-Items
    static ConfigItem items[] = {
        // Display
        {
            .key = "backlightDefault",
            .category = "Display",
            .type = ConfigType::UINT8,
            .valuePtr = &config.backlightDefault,
            .defaultPtr = &defaults.backlightDefault,
            .hasRange = true,
            .minValue = 0,
            .maxValue = 255,
            .maxLength = 0
        },
        
        // Touch
        {
            .key = "touchMinX",
            .category = "Touch",
            .type = ConfigType::INT16,
            .valuePtr = &config.touchMinX,
            .defaultPtr = &defaults.touchMinX,
            .hasRange = false,
            .minValue = 0,
            .maxValue = 0,
            .maxLength = 0
        },
        {
            .key = "touchMaxX",
            .category = "Touch",
            .type = ConfigType::INT16,
            .valuePtr = &config.touchMaxX,
            .defaultPtr = &defaults.touchMaxX,
            .hasRange = false,
            .minValue = 0,
            .maxValue = 0,
            .maxLength = 0
        },
        {
            .key = "touchMinY",
            .category = "Touch",
            .type = ConfigType::INT16,
            .valuePtr = &config.touchMinY,
            .defaultPtr = &defaults.touchMinY,
            .hasRange = false,
            .minValue = 0,
            .maxValue = 0,
            .maxLength = 0
        },
        {
            .key = "touchMaxY",
            .category = "Touch",
            .type = ConfigType::INT16,
            .valuePtr = &config.touchMaxY,
            .defaultPtr = &defaults.touchMaxY,
            .hasRange = false,
            .minValue = 0,
            .maxValue = 0,
            .maxLength = 0
        },
        {
            .key = "touchThreshold",
            .category = "Touch",
            .type = ConfigType::UINT16,
            .valuePtr = &config.touchThreshold,
            .defaultPtr = &defaults.touchThreshold,
            .hasRange = true,
            .minValue = 1,
            .maxValue = 255,
            .maxLength = 0
        },
        {
            .key = "touchRotation",
            .category = "Touch",
            .type = ConfigType::UINT8,
            .valuePtr = &config.touchRotation,
            .defaultPtr = &defaults.touchRotation,
            .hasRange = true,
            .minValue = 0,
            .maxValue = 3,
            .maxLength = 0
        },
        
        // ESP-NOW
        {
            .key = "espnowChannel",
            .category = "ESP-Now",
            .type = ConfigType::UINT8,
            .valuePtr = &config.espnowChannel,
            .defaultPtr = &defaults.espnowChannel,
            .hasRange = true,
            .minValue = 0,
            .maxValue = 14,
            .maxLength = 0
        },
        {
            .key = "espnowMaxPeers",
            .category = "ESP-Now",
            .type = ConfigType::UINT8,
            .valuePtr = &config.espnowMaxPeers,
            .defaultPtr = &defaults.espnowMaxPeers,
            .hasRange = true,
            .minValue = 1,
            .maxValue = 20,
            .maxLength = 0
        },
        {
            .key = "espnowHeartbeat",
            .category = "ESP-Now",
            .type = ConfigType::UINT32,
            .valuePtr = &config.espnowHeartbeat,
            .defaultPtr = &defaults.espnowHeartbeat,
            .hasRange = true,
            .minValue = 100,
            .maxValue = 10000,
            .maxLength = 0
        },
        {
            .key = "espnowTimeout",
            .category = "ESP-Now",
            .type = ConfigType::UINT32,
            .valuePtr = &config.espnowTimeout,
            .defaultPtr = &defaults.espnowTimeout,
            .hasRange = true,
            .minValue = 500,
            .maxValue = 30000,
            .maxLength = 0
        },
        {
            .key = "espnowPeerMac",
            .category = "ESP-Now",
            .type = ConfigType::STRING,
            .valuePtr = &config.espnowPeerMac,
            .defaultPtr = &defaults.espnowPeerMac,
            .hasRange = false,
            .minValue = 0,
            .maxValue = 0,
            .maxLength = sizeof(config.espnowPeerMac)
        },
        
        // Joystick
        {
            .key = "joyDeadzone",
            .category = "Joystick",
            .type = ConfigType::UINT8,
            .valuePtr = &config.joyDeadzone,
            .defaultPtr = &defaults.joyDeadzone,
            .hasRange = true,
            .minValue = 0,
            .maxValue = 50,
            .maxLength = 0
        },
        {
            .key = "joyUpdateInterval",
            .category = "Joystick",
            .type = ConfigType::UINT16,
            .valuePtr = &config.joyUpdateInterval,
            .defaultPtr = &defaults.joyUpdateInterval,
            .hasRange = true,
            .minValue = 10,
            .maxValue = 1000,
            .maxLength = 0
        },
        {
            .key = "joyInvertX",
            .category = "Joystick",
            .type = ConfigType::BOOL,
            .valuePtr = &config.joyInvertX,
            .defaultPtr = &defaults.joyInvertX,
            .hasRange = false,
            .minValue = 0,
            .maxValue = 0,
            .maxLength = 0
        },
        {
            .key = "joyInvertY",
            .category = "Joystick",
            .type = ConfigType::BOOL,
            .valuePtr = &config.joyInvertY,
            .defaultPtr = &defaults.joyInvertY,
            .hasRange = false,
            .minValue = 0,
            .maxValue = 0,
            .maxLength = 0
        },
        
        // Joystick Kalibrierung
        {
            .key = "joyCalXMin",
            .category = "Joystick",
            .type = ConfigType::INT16,
            .valuePtr = &config.joyCalXMin,
            .defaultPtr = &defaults.joyCalXMin,
            .hasRange = false,
            .minValue = 0,
            .maxValue = 0,
            .maxLength = 0
        },
        {
            .key = "joyCalXCenter",
            .category = "Joystick",
            .type = ConfigType::INT16,
            .valuePtr = &config.joyCalXCenter,
            .defaultPtr = &defaults.joyCalXCenter,
            .hasRange = false,
            .minValue = 0,
            .maxValue = 0,
            .maxLength = 0
        },
        {
            .key = "joyCalXMax",
            .category = "Joystick",
            .type = ConfigType::INT16,
            .valuePtr = &config.joyCalXMax,
            .defaultPtr = &defaults.joyCalXMax,
            .hasRange = false,
            .minValue = 0,
            .maxValue = 0,
            .maxLength = 0
        },
        {
            .key = "joyCalYMin",
            .category = "Joystick",
            .type = ConfigType::INT16,
            .valuePtr = &config.joyCalYMin,
            .defaultPtr = &defaults.joyCalYMin,
            .hasRange = false,
            .minValue = 0,
            .maxValue = 0,
            .maxLength = 0
        },
        {
            .key = "joyCalYCenter",
            .category = "Joystick",
            .type = ConfigType::INT16,
            .valuePtr = &config.joyCalYCenter,
            .defaultPtr = &defaults.joyCalYCenter,
            .hasRange = false,
            .minValue = 0,
            .maxValue = 0,
            .maxLength = 0
        },
        {
            .key = "joyCalYMax",
            .category = "Joystick",
            .type = ConfigType::INT16,
            .valuePtr = &config.joyCalYMax,
            .defaultPtr = &defaults.joyCalYMax,
            .hasRange = false,
            .minValue = 0,
            .maxValue = 0,
            .maxLength = 0
        },
        
        // Power
        {
            .key = "autoShutdownEnabled",
            .category = "Power",
            .type = ConfigType::BOOL,
            .valuePtr = &config.autoShutdownEnabled,
            .defaultPtr = &defaults.autoShutdownEnabled,
            .hasRange = false,
            .minValue = 0,
            .maxValue = 0,
            .maxLength = 0
        },
        
        // Debug
        {
            .key = "debugSerialEnabled",
            .category = "Debug",
            .type = ConfigType::BOOL,
            .valuePtr = &config.debugSerialEnabled,
            .defaultPtr = &defaults.debugSerialEnabled,
            .hasRange = false,
            .minValue = 0,
            .maxValue = 0,
            .maxLength = 0
        }
    };
    
    // Scheme zurückgeben
    ConfigScheme scheme;
    scheme.items = items;
    scheme.count = sizeof(items) / sizeof(ConfigItem);
    
    return scheme;
}

void UserConfig::initDefaults() {
    // Display
    defaults.backlightDefault = BACKLIGHT_DEFAULT;
    
    // Touch
    defaults.touchMinX = TOUCH_MIN_X;
    defaults.touchMaxX = TOUCH_MAX_X;
    defaults.touchMinY = TOUCH_MIN_Y;
    defaults.touchMaxY = TOUCH_MAX_Y;
    defaults.touchThreshold = TOUCH_THRESHOLD;
    defaults.touchRotation = TOUCH_ROTATION;
    
    // ESP-NOW
    defaults.espnowChannel = ESPNOW_CHANNEL;
    defaults.espnowMaxPeers = ESPNOW_MAX_PEERS;
    defaults.espnowHeartbeat = ESPNOW_HEARTBEAT_INTERVAL;
    defaults.espnowTimeout = ESPNOW_TIMEOUT;
    strncpy(defaults.espnowPeerMac, ESPNOW_PEER_MAC, sizeof(defaults.espnowPeerMac) - 1);
    defaults.espnowPeerMac[sizeof(defaults.espnowPeerMac) - 1] = '\0';
    
    // Joystick
    defaults.joyDeadzone = JOY_DEADZONE_PERCENT;
    defaults.joyUpdateInterval = JOY_UPDATE_INTERVAL;
    defaults.joyInvertX = JOY_INVERT_X;
    defaults.joyInvertY = JOY_INVERT_Y;
    
    // Joystick Kalibrierung
    defaults.joyCalXMin = JOY_CAL_X_MIN;
    defaults.joyCalXCenter = JOY_CAL_X_CENTER;
    defaults.joyCalXMax = JOY_CAL_X_MAX;
    defaults.joyCalYMin = JOY_CAL_Y_MIN;
    defaults.joyCalYCenter = JOY_CAL_Y_CENTER;
    defaults.joyCalYMax = JOY_CAL_Y_MAX;
    
    // Power
    defaults.autoShutdownEnabled = AUTO_SHUTDOWN;
    
    // Debug
    defaults.debugSerialEnabled = DEBUG_SERIAL;
}