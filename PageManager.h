/**
 * PageManager.h
 * 
 * Verwaltet mehrere UI-Seiten, Navigation und das zentrale UILayout
 * 
 * Neue Struktur:
 * - PageManager besitzt UILayout (Header/Footer zentral)
 * - Pages verwalten nur Content-Bereich
 * - draw() zeichnet Layout + aktive Page
 */

#ifndef PAGE_MANAGER_H
#define PAGE_MANAGER_H

#include <Arduino.h>
#include <TFT_eSPI.h>
#include <vector>
#include "UIPage.h"
#include "UILayout.h"
#include "UIManager.h"
#include "BatteryMonitor.h"
#include "PowerManager.h"

class PageManager {
public:
    /**
     * Konstruktor
     * @param tft TFT Display Pointer
     * @param ui UIManager Pointer
     */
    PageManager(TFT_eSPI* tft, UIManager* ui);
    
    /**
     * Destruktor
     */
    ~PageManager();

    /**
     * PageManager initialisieren
     * Erstellt UILayout
     * 
     * @param battery BatteryMonitor Pointer
     * @param powerMgr PowerManager Pointer
     * @return true bei Erfolg
     */
    bool init(BatteryMonitor* battery, PowerManager* powerMgr);

    /**
     * Seite hinzufügen
     * @param page UIPage Pointer
     * @param pageId Eindeutige ID für die Seite
     * @return true bei Erfolg
     */
    bool addPage(UIPage* page, int pageId);

    /**
     * Zu Seite wechseln (nach ID)
     * @param pageId Seiten-ID
     * @return true bei Erfolg
     */
    bool showPage(int pageId);
    
    /**
     * Zu Seite wechseln (verzögert)
     * Sicherer Aufruf während ui->update()
     * @param pageId Seiten-ID
     */
    void showPageDeferred(int pageId);

    /**
     * Zu Seite wechseln (nach Index)
     * @param index Seiten-Index
     * @return true bei Erfolg
     */
    bool showPageByIndex(int index);

    /**
     * Nächste Seite anzeigen
     */
    void nextPage();

    /**
     * Vorherige Seite anzeigen
     */
    void previousPage();

    /**
     * Aktuelle Seite abrufen
     */
    UIPage* getCurrentPage();

    /**
     * Seite nach ID abrufen
     */
    UIPage* getPage(int pageId);

    /**
     * Joystick-Position an RemoteControlPage weiterleiten
     * Entkoppelt .ino von konkreter Page-Implementierung
     * @param x X-Achse (-100 bis 100)
     * @param y Y-Achse (-100 bis 100)
     */
    void updateJoystick(int16_t x, int16_t y);

    /**
     * Aktuelle Seiten-ID
     */
    int getCurrentPageId() const { return currentPageId; }

    /**
     * Anzahl der Seiten
     */
    int getPageCount() const { return pages.size(); }

    /**
     * Update (ruft update() der aktuellen Seite auf)
     */
    void update();

    /**
     * Alles zeichnen: Layout (Header/Footer) + aktive Page Content
     * In loop() aufrufen!
     */
    void draw();

    /**
     * UILayout abrufen (für direkten Zugriff)
     */
    UILayout* getLayout() { return &layout; }

private:
    struct PageEntry {
        UIPage* page;
        int pageId;
    };
    
    TFT_eSPI* tft;                  // Display
    UIManager* ui;                  // UI-Manager
    UILayout layout;                // Zentrales Layout (Header/Footer)
    
    std::vector<PageEntry> pages;   // Registrierte Pages
    int currentPageId;              // Aktuelle Seiten-ID
    int currentPageIndex;           // Aktueller Seiten-Index
    
    bool initialized;               // Initialisierungs-Flag
    
    // Deferred page change (verhindert Crash während ui->update())
    int deferredPageId;             // -1 = kein Wechsel ausstehend
    
    /**
     * Seiten-Index nach ID finden
     */
    int findPageIndex(int pageId);
};

#endif // PAGE_MANAGER_H