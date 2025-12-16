/**
 * PageManager.cpp
 * 
 * Implementation des Page-Managers mit zentralem Layout
 */

#include "PageManager.h"
#include "HomePage.h"
#include "RemoteControlPage.h"
#include "ConnectionPage.h"
#include "SettingsPage.h"
#include "InfoPage.h"
#include "UserConfig.h"

// Externe Referenzen
extern UserConfig userConfig;

// Interne Page-Instanzen
static HomePage* homePage = nullptr;
static RemoteControlPage* remotePage = nullptr;
static ConnectionPage* connectionPage = nullptr;
static SettingsPage* settingsPage = nullptr;
static InfoPage* infoPage = nullptr;

PageManager::PageManager(TFT_eSPI* display, UIManager* uiMgr)
    : tft(display)
    , ui(uiMgr)
    , currentPageId(-1)
    , currentPageIndex(-1)
    , initialized(false)
    , deferredPageId(-1)
{
}

PageManager::~PageManager() {
    // Pages löschen
    if (homePage) delete homePage;
    if (remotePage) delete remotePage;
    if (connectionPage) delete connectionPage;
    if (settingsPage) delete settingsPage;
    if (infoPage) delete infoPage;
    
    pages.clear();
}

bool PageManager::init(BatteryMonitor* battery, PowerManager* powerMgr) {
    if (!tft || !ui || !battery || !powerMgr) {
        Serial.println("PageManager: ❌ Ungültige Parameter!");
        return false;
    }
    
    Serial.println("PageManager: Initialisiere...");
    
    // ═══════════════════════════════════════════════════════════════
    // UILayout initialisieren (erstellt Header/Footer Widgets)
    // ═══════════════════════════════════════════════════════════════
    if (!layout.init(ui, tft, battery, powerMgr)) {
        Serial.println("PageManager: ❌ UILayout init fehlgeschlagen!");
        return false;
    }
    
    // PageManager Referenz an Layout übergeben (für Zurück-Button)
    layout.setPageManager(this);
    
    initialized = true;
    
    Serial.println("PageManager: ✅ Initialisierung abgeschlossen!");
    
    // ═══════════════════════════════════════════════════════════════
    // Pages erstellen und registrieren
    // ═══════════════════════════════════════════════════════════════
    Serial.println("PageManager: Erstelle Pages...");
    
    homePage = new HomePage(ui, tft);
    remotePage = new RemoteControlPage(ui, tft);
    connectionPage = new ConnectionPage(ui, tft);
    settingsPage = new SettingsPage(ui, tft);
    infoPage = new InfoPage(ui, tft);
    
    // Peer MAC aus Config an ConnectionPage übergeben
    connectionPage->setPeerMac(userConfig.getEspnowPeerMac());
    
    Serial.println("  ✅ Pages erstellt");
    
    // Pages registrieren
    Serial.println("PageManager: Registriere Pages...");
    addPage(homePage, PAGE_HOME);
    addPage(remotePage, PAGE_REMOTE);
    addPage(connectionPage, PAGE_CONNECTION);
    addPage(settingsPage, PAGE_SETTINGS);
    addPage(infoPage, PAGE_INFO);
    
    Serial.printf("  ✅ %d Pages registriert\n", getPageCount());
    
    return true;
}

bool PageManager::addPage(UIPage* page, int pageId) {
    if (!page) return false;
    
    // Prüfen ob ID bereits existiert
    if (findPageIndex(pageId) != -1) {
        Serial.printf("PageManager: Seiten-ID %d existiert bereits!\n", pageId);
        return false;
    }
    
    PageEntry entry;
    entry.page = page;
    entry.pageId = pageId;
    
    pages.push_back(entry);
    
    // UILayout an Page übergeben
    page->setLayout(&layout);
    
    Serial.printf("PageManager: Seite '%s' registriert (ID=%d)\n", page->getPageName(), pageId);
    
    return true;
}

bool PageManager::showPage(int pageId) {
    if (!initialized) {
        Serial.println("PageManager: ❌ Nicht initialisiert!");
        return false;
    }
    
    int index = findPageIndex(pageId);
    
    if (index == -1) {
        Serial.printf("PageManager: Seite mit ID %d nicht gefunden!\n", pageId);
        return false;
    }
    
    // Aktuelle Seite verstecken
    if (currentPageIndex != -1 && currentPageIndex < pages.size()) {
        pages[currentPageIndex].page->hide();
    }
    
    // Neue Seite anzeigen
    currentPageIndex = index;
    currentPageId = pageId;
    pages[currentPageIndex].page->show();
    
    Serial.printf("PageManager: Zeige Seite '%s' (ID=%d)\n", 
                 pages[currentPageIndex].page->getPageName(), pageId);
    
    return true;
}

void PageManager::showPageDeferred(int pageId) {
    Serial.printf("PageManager: Verzögerter Wechsel zu Seite ID=%d\n", pageId);
    deferredPageId = pageId;
}

bool PageManager::showPageByIndex(int index) {
    if (index < 0 || index >= pages.size()) {
        return false;
    }
    
    return showPage(pages[index].pageId);
}

void PageManager::nextPage() {
    if (pages.empty()) return;
    
    int nextIndex = (currentPageIndex + 1) % pages.size();
    showPageByIndex(nextIndex);
}

void PageManager::previousPage() {
    if (pages.empty()) return;
    
    int prevIndex = currentPageIndex - 1;
    if (prevIndex < 0) prevIndex = pages.size() - 1;
    
    showPageByIndex(prevIndex);
}

UIPage* PageManager::getCurrentPage() {
    if (currentPageIndex >= 0 && currentPageIndex < pages.size()) {
        return pages[currentPageIndex].page;
    }
    return nullptr;
}

UIPage* PageManager::getPage(int pageId) {
    int index = findPageIndex(pageId);
    if (index != -1) {
        return pages[index].page;
    }
    return nullptr;
}

void PageManager::updateJoystick(int16_t x, int16_t y) {
    // Direkt an RemoteControlPage weiterleiten (ohne cast im .ino)
    if (remotePage) {
        remotePage->setJoystickPosition(x, y);
    }
}

void PageManager::update() {
    if (!initialized || !ui) return;
    
    // KRITISCH: UI-Manager update für Touch-Handling!
    ui->update();
    
    // Deferred page change verarbeiten (NACH ui->update!)
    if (deferredPageId != -1) {
        Serial.printf("  Verarbeite verzögerten Page-Wechsel zu ID=%d\n", deferredPageId);
        showPage(deferredPageId);
        deferredPageId = -1;
    }
    
    // Page update
    UIPage* currentPage = getCurrentPage();
    if (currentPage && currentPage->isVisible()) {
        currentPage->update();
    }
}

void PageManager::draw() {
    if (!initialized | !ui) {
        return;
    }
        
    // UI-Manager zeichnet alle Widgets (Header/Footer/Content)
    ui->drawUpdates();
    
}

int PageManager::findPageIndex(int pageId) {
    for (int i = 0; i < pages.size(); i++) {
        if (pages[i].pageId == pageId) {
            return i;
        }
    }
    return -1;
}