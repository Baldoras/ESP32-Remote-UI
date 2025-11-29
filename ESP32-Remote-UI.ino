/**
 * MultiPageUI.ino
 * 
 * Multi-Page UI-System mit GlobalUI
 * 
 * Seiten:
 * - Home (ID=0): Startseite mit Navigation
 * - Settings (ID=1): Einstellungen
 * - Info (ID=2): System-Informationen
 * 
 * NEU: GlobalUI verwaltet Header, Footer und Battery-Icon zentral!
 */

#include "DisplayHandler.h"
#include "TouchManager.h"
#include "BatteryMonitor.h"
#include "GlobalUI.h"
#include "UIPage.h"
#include "UIPageManager.h"
#include "UIButton.h"
#include "UILabel.h"
#include "UICheckBox.h"
#include "UISlider.h"
#include "UITextBox.h"

// Hardware
DisplayHandler display;
TouchManager touch;
BatteryMonitor battery;

// Global UI (Header/Footer/Battery-Icon)
GlobalUI globalUI;

// Page Manager
UIPageManager pageManager;

// Seiten-IDs
enum PageID {
    PAGE_HOME = 0,
    PAGE_SETTINGS = 1,
    PAGE_INFO = 2
};

// ═══════════════════════════════════════════════════════════════════════════
// HomePage - Startseite
// ═══════════════════════════════════════════════════════════════════════════
class HomePage : public UIPage {
public:
    HomePage(UIManager* ui, TFT_eSPI* tft) 
        : UIPage("Home", ui, tft) {
        // KEIN Zurück-Button auf Home-Page
        setBackButton(false);
    }
    
    void build() override {
        Serial.println("Building HomePage...");
        
        // Willkommens-Text
        UILabel* lblWelcome = new UILabel(
            layout.contentX + 20, 
            layout.contentY + 20, 
            layout.contentWidth - 40, 
            40, 
            "Willkommen!"
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
            60, 
            "Nutze die Buttons um zwischen den Seiten zu navigieren."
        );
        lblInfo->setFontSize(1);
        lblInfo->setAlignment(TextAlignment::CENTER);
        lblInfo->setTransparent(true);
        addContentElement(lblInfo);
        
        // Navigation Buttons
        UIButton* btnSettings = new UIButton(
            layout.contentX + 20, 
            layout.contentY + 150, 
            200, 
            50, 
            "Einstellungen"
        );
        btnSettings->on(EventType::CLICK, [this](EventData* data) {
            Serial.println("→ Einstellungen");
            pageManager->showPage(PAGE_SETTINGS);
        });
        addContentElement(btnSettings);
        
        UIButton* btnInfo = new UIButton(
            layout.contentX + 240, 
            layout.contentY + 150, 
            200, 
            50, 
            "Info"
        );
        btnInfo->on(EventType::CLICK, [this](EventData* data) {
            Serial.println("→ System Info");
            pageManager->showPage(PAGE_INFO);
        });
        addContentElement(btnInfo);
        
        Serial.println("  ✅ HomePage build complete");
    }
};

// ═══════════════════════════════════════════════════════════════════════════
// SettingsPage - Einstellungen
// ═══════════════════════════════════════════════════════════════════════════
class SettingsPage : public UIPage {
private:
    UITextBox* txtBattery;
    
public:
    SettingsPage(UIManager* ui, TFT_eSPI* tft) 
        : UIPage("Einstellungen", ui, tft), txtBattery(nullptr) {
        // Zurück-Button zu Home
        setBackButton(true, PAGE_HOME);
    }
    
    void build() override {
        Serial.println("Building SettingsPage (TEST: Label + Slider + TextBox)...");
        
        // Test Label
        UILabel* lblTest = new UILabel(
            layout.contentX + 20, 
            layout.contentY + 20, 
            200, 
            40, 
            "Helligkeit:"
        );
        lblTest->setFontSize(2);
        lblTest->setAlignment(TextAlignment::LEFT);
        lblTest->setTransparent(true);
        addContentElement(lblTest);
        
        // Slider
        UISlider* sliderBrightness = new UISlider(
            layout.contentX + 20, 
            layout.contentY + 70, 
            380,
            30
        );
        sliderBrightness->setValue(20);
        sliderBrightness->setShowValue(true);
        sliderBrightness->on(EventType::VALUE_CHANGED, [](EventData* data) {
            uint8_t brightness = map(data->value, 0, 100, BACKLIGHT_MIN, BACKLIGHT_MAX);
            display.setBacklight(brightness);
            Serial.printf("Helligkeit: %d%%\n", data->value);
        });
        addContentElement(sliderBrightness);
        
        // Battery Label
        UILabel* lblBattery = new UILabel(
            layout.contentX + 20, 
            layout.contentY + 110, 
            200, 
            25, 
            "Batterie-Info:"
        );
        lblBattery->setFontSize(1);
        lblBattery->setAlignment(TextAlignment::LEFT);
        lblBattery->setTransparent(true);
        addContentElement(lblBattery);
        
        // TEXTBOX ERSETZT DURCH LABEL (TEST)
        UILabel* lblBatteryInfo = new UILabel(
            layout.contentX + 20, 
            layout.contentY + 140, 
            layout.contentWidth - 40, 
            80,
            "Battery: 7.8V / 85%"
        );
        lblBatteryInfo->setFontSize(1);
        lblBatteryInfo->setAlignment(TextAlignment::LEFT);
        lblBatteryInfo->setTransparent(false);
        addContentElement(lblBatteryInfo);
        
        Serial.println("  ✅ SettingsPage build complete (Label + Slider + LABEL statt TextBox)");
    }
    
    void update() override {
        // DEAKTIVIERT FÜR TEST
        /*
        // Battery Info aktualisieren (alle 2 Sekunden)
        static unsigned long lastUpdate = 0;
        if (millis() - lastUpdate > 2000) {
            updateBatteryInfo();
            lastUpdate = millis();
        }
        */
    }
    
private:
    void updateBatteryInfo() {
        if (!txtBattery) return;
        
        txtBattery->clear();
        
        char buffer[64];
        
        // Spannung
        sprintf(buffer, "Spannung:        %.2fV", battery.getVoltage());
        txtBattery->appendLine(buffer);
        
        // Ladezustand
        sprintf(buffer, "Ladezustand:     %d%%", battery.getPercent());
        txtBattery->appendLine(buffer);
        
        // Warnung-Level
        sprintf(buffer, "Warnung bei:     %.2fV", VOLTAGE_ALARM_LOW);
        txtBattery->appendLine(buffer);
        
        // Shutdown-Level
        sprintf(buffer, "Shutdown bei:    %.2fV", VOLTAGE_SHUTDOWN);
        txtBattery->appendLine(buffer);
        
        txtBattery->scrollToTop();
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
        // Zurück-Button zu Home
        setBackButton(true, PAGE_HOME);
    }
    
    void build() override {
        Serial.println("Building InfoPage...");
        
        // Button-Höhe und Abstand definieren
        const int buttonHeight = 50;
        const int buttonMargin = 10;
        const int totalButtonArea = buttonHeight + buttonMargin;
        
        // TextBox mit korrekter Größe
        txtInfo = new UITextBox(
            layout.contentX + 20, 
            layout.contentY + 20, 
            layout.contentWidth - 40, 
            layout.contentHeight - totalButtonArea - 30  // -30 für Abstand oben/unten
        );
        txtInfo->setFontSize(1);
        txtInfo->setPadding(8);
        
        // Initial mit Daten füllen
        updateInfo();
        addContentElement(txtInfo);
        
        // Refresh Button (am unteren Rand, mittig)
        UIButton* btnRefresh = new UIButton(
            layout.contentX + (layout.contentWidth - 150) / 2,  // Zentriert
            layout.contentY + layout.contentHeight - buttonHeight - 10, 
            150, 
            buttonHeight, 
            "Aktualisieren"
        );
        btnRefresh->on(EventType::CLICK, [this](EventData* data) {
            Serial.println("Info aktualisiert");
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
SettingsPage* settingsPage = nullptr;
InfoPage* infoPage = nullptr;

void setup() {
    Serial.begin(115200);
    DEBUG_PRINTLN("\n");
    DEBUG_PRINTLN("╔════════════════════════════════════════╗");
    DEBUG_PRINTLN("║   ESP32-S3 Remote Control Startup      ║");
    DEBUG_PRINTLN("║   WITH GLOBAL UI SYSTEM                ║");
    DEBUG_PRINTLN("╚════════════════════════════════════════╝\n");
    
    // ═══════════════════════════════════════════════════════════════
    // GPIO-Pins initialisieren
    // ═══════════════════════════════════════════════════════════════
    Serial.println("→ Initialisiere GPIO-Pins...");
    
    // Touch CS auf HIGH (inaktiv) - WICHTIG vor Display-Init!
    pinMode(TOUCH_CS, OUTPUT);
    digitalWrite(TOUCH_CS, HIGH);
    
    // Backlight GPIO
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
        battery.printInfo();
    } else {
        Serial.println("  ❌ Battery Monitor init failed!");
    }
    
    // ═══════════════════════════════════════════════════════════════
    // Global UI initialisieren (Header/Footer/Battery-Icon)
    // ═══════════════════════════════════════════════════════════════
    Serial.println("→ Initialisiere Global UI...");
    
    UIManager* ui = display.getUI();
    TFT_eSPI* tft = &display.getTft();
    
    if (globalUI.init(ui, tft, &battery)) {
        Serial.println("  ✅ Global UI initialisiert");
    } else {
        Serial.println("  ❌ Global UI init failed!");
        while (1) delay(100);
    }
    
    // ═══════════════════════════════════════════════════════════════
    // Seiten erstellen
    // ═══════════════════════════════════════════════════════════════
    Serial.println("→ Erstelle Seiten...");
    
    homePage = new HomePage(ui, tft);
    settingsPage = new SettingsPage(ui, tft);
    infoPage = new InfoPage(ui, tft);
    
    // GlobalUI und PageManager Pointer setzen
    homePage->setGlobalUI(&globalUI);
    homePage->setPageManager(&pageManager);
    
    settingsPage->setGlobalUI(&globalUI);
    settingsPage->setPageManager(&pageManager);
    
    infoPage->setGlobalUI(&globalUI);
    infoPage->setPageManager(&pageManager);
    
    Serial.println("  ✅ Pages erstellt und konfiguriert");
    
    // ═══════════════════════════════════════════════════════════════
    // Seiten zum PageManager hinzufügen
    // ═══════════════════════════════════════════════════════════════
    Serial.println("→ Registriere Seiten...");
    
    pageManager.addPage(homePage, PAGE_HOME);
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
    Serial.println("   - Global UI: Header + Footer + Battery-Icon");
    Serial.println("   - 3 Pages: Home, Settings, Info");
    Serial.println("   - Battery Monitor: Auto-Shutdown aktiviert");
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
    
    // Display + UI aktualisieren
    display.update();
    
    // Aktuelle Seite aktualisieren
    pageManager.update();
    
    delay(10);
}
