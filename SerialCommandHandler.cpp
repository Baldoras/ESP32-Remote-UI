/**
 * SerialCommandHandler.cpp
 */

#include "include/SerialCommandHandler.h"
#include "include/setupConf.h"

SerialCommandHandler::SerialCommandHandler() 
    : sdHandler(nullptr), logger(nullptr), battery(nullptr), 
      espNow(nullptr), config(nullptr) {
}

void SerialCommandHandler::begin(SDCardHandler* sdHandler, 
                                 LogHandler* logger,
                                 BatteryMonitor* battery,
                                 ESPNowManager* espNow,
                                 UserConfig* config) {
    this->sdHandler = sdHandler;
    this->logger = logger;
    this->battery = battery;
    this->espNow = espNow;
    this->config = config;
    
    Serial.println("\n╔═══════════════════════════════════════════╗");
    Serial.println("║  Serial Command Interface bereit          ║");
    Serial.println("║  Tippe 'help' für Befehlsliste            ║");
    Serial.println("╚═══════════════════════════════════════════╝\n");
}

void SerialCommandHandler::update() {
    while (Serial.available() > 0) {
        char c = Serial.read();
        
        if (c == '\n' || c == '\r') {
            if (inputBuffer.length() > 0) {
                processCommand(inputBuffer);
                inputBuffer = "";
                Serial.print("\n> ");  // Prompt
            }
        } else if (c == 127 || c == 8) {  // Backspace
            if (inputBuffer.length() > 0) {
                inputBuffer.remove(inputBuffer.length() - 1);
                Serial.print("\b \b");
            }
        } else if (c >= 32 && c < 127) {  // Druckbare Zeichen
            inputBuffer += c;
            Serial.print(c);  // Echo
        }
    }
}

void SerialCommandHandler::processCommand(const String& cmd) {
    String trimmedCmd = cmd;
    trimmedCmd.trim();
    
    if (trimmedCmd.length() == 0) return;
    
    Serial.println();  // Neue Zeile nach Command
    
    // Command und Argumente trennen
    int spaceIndex = trimmedCmd.indexOf(' ');
    String command = (spaceIndex > 0) ? trimmedCmd.substring(0, spaceIndex) : trimmedCmd;
    String args = (spaceIndex > 0) ? trimmedCmd.substring(spaceIndex + 1) : "";
    
    command.toLowerCase();
    args.trim();
    
    // ═══════════════════════════════════════════════════════════════
    // COMMAND ROUTING
    // ═══════════════════════════════════════════════════════════════
    
    if (command == "help" || command == "?") {
        handleHelp();
    }
    else if (command == "logs") {
        handleLogs();
    }
    else if (command == "read") {
        if (args.length() == 0) {
            Serial.println("❌ Fehler: Dateiname fehlt");
            Serial.println("   Verwendung: read <filename>");
        } else {
            handleRead(args);
        }
    }
    else if (command == "tail") {
        int spaceIdx = args.indexOf(' ');
        if (spaceIdx < 0) {
            Serial.println("❌ Fehler: Anzahl Zeilen fehlt");
            Serial.println("   Verwendung: tail <filename> <anzahl>");
        } else {
            String filename = args.substring(0, spaceIdx);
            int lines = args.substring(spaceIdx + 1).toInt();
            if (lines <= 0) lines = 10;
            handleTail(filename, lines);
        }
    }
    else if (command == "head") {
        int spaceIdx = args.indexOf(' ');
        if (spaceIdx < 0) {
            Serial.println("❌ Fehler: Anzahl Zeilen fehlt");
            Serial.println("   Verwendung: head <filename> <anzahl>");
        } else {
            String filename = args.substring(0, spaceIdx);
            int lines = args.substring(spaceIdx + 1).toInt();
            if (lines <= 0) lines = 10;
            handleHead(filename, lines);
        }
    }
    else if (command == "clear") {
        if (args.length() == 0) {
            Serial.println("❌ Fehler: Dateiname fehlt");
            Serial.println("   Verwendung: clear <filename>");
        } else {
            handleClear(args);
        }
    }
    else if (command == "clearall") {
        handleClearAll();
    }
    else if (command == "sysinfo") {
        handleSysInfo();
    }
    else if (command == "config") {
        handleConfig();
    }
    else if (command == "battery") {
        handleBattery();
    }
    else if (command == "espnow") {
        handleESPNow();
    }
    else {
        Serial.printf("❌ Unbekannter Befehl: '%s'\n", command.c_str());
        Serial.println("   Tippe 'help' für Befehlsliste");
    }
}

// ═══════════════════════════════════════════════════════════════════
// COMMAND HANDLER
// ═══════════════════════════════════════════════════════════════════

void SerialCommandHandler::handleHelp() {
    printHeader("Verfügbare Befehle");
    
    Serial.println("📋 LOG-BEFEHLE:");
    Serial.println("  logs                  - Alle Log-Dateien auflisten");
    Serial.println("  read <file>           - Log-Datei komplett lesen");
    Serial.println("  tail <file> <n>       - Letzte N Zeilen anzeigen");
    Serial.println("  head <file> <n>       - Erste N Zeilen anzeigen");
    Serial.println("  clear <file>          - Log-Datei löschen");
    Serial.println("  clearall              - Alle Log-Dateien löschen");
    Serial.println();
    Serial.println("ℹ️  SYSTEM-BEFEHLE:");
    Serial.println("  sysinfo               - System-Informationen");
    Serial.println("  config                - Konfiguration anzeigen");
    Serial.println("  battery               - Battery-Status");
    Serial.println("  espnow                - ESP-NOW Status");
    Serial.println();
    Serial.println("❓ HILFE:");
    Serial.println("  help                  - Diese Hilfe anzeigen");
    
    printSeparator();
}

void SerialCommandHandler::handleLogs() {
    if (!sdHandler || !sdHandler->isAvailable()) {
        Serial.println("❌ SD-Karte nicht verfügbar");
        return;
    }
    
    printHeader("Log-Dateien");
    
    listDirectory(LOG_DIR);
    
    printSeparator();
}

void SerialCommandHandler::handleRead(const String& filename) {
    if (!sdHandler || !sdHandler->isAvailable()) {
        Serial.println("❌ SD-Karte nicht verfügbar");
        return;
    }
    
    String filepath = String(LOG_DIR) + "/" + filename;
    
    if (!sdHandler->fileExists(filepath.c_str())) {
        Serial.printf("❌ Datei nicht gefunden: %s\n", filepath.c_str());
        return;
    }
    
    printHeader(filename.c_str());
    
    readLogFile(filepath.c_str());
    
    printSeparator();
}

void SerialCommandHandler::handleTail(const String& filename, int lines) {
    if (!sdHandler || !sdHandler->isAvailable()) {
        Serial.println("❌ SD-Karte nicht verfügbar");
        return;
    }
    
    String filepath = String(LOG_DIR) + "/" + filename;
    
    if (!sdHandler->fileExists(filepath.c_str())) {
        Serial.printf("❌ Datei nicht gefunden: %s\n", filepath.c_str());
        return;
    }
    
    char headerBuf[64];
    snprintf(headerBuf, sizeof(headerBuf), "%s (letzte %d Zeilen)", filename.c_str(), lines);
    printHeader(headerBuf);
    
    readLogFileTail(filepath.c_str(), lines);
    
    printSeparator();
}

void SerialCommandHandler::handleHead(const String& filename, int lines) {
    if (!sdHandler || !sdHandler->isAvailable()) {
        Serial.println("❌ SD-Karte nicht verfügbar");
        return;
    }
    
    String filepath = String(LOG_DIR) + "/" + filename;
    
    if (!sdHandler->fileExists(filepath.c_str())) {
        Serial.printf("❌ Datei nicht gefunden: %s\n", filepath.c_str());
        return;
    }
    
    char headerBuf[64];
    snprintf(headerBuf, sizeof(headerBuf), "%s (erste %d Zeilen)", filename.c_str(), lines);
    printHeader(headerBuf);
    
    readLogFileHead(filepath.c_str(), lines);
    
    printSeparator();
}

void SerialCommandHandler::handleClear(const String& filename) {
    if (!sdHandler || !sdHandler->isAvailable()) {
        Serial.println("❌ SD-Karte nicht verfügbar");
        return;
    }
    
    String filepath = String(LOG_DIR) + "/" + filename;
    
    if (!sdHandler->fileExists(filepath.c_str())) {
        Serial.printf("❌ Datei nicht gefunden: %s\n", filepath.c_str());
        return;
    }
    
    Serial.printf("⚠️  Datei '%s' wirklich löschen? (j/n): ", filename.c_str());
    
    // Warte auf Bestätigung
    unsigned long timeout = millis() + 10000;  // 10 Sekunden
    while (millis() < timeout) {
        if (Serial.available()) {
            char c = Serial.read();
            Serial.println(c);
            
            if (c == 'j' || c == 'J' || c == 'y' || c == 'Y') {
                if (sdHandler->deleteFile(filepath.c_str())) {
                    Serial.printf("✅ Datei '%s' gelöscht\n", filename.c_str());
                } else {
                    Serial.printf("❌ Fehler beim Löschen von '%s'\n", filename.c_str());
                }
                return;
            } else if (c == 'n' || c == 'N') {
                Serial.println("❌ Abgebrochen");
                return;
            }
        }
        delay(10);
    }
    
    Serial.println("❌ Timeout - Abgebrochen");
}

void SerialCommandHandler::handleClearAll() {
    if (!sdHandler || !sdHandler->isAvailable()) {
        Serial.println("❌ SD-Karte nicht verfügbar");
        return;
    }
    
    Serial.print("⚠️  ALLE Log-Dateien löschen? (j/n): ");
    
    // Warte auf Bestätigung
    unsigned long timeout = millis() + 10000;
    while (millis() < timeout) {
        if (Serial.available()) {
            char c = Serial.read();
            Serial.println(c);
            
            if (c == 'j' || c == 'J' || c == 'y' || c == 'Y') {
                if (logger) {
                    logger->clearAllLogs();
                    Serial.println("✅ Alle Log-Dateien gelöscht");
                } else {
                    Serial.println("❌ LogHandler nicht verfügbar");
                }
                return;
            } else if (c == 'n' || c == 'N') {
                Serial.println("❌ Abgebrochen");
                return;
            }
        }
        delay(10);
    }
    
    Serial.println("❌ Timeout - Abgebrochen");
}

void SerialCommandHandler::handleSysInfo() {
    printHeader("System-Informationen");
    
    Serial.printf("Firmware:      %s\n", FIRMWARE_VERSION);
    Serial.printf("Chip:          ESP32-S3\n");
    Serial.printf("CPU Freq:      %d MHz\n", ESP.getCpuFreqMHz());
    Serial.printf("Flash Size:    %d MB\n", ESP.getFlashChipSize() / (1024 * 1024));
    Serial.printf("PSRAM:         %d KB\n", ESP.getPsramSize() / 1024);
    Serial.printf("Free Heap:     %d KB\n", ESP.getFreeHeap() / 1024);
    Serial.printf("Free PSRAM:    %d KB\n", ESP.getFreePsram() / 1024);
    Serial.printf("Uptime:        %lu s\n", millis() / 1000);
    
    if (sdHandler && sdHandler->isAvailable()) {
        Serial.println();
        Serial.printf("SD Total:      %.2f MB\n", sdHandler->getTotalSpace() / (1024.0 * 1024.0));
        Serial.printf("SD Used:       %.2f MB\n", sdHandler->getUsedSpace() / (1024.0 * 1024.0));
        Serial.printf("SD Free:       %.2f MB\n", sdHandler->getFreeSpace() / (1024.0 * 1024.0));
    }
    
    printSeparator();
}

void SerialCommandHandler::handleConfig() {
    if (!config) {
        Serial.println("❌ UserConfig nicht verfügbar");
        return;
    }
    
    printHeader("Konfiguration");
    
    config->printInfo();
    
    printSeparator();
}

void SerialCommandHandler::handleBattery() {
    if (!battery) {
        Serial.println("❌ BatteryMonitor nicht verfügbar");
        return;
    }
    
    printHeader("Battery Status");
    
    Serial.printf("Voltage:       %.2f V\n", battery->getVoltage());
    Serial.printf("Percent:       %d %%\n", battery->getPercent());
    Serial.printf("Low:           %s\n", battery->isLow() ? "JA" : "NEIN");
    Serial.printf("Critical:      %s\n", battery->isCritical() ? "JA" : "NEIN");
    //Serial.printf("Charging:      %s\n", battery->isCharging() ? "JA" : "NEIN");
    
    printSeparator();
}

void SerialCommandHandler::handleESPNow() {
    if (!espNow) {
        Serial.println("❌ ESPNowManager nicht verfügbar");
        return;
    }
    
    printHeader("ESP-NOW Status");
    
    Serial.printf("Own MAC:       %s\n", espNow->getOwnMacString().c_str());
    Serial.printf("Connected:     %s\n", espNow->isConnected() ? "JA" : "NEIN");
    
    int rxPending, txPending, resultPending;
    espNow->getQueueStats(&rxPending, &txPending, &resultPending);
    
    Serial.println();
    Serial.printf("RX Queue:      %d\n", rxPending);
    Serial.printf("TX Queue:      %d\n", txPending);
    Serial.printf("Result Queue:  %d\n", resultPending);
    
    printSeparator();
}

// ═══════════════════════════════════════════════════════════════════
// HILFSFUNKTIONEN
// ═══════════════════════════════════════════════════════════════════

void SerialCommandHandler::listDirectory(const char* dirname) {
    int fileCount = 0;
    uint64_t totalSize = 0;
    
    sdHandler->listDir(dirname, [](const char* name, bool isDir, size_t size) {
        static int count = 0;
        static uint64_t total = 0;
        
        if (count == 0) {
            Serial.println("Dateiname                      Größe");
            Serial.println("───────────────────────────────────────────");
        }
        
        if (!isDir) {
            Serial.printf("%-30s %6zu B\n", name, size);
            count++;
            total += size;
        }
    });
    
    Serial.println("───────────────────────────────────────────");
    Serial.printf("Gesamt: %d Dateien\n", fileCount);
}

void SerialCommandHandler::readLogFile(const char* filepath) {
    File file = SD.open(filepath, FILE_READ);
    
    if (!file) {
        Serial.printf("❌ Fehler beim Öffnen von '%s'\n", filepath);
        return;
    }
    
    int lineCount = 0;
    
    while (file.available()) {
        String line = file.readStringUntil('\n');
        Serial.println(line);
        lineCount++;
        
        // Alle 50 Zeilen kleine Pause für Serial Buffer
        if (lineCount % 50 == 0) {
            delay(10);
        }
    }
    
    file.close();
    
    Serial.println();
    Serial.printf("Zeilen: %d\n", lineCount);
}

void SerialCommandHandler::readLogFileTail(const char* filepath, int lines) {
    File file = SD.open(filepath, FILE_READ);
    
    if (!file) {
        Serial.printf("❌ Fehler beim Öffnen von '%s'\n", filepath);
        return;
    }
    
    // Zuerst alle Zeilen zählen
    int totalLines = 0;
    while (file.available()) {
        file.readStringUntil('\n');
        totalLines++;
    }
    
    // Zurück zum Anfang
    file.seek(0);
    
    // Überspringe erste (totalLines - lines) Zeilen
    int skipLines = (totalLines > lines) ? (totalLines - lines) : 0;
    
    for (int i = 0; i < skipLines; i++) {
        file.readStringUntil('\n');
    }
    
    // Restliche Zeilen ausgeben
    while (file.available()) {
        String line = file.readStringUntil('\n');
        Serial.println(line);
    }
    
    file.close();
}

void SerialCommandHandler::readLogFileHead(const char* filepath, int lines) {
    File file = SD.open(filepath, FILE_READ);
    
    if (!file) {
        Serial.printf("❌ Fehler beim Öffnen von '%s'\n", filepath);
        return;
    }
    
    int count = 0;
    
    while (file.available() && count < lines) {
        String line = file.readStringUntil('\n');
        Serial.println(line);
        count++;
    }
    
    file.close();
}

void SerialCommandHandler::printSeparator() {
    Serial.println("═══════════════════════════════════════════════════════");
}

void SerialCommandHandler::printHeader(const char* title) {
    printSeparator();
    Serial.printf("  %s\n", title);
    printSeparator();
}