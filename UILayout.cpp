/**
 * UILayout.cpp
 * 
 * Implementation des Layout-Managers
 */

#include "include/UILayout.h"
#include "include/PageManager.h"

UILayout::UILayout()
    : ui(nullptr)
    , tft(nullptr)
    , battery(nullptr)
    , powerMgr(nullptr)
    , pageManager(nullptr)
    , btnBack(nullptr)
    , lblTitle(nullptr)
    , btnSleep(nullptr)
    , lblBattery(nullptr)
    , lblFooter(nullptr)
    , initialized(false)
{
}

UILayout::~UILayout() {
    // Widgets werden vom UIManager verwaltet
}

bool UILayout::init(UIManager* uiMgr, TFT_eSPI* display, BatteryMonitor* bat, PowerManager* pm) {
    if (!uiMgr || !display || !bat || !pm) {
        Serial.println("UILayout: ❌ Ungültige Parameter!");
        return false;
    }
    
    ui = uiMgr;
    tft = display;
    battery = bat;
    powerMgr = pm;
    
    Serial.println("UILayout: Initialisiere Layout...");
    
    // ═══════════════════════════════════════════════════════════════
    // Layout-Bereiche berechnen
    // ═══════════════════════════════════════════════════════════════
    calculateBounds();
    
    // ═══════════════════════════════════════════════════════════════
    // Header & Footer Hintergrund zeichnen
    // ═══════════════════════════════════════════════════════════════
    drawHeaderBackground();
    drawFooterBackground();
    
    // ═══════════════════════════════════════════════════════════════
    // Zurück-Button erstellen (links im Header)
    // ═══════════════════════════════════════════════════════════════
    btnBack = new UIButton(5, 3, 60, 34, "<");
    btnBack->setVisible(false);  // Initial versteckt
    ElementStyle backStyle;
    backStyle.bgColor = COLOR_BLUE;
    backStyle.borderColor = COLOR_WHITE;
    backStyle.textColor = COLOR_WHITE;
    backStyle.borderWidth = 2;
    backStyle.cornerRadius = 5;
    btnBack->setStyle(backStyle);
    ui->add(btnBack);
    
    Serial.println("  ✅ Zurück-Button erstellt");
    
    // ═══════════════════════════════════════════════════════════════
    // Seiten-Titel Label (zentriert im Header)
    // ═══════════════════════════════════════════════════════════════
    lblTitle = new UILabel(70, 5, 280, 30, "");
    lblTitle->setFontSize(2);
    lblTitle->setAlignment(TextAlignment::CENTER);
    lblTitle->setTransparent(true);
    ElementStyle titleStyle;
    titleStyle.bgColor = COLOR_DARKGRAY;
    titleStyle.textColor = COLOR_WHITE;
    titleStyle.borderWidth = 0;
    titleStyle.cornerRadius = 0;
    lblTitle->setStyle(titleStyle);
    lblTitle->setVisible(true);
    lblTitle->setNeedsRedraw(true);
    ui->add(lblTitle);
    
    Serial.println("  ✅ Titel-Label erstellt");
    
    // ═══════════════════════════════════════════════════════════════
    // Sleep-Button (rechts vor Battery-Icon)
    // ═══════════════════════════════════════════════════════════════
    btnSleep = new UIButton(360, 3, 50, 34, "Z");
    ElementStyle sleepStyle;
    sleepStyle.bgColor = COLOR_PURPLE;
    sleepStyle.borderColor = COLOR_WHITE;
    sleepStyle.textColor = COLOR_WHITE;
    sleepStyle.borderWidth = 2;
    sleepStyle.cornerRadius = 5;
    btnSleep->setStyle(sleepStyle);
    btnSleep->setVisible(true);
    btnSleep->setNeedsRedraw(true);
    
    // Sleep-Button Event-Handler
    btnSleep->on(EventType::CLICK, [this](EventData* data) {
        this->onSleepClicked();
    });
    
    ui->add(btnSleep);
    
    Serial.println("  ✅ Sleep-Button erstellt");
    
    // ═══════════════════════════════════════════════════════════════
    // Battery-Icon (ganz rechts im Header)
    // ═══════════════════════════════════════════════════════════════
    lblBattery = new UILabel(420, 5, 55, 28, "");
    lblBattery->setFontSize(1);
    lblBattery->setAlignment(TextAlignment::CENTER);
    lblBattery->setTransparent(false);
    ElementStyle batteryStyle;
    batteryStyle.bgColor = COLOR_GREEN;
    batteryStyle.borderColor = COLOR_WHITE;
    batteryStyle.textColor = COLOR_WHITE;
    batteryStyle.borderWidth = 2;
    batteryStyle.cornerRadius = 3;
    lblBattery->setStyle(batteryStyle);
    
    // Initial-Text setzen (wird in loop() aktualisiert)
    lblBattery->setText("---%");
    lblBattery->setVisible(true);
    lblBattery->setNeedsRedraw(true);
    
    ui->add(lblBattery);
    
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
    lblFooter->setVisible(true);
    lblFooter->setNeedsRedraw(true);
    ui->add(lblFooter);
    
    Serial.println("  ✅ Footer-Label erstellt");
    
    initialized = true;
    
    Serial.println("UILayout: ✅ Initialisierung abgeschlossen!");
    
    return true;
}

void UILayout::setPageManager(PageManager* pm) {
    pageManager = pm;
    Serial.printf("UILayout: PageManager gesetzt: %p\n", pm);
}

void UILayout::calculateBounds() {
    // Header (oben)
    headerBounds.x = 0;
    headerBounds.y = 0;
    headerBounds.width = DISPLAY_WIDTH;
    headerBounds.height = HEADER_HEIGHT;
    
    // Content (Mitte)
    contentBounds.x = 0;
    contentBounds.y = CONTENT_Y;
    contentBounds.width = DISPLAY_WIDTH;
    contentBounds.height = CONTENT_HEIGHT;
    
    // Footer (unten)
    footerBounds.x = 0;
    footerBounds.y = DISPLAY_HEIGHT - FOOTER_HEIGHT;
    footerBounds.width = DISPLAY_WIDTH;
    footerBounds.height = FOOTER_HEIGHT;
}

void UILayout::drawHeader() {
    if (!initialized) return;
    
    drawHeaderBackground();
    
    // Widgets neu zeichnen
    if (btnBack && btnBack->isVisible()) {
        btnBack->setNeedsRedraw(true);
    }
    if (lblTitle) {
        lblTitle->setNeedsRedraw(true);
    }
    if (btnSleep) {
        btnSleep->setNeedsRedraw(true);
    }
    if (lblBattery) {
        lblBattery->setNeedsRedraw(true);
    }
}

void UILayout::drawFooter() {
    if (!initialized) return;
    
    drawFooterBackground();
    
    if (lblFooter) {
        lblFooter->setNeedsRedraw(true);
    }
}

void UILayout::clearContent(uint16_t color) {
    if (!initialized || !tft) return;
    
    tft->fillRect(
        contentBounds.x,
        contentBounds.y,
        contentBounds.width,
        contentBounds.height,
        color
    );
}

void UILayout::setPageTitle(const char* title) {
    if (!initialized || !lblTitle) return;
    
    lblTitle->setText(title);
    Serial.printf("UILayout: Titel: '%s'\n", title);
}

void UILayout::setBackButton(bool show, int targetPageId) {
    if (!initialized || !btnBack) return;
    
    btnBack->setVisible(show);
    
    if (show && pageManager && targetPageId >= 0) {
        static int lastTargetPageId = -1;
        
        if (lastTargetPageId != targetPageId) {
            btnBack->off(EventType::CLICK);
            
            PageManager* pm = pageManager;
            
            btnBack->on(EventType::CLICK, [pm, targetPageId](EventData* data) {
                Serial.printf("UILayout: Zurück → Page %d\n", targetPageId);
                if (pm) {
                    pm->showPageDeferred(targetPageId);
                }
            });
            
            lastTargetPageId = targetPageId;
        }
    }
    
    Serial.printf("UILayout: Zurück-Button: %s\n", show ? "sichtbar" : "versteckt");
}

void UILayout::updateBattery() {
    if (!initialized || !lblBattery || !battery) return;
    
    uint8_t percent = battery->getPercent();
    
    // Text: Prozent-Anzeige
    char buffer[8];
    sprintf(buffer, "%d%%", percent);
    lblBattery->setText(buffer);
    
    // Farbe aktualisieren
    updateBatteryColor(percent);
}

void UILayout::setFooterText(const char* text) {
    if (!initialized || !lblFooter) return;
    
    lblFooter->setText(text);
}

// ═══════════════════════════════════════════════════════════════
// Private Methoden
// ═══════════════════════════════════════════════════════════════

void UILayout::drawHeaderBackground() {
    if (!tft) return;
    
    tft->fillRect(
        headerBounds.x,
        headerBounds.y,
        headerBounds.width,
        headerBounds.height,
        COLOR_DARKGRAY
    );
    
    // Trennlinie unten
    tft->drawLine(
        0,
        headerBounds.height - 1,
        DISPLAY_WIDTH,
        headerBounds.height - 1,
        COLOR_WHITE
    );
}

void UILayout::drawFooterBackground() {
    if (!tft) return;
    
    // Trennlinie oben
    tft->drawLine(
        0,
        footerBounds.y,
        DISPLAY_WIDTH,
        footerBounds.y,
        COLOR_WHITE
    );
    
    // Footer Hintergrund
    tft->fillRect(
        footerBounds.x,
        footerBounds.y + 1,
        footerBounds.width,
        footerBounds.height - 1,
        COLOR_DARKGRAY
    );
}

void UILayout::updateBatteryColor(uint8_t percent) {
    if (!lblBattery || !battery) return;
    
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
    
    ElementStyle iconStyle;
    iconStyle.bgColor = color;
    iconStyle.borderColor = TFT_WHITE;
    iconStyle.textColor = TFT_WHITE;
    iconStyle.borderWidth = 2;
    iconStyle.cornerRadius = 3;
    lblBattery->setStyle(iconStyle);
}

void UILayout::onSleepClicked() {
    if (!initialized || !powerMgr || !tft) return;
    
    Serial.println("UILayout: Sleep-Button geklickt!");
    
    // Nachricht anzeigen
    clearContent(COLOR_BLACK);
    tft->setTextDatum(MC_DATUM);
    tft->setTextColor(COLOR_WHITE);
    tft->setTextSize(3);
    tft->drawString("Sleep Mode", DISPLAY_WIDTH / 2, DISPLAY_HEIGHT / 2 - 20);
    tft->setTextSize(2);
    tft->drawString("Touch to wake up", DISPLAY_WIDTH / 2, DISPLAY_HEIGHT / 2 + 20);
    
    delay(2000);
    
    // Deep-Sleep aktivieren
    powerMgr->sleep(WakeSource::TOUCH);
}