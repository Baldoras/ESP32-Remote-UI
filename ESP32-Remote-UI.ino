/**
 * ESP32-Remote-UI.ino
 * 
 * Fernsteuerung mit ESP-NOW und Multi-Page UI
 * 
 * Seiten:
 * - Remote (ID=0): Hauptsteuerung mit Joystick
 * - Connection (ID=1): ESP-NOW Verbindungsstatus
 * - Settings (ID=2): Einstellungen
 * - Info (ID=3): System-Informationen
 */

#include "DisplayHandler.h"
#include "TouchManager.h"
#include "BatteryMonitor.h"
#include "GlobalUI.h"
#include "UIPage.h"
#include "UIPageManager.h"
#include "UIButton.h"
#include "UILabel.h"
#include "UISlider.h"
#include "UITextBox.h"
#include "UIProgressBar.h"
#include "ESPNowManager.h"
#include "ConnectionPage.h"
#include "RemoteControlPage.h"
#include "ESPNowUI.h"

// Hardware
DisplayHandler display;
TouchManager touch;
BatteryMonitor battery;

// GlobalUI (Header/Footer/Battery zentral)
GlobalUI globalUI;

// ESP-NOW UI Helper
ESPNowUI espnowUI;

// Page Manager
UIPageManager pageManager;

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
            layout.contentY + 80, 
            layout.contentWidth - 40, 
            40, 
            "Wähle eine Seite:"
        );
        lblInfo->setFontSize(2);
        lblInfo->setAlignment(TextAlignment::CENTER);
        lblInfo->setTransparent(true);
        addContentElement(lblInfo);
        
        // Status-Übersicht
        int16_t statusY = layout.contentY + 130;
        
        // Battery Status
        UILabel* lblBattery = new UILabel(
            layout.contentX + 20, 
            statusY, 
            200, 
            25, 
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
            25, 
            "ESP-NOW: Not Connected"
        );
        lblConnection->setFontSize(1);
        lblConnection->setAlignment(TextAlignment::LEFT);
        lblConnection->setTextColor(COLOR_RED);
        lblConnection->setTransparent(true);
        addContentElement(lblConnection);
        
        // Navigation Buttons (2x2 Grid)
        int16_t btnY = layout.contentY + 170;
        int16_t btnWidth = 200;
        int16_t btnHeight = 50;
        int16_t btnSpacing = 20;
        int16_t btnX1 = layout.contentX + 20;
        int16_t btnX2 = btnX1 + btnWidth + btnSpacing;
        
        // Remote Button
        UIButton* btnRemote = new UIButton(btnX1, btnY, btnWidth, btnHeight, "Remote Control");
        btnRemote->on(EventType::CLICK, [](EventData* data) {
            Serial.println("→ Remote Control");
            extern UIPageManager pageManager;
            pageManager.showPage(PAGE_REMOTE);
        });
        addContentElement(btnRemote);
        
        // Connection Button
        UIButton* btnConnection = new UIButton(btnX2, btnY, btnWidth, btnHeight, "Connection");
        btnConnection->on(EventType::CLICK, [](EventData* data) {
            Serial.println("→ Connection");
            extern UIPageManager pageManager;
            pageManager.showPage(PAGE_CONNECTION);
        });
        addContentElement(btnConnection);
        
        btnY += btnHeight + btnSpacing;
        
        // Settings Button
        UIButton* btnSettings = new UIButton(btnX1, btnY, btnWidth, btnHeight, "Settings");
        btnSettings->on(EventType::CLICK, [](EventData* data) {
            Serial.println("→ Settings");
            extern UIPageManager pageManager;
            pageManager.showPage(PAGE_SETTINGS);
        });
        addContentElement(btnSettings);
        
        // Info Button
        UIButton* btnInfo = new UIButton(btnX2, btnY, btnWidth, btnHeight, "System Info");
        btnInfo->on(EventType::CLICK, [](EventData* data) {
            Serial.println("→ System Info");
            extern UIPageManager pageManager;
            pageManager.showPage(PAGE_INFO);
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
        if (contentElements.size() > 3) {
            UILabel* lblBattery = static_cast<UILabel*>(contentElements[3]);
            if (lblBattery) {
                char buffer[64];
                sprintf(buffer, "Battery: %.1fV (%d%%)", 
                        battery.getVoltage(), battery.getPercent());
                lblBattery->setText(buffer);
                
                // Farbe basierend auf Status
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
        if (contentElements.size() > 4) {
            UILabel* lblConnection = static_cast<UILabel*>(contentElements[4]);
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
// SettingsPage - Einstellungen
// ═══════════════════════════════════════════════════════════════════════════
class SettingsPage : public UIPage {
public:
    SettingsPage(UIManager* ui, TFT_eSPI* tft) 
        : UIPage("Einstellungen", ui, tft) {
    }
    
    void build() override {
        Serial.println("Building SettingsPage...");
        
        // Helligkeit Label
        UILabel* lblBrightness = new UILabel(
            layout.contentX + 20, 
            layout.contentY + 20, 
            200, 
            30, 
            "Helligkeit:"
        );
        lblBrightness->setFontSize(2);
        lblBrightness->setAlignment(TextAlignment::LEFT);
        lblBrightness->setTransparent(true);
        addContentElement(lblBrightness);
        
        // Helligkeit Slider
        UISlider* sliderBrightness = new UISlider(
            layout.contentX + 20, 
            layout.contentY + 60, 
            layout.contentWidth - 40,
            30
        );
        sliderBrightness->setValue(50);
        sliderBrightness->setShowValue(true);
        sliderBrightness->on(EventType::VALUE_CHANGED, [](EventData* data) {
            uint8_t brightness = map(data->value, 0, 100, BACKLIGHT_MIN, BACKLIGHT_MAX);
            display.setBacklight(brightness);
        });
        addContentElement(sliderBrightness);
        
        // Peer MAC Label
        UILabel* lblPeerMac = new UILabel(
            layout.contentX + 20, 
            layout.contentY + 110, 
            200, 
            25, 
            "Peer MAC:"
        );
        lblPeerMac->setFontSize(1);
        lblPeerMac->setAlignment(TextAlignment::LEFT);
        lblPeerMac->setTransparent(true);
        addContentElement(lblPeerMac);
        
        // Peer MAC Value (Beispiel - sollte aus EEPROM/Preferences geladen werden)
        UILabel* lblPeerMacValue = new UILabel(
            layout.contentX + 20, 
            layout.contentY + 140, 
            layout.contentWidth - 40, 
            25,
            "FF:FF:FF:FF:FF:FF"
        );
        lblPeerMacValue->setFontSize(1);
        lblPeerMacValue->setAlignment(TextAlignment::LEFT);
        lblPeerMacValue->setTextColor(COLOR_CYAN);
        lblPeerMacValue->setTransparent(true);
        addContentElement(lblPeerMacValue);
        
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
    }
    
    void build() override {
        Serial.println("Building InfoPage...");
        
        // Info TextBox
        int16_t tbY = layout.contentY + 10;
        int16_t tbHeight = layout.contentHeight - 70;
        
        txtInfo = new UITextBox(
            layout.contentX + 10, 
            tbY, 
            layout.contentWidth - 20, 
            tbHeight
        );
        txtInfo->setFontSize(1);
        txtInfo->setLineHeight(14);
        addContentElement(txtInfo);
        
        // Initial befüllen
        updateInfo();
        
        // Refresh Button
        int16_t buttonHeight = 40;
        UIButton* btnRefresh = new UIButton(
            layout.contentX + (layout.contentWidth - 150) / 2,
            layout.contentY + layout.contentHeight - buttonHeight - 10, 
            150, 
            buttonHeight, 
            "Refresh"
        );
        btnRefresh->on(EventType::CLICK, [this](EventData* data) {
            updateInfo();
        });
        addContentElement(btnRefresh);
        
        Serial.println("  ✅ InfoPage build complete");
    }
    
    void update() override {
        // Auto-refresh alle 10 Sekunden
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
        
        // Hardware
        txtInfo->appendLine("Hardware:");
        txtInfo->appendLine("  Board: ESP32-S3-N16R8");
        txtInfo->appendLine("  Flash: 16MB");
        txtInfo->appendLine("  PSRAM: 8MB");
        txtInfo->appendLine("");
        
        // Display
        txtInfo->appendLine("Display:");
        txtInfo->appendLine("  Controller: ST7796");
        txtInfo->appendLine("  Resolution: 480x320");
        char buffer[64];
        sprintf(buffer, "  Touch: %s", touch.isAvailable() ? "OK" : "N/A");
        txtInfo->appendLine(buffer);
        txtInfo->appendLine("");
        
        // Battery
        txtInfo->appendLine("Battery:");
        sprintf(buffer, "  Voltage: %.2fV", battery.getVoltage());
        txtInfo->appendLine(buffer);
        sprintf(buffer, "  Charge: %d%%", battery.getPercent());
        txtInfo->appendLine(buffer);
        sprintf(buffer, "  Status: %s", 
                battery.isCritical() ? "CRITICAL" : 
                battery.isLow() ? "LOW" : "OK");
        txtInfo->appendLine(buffer);
        txtInfo->appendLine("");
        
        // ESP-NOW
        txtInfo->appendLine("ESP-NOW:");
        EspNowManager& espnow = EspNowManager::getInstance();
        sprintf(buffer, "  Initialized: %s", espnow.isInitialized() ? "Yes" : "No");
        txtInfo->appendLine(buffer);
        sprintf(buffer, "  Connected: %s", espnow.isConnected() ? "Yes" : "No");
        txtInfo->appendLine(buffer);
        sprintf(buffer, "  Send Rate: %d pkt/s", espnowUI.getSendRate());
        txtInfo->appendLine(buffer);
        sprintf(buffer, "  Recv Rate: %d pkt/s", espnowUI.getReceiveRate());
        txtInfo->appendLine(buffer);
        txtInfo->appendLine("");
        
        // System
        txtInfo->appendLine("System:");
        sprintf(buffer, "  Free Heap: %d bytes", ESP.getFreeHeap());
        txtInfo->appendLine(buffer);
        sprintf(buffer, "  CPU Freq: %d MHz", ESP.getCpuFreqMHz());
        txtInfo->appendLine(buffer);
        sprintf(buffer, "  Uptime: %lu sec", millis() / 1000);
        txtInfo->appendLine(buffer);
        txtInfo->appendLine("");
        
        // Software
        txtInfo->appendLine("Software:");
        txtInfo->appendLine("  Firmware: v1.0.0");
        txtInfo->appendLine("  Build: " __DATE__ " " __TIME__);
        
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
    Serial.begin(115200);
    delay(100);
    
    DEBUG_PRINTLN("\n");
    DEBUG_PRINTLN("╔════════════════════════════════════════╗");
    DEBUG_PRINTLN("║   ESP32-S3 Remote Control Startup      ║");
    DEBUG_PRINTLN("║   WITH ESP-NOW                         ║");
    DEBUG_PRINTLN("╚════════════════════════════════════════╝\n");
    
    // ═══════════════════════════════════════════════════════════════
    // GPIO initialisieren
    // ═══════════════════════════════════════════════════════════════
    Serial.println("→ Initialisiere GPIO-Pins...");
    
    pinMode(TOUCH_CS, OUTPUT);
    digitalWrite(TOUCH_CS, HIGH);
    pinMode(TFT_BL, OUTPUT);
    
    Serial.println("  ✅ GPIO initialisiert");
    
    // ═══════════════════════════════════════════════════════════════
    // Display initialisieren
    // ═══════════════════════════════════════════════════════════════
    Serial.println("→ Initialisiere Display...");
    
    if (!display.begin()) {
        Serial.println("❌ Display init failed!");
        while (1) delay(100);
    }
    
    Serial.println("  ✅ Display initialisiert");
    
    // ═══════════════════════════════════════════════════════════════
    // Touch initialisieren
    // ═══════════════════════════════════════════════════════════════
    Serial.println("→ Initialisiere Touch...");
    
    SPIClass* hspi = &display.getTft().getSPIinstance();
    
    if (touch.begin(hspi)) {
        Serial.println("  ✅ Touch initialisiert");
        display.enableUI(&touch);
        Serial.println("  ✅ UI aktiviert");
    } else {
        Serial.println("  ⚠️  Touch nicht verfügbar");
    }
    
    // ═══════════════════════════════════════════════════════════════
    // Battery Monitor initialisieren
    // ═══════════════════════════════════════════════════════════════
    Serial.println("→ Initialisiere Battery Monitor...");
    
    if (battery.begin()) {
        Serial.println("  ✅ Battery Monitor initialisiert");
    } else {
        Serial.println("  ❌ Battery Monitor init failed!");
    }
    
    // ═══════════════════════════════════════════════════════════════
    // ESP-NOW initialisieren
    // ═══════════════════════════════════════════════════════════════
    Serial.println("→ Initialisiere ESP-NOW...");
    
    EspNowManager& espnow = EspNowManager::getInstance();
    
    if (espnow.begin(ESPNOW_CHANNEL)) {
        Serial.println("  ✅ ESP-NOW initialisiert");
        Serial.printf("  MAC: %s\n", espnow.getOwnMacString().c_str());
        
        // Heartbeat aktivieren
        espnow.setHeartbeat(true, ESPNOW_HEARTBEAT_INTERVAL);
        Serial.println("  ✅ Heartbeat aktiviert");
        
    } else {
        Serial.println("  ❌ ESP-NOW init failed!");
    }
    
    // ═══════════════════════════════════════════════════════════════
    // GlobalUI initialisieren
    // ═══════════════════════════════════════════════════════════════
    Serial.println("→ Initialisiere GlobalUI...");
    
    UIManager* ui = display.getUI();
    TFT_eSPI* tft = &display.getTft();
    
    if (!globalUI.init(ui, tft, &battery)) {
        Serial.println("  ❌ GlobalUI init failed!");
        while (1) delay(100);
    }
    
    Serial.println("  ✅ GlobalUI initialisiert");
    
    // ═══════════════════════════════════════════════════════════════
    // Seiten erstellen
    // ═══════════════════════════════════════════════════════════════
    Serial.println("→ Erstelle Seiten...");
    
    homePage = new HomePage(ui, tft);
    remotePage = new RemoteControlPage(ui, tft);
    connectionPage = new ConnectionPage(ui, tft);
    settingsPage = new SettingsPage(ui, tft);
    infoPage = new InfoPage(ui, tft);
    
    // GlobalUI und PageManager setzen
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
    
    Serial.println("  ✅ Pages erstellt und konfiguriert");
    
    // ═══════════════════════════════════════════════════════════════
    // ESP-NOW UI Helper initialisieren
    // ═══════════════════════════════════════════════════════════════
    Serial.println("→ Initialisiere ESP-NOW UI...");
    
    espnowUI.begin(connectionPage, remotePage);
    
    // Peer MAC setzen (BEISPIEL - sollte aus Settings/EEPROM kommen)
    // espnowUI.setPeerMac("AA:BB:CC:DD:EE:FF");
    
    Serial.println("  ✅ ESP-NOW UI initialisiert");
    
    // ═══════════════════════════════════════════════════════════════
    // Seiten registrieren
    // ═══════════════════════════════════════════════════════════════
    Serial.println("→ Registriere Seiten...");
    
    pageManager.addPage(homePage, PAGE_HOME);
    pageManager.addPage(remotePage, PAGE_REMOTE);
    pageManager.addPage(connectionPage, PAGE_CONNECTION);
    pageManager.addPage(settingsPage, PAGE_SETTINGS);
    pageManager.addPage(infoPage, PAGE_INFO);
    
    Serial.printf("  ✅ %d Seiten registriert\n", pageManager.getPageCount());
    
    // ═══════════════════════════════════════════════════════════════
    // Home-Page als Start-Seite anzeigen
    // ═══════════════════════════════════════════════════════════════
    Serial.println("→ Zeige Home-Page...");
    pageManager.showPage(PAGE_HOME);
    
    Serial.println();
    Serial.println("✅ Setup complete!");
    Serial.println("   - 5 Pages: Home, Remote, Connection, Settings, Info");
    Serial.println("   - ESP-NOW: Ready");
    Serial.println("   - Battery Monitor: Active");
    Serial.println();
}

void loop() {
    // Battery Monitor aktualisieren
    battery.update();
    
    // Battery-Icon aktualisieren (alle 2 Sekunden)
    static unsigned long lastBatteryUpdate = 0;
    if (millis() - lastBatteryUpdate > 2000) {
        globalUI.updateBatteryIcon();
        lastBatteryUpdate = millis();
    }
    
    // Touch aktualisieren
    if (touch.isAvailable()) {
        touch.update();
    }
    
    // ESP-NOW Manager aktualisieren
    EspNowManager& espnow = EspNowManager::getInstance();
    espnow.update();
    
    // ESP-NOW UI Helper aktualisieren (verarbeitet empfangene Daten)
    espnowUI.update();
    
    // Remote Battery an UI übergeben (alle 2 Sekunden)
    static unsigned long lastRemoteBatteryUpdate = 0;
    if (millis() - lastRemoteBatteryUpdate > 2000) {
        if (remotePage) {
            remotePage->setRemoteBattery(battery.getVoltage(), battery.getPercent());
        }
        lastRemoteBatteryUpdate = millis();
    }
    
    // Display + UI aktualisieren
    display.update();
    
    // Aktuelle Seite aktualisieren
    pageManager.update();
    
    // TODO: Joystick auslesen und senden
    // Beispiel:
    // int16_t joyX = analogRead(JOYSTICK_X_PIN);
    // int16_t joyY = analogRead(JOYSTICK_Y_PIN);
    // joyX = map(joyX, 0, 4095, -100, 100);
    // joyY = map(joyY, 0, 4095, -100, 100);
    // espnowUI.sendJoystick(joyX, joyY);
    
    delay(10);
}
