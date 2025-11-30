/**
 * UIPage.cpp
 * 
 * Vereinfachte Implementation - Header/Footer werden von GlobalUI verwaltet
 */

#include "UIPage.h"
#include "GlobalUI.h"
#include "UIPageManager.h"

UIPage::UIPage(const char* name, UIManager* uiMgr, TFT_eSPI* display)
    : ui(uiMgr)
    , tft(display)
    , globalUI(nullptr)
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
    
    // ═══════════════════════════════════════════════════════════════
    // GlobalUI aktualisieren (Header/Footer komplett neu zeichnen!)
    // ═══════════════════════════════════════════════════════════════
    Serial.printf("UIPage::show() - globalUI pointer: %p\n", globalUI);
    
    if (globalUI) {
        Serial.println("  Calling redrawHeader()...");
        // Header und Footer komplett neu zeichnen (um Überlagerungen zu vermeiden)
        globalUI->drawHeader();
        
        Serial.println("  Calling redrawFooter()...");
        globalUI->drawFooter();
                
        Serial.println("  Calling setPageTitle()...");
        // Seiten-Titel setzen
        globalUI->setHeaderText(pageName);
        
        Serial.println("  Calling showBackButton()...");
        // Zurück-Button konfigurieren
        if (hasBackButton) {
            globalUI->showBackButton(true, pageManager, backButtonTarget);
        } else {
            globalUI->showBackButton(false);  // Nutzt Default-Parameter
        }
        
        Serial.println("  Calling updateBatteryIcon()...");
        // Battery-Icon aktualisieren
        globalUI->updateBatteryIcon();
        
        Serial.println("  GlobalUI updates complete");
    } else {
        Serial.println("  WARNING: globalUI is NULL!");
    }
    
    // ═══════════════════════════════════════════════════════════════
    // Content-Elemente sichtbar machen und zeichnen
    // ═══════════════════════════════════════════════════════════════
    Serial.printf("  Making %d content elements visible...\n", contentElements.size());
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
    
    visible = false;
    
    Serial.printf("UIPage: '%s' versteckt\n", pageName);
}

void UIPage::update() {
    // Kann in abgeleiteten Klassen überschrieben werden
    // für seiten-spezifische Updates
}

void UIPage::setBackButton(bool enabled, int targetPageId) {
    hasBackButton = enabled;
    backButtonTarget = targetPageId;
}

void UIPage::setGlobalUI(GlobalUI* globalUIPtr) {
    globalUI = globalUIPtr;
}

void UIPage::setPageManager(UIPageManager* pageManagerPtr) {
    pageManager = pageManagerPtr;
}

void UIPage::addContentElement(UIElement* element) {
    if (!element || !ui) return;
    
    contentElements.push_back(element);
    ui->add(element);
}

void UIPage::initDefaultLayout() {
    // Layout nur für Content-Bereich
    // Header (0-40px) und Footer (280-320px) werden von GlobalUI verwaltet
    
    layout.contentX = 0;
    layout.contentY = 40;           // Nach Header
    layout.contentWidth = 480;      // Volle Breite
    layout.contentHeight = 240;     // 280 - 40 = 240px
    layout.contentBgColor = COLOR_BLACK;
}