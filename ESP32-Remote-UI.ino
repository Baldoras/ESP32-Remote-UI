/**
 * ESP32-Remote-UI.ino
 * 
 * Fernsteuerung mit ESP-NOW und Multi-Page UI + SD-Card Logging
 * 
 * Seiten:
 * - Home (ID=0): Startseite mit Navigation
 * - Remote (ID=1): Hauptsteuerung mit Joystick
 * - Connection (ID=2): ESP-NOW Verbindungsstatus
 * - Settings (ID=3): Einstellungen
 * - Info (ID=4): System-Informationen
 */

#include "DisplayHandler.h"
#include "TouchManager.h"
#include "BatteryMonitor.h"
#include "SDCardHandler.h"
#include "GlobalUI.h"
#include "UIPage.h"
#include "UIPageManager.h"
#include "UIButton.h"
#include "UILabel.h"
#include "UISlider.h"
#include "UITextBox.h"
#include "UIProgressBar.h"
#include "ESPNowManager.h"

// Hardware
DisplayHandler display;
TouchManager touch;
BatteryMonitor battery;
SDCardHandler sdCard;  // ⭐ SD-Karte

// Config (aus SD-Karte geladen)
SDConfig config;  // ⭐ Config

// GlobalUI (Header/Footer/Battery zentral)
GlobalUI globalUI;

// Page Manager
UIPageManager pageManager;

// Timing für Logging
unsigned long lastBatteryLog = 0;
unsigned long lastConnectionLog = 0;
unsigned long setupStartTime = 0;

// Seiten-IDs
enum PageID {
    PAGE_HOME = 0,
    PAGE_REMOTE = 1,
    PAGE_CONNECTION = 2,
    PAGE_SETTINGS = 3,
    PAGE_INFO = 4
};

// ═══════════════════════════════════════════════════════════════════════════
// HomePage - Startseite mit Navigation
// ═══════════════════════════════════════════════════════════════════════════
class HomePage : public UIPage {
public:
    HomePage(UIManager* ui, TFT_eSPI* tft) 
        : UIPage("Home", ui, tft) {
    }
    
    void build() override {
        Serial.println("Building HomePage...");
        
        // Willkommens-Text
        UILabel* lblWelcome = new UILabel(
            layout.contentX + 20, 
            layout.contentY + 20, 
            layout.contentWidth - 40, 
            50, 
            "ESP32 Remote Control"
        );
        lblWelcome->setFontSize(3);
        lblWelcome->setAlignment(TextAlignment::CENTER);
        lblWelcome->setTransparent(true);
        addContentElement(lblWelcome);
        
        // Info-Text
        UILabel* lblInfo = new UILabel(
            layout.contentX + 20, 
            layout.contentY + 75, 
            layout.contentWidth - 40, 
            30, 
            "Choose a page:"
        );
        lblInfo->setFontSize(2);
        lblInfo->setAlignment(TextAlignment::CENTER);
        lblInfo->setTransparent(true);
        addContentElement(lblInfo);
        
        // Status-Übersicht
        int16_t statusY = layout.contentY + 115;
        
        // Battery Status
        UILabel* lblBattery = new UILabel(
            layout.contentX + 20, 
            statusY, 
            200, 
            20, 
            "Battery: --.-V (---%)"
        );
        lblBattery->setFontSize(1);
        lblBattery->setAlignment(TextAlignment::LEFT);
        lblBattery->setTextColor(COLOR_GREEN);
        lblBattery->setTransparent(true);
        addContentElement(lblBattery);
        
        // Connection Status
        UILabel* lblConnection = new UILabel(
            layout.contentX + 240, 
            statusY, 
            200, 
            20, 
            "ESP-NOW: Not Connected"
        );
        lblConnection->setFontSize(1);
        lblConnection->setAlignment(TextAlignment::LEFT);
        lblConnection->setTextColor(COLOR_RED);
        lblConnection->setTransparent(true);
        addContentElement(lblConnection);
        
        // Navigation Buttons (2x2 Grid)
        int16_t btnY = layout.contentY + 145;
        int16_t btnWidth = 200;
        int16_t btnHeight = 45;
        int16_t btnSpacing = 10;
        int16_t btnX1 = layout.contentX + 30;
        int16_t btnX2 = btnX1 + btnWidth + btnSpacing;
        
        // Remote Button
        UIButton* btnRemote = new UIButton(btnX1, btnY, btnWidth, btnHeight, "Remote Control");
        btnRemote->on(EventType::CLICK, [](EventData* data) {
            Serial.println("→ Remote Control");
            ::pageManager.showPage(PAGE_REMOTE);
        });
        addContentElement(btnRemote);
        
        // Connection Button
        UIButton* btnConnection = new UIButton(btnX2, btnY, btnWidth, btnHeight, "Connection");
        btnConnection->on(EventType::CLICK, [](EventData* data) {
            Serial.println("→ Connection");
            ::pageManager.showPage(PAGE_CONNECTION);
        });
        addContentElement(btnConnection);
        
        btnY += btnHeight + btnSpacing;
        
        // Settings Button
        UIButton* btnSettings = new UIButton(btnX1, btnY, btnWidth, btnHeight, "Settings");
        btnSettings->on(EventType::CLICK, [](EventData* data) {
            Serial.println("→ Settings");
            ::pageManager.showPage(PAGE_SETTINGS);
        });
        addContentElement(btnSettings);
        
        // Info Button
        UIButton* btnInfo = new UIButton(btnX2, btnY, btnWidth, btnHeight, "System Info");
        btnInfo->on(EventType::CLICK, [](EventData* data) {
            Serial.println("→ System Info");
            ::pageManager.showPage(PAGE_INFO);
        });
        addContentElement(btnInfo);
        
        Serial.println("  ✅ HomePage build complete");
    }
    
    void update() override {
        // Status-Labels aktualisieren (alle 2 Sekunden)
        static unsigned long lastUpdate = 0;
        if (millis() - lastUpdate > 2000) {
            updateStatus();
            lastUpdate = millis();
        }
    }
    
private:
    void updateStatus() {
        // Battery Status aktualisieren
        if (contentElements.size() > 2) {
            UILabel* lblBattery = static_cast<UILabel*>(contentElements[2]);
            if (lblBattery) {
                char buffer[64];
                sprintf(buffer, "Battery: %.1fV (%d%%)", 
                        battery.getVoltage(), battery.getPercent());
                lblBattery->setText(buffer);
                
                if (battery.isCritical()) {
                    lblBattery->setTextColor(COLOR_RED);
                } else if (battery.isLow()) {
                    lblBattery->setTextColor(COLOR_YELLOW);
                } else {
                    lblBattery->setTextColor(COLOR_GREEN);
                }
            }
        }
        
        // Connection Status aktualisieren
        if (contentElements.size() > 3) {
            UILabel* lblConnection = static_cast<UILabel*>(contentElements[3]);
            if (lblConnection) {
                EspNowManager& espnow = EspNowManager::getInstance();
                if (espnow.isConnected()) {
                    lblConnection->setText("ESP-NOW: Connected");
                    lblConnection->setTextColor(COLOR_GREEN);
                } else {
                    lblConnection->setText("ESP-NOW: Not Connected");
                    lblConnection->setTextColor(COLOR_RED);
                }
            }
        }
    }
};

// ═══════════════════════════════════════════════════════════════════════════
// RemoteControlPage - aus vorhandener Datei
// ═══════════════════════════════════════════════════════════════════════════
class RemoteControlPage : public UIPage {
private:
    int16_t joystickX, joystickY;
    int16_t joystickAreaX, joystickAreaY, joystickAreaSize;
    int16_t joystickCenterX, joystickCenterY;
    UILabel* labelConnectionStatus;
    UILabel* labelJoystickX;
    UILabel* labelJoystickY;
    UIProgressBar* barRemoteBattery;
    UILabel* labelRemoteBatteryValue;
    
public:
    RemoteControlPage(UIManager* ui, TFT_eSPI* tft)
        : UIPage("Remote Control", ui, tft), joystickX(0), joystickY(0) {
        setBackButton(true, PAGE_HOME);
    }
    
    void build() override {
        Serial.println("Building RemoteControlPage...");
        
        // Status-Bar
        int16_t statusY = layout.contentY + 5;
        labelConnectionStatus = new UILabel(layout.contentX + 10, statusY, 150, 25, "● DISCONNECTED");
        labelConnectionStatus->setAlignment(TextAlignment::LEFT);
        labelConnectionStatus->setFontSize(1);
        labelConnectionStatus->setTextColor(COLOR_RED);
        labelConnectionStatus->setTransparent(true);
        addContentElement(labelConnectionStatus);
        
        // Joystick-Bereich
        int16_t joyStartY = layout.contentY + 40;
        joystickAreaSize = 180;
        joystickAreaX = layout.contentX + 20;
        joystickAreaY = joyStartY;
        joystickCenterX = joystickAreaX + joystickAreaSize / 2;
        joystickCenterY = joystickAreaY + joystickAreaSize / 2;
        
        int16_t joyValuesY = joystickAreaY + joystickAreaSize + 10;
        
        labelJoystickX = new UILabel(joystickAreaX, joyValuesY, joystickAreaSize / 2 - 5, 25, "X: 0");
        labelJoystickX->setAlignment(TextAlignment::CENTER);
        labelJoystickX->setFontSize(1);
        labelJoystickX->setTransparent(true);
        addContentElement(labelJoystickX);
        
        labelJoystickY = new UILabel(joystickAreaX + joystickAreaSize / 2 + 5, joyValuesY, joystickAreaSize / 2 - 5, 25, "Y: 0");
        labelJoystickY->setAlignment(TextAlignment::CENTER);
        labelJoystickY->setFontSize(1);
        labelJoystickY->setTransparent(true);
        addContentElement(labelJoystickY);
        
        // Battery-Anzeige
        int16_t batteryY = layout.contentY + layout.contentHeight - 60;
        UILabel* labelRemoteBattery = new UILabel(layout.contentX + 10, batteryY, 150, 25, "Remote Battery:");
        labelRemoteBattery->setAlignment(TextAlignment::LEFT);
        labelRemoteBattery->setFontSize(1);
        labelRemoteBattery->setTransparent(true);
        addContentElement(labelRemoteBattery);
        
        batteryY += 25;
        int16_t batteryWidth = (layout.contentWidth - 20) / 2;
        
        barRemoteBattery = new UIProgressBar(layout.contentX + 10, batteryY, batteryWidth - 80, 25);
        barRemoteBattery->setValue(0);
        barRemoteBattery->setBarColor(COLOR_RED);
        barRemoteBattery->setShowText(false);
        addContentElement(barRemoteBattery);
        
        labelRemoteBatteryValue = new UILabel(layout.contentX + 10 + batteryWidth - 70, batteryY, 70, 25, "0.0V");
        labelRemoteBatteryValue->setAlignment(TextAlignment::CENTER);
        labelRemoteBatteryValue->setFontSize(1);
        labelRemoteBatteryValue->setTransparent(true);
        addContentElement(labelRemoteBatteryValue);
        
        Serial.println("  ✅ RemoteControlPage build complete");
    }
    
    void update() override {
        static int16_t lastJoyX = 0, lastJoyY = 0;
        if (joystickX != lastJoyX || joystickY != lastJoyY) {
            drawJoystickPosition();
            lastJoyX = joystickX;
            lastJoyY = joystickY;
            
            char buffer[16];
            sprintf(buffer, "X: %d", joystickX);
            labelJoystickX->setText(buffer);
            sprintf(buffer, "Y: %d", joystickY);
            labelJoystickY->setText(buffer);
        }
    }
    
    void setJoystickPosition(int16_t x, int16_t y) {
        joystickX = constrain(x, -100, 100);
        joystickY = constrain(y, -100, 100);
    }
    
private:
    void drawJoystickPosition() {
        if (!tft) return;
        
        // Bereich löschen
        tft->fillRect(joystickAreaX, joystickAreaY, joystickAreaSize, joystickAreaSize, COLOR_BLACK);
        
        // Kreis + Mittelkreuz zeichnen
        int16_t radius = joystickAreaSize / 2 - 5;
        tft->drawCircle(joystickCenterX, joystickCenterY, radius, COLOR_WHITE);
        tft->drawLine(joystickCenterX - 10, joystickCenterY, joystickCenterX + 10, joystickCenterY, COLOR_GRAY);
        tft->drawLine(joystickCenterX, joystickCenterY - 10, joystickCenterX, joystickCenterY + 10, COLOR_GRAY);
        
        // Joystick-Punkt
        int16_t posX = joystickCenterX + (joystickX * radius) / 100;
        int16_t posY = joystickCenterY - (joystickY * radius) / 100;  // Y invertiert
        
        tft->fillCircle(posX + 1, posY + 1, 8, COLOR_DARKGRAY);
        tft->fillCircle(posX, posY, 8, COLOR_BLUE);
        tft->fillCircle(posX - 2, posY - 2, 3, COLOR_CYAN);
    }
};

// ═══════════════════════════════════════════════════════════════════════════
// ConnectionPage - ESP-NOW Verbindungsstatus
// ═══════════════════════════════════════════════════════════════════════════
class ConnectionPage : public UIPage {
private:
    char peerMacStr[18];
    uint8_t peerMac[6];
    bool isPaired;
    bool isConnected;
    
    UILabel* labelStatusValue;
    UILabel* labelOwnMacValue;
    UILabel* labelPeerMacValue;
    UIButton* btnPair;
    UIButton* btnDisconnect;
    
public:
    ConnectionPage(UIManager* ui, TFT_eSPI* tft)
        : UIPage("Connection", ui, tft)
        , isPaired(false)
        , isConnected(false)
        , labelStatusValue(nullptr)
        , labelOwnMacValue(nullptr)
        , labelPeerMacValue(nullptr)
        , btnPair(nullptr)
        , btnDisconnect(nullptr)
    {
        strncpy(peerMacStr, "00:00:00:00:00:00", sizeof(peerMacStr));
        memset(peerMac, 0, 6);
        setBackButton(true, PAGE_HOME);
    }
    
    void build() override {
        Serial.println("Building ConnectionPage...");
        
        int16_t contentX = layout.contentX;
        int16_t contentY = layout.contentY;
        int16_t contentW = layout.contentWidth;
        
        int16_t yPos = contentY + 10;
        int16_t labelWidth = 120;
        int16_t valueX = contentX + labelWidth + 10;
        int16_t valueWidth = contentW - labelWidth - 20;
        
        // Status
        UILabel* labelStatus = new UILabel(contentX + 10, yPos, labelWidth, 30, "Status:");
        labelStatus->setAlignment(TextAlignment::LEFT);
        labelStatus->setFontSize(2);
        labelStatus->setTransparent(true);
        addContentElement(labelStatus);
        
        labelStatusValue = new UILabel(valueX, yPos, valueWidth, 30, "Disconnected");
        labelStatusValue->setAlignment(TextAlignment::LEFT);
        labelStatusValue->setFontSize(2);
        labelStatusValue->setTextColor(COLOR_RED);
        labelStatusValue->setTransparent(true);
        addContentElement(labelStatusValue);
        
        yPos += 40;
        
        // Eigene MAC
        UILabel* labelOwnMac = new UILabel(contentX + 10, yPos, labelWidth, 25, "Own MAC:");
        labelOwnMac->setAlignment(TextAlignment::LEFT);
        labelOwnMac->setFontSize(1);
        labelOwnMac->setTransparent(true);
        addContentElement(labelOwnMac);
        
        labelOwnMacValue = new UILabel(valueX, yPos, valueWidth, 25, "00:00:00:00:00:00");
        labelOwnMacValue->setAlignment(TextAlignment::LEFT);
        labelOwnMacValue->setFontSize(1);
        labelOwnMacValue->setTextColor(COLOR_CYAN);
        labelOwnMacValue->setTransparent(true);
        addContentElement(labelOwnMacValue);
        
        yPos += 30;
        
        // Peer MAC
        UILabel* labelPeerMac = new UILabel(contentX + 10, yPos, labelWidth, 25, "Peer MAC:");
        labelPeerMac->setAlignment(TextAlignment::LEFT);
        labelPeerMac->setFontSize(1);
        labelPeerMac->setTransparent(true);
        addContentElement(labelPeerMac);
        
        labelPeerMacValue = new UILabel(valueX, yPos, valueWidth, 25, peerMacStr);
        labelPeerMacValue->setAlignment(TextAlignment::LEFT);
        labelPeerMacValue->setFontSize(1);
        labelPeerMacValue->setTextColor(COLOR_YELLOW);
        labelPeerMacValue->setTransparent(true);
        addContentElement(labelPeerMacValue);
        
        yPos += 50;
        
        // Buttons
        int16_t btnWidth = 140;
        int16_t btnHeight = 40;
        int16_t btnSpacing = 10;
        int16_t btnX = contentX + 10;
        
        btnPair = new UIButton(btnX, yPos, btnWidth, btnHeight, "PAIR");
        btnPair->on(EventType::CLICK, [this](EventData* data) {
            this->onPairClicked();
        });
        addContentElement(btnPair);
        
        btnX += btnWidth + btnSpacing;
        
        btnDisconnect = new UIButton(btnX, yPos, btnWidth, btnHeight, "DISCONNECT");
        btnDisconnect->on(EventType::CLICK, [this](EventData* data) {
            this->onDisconnectClicked();
        });
        btnDisconnect->setEnabled(false);
        addContentElement(btnDisconnect);
        
        // Eigene MAC anzeigen
        EspNowManager& espnow = EspNowManager::getInstance();
        String ownMac = espnow.getOwnMacString();
        labelOwnMacValue->setText(ownMac.c_str());
        
        Serial.println("  ✅ ConnectionPage build complete");
    }
    
    void update() override {
        updateConnectionStatus();
    }
    
    void setPeerMac(const char* macStr) {
        strncpy(peerMacStr, macStr, sizeof(peerMacStr) - 1);
        peerMacStr[sizeof(peerMacStr) - 1] = '\0';
        EspNowManager::stringToMac(macStr, peerMac);
        
        if (labelPeerMacValue) {
            labelPeerMacValue->setText(peerMacStr);
        }
    }
    
private:
    void updateConnectionStatus() {
        EspNowManager& espnow = EspNowManager::getInstance();
        bool wasConnected = isConnected;
        
        isConnected = espnow.isPeerConnected(peerMac);
        isPaired = espnow.hasPeer(peerMac);
        
        if (wasConnected != isConnected && labelStatusValue) {
            if (isConnected) {
                labelStatusValue->setText("Connected");
                labelStatusValue->setTextColor(COLOR_GREEN);
                if (btnPair) btnPair->setEnabled(false);
                if (btnDisconnect) btnDisconnect->setEnabled(true);
            } else if (isPaired) {
                labelStatusValue->setText("Paired (Waiting...)");
                labelStatusValue->setTextColor(COLOR_YELLOW);
                if (btnPair) btnPair->setEnabled(false);
                if (btnDisconnect) btnDisconnect->setEnabled(true);
            } else {
                labelStatusValue->setText("Disconnected");
                labelStatusValue->setTextColor(COLOR_RED);
                if (btnPair) btnPair->setEnabled(true);
                if (btnDisconnect) btnDisconnect->setEnabled(false);
            }
        }
    }
    
    void onPairClicked() {
        Serial.println("ConnectionPage: Pair clicked");
        EspNowManager& espnow = EspNowManager::getInstance();
        
        if (espnow.addPeer(peerMac)) {
            Serial.println("  ✅ Peer added");
            isPaired = true;
            updateConnectionStatus();
            
            // ⭐ Log Pairing
            sdCard.logConnection(peerMacStr, "paired");
        } else {
            Serial.println("  ❌ Add peer failed");
        }
    }
    
    void onDisconnectClicked() {
        Serial.println("ConnectionPage: Disconnect clicked");
        EspNowManager& espnow = EspNowManager::getInstance();
        
        if (espnow.removePeer(peerMac)) {
            Serial.println("  ✅ Peer removed");
            isPaired = false;
            isConnected = false;
            updateConnectionStatus();
        } else {
            Serial.println("  ❌ Remove peer failed");
        }
    }
};

// ═══════════════════════════════════════════════════════════════════════════
// SettingsPage - Einstellungen (vereinfacht)
// ═══════════════════════════════════════════════════════════════════════════
class SettingsPage : public UIPage {
public:
    SettingsPage(UIManager* ui, TFT_eSPI* tft) 
        : UIPage("Settings", ui, tft) {
        setBackButton(true, PAGE_HOME);
    }
    
    void build() override {
        Serial.println("Building SettingsPage...");
        
        UILabel* lblTitle = new UILabel(layout.contentX + 20, layout.contentY + 20, layout.contentWidth - 40, 30, "Settings");
        lblTitle->setFontSize(2);
        lblTitle->setAlignment(TextAlignment::CENTER);
        lblTitle->setTransparent(true);
        addContentElement(lblTitle);
        
        UILabel* lblInfo = new UILabel(layout.contentX + 20, layout.contentY + 60, layout.contentWidth - 40, 60, "Config via SD-Card\nconfig.conf");
        lblInfo->setFontSize(1);
        lblInfo->setAlignment(TextAlignment::CENTER);
        lblInfo->setTransparent(true);
        addContentElement(lblInfo);
        
        Serial.println("  ✅ SettingsPage build complete");
    }
};

// ═══════════════════════════════════════════════════════════════════════════
// InfoPage - System-Informationen
// ═══════════════════════════════════════════════════════════════════════════
class InfoPage : public UIPage {
private:
    UITextBox* txtInfo;
    unsigned long lastUpdate;
    
public:
    InfoPage(UIManager* ui, TFT_eSPI* tft) 
        : UIPage("System Info", ui, tft), txtInfo(nullptr), lastUpdate(0) {
        setBackButton(true, PAGE_HOME);
    }
    
    void build() override {
        Serial.println("Building InfoPage...");
        
        int16_t tbY = layout.contentY + 10;
        int16_t tbHeight = layout.contentHeight - 60;
        
        txtInfo = new UITextBox(layout.contentX + 10, tbY, layout.contentWidth - 20, tbHeight);
        txtInfo->setFontSize(1);
        txtInfo->setLineHeight(14);
        addContentElement(txtInfo);
        
        updateInfo();
        
        int16_t buttonHeight = 40;
        UIButton* btnRefresh = new UIButton(
            layout.contentX + (layout.contentWidth - 150) / 2,
            layout.contentY + layout.contentHeight - buttonHeight - 10, 
            150, buttonHeight, "Refresh"
        );
        btnRefresh->on(EventType::CLICK, [this](EventData* data) { updateInfo(); });
        addContentElement(btnRefresh);
        
        Serial.println("  ✅ InfoPage build complete");
    }
    
    void update() override {
        if (millis() - lastUpdate > 10000) {
            updateInfo();
        }
    }
    
private:
    void updateInfo() {
        if (!txtInfo) return;
        
        txtInfo->clear();
        txtInfo->appendLine("=== System Information ===");
        txtInfo->appendLine("");
        txtInfo->appendLine("Hardware:");
        txtInfo->appendLine("  ESP32-S3-N16R8");
        txtInfo->appendLine("  Flash: 16MB / PSRAM: 8MB");
        txtInfo->appendLine("");
        
        txtInfo->appendLine("Display:");
        txtInfo->appendLine("  ST7796 480x320");
        char buffer[64];
        sprintf(buffer, "  Touch: %s", touch.isAvailable() ? "OK" : "N/A");
        txtInfo->appendLine(buffer);
        txtInfo->appendLine("");
        
        txtInfo->appendLine("Battery:");
        sprintf(buffer, "  Voltage: %.2fV", battery.getVoltage());
        txtInfo->appendLine(buffer);
        sprintf(buffer, "  Charge: %d%%", battery.getPercent());
        txtInfo->appendLine(buffer);
        txtInfo->appendLine("");
        
        txtInfo->appendLine("SD-Card:");
        if (sdCard.isAvailable()) {
            sprintf(buffer, "  Free: %.1f MB", sdCard.getFreeSpace() / 1024.0 / 1024.0);
            txtInfo->appendLine(buffer);
        } else {
            txtInfo->appendLine("  Not available");
        }
        txtInfo->appendLine("");
        
        txtInfo->appendLine("ESP-NOW:");
        EspNowManager& espnow = EspNowManager::getInstance();
        sprintf(buffer, "  Init: %s", espnow.isInitialized() ? "Yes" : "No");
        txtInfo->appendLine(buffer);
        sprintf(buffer, "  Connected: %s", espnow.isConnected() ? "Yes" : "No");
        txtInfo->appendLine(buffer);
        txtInfo->appendLine("");
        
        txtInfo->appendLine("System:");
        sprintf(buffer, "  Free Heap: %d bytes", ESP.getFreeHeap());
        txtInfo->appendLine(buffer);
        sprintf(buffer, "  Uptime: %lu sec", millis() / 1000);
        txtInfo->appendLine(buffer);
        
        txtInfo->scrollToTop();
        lastUpdate = millis();
    }
};

// Globale Seiten-Objekte
HomePage* homePage = nullptr;
RemoteControlPage* remotePage = nullptr;
ConnectionPage* connectionPage = nullptr;
SettingsPage* settingsPage = nullptr;
InfoPage* infoPage = nullptr;

void setup() {
    setupStartTime = millis();
    
    Serial.begin(115200);
    delay(100);
    
    DEBUG_PRINTLN("\n╔════════════════════════════════════════╗");
    DEBUG_PRINTLN("║   ESP32-S3 Remote Control Startup      ║");
    DEBUG_PRINTLN("║   WITH SD-CARD LOGGING                 ║");
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
        Serial.println("  ⚠️  SD-Card N/A (using defaults)");
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
    
    Serial.println("  ✅ Config defaults loaded (config.h)");
    
    // 2. Optional: Wenn SD-Karte verfügbar → Override aus config.conf
    if (sdCard.isAvailable()) {
        if (sdCard.loadConfig(config)) {
            Serial.println("  ✅ Config override loaded (config.conf)");
        } else {
            Serial.println("  ⚠️  No config.conf, using defaults");
        }
    } else {
        Serial.println("  ⚠️  No SD-Card, using defaults");
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
    
    Serial.println("  ✅ GPIO OK");
    sdCard.logSetupStep("GPIO", true);
    
    // ═══════════════════════════════════════════════════════════════
    // Display
    // ═══════════════════════════════════════════════════════════════
    Serial.println("→ Display...");
    if (!display.begin()) {
        Serial.println("❌ Display init failed!");
        sdCard.logSetupStep("Display", false, "Init failed");
        sdCard.logError("Display", ERR_DISPLAY_INIT, "begin() failed");
        while (1) delay(100);
    }
    display.setBacklight(config.backlightDefault);  // ⭐ Config
    Serial.println("  ✅ Display OK");
    sdCard.logSetupStep("Display", true, "480x320 @ Rotation 3");
    
    // ═══════════════════════════════════════════════════════════════
    // Touch
    // ═══════════════════════════════════════════════════════════════
    Serial.println("→ Touch...");
    SPIClass* hspi = &display.getTft().getSPIinstance();
    if (touch.begin(hspi)) {
        Serial.println("  ✅ Touch OK");
        
        // ⭐ Kalibrierung aus Config
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
        Serial.println("  ⚠️  Touch N/A");
        sdCard.logSetupStep("Touch", false, "XPT2046 not responding");
    }
    
    // ═══════════════════════════════════════════════════════════════
    // Battery
    // ═══════════════════════════════════════════════════════════════
    Serial.println("→ Battery...");
    if (battery.begin()) {
        Serial.println("  ✅ Battery OK");
        sdCard.logSetupStep("Battery", true);
        
        // Initial-Status loggen
        sdCard.logBattery(battery.getVoltage(), battery.getPercent(), 
                         battery.isLow(), battery.isCritical());
    } else {
        sdCard.logSetupStep("Battery", false, "Sensor error");
        sdCard.logError("Battery", ERR_BATTERY_INIT, "begin() failed");
    }
    
    // ═══════════════════════════════════════════════════════════════
    // ESP-NOW
    // ═══════════════════════════════════════════════════════════════
    Serial.println("→ ESP-NOW...");
    EspNowManager& espnow = EspNowManager::getInstance();
    if (espnow.begin(ESPNOW_CHANNEL)) {
        Serial.println("  ✅ ESP-NOW OK");
        Serial.printf("  MAC: %s\n", espnow.getOwnMacString().c_str());
        
        // ⭐ Heartbeat aus Config
        espnow.setHeartbeat(true, config.espnowHeartbeatInterval);
        espnow.setTimeout(config.espnowTimeout);
        
        sdCard.logSetupStep("ESP-NOW", true, espnow.getOwnMacString().c_str());
        
        // ⭐ Events für Logging registrieren
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
    if (!globalUI.init(ui, tft, &battery)) {
        Serial.println("  ❌ GlobalUI failed!");
        sdCard.logSetupStep("GlobalUI", false, "Init error");
        while (1) delay(100);
    }
    Serial.println("  ✅ GlobalUI OK");
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
    
    Serial.println("  ✅ Pages created");
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
    
    Serial.println();
    Serial.println("✅ Setup complete!");
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
    
    // ⭐ Battery-Status loggen (alle 60 Sekunden)
    if (sdCard.isAvailable() && (millis() - lastBatteryLog > 60000)) {
        sdCard.logBattery(battery.getVoltage(), battery.getPercent(), 
                         battery.isLow(), battery.isCritical());
        lastBatteryLog = millis();
    }
    
    // ⭐ Battery Critical → Error-Log
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
    // ESP-NOW
    // ═══════════════════════════════════════════════════════════════
    EspNowManager& espnow = EspNowManager::getInstance();
    espnow.update();
    
    // ⭐ Connection-Stats loggen (alle 5 Minuten)
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
    
    // TODO: Joystick auslesen
    // remotePage->setJoystickPosition(joyX, joyY);
    
    delay(10);
}