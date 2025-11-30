/**
 * GlobalUI.cpp
 * 
 * Implementation der zentralen UI-Verwaltung
 */

#include "GlobalUI.h"
#include "UIPageManager.h"

GlobalUI::GlobalUI()
    : ui(nullptr)
    , tft(nullptr)
    , battery(nullptr)
    , pageManager(nullptr)
    , headerHeight(40)
    , footerHeight(40)
    , contentHeight(0)
    , displayWidth(0)
    , displayHeight(0)
    , headerLabel(nullptr)
    , backButton(nullptr)
    , footerLabel(nullptr)
    , batteryIconX(0)
    , batteryIconY(0)
    , batteryIconWidth(50)
    , batteryIconHeight(20)
    , backTargetPageId(0)
{
}

GlobalUI::~GlobalUI() {
    // UI-Elemente werden vom UIManager aufgeräumt
}

bool GlobalUI::init(UIManager* uiMgr, TFT_eSPI* display, BatteryMonitor* bat) {
    if (!uiMgr || !display) {
        DEBUG_PRINTLN("GlobalUI: ❌ ui oder tft ist nullptr!");
        return false;
    }
    
    ui = uiMgr;
    tft = display;
    battery = bat;
    
    displayWidth = tft->width();
    displayHeight = tft->height();
    contentHeight = displayHeight - headerHeight - footerHeight;
    
    DEBUG_PRINTLN("GlobalUI: Initialisiere...");
    
    // ═══════════════════════════════════════════════════════════════════════
    // Header-Elemente erstellen
    // ═══════════════════════════════════════════════════════════════════════
    
    // Header Label (Seitentitel - zentral)
    headerLabel = new UILabel(0, 0, displayWidth, headerHeight, "Home");
    headerLabel->setFontSize(2);
    headerLabel->setAlignment(TextAlignment::CENTER);
    headerLabel->setTransparent(false);
    ElementStyle headerStyle;
    headerStyle.bgColor = COLOR_DARKGRAY;
    headerStyle.textColor = COLOR_WHITE;
    headerStyle.borderWidth = 0;
    headerStyle.cornerRadius = 0;
    headerLabel->setStyle(headerStyle);
    headerLabel->setVisible(true);
    ui->add(headerLabel);
    
    // Back-Button (rechts im Header, initial versteckt)
    backButton = new UIButton(displayWidth - 110, 5, 50, 30, "<");
    backButton->setVisible(false);  // Startet versteckt
    backButton->on(EventType::CLICK, [this](EventData* data) {
        if (pageManager) {
            DEBUG_PRINTF("GlobalUI: Back-Button → Page %d\n", backTargetPageId);
            pageManager->showPage(backTargetPageId);
        }
    });
    ui->add(backButton);
    
    // Battery-Icon Position (ganz rechts im Header)
    batteryIconX = displayWidth - 55;
    batteryIconY = 10;
    
    // ═══════════════════════════════════════════════════════════════════════
    // Footer-Elemente erstellen
    // ═══════════════════════════════════════════════════════════════════════
    
    int16_t footerY = displayHeight - footerHeight;
    
    footerLabel = new UILabel(0, footerY, displayWidth, footerHeight, "v1.0.0 | Ready");
    footerLabel->setFontSize(1);
    footerLabel->setAlignment(TextAlignment::CENTER);
    footerLabel->setTransparent(false);
    ElementStyle footerStyle;
    footerStyle.bgColor = COLOR_DARKGRAY;
    footerStyle.textColor = COLOR_GRAY;
    footerStyle.borderWidth = 0;
    footerStyle.cornerRadius = 0;
    footerLabel->setStyle(footerStyle);
    footerLabel->setVisible(true);
    ui->add(footerLabel);
    
    // Layout zeichnen
    drawLayout();
    
    DEBUG_PRINTLN("GlobalUI: ✅ Initialisiert");
    DEBUG_PRINTF("  Header: %d px\n", headerHeight);
    DEBUG_PRINTF("  Content: %d px\n", contentHeight);
    DEBUG_PRINTF("  Footer: %d px\n", footerHeight);
    
    return true;
}

void GlobalUI::setPageManager(UIPageManager* pm) {
    pageManager = pm;
}

void GlobalUI::setHeaderText(const char* title) {
    if (headerLabel) {
        headerLabel->setText(title);
    }
}

void GlobalUI::setFooterText(const char* text) {
    if (footerLabel) {
        footerLabel->setText(text);
    }
}

void GlobalUI::showBackButton(bool show, int targetPageId) {
    if (backButton) {
        backButton->setVisible(show);
        backTargetPageId = targetPageId;
    }
}

void GlobalUI::updateBatteryIcon() {
    if (!battery) return;
    
    uint8_t percent = battery->getPercent();
    drawBatteryIcon(percent, false);
}

void GlobalUI::redrawHeader() {
    if (!tft) return;
    
    // Header Hintergrund
    tft->fillRect(0, 0, displayWidth, headerHeight, COLOR_DARKGRAY);
    
    // Header Trennlinie
    tft->drawLine(0, headerHeight - 1, displayWidth, headerHeight - 1, COLOR_WHITE);
}

void GlobalUI::redrawFooter() {
    if (!tft) return;
    
    int16_t footerY = displayHeight - footerHeight;
    
    // Footer Trennlinie
    tft->drawLine(0, footerY, displayWidth, footerY, COLOR_WHITE);
    
    // Footer Hintergrund
    tft->fillRect(0, footerY + 1, displayWidth, footerHeight - 1, COLOR_DARKGRAY);
}

void GlobalUI::drawLayout() {
    drawHeader();
    drawFooter();
    
    // Content-Bereich (schwarz)
    tft->fillRect(0, headerHeight, displayWidth, contentHeight, COLOR_BLACK);
}

// ═══════════════════════════════════════════════════════════════════════════
// PRIVATE METHODEN
// ═══════════════════════════════════════════════════════════════════════════

void GlobalUI::drawBatteryIcon(uint8_t percent, bool charging) {
    if (!tft) return;
    
    // Battery-Icon (vereinfacht)
    int16_t x = batteryIconX;
    int16_t y = batteryIconY;
    int16_t w = batteryIconWidth;
    int16_t h = batteryIconHeight;
    
    // Hintergrund löschen
    tft->fillRect(x - 2, y - 2, w + 4, h + 4, COLOR_DARKGRAY);
    
    // Rahmen
    uint16_t color = getBatteryColor(percent);
    tft->drawRect(x, y, w - 4, h, color);
    tft->drawRect(x + 1, y + 1, w - 6, h - 2, color);
    
    // Plus-Pol (rechts)
    tft->fillRect(x + w - 4, y + 6, 4, h - 12, color);
    
    // Füllung (basierend auf Prozent)
    int16_t fillWidth = (w - 10) * percent / 100;
    if (fillWidth > 0) {
        tft->fillRect(x + 3, y + 3, fillWidth, h - 6, color);
    }
    
    // Prozent-Text (klein, nur wenn Platz)
    if (w > 30) {
        tft->setTextDatum(MC_DATUM);
        tft->setTextColor(COLOR_WHITE, COLOR_DARKGRAY);
        tft->setTextSize(1);
        char buffer[8];
        sprintf(buffer, "%d%%", percent);
        tft->drawString(buffer, x - 15, y + h / 2);
    }
}

uint16_t GlobalUI::getBatteryColor(uint8_t percent) {
    if (percent >= 80) return COLOR_GREEN;
    if (percent >= 50) return COLOR_YELLOW;
    if (percent >= 20) return COLOR_ORANGE;
    return COLOR_RED;
}
