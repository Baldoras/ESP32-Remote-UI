/**
 * ESP32-Remote-UI.ino
 * 
 * Fernsteuerung mit ESP-NOW und Multi-Page UI + SD-Card Logging + PowerManager
 * 
 */

#include "Globals.h"
#include "TouchManager.h"
#include "BatteryMonitor.h"
#include "SDCardHandler.h"
#include "JoystickHandler.h"
#include "PowerManager.h"
#include "GlobalUI.h"
#include "UIPageManager.h"
#include "ESPNowManager.h"
#include "Pages.cpp"

// Hardware
DisplayHandler display;
TouchManager touch;
BatteryMonitor battery;
SDCardHandler sdCard;
JoystickHandler joystick;
PowerManager powerMgr;

// Config (aus SD-Karte geladen)
SDConfig config;

// GlobalUI (Header/Footer/Battery zentral)
GlobalUI globalUI;

// Page Manager
UIPageManager pageManager;

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
    DEBUG_PRINTLN("║   WITH SD-CARD LOGGING + POWERMANAGER  ║");
    DEBUG_PRINTLN("╚════════════════════════════════════════╝\n");
    
    // ═══════════════════════════════════════════════════════════════
    // SD-KARTE (optional - Logging & Config)
    // ═══════════════════════════════════════════════════════════════
    Serial.println("→ SD-Card...");
    bool sdAvailable = sdCard.begin();

    if (sdAvailable) {
        Serial.println("  SD-Card OK");
        sdCard.logBootStart("PowerOn", ESP.getFreeHeap(), FIRMWARE_VERSION);
    } else {
        Serial.println("  SD-Card N/A (using defaults)");
    }
    
    // ═══════════════════════════════════════════════════════════════
    // CONFIG LADEN (Defaults aus config.h, optional Override aus SD)
    // ═══════════════════════════════════════════════════════════════
    
    // 1. Defaults aus config.h laden
    config.backlightDefault = BACKLIGHT_DEFAULT;
    config.touchMinX = TOUCH_MIN_X;
    config.touchMaxX = TOUCH_MAX_X;
    config.touchMinY = TOUCH_MIN_Y;
    config.touchMaxY = TOUCH_MAX_Y;
    config.touchThreshold = TOUCH_THRESHOLD;
    config.espnowHeartbeatInterval = ESPNOW_HEARTBEAT_INTERVAL;
    config.espnowTimeout = ESPNOW_TIMEOUT_MS;
    config.debugSerialEnabled = DEBUG_SERIAL;
    
    Serial.println("  Config defaults loaded (config.h)");
    
    // 2. Optional: Wenn SD-Karte verfügbar → Override aus config.conf
    if (sdCard.isAvailable()) {
        if (sdCard.loadConfig(config)) {
            Serial.println("  Config override loaded (config.conf)");
        } else {
            Serial.println("  No config.conf, using defaults");
        }
    } else {
        Serial.println("  No SD-Card, using defaults");
    }
    
    // ═══════════════════════════════════════════════════════════════
    // GPIO
    // ═══════════════════════════════════════════════════════════════
    Serial.println("→ GPIO...");
    
    // Touch CS (muss HIGH sein, sonst stört es Display!)
    pinMode(TOUCH_CS, OUTPUT);
    digitalWrite(TOUCH_CS, HIGH);
    
    // Backlight
    pinMode(TFT_BL, OUTPUT);
    
    Serial.println("  GPIO OK");
    sdCard.logSetupStep("GPIO", true);
    
    // ═══════════════════════════════════════════════════════════════
    // Display
    // ═══════════════════════════════════════════════════════════════
    Serial.println("→ Display...");
    if (!display.begin()) {
        sdCard.logSetupStep("Display", false, "Init failed");
        sdCard.logError("Display", ERR_DISPLAY_INIT, "begin() failed");
        while (1) delay(100);
    }
    display.setBacklight(BACKLIGHT_DEFAULT);  // Config
    display.setBacklightOn(true);
    Serial.println("  Display OK");
    sdCard.logSetupStep("Display", true, "480x320 @ Rotation 3");
    
    // ═══════════════════════════════════════════════════════════════
    // Touch
    // ═══════════════════════════════════════════════════════════════
    Serial.println("→ Touch...");
    SPIClass* hspi = &display.getTft().getSPIinstance();

    if (touch.begin(hspi)) {
        Serial.println("  Touch OK");
        
        // Kalibrierung aus Config
        touch.setCalibration(config.touchMinX, config.touchMaxX, 
                            config.touchMinY, config.touchMaxY);
        touch.setThreshold(config.touchThreshold);
        
        Serial.printf("  Calibration: X=%d-%d, Y=%d-%d, Threshold=%d\n",
                     config.touchMinX, config.touchMaxX,
                     config.touchMinY, config.touchMaxY,
                     config.touchThreshold);
        
        display.enableUI(&touch);
        sdCard.logSetupStep("Touch", true, "XPT2046 calibrated");
    } else {
        Serial.println("  Touch N/A");
        sdCard.logSetupStep("Touch", false, "XPT2046 not responding");
    }
    
    // ═══════════════════════════════════════════════════════════════
    // Battery
    // ═══════════════════════════════════════════════════════════════
    Serial.println("→ Battery...");
    if (battery.begin()) {
        Serial.println("  Battery OK");
        sdCard.logSetupStep("Battery", true);
        
        // Initial-Status loggen
        sdCard.logBattery(battery.getVoltage(), battery.getPercent(), 
                         battery.isLow(), battery.isCritical());
    } else {
        sdCard.logSetupStep("Battery", false, "Sensor error");
        sdCard.logError("Battery", ERR_BATTERY_INIT, "begin() failed");
    }
    
    // ═══════════════════════════════════════════════════════════════
    // Joystick initialisieren (nach Battery Monitor, vor ESP-NOW)
    // ═══════════════════════════════════════════════════════════════
    
    if (joystick.begin()) {
        joystick.setDeadzone(10);  // 10% Deadzone
        joystick.setUpdateInterval(50);  // 50ms = 20Hz
        joystick.printInfo();
    } else {
        Serial.println("  Joystick init failed!");
        sdCard.logSetupStep("Joystick", false, "Init failed");
    }

    // ═══════════════════════════════════════════════════════════════
    // PowerManager
    // ═══════════════════════════════════════════════════════════════
    Serial.println("→ PowerManager...");
    if (powerMgr.begin(&battery, &display)) {
        Serial.println("  PowerManager OK");
        
        // Auto-Sleep bei kritischer Batterie aktivieren (Wake via Touch)
        powerMgr.setAutoSleepOnCritical(true, WakeSource::TOUCH);
        
        // Before-Sleep Callback: Ressourcen aufräumen
        powerMgr.setBeforeSleepCallback([]() {
            DEBUG_PRINTLN("PowerManager: Cleanup before sleep...");
            
            // Pages dekonstruieren
            if (homePage) { delete homePage; homePage = nullptr; }
            if (remotePage) { delete remotePage; remotePage = nullptr; }
            if (connectionPage) { delete connectionPage; connectionPage = nullptr; }
            if (settingsPage) { delete settingsPage; settingsPage = nullptr; }
            if (infoPage) { delete infoPage; infoPage = nullptr; }
            
            DEBUG_PRINTLN("  Pages deleted");
            
            // SD-Karte flushen
            if (sdCard.isAvailable()) {
                sdCard.flush();
                DEBUG_PRINTLN("  SD-Card flushed");
            }
            
            // ESP-NOW beenden
            EspNowManager& espnow = EspNowManager::getInstance();
            if (espnow.isInitialized()) {
                espnow.end();
                DEBUG_PRINTLN("  ESP-NOW ended");
            }
            
            DEBUG_PRINTLN("  Cleanup complete");
        });
        
        // Wake-Up Grund ausgeben
        String wakeReason = powerMgr.getWakeupReason();
        Serial.printf("  Wake-Up: %s\n", wakeReason.c_str());
        
        sdCard.logSetupStep("PowerManager", true, wakeReason.c_str());
    } else {
        Serial.println("  PowerManager init failed!");
        sdCard.logSetupStep("PowerManager", false, "Init failed");
    }

    // ═══════════════════════════════════════════════════════════════
    // ESP-NOW
    // ═══════════════════════════════════════════════════════════════
    Serial.println("→ ESP-NOW...");
    EspNowManager& espnow = EspNowManager::getInstance();

    if (espnow.begin(ESPNOW_CHANNEL)) {
        Serial.println("  ESP-NOW OK");
        Serial.printf("  MAC: %s\n", espnow.getOwnMacString().c_str());
        
        // Heartbeat aus Config
        espnow.setHeartbeat(true, config.espnowHeartbeatInterval);
        espnow.setTimeout(config.espnowTimeout);
        
        sdCard.logSetupStep("ESP-NOW", true, espnow.getOwnMacString().c_str());
        
        // Events für Logging registrieren
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
    } else {
        sdCard.logSetupStep("ESP-NOW", false, "WiFi init error");
        sdCard.logError("ESP-NOW", 3, "esp_now_init() failed");
    }
    
    // ═══════════════════════════════════════════════════════════════
    // GlobalUI
    // ═══════════════════════════════════════════════════════════════
    Serial.println("→ GlobalUI...");
    UIManager* ui = display.getUI();
    TFT_eSPI* tft = &display.getTft();

    if (!globalUI.init(ui, tft, &battery, &powerMgr)) {
        Serial.println("  GlobalUI failed!");
        sdCard.logSetupStep("GlobalUI", false, "Init error");
        while (1) delay(100);
    }

    Serial.println("  GlobalUI OK");
    sdCard.logSetupStep("GlobalUI", true);
    
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
    
    Serial.println("  Pages created");
    sdCard.logSetupStep("UI-Pages", true, "5 pages created");
    
    // ═══════════════════════════════════════════════════════════════
    // Pages registrieren
    // ═══════════════════════════════════════════════════════════════
    Serial.println("→ Register Pages...");
    pageManager.addPage(homePage, PAGE_HOME);
    pageManager.addPage(remotePage, PAGE_REMOTE);
    pageManager.addPage(connectionPage, PAGE_CONNECTION);
    pageManager.addPage(settingsPage, PAGE_SETTINGS);
    pageManager.addPage(infoPage, PAGE_INFO);
    Serial.printf("  %d Pages registered\n", pageManager.getPageCount());
    
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
    Serial.println("Setup complete!");
    Serial.printf("   Setup-Zeit: %lu ms\n", setupTime);
    Serial.println();
    
    sdCard.logBootComplete(setupTime, true);
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
    // PowerManager (Auto-Sleep Check)
    // ═══════════════════════════════════════════════════════════════
    powerMgr.update();
    
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
        // TODO: Implement ESP-NOW sending
    }

    // ═══════════════════════════════════════════════════════════════
    // ESP-NOW
    // ═══════════════════════════════════════════════════════════════
    EspNowManager& espnow = EspNowManager::getInstance();
    espnow.update();
    
    // Connection-Stats loggen (alle 5 Minuten)
    if (sdCard.isAvailable() && (millis() - lastConnectionLog > 300000)) {
        if (espnow.isConnected()) {
            // Vereinfacht: Stats vom System
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
