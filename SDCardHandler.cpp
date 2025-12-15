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
    
    char buffer[256];
    snprintf(buffer, sizeof(buffer), 
             "[%s] BOOT: reason=%s, heap=%u, ver=%s, build=%s %s, chip=%s, cpu=%dMHz\n",
             getTimestamp().c_str(), reason, freeHeap, version,
             BUILD_DATE, BUILD_TIME, ESP.getChipModel(), ESP.getCpuFreqMHz());
    
    rotateLogIfNeeded(LOG_FILE_BOOT);
    bool success = appendFile(LOG_FILE_BOOT, buffer);
    
    if (success) checkAutoFlush();
    return success;
}

bool SDCardHandler::logSetupStep(const char* module, bool success, const char* message) {
    if (!mounted) return false;
    
    char buffer[256];
    if (message) {
        snprintf(buffer, sizeof(buffer), "[%s] SETUP: module=%s, status=%s, msg=%s\n",
                 getTimestamp().c_str(), module, success ? "OK" : "FAIL", message);
    } else {
        snprintf(buffer, sizeof(buffer), "[%s] SETUP: module=%s, status=%s\n",
                 getTimestamp().c_str(), module, success ? "OK" : "FAIL");
    }
    
    rotateLogIfNeeded(LOG_FILE_BOOT);
    bool result = appendFile(LOG_FILE_BOOT, buffer);
    if (result) checkAutoFlush();
    return result;
}

bool SDCardHandler::logBootComplete(uint32_t totalTimeMs, bool success) {
    if (!mounted) return false;
    
    char buffer[256];
    snprintf(buffer, sizeof(buffer), "[%s] BOOT_COMPLETE: time=%ums, status=%s, heap=%u\n",
             getTimestamp().c_str(), totalTimeMs, success ? "OK" : "FAIL", ESP.getFreeHeap());
    
    rotateLogIfNeeded(LOG_FILE_BOOT);
    bool result = appendFile(LOG_FILE_BOOT, buffer);
    if (result) checkAutoFlush();
    return result;
}

// ═══════════════════════════════════════════════════════════════════════════
// BATTERY LOG
// ═══════════════════════════════════════════════════════════════════════════

bool SDCardHandler::logBattery(float voltage, uint8_t percent, bool isLow, bool isCritical) {
    if (!mounted) return false;
    
    char buffer[128];
    snprintf(buffer, sizeof(buffer), "[%s] BAT: V=%.2f, %%=%u, low=%d, crit=%d\n",
             getTimestamp().c_str(), voltage, percent, isLow, isCritical);
    
    rotateLogIfNeeded(LOG_FILE_BATTERY);
    bool result = appendFile(LOG_FILE_BATTERY, buffer);
    if (result) checkAutoFlush();
    return result;
}

// ═══════════════════════════════════════════════════════════════════════════
// CONNECTION LOG
// ═══════════════════════════════════════════════════════════════════════════

bool SDCardHandler::logConnection(const char* peerMac, const char* event, int8_t rssi) {
    if (!mounted) return false;
    
    char buffer[128];
    if (rssi != 0) {
        snprintf(buffer, sizeof(buffer), "[%s] CONN: peer=%s, event=%s, rssi=%d\n",
                 getTimestamp().c_str(), peerMac, event, rssi);
    } else {
        snprintf(buffer, sizeof(buffer), "[%s] CONN: peer=%s, event=%s\n",
                 getTimestamp().c_str(), peerMac, event);
    }
    
    rotateLogIfNeeded(LOG_FILE_CONNECTION);
    bool result = appendFile(LOG_FILE_CONNECTION, buffer);
    if (result) checkAutoFlush();
    return result;
}

bool SDCardHandler::logConnectionStats(const char* peerMac, uint32_t packetsSent, 
                                        uint32_t packetsReceived, uint32_t packetsLost,
                                        uint16_t sendRate, uint16_t receiveRate, int8_t avgRssi) {
    if (!mounted) return false;
    
    float lossRate = 0.0f;
    if (packetsSent > 0) {
        lossRate = (packetsLost * 100.0f) / packetsSent;
    }
    
    char buffer[256];
    snprintf(buffer, sizeof(buffer), 
             "[%s] STATS: peer=%s, sent=%u, recv=%u, lost=%u, loss=%.2f%%, send_rate=%u, recv_rate=%u, rssi=%d\n",
             getTimestamp().c_str(), peerMac, packetsSent, packetsReceived, packetsLost, 
             lossRate, sendRate, receiveRate, avgRssi);
    
    rotateLogIfNeeded(LOG_FILE_CONNECTION);
    bool result = appendFile(LOG_FILE_CONNECTION, buffer);
    if (result) checkAutoFlush();
    return result;
}

// ═══════════════════════════════════════════════════════════════════════════
// ERROR LOG
// ═══════════════════════════════════════════════════════════════════════════

// ═══════════════════════════════════════════════════════════════════════════
// ERROR LOG
// ═══════════════════════════════════════════════════════════════════════════

bool SDCardHandler::logError(const char* module, int errorCode, const char* message, uint32_t freeHeap) {
    if (!mounted) return false;
    
    uint32_t heap = (freeHeap > 0) ? freeHeap : ESP.getFreeHeap();
    
    char buffer[256];
    snprintf(buffer, sizeof(buffer), "[%s] ERROR: module=%s, code=%d, msg=%s, heap=%u\n",
             getTimestamp().c_str(), module, errorCode, message, heap);
    
    rotateLogIfNeeded(LOG_FILE_ERROR);
    bool result = appendFile(LOG_FILE_ERROR, buffer);
    if (result) checkAutoFlush();
    return result;
}

bool SDCardHandler::logCrash(uint32_t pc, uint32_t excvaddr, uint32_t exccause, const char* stackTrace) {
    if (!mounted) return false;
    
    char buffer[512];
    if (stackTrace) {
        snprintf(buffer, sizeof(buffer), 
                 "[%s] CRASH: pc=0x%08X, excvaddr=0x%08X, cause=%u, heap=%u\n  Stack: %s\n",
                 getTimestamp().c_str(), pc, excvaddr, exccause, ESP.getFreeHeap(), stackTrace);
    } else {
        snprintf(buffer, sizeof(buffer), 
                 "[%s] CRASH: pc=0x%08X, excvaddr=0x%08X, cause=%u, heap=%u\n",
                 getTimestamp().c_str(), pc, excvaddr, exccause, ESP.getFreeHeap());
    }
    
    rotateLogIfNeeded(LOG_FILE_ERROR);
    bool result = appendFile(LOG_FILE_ERROR, buffer);
    if (result) checkAutoFlush();
    return result;
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