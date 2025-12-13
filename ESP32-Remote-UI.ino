/**
 * ESP32-Remote-UI.ino
 * 
 * Fernsteuerung mit ESP-NOW und Multi-Page UI + SD-Card Logging + Config-System
 */

#include "TouchManager.h"
#include "BatteryMonitor.h"
#include "SDCardHandler.h"
#include "JoystickHandler.h"
#include "GlobalUI.h"
#include "UIPageManager.h"
#include "ESPNowManager.h"
#include "UserConfig.h"
#include "Globals.h"
#include "Pages.cpp"

// GlobalUI (Header/Footer/Battery zentral)
GlobalUI globalUI;

// Timing für Logging
unsigned long lastBatteryLog = 0;
unsigned long lastConnectionLog = 0;
unsigned long setupStartTime = 0;

// Globale Seiten-Objekte
HomePage* homePage = nullptr;
RemoteControlPage* remotePage = nullptr;
ConnectionPage* connectionPage = nullptr;
SettingsPage* settingsPage = nullptr;
InfoPage* infoPage = nullptr;

void setup() {
    setupStartTime = millis();
    
    Serial.begin(115200);
    delay(2000);
    
    DEBUG_PRINTLN("\n╔════════════════════════════════════════╗");
    DEBUG_PRINTLN("║   ESP32-S3 Remote Control Startup      ║");
    DEBUG_PRINTLN("║   WITH CONFIG SYSTEM                   ║");
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
    
    // UserConfig initialisieren
    userConfig.init("/config.json", &sdCard);
    
    if (sdAvailable && userConfig.isStorageAvailable()) {
        // Config von SD-Card laden
        if (userConfig.load()) {
            Serial.println("  ✅ Config geladen (SD-Card)");
        } else {
            Serial.println("  ⚠️ Config laden fehlgeschlagen - verwende Defaults");
            userConfig.reset();  // Defaults laden
        }
    } else {
        // Keine SD-Card - Defaults verwenden
        Serial.println("  ℹ️ Verwende Default-Config (keine SD-Card)");
        userConfig.reset();
    }
    
    // Config-Info ausgeben
    userConfig.printInfo();
    
    // ═══════════════════════════════════════════════════════════════
    // GPIO
    // ═══════════════════════════════════════════════════════════════
    Serial.println("→ GPIO...");
    
    // Touch CS (muss HIGH sein, sonst stört es Display!)
    pinMode(TOUCH_CS, OUTPUT);
    digitalWrite(TOUCH_CS, HIGH);
    
    // Backlight
    pinMode(TFT_BL, OUTPUT);
    
    Serial.println("  ✅ GPIO OK");
    if (sdAvailable) sdCard.logSetupStep("GPIO", true);
    
    // ═══════════════════════════════════════════════════════════════
    // Display
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
    
    // Backlight aus Config
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
        
        // Kalibrierung aus Config
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
        
        display.enableUI(&touch);
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
        
        // Parameter aus Config
        joystick.setDeadzone(userConfig.getJoyDeadzone());
        joystick.setUpdateInterval(userConfig.getJoyUpdateInterval());
        joystick.setInvertX(userConfig.getJoyInvertX());
        joystick.setInvertY(userConfig.getJoyInvertY());
        
        // Kalibrierung aus Config
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
    EspNowManager& espnow = EspNowManager::getInstance();

    if (espnow.begin(ESPNOW_CHANNEL)) {
        Serial.println("  ✅ ESP-NOW OK");
        Serial.printf("  MAC: %s\n", espnow.getOwnMacString().c_str());
        
        // Parameter aus Config
        espnow.setHeartbeat(true, userConfig.getEspnowHeartbeat());
        espnow.setTimeout(userConfig.getEspnowTimeout());
        
        Serial.printf("  Heartbeat: %dms, Timeout: %dms\n",
                     userConfig.getEspnowHeartbeat(),
                     userConfig.getEspnowTimeout());
        
        if (sdAvailable) sdCard.logSetupStep("ESP-NOW", true, espnow.getOwnMacString().c_str());
        
        // Events für Logging registrieren
        if (sdAvailable) {
            espnow.onEvent(EspNowEvent::PEER_CONNECTED, [](EspNowEventData* data) {
                String mac = EspNowManager::macToString(data->mac);
                sdCard.logConnection(mac.c_str(), "connected");
                Serial.printf("ESP-NOW: Peer %s connected\n", mac.c_str());
            });
            
            espnow.onEvent(EspNowEvent::PEER_DISCONNECTED, [](EspNowEventData* data) {
                String mac = EspNowManager::macToString(data->mac);
                sdCard.logConnection(mac.c_str(), "disconnected");
                Serial.printf("ESP-NOW: Peer %s disconnected\n", mac.c_str());
            });
            
            espnow.onEvent(EspNowEvent::HEARTBEAT_TIMEOUT, [](EspNowEventData* data) {
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
    // GlobalUI
    // ═══════════════════════════════════════════════════════════════
    Serial.println("→ GlobalUI...");
    UIManager* ui = display.getUI();
    TFT_eSPI* tft = &display.getTft();

    if (!globalUI.init(ui, tft, &battery, &powerMgr)) {
        Serial.println("  ❌ GlobalUI failed!");
        if (sdAvailable) sdCard.logSetupStep("GlobalUI", false, "Init error");
        while (1) delay(100);
    }

    Serial.println("  ✅ GlobalUI OK");
    if (sdAvailable) sdCard.logSetupStep("GlobalUI", true);
    
    globalUI.setPageManager(&pageManager);
    
    // ═══════════════════════════════════════════════════════════════
    // Pages erstellen
    // ═══════════════════════════════════════════════════════════════
    Serial.println("→ Pages...");
    homePage = new HomePage(ui, tft);
    remotePage = new RemoteControlPage(ui, tft);
    connectionPage = new ConnectionPage(ui, tft);
    settingsPage = new SettingsPage(ui, tft);
    infoPage = new InfoPage(ui, tft);
    
    homePage->setGlobalUI(&globalUI);
    homePage->setPageManager(&pageManager);
    remotePage->setGlobalUI(&globalUI);
    remotePage->setPageManager(&pageManager);
    connectionPage->setGlobalUI(&globalUI);
    connectionPage->setPageManager(&pageManager);
    settingsPage->setGlobalUI(&globalUI);
    settingsPage->setPageManager(&pageManager);
    infoPage->setGlobalUI(&globalUI);
    infoPage->setPageManager(&pageManager);
    
    // Peer MAC aus Config an ConnectionPage übergeben
    connectionPage->setPeerMac(userConfig.getEspnowPeerMac());
    
    Serial.println("  ✅ Pages created");
    if (sdAvailable) sdCard.logSetupStep("UI-Pages", true, "5 pages created");
    
    // ═══════════════════════════════════════════════════════════════
    // Pages registrieren
    // ═══════════════════════════════════════════════════════════════
    Serial.println("→ Register Pages...");
    pageManager.addPage(homePage, PAGE_HOME);
    pageManager.addPage(remotePage, PAGE_REMOTE);
    pageManager.addPage(connectionPage, PAGE_CONNECTION);
    pageManager.addPage(settingsPage, PAGE_SETTINGS);
    pageManager.addPage(infoPage, PAGE_INFO);
    Serial.printf("  ✅ %d Pages registered\n", pageManager.getPageCount());
    
    // ═══════════════════════════════════════════════════════════════
    // Home-Page anzeigen
    // ═══════════════════════════════════════════════════════════════
    Serial.println("→ Show HomePage...");
    pageManager.showPage(PAGE_HOME);
    
    // ═══════════════════════════════════════════════════════════════
    // Setup Complete
    // ═══════════════════════════════════════════════════════════════
    unsigned long setupTime = millis() - setupStartTime;
    display.setBacklightOn(true);

    Serial.println();
    Serial.println("✅ Setup complete!");
    Serial.printf("   Setup-Zeit: %lu ms\n", setupTime);
    Serial.println();
    
    if (sdAvailable) sdCard.logBootComplete(setupTime, true);
    
    // Config dirty? -> Speichern
    if (userConfig.isDirty()) {
        Serial.println("Config geändert - speichere...");
        userConfig.save();
    }
}

void loop() {
    // ═══════════════════════════════════════════════════════════════
    // Battery Monitor
    // ═══════════════════════════════════════════════════════════════
    battery.update();
    
    static unsigned long lastBatteryUpdate = 0;
    if (millis() - lastBatteryUpdate > 2000) {
        globalUI.updateBatteryIcon();
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
        // Joystick-Werte haben sich geändert
        int16_t joyX = joystick.getX();  // -100 bis +100
        int16_t joyY = joystick.getY();  // -100 bis +100
        
        // An RemoteControlPage senden (für lokale Anzeige)
        if (remotePage) {
            remotePage->setJoystickPosition(joyX, joyY);
        }
        
        // Via ESP-NOW senden
        EspNowManager& espnow = EspNowManager::getInstance();
        if (espnow.isConnected()) {
            EspNowPacket packet;
            packet.begin(MainCmd::DATA_REQUEST)
                  .addInt16(DataCmd::JOYSTICK_X, joyX)
                  .addInt16(DataCmd::JOYSTICK_Y, joyY);
            
            // An konfigurierten Peer senden
            uint8_t peerMac[6];
            if (EspNowManager::stringToMac(userConfig.getEspnowPeerMac(), peerMac)) {
                espnow.send(peerMac, packet);
            }
        }
    }

    // ═══════════════════════════════════════════════════════════════
    // ESP-NOW
    // ═══════════════════════════════════════════════════════════════
    EspNowManager& espnow = EspNowManager::getInstance();
    espnow.update();
    
    // Connection-Stats loggen (alle 5 Minuten)
    if (sdCard.isAvailable() && (millis() - lastConnectionLog > 300000)) {
        if (espnow.isConnected()) {
            int rxPending, txPending, resultPending;
            espnow.getQueueStats(&rxPending, &txPending, &resultPending);
            
            String mac = espnow.getOwnMacString();
            sdCard.logConnectionStats(mac.c_str(), 0, 0, 0, 0, 0, -65);
        }
        lastConnectionLog = millis();
    }
    
    // ═══════════════════════════════════════════════════════════════
    // Display + UI
    // ═══════════════════════════════════════════════════════════════
    display.update();
    pageManager.update();
    
    delay(10);
}