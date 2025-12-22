/**
 * ESP32-Remote-UI.ino
 * 
 * Fernsteuerung mit ESP-NOW und Multi-Page UI + SD-Card Logging + Config-System
 * + Serial Command Interface für Log-Zugriff
 * 
 * NEUE STRUKTUR:
 * - DisplayHandler: nur Hardware (kein UIManager mehr)
 * - PageManager: verwaltet UILayout + Pages + UIManager
 * - UILayout: zentrales Header/Footer/Content Layout
 * - Pages werden vom PageManager erstellt und verwaltet
 * - SerialCommandHandler: Log-Dateien über Serial auslesen
 */

#include "include/TouchManager.h"
#include "include/BatteryMonitor.h"
#include "include/SDCardHandler.h"
#include "include/LogHandler.h"
#include "include/JoystickHandler.h"
#include "include/UIManager.h"
#include "include/PageManager.h"
#include "include/ESPNowManager.h"
#include "include/UserConfig.h"
#include "include/userConf.h"
#include "include/Globals.h"
#include "include/SerialCommandHandler.h"

// UIManager (für Widget-Verwaltung)
UIManager* ui = nullptr;

// Serial Command Handler
SerialCommandHandler cmdHandler;

// Timing für Logging
unsigned long lastBatteryLog = 0;
unsigned long lastConnectionLog = 0;
unsigned long setupStartTime = 0;
unsigned long lastTouchUpdate = 0;
unsigned long lastLoopStart = 0;

void setup() {
    setupStartTime = millis();
    
    Serial.begin(115200);
    delay(2000);
    Serial.println("ok");
    
    DEBUG_PRINTLN("\n╔════════════════════════════════════════╗");
    DEBUG_PRINTLN("║   ESP32-S3 Remote Control Startup      ║");
    DEBUG_PRINTLN("║   NEW UI STRUCTURE WITH UILAYOUT       ║");
    DEBUG_PRINTLN("╚════════════════════════════════════════╝\n");
    
    // ═══════════════════════════════════════════════════════════════
    // SD-KARTE (optional - Logging & Config)
    // ═══════════════════════════════════════════════════════════════
    Serial.println("→ SD-Card...");
    bool sdAvailable = sdCard.begin();

    if (sdAvailable) {
        Serial.println("  ✅ SD-Card OK");
        logger.setSDHandler(&sdCard);
        logger.logBootStart("PowerOn", ESP.getFreeHeap(), FIRMWARE_VERSION);
    } else {
        Serial.println("  ⚠️ SD-Card N/A (using defaults)");
        logger.logBootStart("PowerOn", ESP.getFreeHeap(), FIRMWARE_VERSION);
    }
    
    // ═══════════════════════════════════════════════════════════════
    // CONFIG LADEN
    // ═══════════════════════════════════════════════════════════════
    Serial.println("→ Config...");
    
    userConfig.init("/config.json", &sdCard);
    
    if (sdAvailable) {
        if (userConfig.load()) {
            Serial.println("  ✅ Config geladen (SD-Card)");
        } else {
            Serial.println("  ⚠️ Config laden fehlgeschlagen - verwende Defaults");
            userConfig.reset();
        }
    } else {
        Serial.println("  ℹ️ Verwende Default-Config (keine SD-Card)");
        userConfig.reset();
    }
    
    userConfig.printInfo();
    
    // ═══════════════════════════════════════════════════════════════
    // GPIO
    // ═══════════════════════════════════════════════════════════════
    Serial.println("→ GPIO...");
    
    pinMode(TOUCH_CS, OUTPUT);
    digitalWrite(TOUCH_CS, HIGH);
    pinMode(TFT_BL, OUTPUT);
    
    Serial.println("  ✅ GPIO OK");
    logger.logBootStep("GPIO", true);
    
    // ═══════════════════════════════════════════════════════════════
    // Display (nur Hardware!)
    // ═══════════════════════════════════════════════════════════════
    Serial.println("→ Display...");
    if (!display.begin(&userConfig)) {
        Serial.println("  ❌ Display init failed!");
        logger.logBootStep("Display", false, "Init failed");
        logger.error("Display", "begin() failed", ERR_DISPLAY_INIT);
        while (1) delay(100);
    }
    
    display.setBacklight(userConfig.getBacklightDefault());
    display.setBacklightOn(true);
    
    Serial.println("  ✅ Display OK");
    logger.logBootStep("Display", true, "480x320 @ Rotation 3");
    
    // ═══════════════════════════════════════════════════════════════
    // Touch
    // ═══════════════════════════════════════════════════════════════
    Serial.println("→ Touch...");
    SPIClass* hspi = &display.getTft().getSPIinstance();

    if (touch.begin(hspi, &userConfig)) {
        Serial.println("  ✅ Touch OK");
        
        touch.setCalibration(
            userConfig.getTouchMinX(),
            userConfig.getTouchMaxX(), 
            userConfig.getTouchMinY(),
            userConfig.getTouchMaxY()
        );
        touch.setThreshold(userConfig.getTouchThreshold());
        touch.setRotation(userConfig.getTouchRotation());
        
        Serial.printf("  Calibration: X=%d-%d, Y=%d-%d, Threshold=%d, Rotation=%d\n",
                     userConfig.getTouchMinX(), userConfig.getTouchMaxX(),
                     userConfig.getTouchMinY(), userConfig.getTouchMaxY(),
                     userConfig.getTouchThreshold(), userConfig.getTouchRotation());
        
        logger.logBootStep("Touch", true, "XPT2046 calibrated");
    } else {
        Serial.println("  ⚠️ Touch N/A");
        logger.logBootStep("Touch", false, "XPT2046 not responding");
    }
    
    // ═══════════════════════════════════════════════════════════════
    // Battery
    // ═══════════════════════════════════════════════════════════════
    Serial.println("→ Battery...");
    if (battery.begin()) {
        Serial.println("  ✅ Battery OK");
        logger.logBootStep("Battery", true);
        logger.logBattery(battery.getVoltage(), battery.getPercent(), 
                         battery.isLow(), battery.isCritical());
    } else {
        logger.logBootStep("Battery", false, "Sensor error");
        logger.error("Battery", "begin() failed", ERR_BATTERY_INIT);
    }
    
    // ═══════════════════════════════════════════════════════════════
    // Joystick
    // ═══════════════════════════════════════════════════════════════
    Serial.println("→ Joystick...");
    
    if (joystick.begin()) {
        Serial.println("  ✅ Joystick OK");
        
        joystick.setDeadzone(userConfig.getJoyDeadzone());
        joystick.setUpdateInterval(userConfig.getJoyUpdateInterval());
        joystick.setInvertX(userConfig.getJoyInvertX());
        joystick.setInvertY(userConfig.getJoyInvertY());
        
        joystick.setCalibration(0, 
            userConfig.getJoyCalXMin(),
            userConfig.getJoyCalXCenter(),
            userConfig.getJoyCalXMax()
        );
        joystick.setCalibration(1,
            userConfig.getJoyCalYMin(),
            userConfig.getJoyCalYCenter(),
            userConfig.getJoyCalYMax()
        );
        
        joystick.printInfo();
        logger.logBootStep("Joystick", true);
    } else {
        Serial.println("  ❌ Joystick init failed!");
        logger.logBootStep("Joystick", false);
    }

    // ═══════════════════════════════════════════════════════════════
    // ESP-NOW
    // ═══════════════════════════════════════════════════════════════
    Serial.println("→ ESP-NOW...");

    if (espNow.begin(ESPNOW_CHANNEL)) {
        Serial.println("  ✅ ESP-NOW OK");
        Serial.printf("  MAC: %s\n", espNow.getOwnMacString().c_str());
        
        espNow.setHeartbeat(true, userConfig.getEspnowHeartbeat());
        espNow.setMaxPeers(ESPNOW_MAX_PEERS);
        espNow.setTimeout(userConfig.getEspnowTimeout());
        
        Serial.printf("  Heartbeat: %dms, Timeout: %dms\n",
                     userConfig.getEspnowHeartbeat(),
                     userConfig.getEspnowTimeout());
        
        logger.logBootStep("ESP-NOW", true, espNow.getOwnMacString().c_str());
        
        // Events für Logging registrieren
        espNow.onEvent(ESPNowEvent::PEER_CONNECTED, [](ESPNowEventData* data) {
            String mac = ESPNowManager::macToString(data->mac);
            logger.logConnection(mac.c_str(), "connected");
            Serial.printf("ESP-NOW: Peer %s connected\n", mac.c_str());
        });
        
        espNow.onEvent(ESPNowEvent::PEER_DISCONNECTED, [](ESPNowEventData* data) {
            String mac = ESPNowManager::macToString(data->mac);
            logger.logConnection(mac.c_str(), "disconnected");
            Serial.printf("ESP-NOW: Peer %s disconnected\n", mac.c_str());
        });
        
        espNow.onEvent(ESPNowEvent::HEARTBEAT_TIMEOUT, [](ESPNowEventData* data) {
            String mac = ESPNowManager::macToString(data->mac);
            logger.logConnection(mac.c_str(), "timeout");
            Serial.printf("ESP-NOW: Peer %s timeout\n", mac.c_str());
        });
    } else {
        logger.logBootStep("ESP-NOW", false, "WiFi init error");
        logger.error("ESP-NOW", "esp_now_init() failed", 3);
    }
    
    // ═══════════════════════════════════════════════════════════════
    // PowerManager
    // ═══════════════════════════════════════════════════════════════
    Serial.println("→ PowerManager...");
    if (powerMgr.begin(&battery, &display)) {
        Serial.println("  ✅ PowerManager OK");
        logger.logBootStep("PowerManager", true);
    } else {
        Serial.println("  ⚠️ PowerManager init failed");
        logger.logBootStep("PowerManager", false);
    }
    
    // ═══════════════════════════════════════════════════════════════
    // UIManager erstellen
    // ═══════════════════════════════════════════════════════════════
    Serial.println("→ UIManager...");
    TFT_eSPI* tft = &display.getTft();
    ui = new UIManager(tft, &touch);
    Serial.println("  ✅ UIManager erstellt");
    logger.logBootStep("UIManager", true);
    
    // ═══════════════════════════════════════════════════════════════
    // PageManager erstellen (mit UILayout) + Pages
    // ═══════════════════════════════════════════════════════════════
    Serial.println("→ PageManager...");
    pageManager = new PageManager(tft, ui);
    
    if (!pageManager->init(&battery, &powerMgr)) {
        Serial.println("  ❌ PageManager init failed!");
        logger.logBootStep("PageManager", false, "Init error");
        while (1) delay(100);
    }
    
    Serial.println("  ✅ PageManager OK (Pages erstellt und registriert)");
    logger.logBootStep("PageManager", true, "UILayout + 5 Pages");
    
    // ═══════════════════════════════════════════════════════════════
    // Serial Command Handler
    // ═══════════════════════════════════════════════════════════════
    Serial.println("→ Serial Command Handler...");
    cmdHandler.begin(&sdCard, &logger, &battery, &espNow, &userConfig);
    Serial.println("  ✅ Command Handler OK");
    logger.logBootStep("SerialCmdHandler", true);
    
    // ═══════════════════════════════════════════════════════════════
    // Home-Page anzeigen
    // ═══════════════════════════════════════════════════════════════
    Serial.println("→ Show HomePage...");
    pageManager->showPage(PAGE_HOME);
    Serial.println("  ✅ HomePage angezeigt");
    
    // ═══════════════════════════════════════════════════════════════
    // Setup Complete
    // ═══════════════════════════════════════════════════════════════
    unsigned long setupTime = millis() - setupStartTime;
    display.setBacklightOn(true);

    Serial.println();
    Serial.println("✅ Setup complete!");
    Serial.printf("   Setup-Zeit: %lu ms\n", setupTime);
    Serial.println("   Starte loop()...");
    Serial.println();
    
    logger.logBootComplete(setupTime, true);
}

void loop() {
    lastLoopStart = millis();
    
    // ═══════════════════════════════════════════════════════════════
    // Serial Command Handler (höchste Priorität)
    // ═══════════════════════════════════════════════════════════════
    cmdHandler.update();
    
    // ═══════════════════════════════════════════════════════════════
    // Battery Monitor
    // ═══════════════════════════════════════════════════════════════
    
    battery.update();
    
    static unsigned long lastBatteryUpdate = 0;
    if (lastLoopStart - lastBatteryUpdate > 2000) {
        // Battery-Icon über UILayout aktualisieren
        if (pageManager) {
            UILayout* layout = pageManager->getLayout();
            layout->updateBattery();
        }
        lastBatteryUpdate = lastLoopStart;
    }
    
    // Battery-Status loggen (alle 60 Sekunden)
    if (lastLoopStart - lastBatteryLog > 60000) {
        logger.logBattery(battery.getVoltage(), battery.getPercent(), 
                         battery.isLow(), battery.isCritical());
        lastBatteryLog = lastLoopStart;
    }
    
    // Battery Critical → Error-Log
    if (battery.isCritical()) {
        static bool criticalLogged = false;
        if (!criticalLogged) {
            logger.error("Battery", "Critical voltage!", ERR_BATTERY_CRITICAL);
            criticalLogged = true;
        }
    }
    
    // ═══════════════════════════════════════════════════════════════
    // Touch
    // ═══════════════════════════════════════════════════════════════

    if (touch.isAvailable() && lastLoopStart - lastTouchUpdate >= 100) {
        touch.updateIfIRQ();
        lastTouchUpdate = lastLoopStart;
    }
    
    // ═══════════════════════════════════════════════════════════════
    // Joystick auslesen und via ESP-NOW senden
    // ═══════════════════════════════════════════════════════════════

    if (joystick.update()) {
        int16_t joyX = joystick.getX();
        int16_t joyY = joystick.getY();
        
        // An RemoteControlPage senden (über PageManager - entkoppelt)
        pageManager->updateJoystick(joyX, joyY);
        
        // Via ESP-NOW senden
        if (espNow.isConnected()) {
            ESPNowPacket packet;
            packet.begin(MainCmd::DATA_REQUEST)
                  .addInt16(DataCmd::JOYSTICK_X, joyX)
                  .addInt16(DataCmd::JOYSTICK_Y, joyY);
            
            uint8_t peerMac[6];
            if (ESPNowManager::stringToMac(userConfig.getEspnowPeerMac(), peerMac)) {
                espNow.send(peerMac, packet);
            }
        }
    }

    // ═══════════════════════════════════════════════════════════════
    // ESP-NOW
    // ═══════════════════════════════════════════════════════════════
    espNow.update();
    
    // Connection-Stats loggen (alle 5 Minuten)
    if (lastLoopStart - lastConnectionLog > 300000) {
        if (espNow.isConnected()) {
            int rxPending = espNow.getQueuePending();
            String mac = espNow.getOwnMacString();
            logger.logConnectionStats(mac.c_str(), 0, 0, rxPending, -65);
        }
        lastConnectionLog = lastLoopStart;
    }
    
    // ═══════════════════════════════════════════════════════════════
    // UI Update & Draw
    // ═══════════════════════════════════════════════════════════════
    
    if (pageManager) {
        pageManager->update();  // Page-Update
        pageManager->draw();    // UI zeichnen (ruft ui->drawUpdates() auf)
    }
    
    delay(10);
}