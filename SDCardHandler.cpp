/**
 * SDCardHandler.cpp
 * 
 * Implementation des SD-Karten Handlers (nur File I/O + Logging)
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
// BOOT LOG
// ═══════════════════════════════════════════════════════════════════════════

bool SDCardHandler::logBootStart(const char* reason, uint32_t freeHeap, const char* version) {
    if (!mounted) return false;
    
    JsonDocument doc;
    
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
    
    JsonDocument doc;
    
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
    
    JsonDocument doc;
    
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
    
    JsonDocument doc;
    
    doc["timestamp"] = getTimestamp();
    doc["type"] = "battery";
    doc["voltage"] = serialized(String(voltage, 2));
    doc["percent"] = percent;
    doc["is_low"] = isLow;
    doc["is_critical"] = isCritical;
    
    return writeJsonLog(LOG_FILE_BATTERY, doc);
}

// ═══════════════════════════════════════════════════════════════════════════
// CONNECTION LOG
// ═══════════════════════════════════════════════════════════════════════════

bool SDCardHandler::logConnection(const char* peerMac, const char* event, int8_t rssi) {
    if (!mounted) return false;
    
    JsonDocument doc;
    
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
    
    JsonDocument doc;
    
    doc["timestamp"] = getTimestamp();
    doc["type"] = "connection_stats";
    doc["peer_mac"] = peerMac;
    doc["packets_sent"] = packetsSent;
    doc["packets_received"] = packetsReceived;
    doc["packets_lost"] = packetsLost;
    doc["send_rate"] = sendRate;
    doc["receive_rate"] = receiveRate;
    doc["avg_rssi"] = avgRssi;
    
    if (packetsSent > 0) {
        float lossRate = (packetsLost * 100.0f) / packetsSent;
        doc["loss_rate"] = serialized(String(lossRate, 2));
    }
    
    return writeJsonLog(LOG_FILE_CONNECTION, doc);
}

// ═══════════════════════════════════════════════════════════════════════════
// ERROR LOG
// ═══════════════════════════════════════════════════════════════════════════

bool SDCardHandler::logError(const char* module, int errorCode, const char* message, uint32_t freeHeap) {
    if (!mounted) return false;
    
    JsonDocument doc;
    
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
    
    JsonDocument doc;
    
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
    if (!mounted || !path || !data) return false;
    
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
    if (!mounted || !path || !data) return false;
    
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
    if (!mounted || !path || !buffer) return -1;
    
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

String SDCardHandler::readFileString(const char* path) {
    if (!mounted || !path) return "";
    
    File file = SD.open(path, FILE_READ);
    if (!file) {
        DEBUG_PRINTF("SDCardHandler: ❌ Kann Datei nicht lesen: %s\n", path);
        return "";
    }
    
    String content = file.readString();
    file.close();
    
    return content;
}

bool SDCardHandler::deleteFile(const char* path) {
    if (!mounted || !path) return false;
    
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
    if (!mounted || !path) return false;
    return SD.exists(path);
}

size_t SDCardHandler::getFileSize(const char* path) {
    if (!mounted || !path) return 0;
    
    File file = SD.open(path, FILE_READ);
    if (!file) return 0;
    
    size_t size = file.size();
    file.close();
    
    return size;
}

bool SDCardHandler::renameFile(const char* oldPath, const char* newPath) {
    if (!mounted || !oldPath || !newPath) return false;
    
    if (!SD.exists(oldPath)) {
        DEBUG_PRINTF("SDCardHandler: Datei existiert nicht: %s\n", oldPath);
        return false;
    }
    
    bool success = SD.rename(oldPath, newPath);
    
    if (success) {
        DEBUG_PRINTF("SDCardHandler: ✅ Datei umbenannt: %s → %s\n", oldPath, newPath);
    } else {
        DEBUG_PRINTF("SDCardHandler: ❌ Umbenennen fehlgeschlagen\n");
    }
    
    return success;
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
    }
    
    DEBUG_PRINTLN("═══════════════════════════════════════════════\n");
}

// ═══════════════════════════════════════════════════════════════════════════
// PRIVATE METHODEN
// ═══════════════════════════════════════════════════════════════════════════

bool SDCardHandler::writeJsonLog(const char* logFile, JsonDocument& doc) {
    if (!mounted) return false;
    
    String jsonString;
    serializeJson(doc, jsonString);
    jsonString += "\n";
    
    rotateLogIfNeeded(logFile);
    
    bool success = appendFile(logFile, jsonString.c_str());
    
    if (success) {
        checkAutoFlush();
    }
    
    return success;
}

String SDCardHandler::getTimestamp() {
    return String(millis());
}

void SDCardHandler::rotateLogIfNeeded(const char* path) {
    size_t fileSize = getFileSize(path);
    
    if (fileSize > LOG_MAX_FILE_SIZE) {
        DEBUG_PRINTF("SDCardHandler: Rotiere Log-Datei: %s (%.2f KB)\n", 
                     path, fileSize / 1024.0);
        
        String backupPath = String(path) + ".1";
        
        if (fileExists(backupPath.c_str())) {
            deleteFile(backupPath.c_str());
        }
        
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