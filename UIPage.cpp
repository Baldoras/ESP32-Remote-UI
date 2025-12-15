/**
 * UIPage.cpp
 * 
 * Vereinfachte Implementation - Header/Footer werden von UILayout verwaltet
 */

#include "UIPage.h"
#include "UILayout.h"
#include "PageManager.h"

UIPage::UIPage(const char* name, UIManager* uiMgr, TFT_eSPI* display)
    : ui(uiMgr)
    , tft(display)
    , uiLayout(nullptr)
    , pageManager(nullptr)
    , visible(false)
    , built(false)
    , hasBackButton(false)
    , backButtonTarget(-1)
{
    strncpy(pageName, name, sizeof(pageName) - 1);
    pageName[sizeof(pageName) - 1] = '\0';
    
    // Standard-Layout initialisieren (Content-Bereich)
    initDefaultLayout();
}

UIPage::~UIPage() {
    // Elemente aufräumen
    contentElements.clear();
}

void UIPage::show() {
    if (!built) {
        // Seite erstmalig bauen
        Serial.printf("UIPage: Building '%s'...\n", pageName);
        build();
        built = true;
    }
    
    visible = true;
    
    // UIManager informieren über aktuelle Page
    if (ui) {
        ui->setCurrentPage(this);
    }
    
    // ═══════════════════════════════════════════════════════════════
    // UILayout aktualisieren (wenn verfügbar)
    // ═══════════════════════════════════════════════════════════════
    if (uiLayout) {
        Serial.printf("UIPage::show() - '%s' - UILayout verfügbar\n", pageName);
        
        // Content-Bereich löschen
        uiLayout->clearContent(layout.contentBgColor);
        
        // Header und Footer neu zeichnen (für saubere Darstellung)
        uiLayout->drawHeader();
        uiLayout->drawFooter();
        
        // Seiten-Titel setzen
        uiLayout->setPageTitle(pageName);
        
        // Zurück-Button konfigurieren
        if (hasBackButton) {
            uiLayout->setBackButton(true, backButtonTarget);
        } else {
            uiLayout->setBackButton(false);
        }
        
        // Battery-Icon aktualisieren
        uiLayout->updateBattery();
        
        Serial.println("  UILayout aktualisiert");
    } else {
        Serial.printf("UIPage::show() - '%s' - WARNUNG: UILayout ist nullptr!\n", pageName);
    }
    
    // ═══════════════════════════════════════════════════════════════
    // Content-Elemente sichtbar machen und zeichnen
    // ═══════════════════════════════════════════════════════════════
    Serial.printf("  Mache %d Content-Elemente sichtbar...\n", contentElements.size());
    for (auto* element : contentElements) {
        element->setVisible(true);
        element->setNeedsRedraw(true);
    }
    
    Serial.printf("UIPage: '%s' angezeigt\n", pageName);
}

void UIPage::hide() {
    // Content-Elemente verstecken
    for (auto* element : contentElements) {
        element->setVisible(false);
    }
    
    // Button-States zurücksetzen (verhindert Ghost-Clicks)
    resetButtonStates();
    
    // Hook für abgeleitete Klassen
    onHide();
    
    visible = false;
    
    Serial.printf("UIPage: '%s' versteckt (Button-States zurückgesetzt)\n", pageName);
}

void UIPage::onHide() {
    // Standard-Implementation (leer)
    // Kann in abgeleiteten Klassen überschrieben werden
}

void UIPage::update() {
    // Kann in abgeleiteten Klassen überschrieben werden
    // für seiten-spezifische Updates
}

void UIPage::setBackButton(bool enabled, int targetPageId) {
    hasBackButton = enabled;
    backButtonTarget = targetPageId;
    
    Serial.printf("UIPage: '%s' - Zurück-Button: %s (Ziel: %d)\n", 
                 pageName, enabled ? "aktiviert" : "deaktiviert", targetPageId);
}

void UIPage::setLayout(UILayout* layout) {
    uiLayout = layout;
    Serial.printf("UIPage: '%s' - UILayout gesetzt: %p\n", pageName, layout);
}

void UIPage::setPageManager(PageManager* pm) {
    pageManager = pm;
    Serial.printf("UIPage: '%s' - PageManager gesetzt: %p\n", pageName, pm);
}

void UIPage::addContentElement(UIElement* element) {
    if (!element || !ui) return;
    
    // Owner-Page setzen
    element->setOwnerPage(this);
    
    contentElements.push_back(element);
    ui->add(element);
}

void UIPage::initDefaultLayout() {
    // Layout nur für Content-Bereich
    // Header (0-40px) und Footer (280-320px) werden von UILayout verwaltet
    
    layout.contentX = 0;
    layout.contentY = 40;           // Nach Header
    layout.contentWidth = 480;      // Volle Breite
    layout.contentHeight = 260;     // 280 - 40 (Header) = 240px
    layout.contentBgColor = COLOR_BLACK;
}

void UIPage::resetButtonStates() {
    Serial.printf("UIPage::resetButtonStates() - Resetting %d elements\n", contentElements.size());
    
    for (auto* element : contentElements) {
        // Simuliere Touch weit außerhalb für alle Elemente
        // Buttons setzen dadurch automatisch pressed=false, wasInside=false
        element->handleTouch(-1000, -1000, false);
    }
    
    Serial.println("  Alle Element-States zurückgesetzt");
}