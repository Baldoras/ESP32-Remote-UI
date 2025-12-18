/**
 * UILayout.h
 * 
 * Verwaltet das feste Layout mit Header, Content und Footer
 * 
 * Layout-Struktur (480x320):
 * ┌─────────────────────────────────────────────┐
 * │ Header (0-40px)                             │ ← Zurück-Button, Titel, Sleep, Battery
 * ├─────────────────────────────────────────────┤
 * │                                             │
 * │ Content (40-280px)                          │ ← Dynamischer Inhalt der Pages
 * │                                             │
 * ├─────────────────────────────────────────────┤
 * │ Footer (280-320px)                          │ ← Status-Text
 * └─────────────────────────────────────────────┘
 * 
 * Features:
 * - Header/Footer werden EINMAL erstellt und bleiben fix
 * - Nur Content-Bereich wird bei Page-Wechsel aktualisiert
 * - Alle Widgets (Buttons, Icons) nur 1x im Speicher
 */

#ifndef UI_LAYOUT_H
#define UI_LAYOUT_H

#include <Arduino.h>
#include <TFT_eSPI.h>
#include "UIManager.h"
#include "UILabel.h"
#include "UIButton.h"
#include "BatteryMonitor.h"
#include "PowerManager.h"
#include "setupConf.h"

// Forward declaration
class PageManager;

/**
 * Layout-Bereiche
 */
struct LayoutBounds {
    int16_t x;
    int16_t y;
    int16_t width;
    int16_t height;
};

class UILayout {
public:
    /**
     * Konstruktor
     */
    UILayout();
    
    /**
     * Destruktor
     */
    ~UILayout();

    /**
     * Layout initialisieren
     * Erstellt Header/Footer Widgets einmalig
     * 
     * @param ui UIManager Pointer
     * @param tft TFT Display Pointer
     * @param battery BatteryMonitor Pointer
     * @param powerMgr PowerManager Pointer
     * @return true bei Erfolg
     */
    bool init(UIManager* ui, TFT_eSPI* tft, BatteryMonitor* battery, PowerManager* powerMgr);

    /**
     * PageManager setzen (für Zurück-Button Navigation)
     * @param pm PageManager Pointer
     */
    void setPageManager(PageManager* pm);

    /**
     * Header-Bereich zeichnen (Hintergrund + Widgets)
     */
    void drawHeader();

    /**
     * Footer-Bereich zeichnen (Hintergrund + Widgets)
     */
    void drawFooter();

    /**
     * Content-Bereich löschen
     * @param color Hintergrundfarbe (default: schwarz)
     */
    void clearContent(uint16_t color = COLOR_BLACK);

    /**
     * Seiten-Titel setzen
     * @param title Neuer Titel
     */
    void setPageTitle(const char* title);

    /**
     * Zurück-Button konfigurieren
     * @param show true = anzeigen, false = verstecken
     * @param targetPageId Ziel-Seite beim Klick (-1 = kein Ziel)
     */
    void setBackButton(bool show, int targetPageId = -1);

    /**
     * Battery-Icon aktualisieren (in loop() alle 2 Sekunden aufrufen)
     */
    void updateBattery();

    /**
     * Footer-Text setzen
     * @param text Neuer Footer-Text
     */
    void setFooterText(const char* text);

    /**
     * Layout-Bereiche abrufen
     */
    const LayoutBounds& getHeaderBounds() const { return headerBounds; }
    const LayoutBounds& getContentBounds() const { return contentBounds; }
    const LayoutBounds& getFooterBounds() const { return footerBounds; }

    // Layout-Konstanten
    static const int16_t HEADER_HEIGHT = 40;
    static const int16_t FOOTER_HEIGHT = 20;
    static const int16_t CONTENT_Y = HEADER_HEIGHT;
    static const int16_t CONTENT_HEIGHT = DISPLAY_HEIGHT - HEADER_HEIGHT - FOOTER_HEIGHT;

private:
    UIManager* ui;                  // UI-Manager
    TFT_eSPI* tft;                  // Display
    BatteryMonitor* battery;        // Battery Monitor
    PowerManager* powerMgr;         // Power Manager
    PageManager* pageManager;     // Page Manager
    
    // Layout-Bereiche
    LayoutBounds headerBounds;
    LayoutBounds contentBounds;
    LayoutBounds footerBounds;
    
    // Header Widgets (einmalig erstellt!)
    UIButton* btnBack;              // Zurück-Button (links)
    UILabel* lblTitle;              // Seiten-Titel (zentriert)
    UIButton* btnSleep;             // Sleep-Button (rechts, vor Battery)
    UILabel* lblBattery;            // Battery-Icon (ganz rechts)
    
    // Footer Widgets
    UILabel* lblFooter;             // Footer-Text (zentriert)
    
    bool initialized;               // Initialisierungs-Flag
    
    /**
     * Layout-Bereiche berechnen
     */
    void calculateBounds();
    
    /**
     * Header Hintergrund zeichnen
     */
    void drawHeaderBackground();
    
    /**
     * Footer Hintergrund zeichnen
     */
    void drawFooterBackground();
    
    /**
     * Battery-Icon Farbe aktualisieren
     */
    void updateBatteryColor(uint8_t percent);
    
    /**
     * Sleep-Button Callback
     */
    void onSleepClicked();
};

#endif // UI_LAYOUT_H