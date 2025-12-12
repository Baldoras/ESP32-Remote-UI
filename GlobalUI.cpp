/**
 * GlobalUI.cpp
 * 
 * Implementation des globalen UI-Managers mit PowerManager Integration
 */

#include "GlobalUI.h"
#include "UIPageManager.h"

GlobalUI::GlobalUI()
    : ui(nullptr)
    , tft(nullptr)
    , battery(nullptr)
    , powerMgr(nullptr)
    , pageManager(nullptr)
    , lblHeaderTitle(nullptr)
    , lblBatteryIcon(nullptr)
    , btnBack(nullptr)
    , btnSleep(nullptr)
    , lblFooter(nullptr)
    , initialized(false)
{
}

GlobalUI::~GlobalUI() {
    // Widgets werden vom UIManager verwaltet (Ownership dort)
}

bool GlobalUI::init(UIManager* uiMgr, TFT_eSPI* display, BatteryMonitor* batteryMon, PowerManager* powerManager) {
    if (!uiMgr || !display || !batteryMon || !powerManager) {
        Serial.println("GlobalUI: ❌ Ungültige Parameter!");
        return false;
    }
    
    ui = uiMgr;
    tft = display;
    battery = batteryMon;
    powerMgr = powerManager;
    
    Serial.println("GlobalUI: Initialisiere globale UI-Elemente...");
    
    // ═══════════════════════════════════════════════════════════════
    // Header Hintergrund zeichnen
    // ═══════════════════════════════════════════════════════════════
    drawHeaderBackground();
    
    // ═══════════════════════════════════════════════════════════════
    // Footer Hintergrund zeichnen
    // ═══════════════════════════════════════════════════════════════
    drawFooterBackground();
    
    // ═══════════════════════════════════════════════════════════════
    // Zurück-Button erstellen (links im Header)
    // ═══════════════════════════════════════════════════════════════
    btnBack = new UIButton(5, 3, 60, 34, "<");  // Größer: 60x34px
    btnBack->setVisible(false);  // Initial versteckt
    ElementStyle backStyle;
    backStyle.bgColor = COLOR_BLUE;
    backStyle.borderColor = COLOR_WHITE;
    backStyle.textColor = COLOR_WHITE;
    backStyle.borderWidth = 2;
    backStyle.cornerRadius = 5;
    btnBack->setStyle(backStyle);
    btnBack->setNeedsRedraw(false);  // KRITISCH: Nicht sofort zeichnen!
    ui->add(btnBack);
    
    Serial.println("  ✅ Zurück-Button erstellt (versteckt)");
    
    // ═══════════════════════════════════════════════════════════════
    // Seiten-Titel Label (zentriert im Header)
    // ═══════════════════════════════════════════════════════════════
    lblHeaderTitle = new UILabel(70, 5, 280, 30, "");  // X von 50 auf 70 (mehr Platz für Back-Button)
    lblHeaderTitle->setFontSize(2);
    lblHeaderTitle->setAlignment(TextAlignment::CENTER);
    lblHeaderTitle->setTransparent(true);
    ElementStyle titleStyle;
    titleStyle.bgColor = COLOR_DARKGRAY;
    titleStyle.textColor = COLOR_WHITE;
    titleStyle.borderWidth = 0;
    titleStyle.cornerRadius = 0;
    lblHeaderTitle->setStyle(titleStyle);
    lblHeaderTitle->setVisible(true);  // Sichtbar
    lblHeaderTitle->setNeedsRedraw(true);  // Sofort zeichnen
    ui->add(lblHeaderTitle);
    
    Serial.println("  ✅ Titel-Label erstellt");
    
    // ═══════════════════════════════════════════════════════════════
    // Sleep-Button (rechts vor Battery-Icon)
    // ═══════════════════════════════════════════════════════════════
    btnSleep = new UIButton(360, 3, 50, 34, "Z");  // 50x34px, Zeichen "Z"
    ElementStyle sleepStyle;
    sleepStyle.bgColor = COLOR_PURPLE;
    sleepStyle.borderColor = COLOR_WHITE;
    sleepStyle.textColor = COLOR_WHITE;
    sleepStyle.borderWidth = 2;
    sleepStyle.cornerRadius = 5;
    btnSleep->setStyle(sleepStyle);
    btnSleep->setVisible(true);  // Immer sichtbar
    btnSleep->setNeedsRedraw(true);
    
    // Sleep-Button Event-Handler
    btnSleep->on(EventType::CLICK, [this](EventData* data) {
        this->onSleepButtonClicked();
    });
    
    ui->add(btnSleep);
    
    Serial.println("  ✅ Sleep-Button erstellt");
    
    // ═══════════════════════════════════════════════════════════════
    // Battery-Icon (ganz rechts im Header)
    // ═══════════════════════════════════════════════════════════════
    lblBatteryIcon = new UILabel(420, 5, 55, 28, "");
    lblBatteryIcon->setFontSize(1);
    lblBatteryIcon->setAlignment(TextAlignment::CENTER);
    lblBatteryIcon->setTransparent(false);
    ElementStyle batteryStyle;
    batteryStyle.bgColor = COLOR_GREEN;
    batteryStyle.borderColor = COLOR_WHITE;
    batteryStyle.textColor = COLOR_WHITE;
    batteryStyle.borderWidth = 2;
    batteryStyle.cornerRadius = 3;
    lblBatteryIcon->setStyle(batteryStyle);
    
    // Initial mit Wert füllen
    updateBatteryIcon();
    lblBatteryIcon->setVisible(true);  // Sichtbar
    lblBatteryIcon->setNeedsRedraw(true);  // Sofort zeichnen
    
    ui->add(lblBatteryIcon);
    
    Serial.println("  ✅ Battery-Icon erstellt");
    
    // ═══════════════════════════════════════════════════════════════
    // Footer Label (zentriert)
    // ═══════════════════════════════════════════════════════════════
    lblFooter = new UILabel(0, 300, DISPLAY_WIDTH, 20, "v1.0.0 | Ready");
    lblFooter->setFontSize(1);
    lblFooter->setAlignment(TextAlignment::CENTER);
    lblFooter->setTransparent(true);
    ElementStyle footerStyle;
    footerStyle.bgColor = COLOR_DARKGRAY;
    footerStyle.textColor = COLOR_WHITE;
    footerStyle.borderWidth = 0;
    footerStyle.cornerRadius = 0;
    lblFooter->setStyle(footerStyle);
    lblFooter->setVisible(true);  // Sichtbar
    lblFooter->setNeedsRedraw(true);  // Sofort zeichnen
    ui->add(lblFooter);
    
    Serial.println("  ✅ Footer-Label erstellt");
    
    initialized = true;
    
    Serial.println("GlobalUI: ✅ Initialisierung abgeschlossen!");
    
    return true;
}

void GlobalUI::setPageTitle(const char* title) {
    if (!initialized || !lblHeaderTitle) return;
    
    lblHeaderTitle->setText(title);
    Serial.printf("GlobalUI: Seiten-Titel: '%s'\n", title);
}

void GlobalUI::setPageManager(UIPageManager* pm) {
    pageManager = pm;
    Serial.printf("GlobalUI: PageManager gesetzt: %p\n", pm);
}

void GlobalUI::showBackButton(bool show, int targetPageId) {
    if (!initialized || !btnBack) return;
    
    btnBack->setVisible(show);
    
    if (show && pageManager && targetPageId >= 0) {
        // Event-Handler nur setzen wenn noch nicht gesetzt ODER Target geändert
        static int lastTargetPageId = -1;
        
        if (lastTargetPageId != targetPageId) {
            btnBack->off(EventType::CLICK);  // Alte Handler entfernen
            
            // WICHTIG: pageManager direkt capturen (nicht this!)
            UIPageManager* pm = pageManager;  // Lokale Kopie für Lambda
            btnBack->on(EventType::CLICK, [pm, targetPageId](EventData* data) {
                Serial.printf("GlobalUI: Zurück-Button → Page %d\n", targetPageId);
                pm->showPage(targetPageId);
            });
            lastTargetPageId = targetPageId;
            Serial.printf("GlobalUI: Zurück-Button Event-Handler gesetzt (Target: %d)\n", targetPageId);
        }
    }
    
    Serial.printf("GlobalUI: Zurück-Button: %s\n", show ? "sichtbar" : "versteckt");
}

void GlobalUI::updateBatteryIcon() {
    if (!initialized || !lblBatteryIcon || !battery) return;
    
    uint8_t percent = battery->getPercent();
    float voltage = battery->getVoltage();
    
    // Text: Prozent-Anzeige
    char buffer[8];
    sprintf(buffer, "%d%%", percent);
    lblBatteryIcon->setText(buffer);
    
    // Farbe basierend auf Ladezustand
    updateBatteryIconColor(percent);
}

void GlobalUI::setFooterText(const char* text) {
    if (!initialized || !lblFooter) return;
    
    lblFooter->setText(text);
}

void GlobalUI::redrawHeader() {
    Serial.println("    GlobalUI::redrawHeader() called");
    Serial.printf("      initialized: %d, tft: %p\n", initialized, tft);
    
    if (!initialized || !tft) {
        Serial.println("      ERROR: Not initialized or tft is NULL!");
        return;
    }
    
    drawHeaderBackground();
    
    // Widgets neu zeichnen
    Serial.printf("      btnBack: %p, visible: %d\n", btnBack, btnBack ? btnBack->isVisible() : false);
    if (btnBack && btnBack->isVisible()) {
        btnBack->setNeedsRedraw(true);
    }
    
    Serial.printf("      lblHeaderTitle: %p\n", lblHeaderTitle);
    if (lblHeaderTitle) {
        lblHeaderTitle->setNeedsRedraw(true);
    }
    
    Serial.printf("      btnSleep: %p\n", btnSleep);
    if (btnSleep) {
        btnSleep->setNeedsRedraw(true);
    }
    
    Serial.printf("      lblBatteryIcon: %p\n", lblBatteryIcon);
    if (lblBatteryIcon) {
        lblBatteryIcon->setNeedsRedraw(true);
    }
    
    Serial.println("    GlobalUI::redrawHeader() complete");
}

void GlobalUI::redrawFooter() {
    Serial.println("    GlobalUI::redrawFooter() called");
    Serial.printf("      initialized: %d, tft: %p\n", initialized, tft);
    
    if (!initialized || !tft) {
        Serial.println("      ERROR: Not initialized or tft is NULL!");
        return;
    }
    
    drawFooterBackground();
    
    Serial.printf("      lblFooter: %p\n", lblFooter);
    if (lblFooter) {
        lblFooter->setNeedsRedraw(true);
    }
    
    Serial.println("    GlobalUI::redrawFooter() complete");
}

void GlobalUI::clearContentArea() {
    if (!initialized || !tft) return;
    
    // Content-Bereich löschen (zwischen Header und Footer)
    tft->fillRect(0, CONTENT_Y, DISPLAY_WIDTH, CONTENT_HEIGHT, COLOR_BLACK);
}

// ═══════════════════════════════════════════════════════════════════════════
// Private Methoden
// ═══════════════════════════════════════════════════════════════════════════

void GlobalUI::drawHeaderBackground() {
    if (!tft) return;
    
    // Header Hintergrund
    tft->fillRect(0, 0, DISPLAY_WIDTH, HEADER_HEIGHT, COLOR_DARKGRAY);
    
    // Trennlinie unten
    tft->drawLine(0, HEADER_HEIGHT - 1, DISPLAY_WIDTH, HEADER_HEIGHT - 1, COLOR_WHITE);
}

void GlobalUI::drawFooterBackground() {
    if (!tft) return;
    
    int16_t footerY = DISPLAY_HEIGHT - FOOTER_HEIGHT;
    
    // Trennlinie oben
    tft->drawLine(0, footerY, DISPLAY_WIDTH, footerY, COLOR_WHITE);
    
    // Footer Hintergrund
    tft->fillRect(0, footerY + 1, DISPLAY_WIDTH, FOOTER_HEIGHT - 1, COLOR_DARKGRAY);
}

void GlobalUI::updateBatteryIconColor(uint8_t percent) {
    if (!lblBatteryIcon || !battery) return;
    
    // Farbe basierend auf Zustand
    uint16_t color;
    
    if (battery->isCritical()) {
        color = TFT_RED;
    } else if (battery->isLow()) {
        color = TFT_ORANGE;
    } else if (percent > 60) {
        color = TFT_DARKGREEN;
    } else {
        color = TFT_DARKCYAN;
    }
    
    // Style aktualisieren
    ElementStyle iconStyle;
    iconStyle.bgColor = color;
    iconStyle.borderColor = TFT_WHITE;
    iconStyle.textColor = TFT_WHITE;
    iconStyle.borderWidth = 2;
    iconStyle.cornerRadius = 3;
    lblBatteryIcon->setStyle(iconStyle);
}

void GlobalUI::onSleepButtonClicked() {
    if (!initialized || !powerMgr || !tft) return;
    
    Serial.println("GlobalUI: Sleep-Button geklickt!");
    
    // Nachricht anzeigen (2 Sekunden)
    tft->fillRect(0, CONTENT_Y, DISPLAY_WIDTH, CONTENT_HEIGHT, COLOR_BLACK);
    tft->setTextDatum(MC_DATUM);
    tft->setTextColor(COLOR_WHITE);
    tft->setTextSize(3);
    tft->drawString("Sleep Mode", DISPLAY_WIDTH / 2, DISPLAY_HEIGHT / 2 - 20);
    tft->setTextSize(2);
    tft->drawString("Touch to wake up", DISPLAY_WIDTH / 2, DISPLAY_HEIGHT / 2 + 20);
    
    delay(2000);
    
    // Deep-Sleep aktivieren (Wake via Touch)
    powerMgr->sleep(WakeSource::TOUCH);
}