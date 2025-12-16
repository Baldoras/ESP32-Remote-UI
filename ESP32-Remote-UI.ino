/**
 * ESP32-Remote-UI.ino
 * 
 * Fernsteuerung mit ESP-NOW und Multi-Page UI + SD-Card Logging + Config-System
 * 
 * NEUE STRUKTUR:
 * - DisplayHandler: nur Hardware (kein UIManager mehr)
 * - PageManager: verwaltet UILayout + Pages + UIManager
 * - UILayout: zentrales Header/Footer/Content Layout
 * - Pages werden vom PageManager erstellt und verwaltet
 */

#include "TouchManager.h"
#include "BatteryMonitor.h"
#include "SDCardHandler.h"
#include "JoystickHandler.h"
#include "UIManager.h"
#include "PageManager.h"
#include "ESPNowManager.h"
#include "UserConfig.h"
#include "Globals.h"

// UIManager (für Widget-Verwaltung)
UIManager* ui = nullptr;

// Timing für Logging
unsigned long lastBatteryLog = 0;
unsigned long lastConnectionLog = 0;
unsigned long setupStartTime = 0;

void setup() {
    setupStartTime = millis();
    
    Serial.begin(115200);
    delay(2000);
    
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
        sdCard.logBootStart("PowerOn", ESP.getFreeHeap(), FIRMWARE_VERSION);
    } else {
        Serial.println("  ⚠️ SD-Card N/A (using defaults)");
    }
    
    // ═══════════════════════════════════════════════════════════════
    // CONFIG LADEN
    // ═══════════════════════════════════════════════════════════════
    Serial.println("→ Config...");
    
    userConfig.init("/config.json", &sdCard);
    
    if (sdAvailable && userConfig.isStorageAvailable()) {
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
    if (sdAvailable) sdCard.logSetupStep("GPIO", true);
    
    // ═══════════════════════════════════════════════════════════════
    // Display (nur Hardware!)
    // ═══════════════════════════════════════════════════════════════
    Serial.println("→ Display...");
    if (!display.begin(&userConfig)) {
        Serial.println("  ❌ Display init failed!");
        if (sdAvailable) {
            sdCard.logSetupStep("Display", false, "Init failed");
            sdCard.logError("Display", ERR_DISPLAY_INIT, "begin() failed");
        }
        while (1) delay(100);
    }
    
    display.setBacklight(userConfig.getBacklightDefault());
    display.setBacklightOn(true);
    
    Serial.println("  ✅ Display OK");
    if (sdAvailable) sdCard.logSetupStep("Display", true, "480x320 @ Rotation 3");
    
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
        
        if (sdAvailable) sdCard.logSetupStep("Touch", true, "XPT2046 calibrated");
    } else {
        Serial.println("  ⚠️ Touch N/A");
        if (sdAvailable) sdCard.logSetupStep("Touch", false, "XPT2046 not responding");
    }
    
    // ═══════════════════════════════════════════════════════════════
    // Battery
    // ═══════════════════════════════════════════════════════════════
    Serial.println("→ Battery...");
    if (battery.begin()) {
        Serial.println("  ✅ Battery OK");
        if (sdAvailable) {
            sdCard.logSetupStep("Battery", true);
            sdCard.logBattery(battery.getVoltage(), battery.getPercent(), 
                             battery.isLow(), battery.isCritical());
        }
    } else {
        if (sdAvailable) {
            sdCard.logSetupStep("Battery", false, "Sensor error");
            sdCard.logError("Battery", ERR_BATTERY_INIT, "begin() failed");
        }
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
        if (sdAvailable) sdCard.logSetupStep("Joystick", true);
    } else {
        Serial.println("  ❌ Joystick init failed!");
        if (sdAvailable) sdCard.logSetupStep("Joystick", false);
    }

    // ═══════════════════════════════════════════════════════════════
    // ESP-NOW
    // ═══════════════════════════════════════════════════════════════
    Serial.println("→ ESP-NOW...");

    if (espNow.begin(userConfig.getEspnowChannel())) {
        Serial.println("  ✅ ESP-NOW OK");
        Serial.printf("  MAC: %s\n", espNow.getOwnMacString().c_str());
        
        espNow.setHeartbeat(true, userConfig.getEspnowHeartbeat());
        espNow.setMaxPeers(userConfig.getEspnowMaxPeers());
        espNow.setTimeout(userConfig.getEspnowTimeout());
        
        Serial.printf("  Heartbeat: %dms, Timeout: %dms\n",
                     userConfig.getEspnowHeartbeat(),
                     userConfig.getEspnowTimeout());
        
        if (sdAvailable) sdCard.logSetupStep("ESP-NOW", true, espNow.getOwnMacString().c_str());
        
        // Events für Logging registrieren
        if (sdAvailable) {
            espNow.onEvent(EspNowEvent::PEER_CONNECTED, [](EspNowEventData* data) {
                String mac = EspNowManager::macToString(data->mac);
                sdCard.logConnection(mac.c_str(), "connected");
                Serial.printf("ESP-NOW: Peer %s connected\n", mac.c_str());
            });
            
            espNow.onEvent(EspNowEvent::PEER_DISCONNECTED, [](EspNowEventData* data) {
                String mac = EspNowManager::macToString(data->mac);
                sdCard.logConnection(mac.c_str(), "disconnected");
                Serial.printf("ESP-NOW: Peer %s disconnected\n", mac.c_str());
            });
            
            espNow.onEvent(EspNowEvent::HEARTBEAT_TIMEOUT, [](EspNowEventData* data) {
                String mac = EspNowManager::macToString(data->mac);
                sdCard.logConnection(mac.c_str(), "timeout");
                Serial.printf("ESP-NOW: Peer %s timeout\n", mac.c_str());
            });
        }
    } else {
        if (sdAvailable) {
            sdCard.logSetupStep("ESP-NOW", false, "WiFi init error");
            sdCard.logError("ESP-NOW", 3, "esp_now_init() failed");
        }
    }
    
    // ═══════════════════════════════════════════════════════════════
    // PowerManager
    // ═══════════════════════════════════════════════════════════════
    Serial.println("→ PowerManager...");
    if (powerMgr.begin(&battery, &display)) {
        Serial.println("  ✅ PowerManager OK");
        if (sdAvailable) sdCard.logSetupStep("PowerManager", true);
    } else {
        Serial.println("  ⚠️ PowerManager init failed");
        if (sdAvailable) sdCard.logSetupStep("PowerManager", false);
    }
    
    // ═══════════════════════════════════════════════════════════════
    // UIManager erstellen
    // ═══════════════════════════════════════════════════════════════
    Serial.println("→ UIManager...");
    TFT_eSPI* tft = &display.getTft();
    ui = new UIManager(tft, &touch);
    Serial.println("  ✅ UIManager erstellt");
    if (sdAvailable) sdCard.logSetupStep("UIManager", true);
    
    // ═══════════════════════════════════════════════════════════════
    // PageManager erstellen (mit UILayout) + Pages
    // ═══════════════════════════════════════════════════════════════
    Serial.println("→ PageManager...");
    pageManager = new PageManager(tft, ui);
    
    if (!pageManager->init(&battery, &powerMgr)) {
        Serial.println("  ❌ PageManager init failed!");
        if (sdAvailable) sdCard.logSetupStep("PageManager", false, "Init error");
        while (1) delay(100);
    }
    
    Serial.println("  ✅ PageManager OK (Pages erstellt und registriert)");
    if (sdAvailable) sdCard.logSetupStep("PageManager", true, "UILayout + 5 Pages");
    
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
    
    if (sdAvailable) sdCard.logBootComplete(setupTime, true);
    
    if (userConfig.isDirty()) {
        Serial.println("Config geändert - speichere...");
        userConfig.save();
    }
}

void loop() {
    static bool firstLoop = true;
    if (firstLoop) {
        Serial.println("\n━━━━━━━ LOOP START ━━━━━━━");
        firstLoop = false;
    }
    
    // ═══════════════════════════════════════════════════════════════
    // Battery Monitor
    // ═══════════════════════════════════════════════════════════════
    
    battery.update();
    
    static unsigned long lastBatteryUpdate = 0;
    if (millis() - lastBatteryUpdate > 2000) {
        // Battery-Icon über UILayout aktualisieren
        if (pageManager) {
            UILayout* layout = pageManager->getLayout();
            layout->updateBattery();
        }
        lastBatteryUpdate = millis();
    }
    
    // Battery-Status loggen (alle 60 Sekunden)
    if (sdCard.isAvailable() && (millis() - lastBatteryLog > 60000)) {
        sdCard.logBattery(battery.getVoltage(), battery.getPercent(), 
                         battery.isLow(), battery.isCritical());
        lastBatteryLog = millis();
    }
    
    // Battery Critical → Error-Log
    if (battery.isCritical()) {
        static bool criticalLogged = false;
        if (!criticalLogged) {
            sdCard.logError("Battery", ERR_BATTERY_CRITICAL, 
                          "Critical voltage!", ESP.getFreeHeap());
            criticalLogged = true;
        }
    }
    
    // ═══════════════════════════════════════════════════════════════
    // Touch
    // ═══════════════════════════════════════════════════════════════

    if (touch.isAvailable()) {
        touch.update();
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
            EspNowPacket packet;
            packet.begin(MainCmd::DATA_REQUEST)
                  .addInt16(DataCmd::JOYSTICK_X, joyX)
                  .addInt16(DataCmd::JOYSTICK_Y, joyY);
            
            uint8_t peerMac[6];
            if (EspNowManager::stringToMac(userConfig.getEspnowPeerMac(), peerMac)) {
                espNow.send(peerMac, packet);
            }
        }
    }

    // ═══════════════════════════════════════════════════════════════
    // ESP-NOW
    // ═══════════════════════════════════════════════════════════════
    espNow.update();
    
    // Connection-Stats loggen (alle 5 Minuten)
    if (sdCard.isAvailable() && (millis() - lastConnectionLog > 300000)) {
        if (espNow.isConnected()) {
            int rxPending, txPending, resultPending;
            espNow.getQueueStats(&rxPending, &txPending, &resultPending);
            
            String mac = espNow.getOwnMacString();
            sdCard.logConnectionStats(mac.c_str(), 0, 0, 0, 0, 0, -65);
        }
        lastConnectionLog = millis();
    }
    
    // ═══════════════════════════════════════════════════════════════
    // UI Update & Draw
    // ═══════════════════════════════════════════════════════════════
    
    
    if (pageManager) {
        pageManager->update();  // Page-Update
        pageManager->draw();    // UI zeichnen (ruft ui->drawUpdates() auf)
    }
    
    //Serial.println("[LOOP] Loop-Ende\n");
    delay(10);
}