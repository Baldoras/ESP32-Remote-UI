/**
 * SDCardHandler.cpp
 * 
 * Implementation des SD-Karten Handlers mit JSON-Logging
 */

#include "SDCardHandler.h"

SDCardHandler::SDCardHandler()
    : mounted(false)
    , vspi(nullptr)
    , lastFlush(0)
{
}

SDCardHandler::~SDCardHandler() {
    end();
}

bool SDCardHandler::begin() {
    DEBUG_PRINTLN("SDCardHandler: Initialisiere SD-Karte...");
    
    // VSPI initialisieren
    vspi = new SPIClass(FSPI);
    vspi->begin(SD_SCK, SD_MISO, SD_MOSI, SD_CS);
    
    // SD-Karte mounten
    if (!SD.begin(SD_CS, *vspi, SD_SPI_FREQUENCY)) {
        DEBUG_PRINTLN("SDCardHandler: ❌ Mount fehlgeschlagen!");
        delete vspi;
        vspi = nullptr;
        return false;
    }
    
    mounted = true;
    lastFlush = millis();
    
    // Card-Typ prüfen
    uint8_t cardType = SD.cardType();
    
    if (cardType == CARD_NONE) {
        DEBUG_PRINTLN("SDCardHandler: ❌ Keine SD-Karte erkannt!");
        end();
        return false;
    }
    
    DEBUG_PRINTLN("SDCardHandler: ✅ SD-Karte gemountet");
    DEBUG_PRINTF("  Typ: %s\n", 
                 cardType == CARD_MMC ? "MMC" :
                 cardType == CARD_SD ? "SDSC" :
                 cardType == CARD_SDHC ? "SDHC" : "UNKNOWN");
    DEBUG_PRINTF("  Größe: %.2f GB\n", getTotalSpace() / 1024.0 / 1024.0 / 1024.0);
    DEBUG_PRINTF("  Frei: %.2f GB\n", getFreeSpace() / 1024.0 / 1024.0 / 1024.0);
    
    return true;
}

void SDCardHandler::end() {
    if (mounted) {
        flush();
        SD.end();
        mounted = false;
        DEBUG_PRINTLN("SDCardHandler: SD-Karte unmounted");
    }
    
    if (vspi) {
        delete vspi;
        vspi = nullptr;
    }
}

uint64_t SDCardHandler::getFreeSpace() {
    if (!mounted) return 0;
    return SD.totalBytes() - SD.usedBytes();
}

uint64_t SDCardHandler::getTotalSpace() {
    if (!mounted) return 0;
    return SD.totalBytes();
}

// ═══════════════════════════════════════════════════════════════════════════
// CONFIG MANAGEMENT
// ═══════════════════════════════════════════════════════════════════════════

bool SDCardHandler::loadConfig(SDConfig& config) {
    if (!mounted) return false;
    
    if (!fileExists(CONFIG_FILE)) {
        DEBUG_PRINTLN("SDCardHandler: config.conf nicht gefunden, erstelle Default...");
        createDefaultConfig();
        return loadConfig(config);  // Rekursiv laden
    }
    
    File file = SD.open(CONFIG_FILE, FILE_READ);
    if (!file) {
        DEBUG_PRINTLN("SDCardHandler: ❌ Kann config.conf nicht öffnen!");
        return false;
    }
    
    // JSON parsen (ArduinoJson V7)
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, file);
    file.close();
    
    if (error) {
        DEBUG_PRINTF("SDCardHandler: ❌ JSON Parse-Fehler: %s\n", error.c_str());
        return false;
    }
    
    // Display
    config.backlightDefault = doc["backlight_default"] | BACKLIGHT_DEFAULT;
    
    // Touch
    config.touchMinX = doc["touch_min_x"] | TOUCH_MIN_X;
    config.touchMaxX = doc["touch_max_x"] | TOUCH_MAX_X;
    config.touchMinY = doc["touch_min_y"] | TOUCH_MIN_Y;
    config.touchMaxY = doc["touch_max_y"] | TOUCH_MAX_Y;
    config.touchThreshold = doc["touch_threshold"] | TOUCH_THRESHOLD;
    
    // ESP-NOW
    config.espnowHeartbeatInterval = doc["espnow_heartbeat"] | ESPNOW_HEARTBEAT_INTERVAL;
    config.espnowTimeout = doc["espnow_timeout"] | ESPNOW_TIMEOUT_MS;
    
    // Debug
    config.debugSerialEnabled = doc["debug_serial"] | DEBUG_SERIAL;
    
    DEBUG_PRINTLN("SDCardHandler: ✅ Config geladen");
    return true;
}

bool SDCardHandler::saveConfig(const SDConfig& config) {
    if (!mounted) return false;
    
    JsonDocument doc;  // ArduinoJson V7
    
    // Display
    doc["backlight_default"] = config.backlightDefault;
    
    // Touch
    doc["touch_min_x"] = config.touchMinX;
    doc["touch_max_x"] = config.touchMaxX;
    doc["touch_min_y"] = config.touchMinY;
    doc["touch_max_y"] = config.touchMaxY;
    doc["touch_threshold"] = config.touchThreshold;
    
    // ESP-NOW
    doc["espnow_heartbeat"] = config.espnowHeartbeatInterval;
    doc["espnow_timeout"] = config.espnowTimeout;
    
    // Debug
    doc["debug_serial"] = config.debugSerialEnabled;
    
    // Datei schreiben
    File file = SD.open(CONFIG_FILE, FILE_WRITE);
    if (!file) {
        DEBUG_PRINTLN("SDCardHandler: ❌ Kann config.conf nicht schreiben!");
        return false;
    }
    
    serializeJsonPretty(doc, file);
    file.close();
    
    DEBUG_PRINTLN("SDCardHandler: ✅ Config gespeichert");
    return true;
}

bool SDCardHandler::createDefaultConfig() {
    if (!mounted) return false;
    
    SDConfig defaultConfig;
    
    // Display
    defaultConfig.backlightDefault = BACKLIGHT_DEFAULT;
    
    // Touch
    defaultConfig.touchMinX = TOUCH_MIN_X;
    defaultConfig.touchMaxX = TOUCH_MAX_X;
    defaultConfig.touchMinY = TOUCH_MIN_Y;
    defaultConfig.touchMaxY = TOUCH_MAX_Y;
    defaultConfig.touchThreshold = TOUCH_THRESHOLD;
    
    // ESP-NOW
    defaultConfig.espnowHeartbeatInterval = ESPNOW_HEARTBEAT_INTERVAL;
    defaultConfig.espnowTimeout = ESPNOW_TIMEOUT_MS;
    
    // Debug
    defaultConfig.debugSerialEnabled = DEBUG_SERIAL;
    
    DEBUG_PRINTLN("SDCardHandler: Erstelle Default-Config...");
    return saveConfig(defaultConfig);
}

// ═══════════════════════════════════════════════════════════════════════════
// BOOT LOG (Setup-Methode)
// ═══════════════════════════════════════════════════════════════════════════

bool SDCardHandler::logBootStart(const char* reason, uint32_t freeHeap, const char* version) {
    if (!mounted) return false;
    
    JsonDocument doc;  // Lokal statt Member
    
    doc["timestamp"] = getTimestamp();
    doc["type"] = "boot_start";
    doc["reason"] = reason;
    doc["free_heap"] = freeHeap;
    doc["version"] = version;
    doc["build"] = BUILD_DATE " " BUILD_TIME;
    doc["chip_model"] = ESP.getChipModel();
    doc["cpu_freq"] = ESP.getCpuFreqMHz();
    
    return writeJsonLog(LOG_FILE_BOOT, doc);
}

bool SDCardHandler::logSetupStep(const char* module, bool success, const char* message) {
    if (!mounted) return false;
    
    JsonDocument doc;  // V7 Local
    
    doc["timestamp"] = getTimestamp();
    doc["type"] = "setup_step";
    doc["module"] = module;
    doc["success"] = success;
    
    if (message) {
        doc["message"] = message;
    }
    
    return writeJsonLog(LOG_FILE_BOOT, doc);
}

bool SDCardHandler::logBootComplete(uint32_t totalTimeMs, bool success) {
    if (!mounted) return false;
    
    JsonDocument doc;  // V7 Local
    
    doc["timestamp"] = getTimestamp();
    doc["type"] = "boot_complete";
    doc["total_time_ms"] = totalTimeMs;
    doc["success"] = success;
    doc["free_heap"] = ESP.getFreeHeap();
    
    return writeJsonLog(LOG_FILE_BOOT, doc);
}

// ═══════════════════════════════════════════════════════════════════════════
// BATTERY LOG
// ═══════════════════════════════════════════════════════════════════════════

bool SDCardHandler::logBattery(float voltage, uint8_t percent, bool isLow, bool isCritical) {
    if (!mounted) return false;
    
    JsonDocument doc;  // V7 Local
    
    doc["timestamp"] = getTimestamp();
    doc["type"] = "battery";
    doc["voltage"] = serialized(String(voltage, 2));  // 2 Dezimalstellen
    doc["percent"] = percent;
    doc["is_low"] = isLow;
    doc["is_critical"] = isCritical;
    
    return writeJsonLog(LOG_FILE_BATTERY, doc);
}

// ═══════════════════════════════════════════════════════════════════════════
// CONNECTION LOG (ESP-NOW)
// ═══════════════════════════════════════════════════════════════════════════

bool SDCardHandler::logConnection(const char* peerMac, const char* event, int8_t rssi) {
    if (!mounted) return false;
    
    JsonDocument doc;  // V7 Local
    
    doc["timestamp"] = getTimestamp();
    doc["type"] = "connection_event";
    doc["peer_mac"] = peerMac;
    doc["event"] = event;
    
    if (rssi != 0) {
        doc["rssi"] = rssi;
    }
    
    return writeJsonLog(LOG_FILE_CONNECTION, doc);
}

bool SDCardHandler::logConnectionStats(const char* peerMac, uint32_t packetsSent, 
                                        uint32_t packetsReceived, uint32_t packetsLost,
                                        uint16_t sendRate, uint16_t receiveRate, int8_t avgRssi) {
    if (!mounted) return false;
    
    JsonDocument doc;  // V7 Local
    
    doc["timestamp"] = getTimestamp();
    doc["type"] = "connection_stats";
    doc["peer_mac"] = peerMac;
    doc["packets_sent"] = packetsSent;
    doc["packets_received"] = packetsReceived;
    doc["packets_lost"] = packetsLost;
    doc["send_rate"] = sendRate;
    doc["receive_rate"] = receiveRate;
    doc["avg_rssi"] = avgRssi;
    
    // Packet-Loss Rate berechnen
    if (packetsSent > 0) {
        float lossRate = (packetsLost * 100.0f) / packetsSent;
        doc["loss_rate"] = serialized(String(lossRate, 2));
    }
    
    return writeJsonLog(LOG_FILE_CONNECTION, doc);
}

// ═══════════════════════════════════════════════════════════════════════════
// ERROR LOG (Exceptions & Errors)
// ═══════════════════════════════════════════════════════════════════════════

bool SDCardHandler::logError(const char* module, int errorCode, const char* message, uint32_t freeHeap) {
    if (!mounted) return false;
    
    JsonDocument doc;  // V7 Local
    
    doc["timestamp"] = getTimestamp();
    doc["type"] = "error";
    doc["module"] = module;
    doc["error_code"] = errorCode;
    doc["message"] = message;
    
    if (freeHeap > 0) {
        doc["free_heap"] = freeHeap;
    } else {
        doc["free_heap"] = ESP.getFreeHeap();
    }
    
    return writeJsonLog(LOG_FILE_ERROR, doc);
}

bool SDCardHandler::logCrash(uint32_t pc, uint32_t excvaddr, uint32_t exccause, const char* stackTrace) {
    if (!mounted) return false;
    
    JsonDocument doc;  // V7 Local
    
    doc["timestamp"] = getTimestamp();
    doc["type"] = "crash";
    doc["pc"] = String(pc, HEX);
    doc["excvaddr"] = String(excvaddr, HEX);
    doc["exccause"] = exccause;
    doc["free_heap"] = ESP.getFreeHeap();
    
    if (stackTrace) {
        doc["stack_trace"] = stackTrace;
    }
    
    return writeJsonLog(LOG_FILE_ERROR, doc);
}

// ═══════════════════════════════════════════════════════════════════════════
// FILE OPERATIONEN
// ═══════════════════════════════════════════════════════════════════════════

bool SDCardHandler::writeFile(const char* path, const char* data) {
    if (!mounted) return false;
    
    File file = SD.open(path, FILE_WRITE);
    if (!file) {
        DEBUG_PRINTF("SDCardHandler: ❌ Kann Datei nicht öffnen: %s\n", path);
        return false;
    }
    
    size_t written = file.print(data);
    file.close();
    
    checkAutoFlush();
    
    return written > 0;
}

bool SDCardHandler::appendFile(const char* path, const char* data) {
    if (!mounted) return false;
    
    File file = SD.open(path, FILE_APPEND);
    if (!file) {
        DEBUG_PRINTF("SDCardHandler: ❌ Kann Datei nicht öffnen: %s\n", path);
        return false;
    }
    
    size_t written = file.print(data);
    file.close();
    
    checkAutoFlush();
    
    return written > 0;
}

int SDCardHandler::readFile(const char* path, char* buffer, size_t maxLen) {
    if (!mounted || !buffer) return -1;
    
    File file = SD.open(path, FILE_READ);
    if (!file) {
        DEBUG_PRINTF("SDCardHandler: ❌ Kann Datei nicht lesen: %s\n", path);
        return -1;
    }
    
    size_t len = file.size();
    if (len > maxLen) len = maxLen;
    
    size_t bytesRead = file.readBytes(buffer, len);
    file.close();
    
    return bytesRead;
}

bool SDCardHandler::deleteFile(const char* path) {
    if (!mounted) return false;
    
    if (!SD.exists(path)) {
        DEBUG_PRINTF("SDCardHandler: Datei existiert nicht: %s\n", path);
        return false;
    }
    
    bool success = SD.remove(path);
    
    if (success) {
        DEBUG_PRINTF("SDCardHandler: ✅ Datei gelöscht: %s\n", path);
    } else {
        DEBUG_PRINTF("SDCardHandler: ❌ Löschen fehlgeschlagen: %s\n", path);
    }
    
    return success;
}

bool SDCardHandler::fileExists(const char* path) {
    if (!mounted) return false;
    return SD.exists(path);
}

size_t SDCardHandler::getFileSize(const char* path) {
    if (!mounted) return 0;
    
    File file = SD.open(path, FILE_READ);
    if (!file) return 0;
    
    size_t size = file.size();
    file.close();
    
    return size;
}

void SDCardHandler::clearAllLogs() {
    if (!mounted) return;
    
    DEBUG_PRINTLN("SDCardHandler: Lösche alle Log-Dateien...");
    
    deleteFile(LOG_FILE_BOOT);
    deleteFile(LOG_FILE_BATTERY);
    deleteFile(LOG_FILE_CONNECTION);
    deleteFile(LOG_FILE_ERROR);
    
    DEBUG_PRINTLN("SDCardHandler: ✅ Logs gelöscht");
}

void SDCardHandler::flush() {
    if (!mounted) return;
    
    // SD.flush() - falls vorhanden in neueren Libs
    // Alternativ: Dateien explizit schließen/öffnen
    lastFlush = millis();
}

void SDCardHandler::printInfo() {
    DEBUG_PRINTLN("\n╔═══════════════════════════════════════════════╗");
    DEBUG_PRINTLN("║          SD CARD HANDLER INFO                 ║");
    DEBUG_PRINTLN("╚═══════════════════════════════════════════════╝");
    
    DEBUG_PRINTF("Status:     %s\n", mounted ? "✅ Mounted" : "❌ Not mounted");
    
    if (mounted) {
        uint8_t cardType = SD.cardType();
        
        DEBUG_PRINTF("Card Type:  %s\n", 
                     cardType == CARD_MMC ? "MMC" :
                     cardType == CARD_SD ? "SDSC" :
                     cardType == CARD_SDHC ? "SDHC" : "UNKNOWN");
        
        uint64_t total = getTotalSpace();
        uint64_t free = getFreeSpace();
        uint64_t used = total - free;
        
        DEBUG_PRINTF("Total:      %.2f GB\n", total / 1024.0 / 1024.0 / 1024.0);
        DEBUG_PRINTF("Used:       %.2f GB\n", used / 1024.0 / 1024.0 / 1024.0);
        DEBUG_PRINTF("Free:       %.2f GB (%.1f%%)\n", 
                     free / 1024.0 / 1024.0 / 1024.0,
                     (free * 100.0) / total);
        
        DEBUG_PRINTLN("\n─── Log Files ─────────────────────────────────");
        
        const char* logFiles[] = {
            LOG_FILE_BOOT,
            LOG_FILE_BATTERY,
            LOG_FILE_CONNECTION,
            LOG_FILE_ERROR
        };
        
        for (int i = 0; i < 4; i++) {
            if (fileExists(logFiles[i])) {
                size_t size = getFileSize(logFiles[i]);
                DEBUG_PRINTF("  %s: %.2f KB\n", logFiles[i], size / 1024.0);
            } else {
                DEBUG_PRINTF("  %s: [not exist]\n", logFiles[i]);
            }
        }
        
        // Config-Datei
        DEBUG_PRINTLN("\n─── Config File ───────────────────────────────");
        if (fileExists(CONFIG_FILE)) {
            size_t size = getFileSize(CONFIG_FILE);
            DEBUG_PRINTF("  %s: %.2f KB\n", CONFIG_FILE, size / 1024.0);
        } else {
            DEBUG_PRINTF("  %s: [not exist]\n", CONFIG_FILE);
        }
    }
    
    DEBUG_PRINTLN("═══════════════════════════════════════════════\n");
}

// ═══════════════════════════════════════════════════════════════════════════
// PRIVATE METHODEN
// ═══════════════════════════════════════════════════════════════════════════

bool SDCardHandler::writeJsonLog(const char* logFile, JsonDocument& doc) {
    if (!mounted) return false;
    
    // JSON serialisieren
    String jsonString;
    serializeJson(doc, jsonString);
    jsonString += "\n";  // Newline für Zeilen-basiertes Logging
    
    // Datei rotieren falls nötig
    rotateLogIfNeeded(logFile);
    
    // An Datei anhängen
    bool success = appendFile(logFile, jsonString.c_str());
    
    if (success) {
        checkAutoFlush();
    }
    
    return success;
}

String SDCardHandler::getTimestamp() {
    // Uptime in Millisekunden als Timestamp
    return String(millis());
}

void SDCardHandler::rotateLogIfNeeded(const char* path) {
    size_t fileSize = getFileSize(path);
    
    if (fileSize > LOG_MAX_FILE_SIZE) {
        DEBUG_PRINTF("SDCardHandler: Rotiere Log-Datei: %s (%.2f KB)\n", 
                     path, fileSize / 1024.0);
        
        // Backup-Name erstellen (z.B. boot.log -> boot.log.1)
        String backupPath = String(path) + ".1";
        
        // Altes Backup löschen
        if (fileExists(backupPath.c_str())) {
            deleteFile(backupPath.c_str());
        }
        
        // Aktuelle Datei zu Backup umbenennen
        SD.rename(path, backupPath.c_str());
        
        DEBUG_PRINTF("SDCardHandler: ✅ Log rotiert zu: %s\n", backupPath.c_str());
    }
}

void SDCardHandler::checkAutoFlush() {
    unsigned long now = millis();
    
    if (now - lastFlush >= LOG_FLUSH_INTERVAL) {
        flush();
    }
}