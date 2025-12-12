/**
 * GlobalUI.h
 * 
 * Globaler UI-Manager für Header, Footer und Battery-Icon
 * 
 * Verwaltet UI-Elemente die auf ALLEN Seiten gleich sind:
 * - Header (0-40px): Zurück-Button + Seiten-Titel + Sleep-Button + Battery-Icon
 * - Footer (280-320px): Status-Text
 * 
 * Vorteile:
 * - Battery-Icon nur 1x erstellt (kein Memory-Problem)
 * - Header/Footer einmalig gezeichnet
 * - Konsistentes Layout über alle Pages
 */

#ifndef GLOBAL_UI_H
#define GLOBAL_UI_H

#include <Arduino.h>
#include <TFT_eSPI.h>
#include "UIManager.h"
#include "UILabel.h"
#include "UIButton.h"
#include "BatteryMonitor.h"
#include "PowerManager.h"
#include "config.h"

// Forward declaration
class UIPageManager;

class GlobalUI {
public:
    /**
     * Konstruktor
     */
    GlobalUI();
    
    /**
     * Destruktor
     */
    ~GlobalUI();

    /**
     * Global UI initialisieren (einmalig in setup() aufrufen)
     * Zeichnet Header/Footer Hintergrund und erstellt globale Widgets
     * 
     * @param ui UIManager Pointer
     * @param tft TFT Display Pointer
     * @param battery BatteryMonitor Pointer
     * @param powerMgr PowerManager Pointer
     * @return true bei Erfolg
     */
    bool init(UIManager* ui, TFT_eSPI* tft, BatteryMonitor* battery, PowerManager* powerMgr);

    /**
     * PageManager setzen (einmalig beim Setup)
     * @param pm UIPageManager Pointer
     */
    void setPageManager(UIPageManager* pm);

    /**
     * Seiten-Titel setzen (wird bei Page-Wechsel aufgerufen)
     * @param title Neuer Titel
     */
    void setPageTitle(const char* title);

    /**
     * Zurück-Button anzeigen/verstecken
     * @param show true = anzeigen, false = verstecken
     * @param targetPageId Ziel-Seite beim Klick (-1 = kein Ziel)
     */
    void showBackButton(bool show, int targetPageId = -1);

    /**
     * Battery-Icon aktualisieren (alle 2 Sekunden in loop() aufrufen)
     */
    void updateBatteryIcon();

    /**
     * Footer-Text setzen
     * @param text Neuer Footer-Text
     */
    void setFooterText(const char* text);

    /**
     * Header komplett neu zeichnen (bei Bedarf)
     */
    void redrawHeader();

    /**
     * Footer komplett neu zeichnen (bei Bedarf)
     */
    void redrawFooter();

    /**
     * Content-Bereich löschen (bei Page-Wechsel)
     */
    void clearContentArea();

    /**
     * Layout-Konstanten für Pages
     * (DISPLAY_WIDTH/HEIGHT aus config.h verwenden!)
     */
    static const int16_t HEADER_HEIGHT = 40;
    static const int16_t FOOTER_HEIGHT = 20;
    static const int16_t CONTENT_Y = 40;
    static const int16_t CONTENT_HEIGHT = 260;  // DISPLAY_HEIGHT - HEADER - FOOTER

private:
    UIManager* ui;                  // UI-Manager
    TFT_eSPI* tft;                  // Display
    BatteryMonitor* battery;        // Battery Monitor
    PowerManager* powerMgr;         // Power Manager
    UIPageManager* pageManager;     // Page Manager (für Back-Button Navigation)
    
    // Global UI Elemente
    UILabel* lblHeaderTitle;        // Seiten-Titel (zentriert)
    UILabel* lblBatteryIcon;        // Battery-Icon (rechts oben)
    UIButton* btnBack;              // Zurück-Button (links oben)
    UIButton* btnSleep;             // Sleep-Button (rechts, vor Battery)
    UILabel* lblFooter;             // Footer-Text (zentriert)
    
    bool initialized;               // Initialisierungs-Flag
    
    /**
     * Header Hintergrund zeichnen
     */
    void drawHeaderBackground();
    
    /**
     * Footer Hintergrund zeichnen
     */
    void drawFooterBackground();
    
    /**
     * Battery-Icon Farbe basierend auf Ladezustand
     */
    void updateBatteryIconColor(uint8_t percent);
    
    /**
     * Sleep-Button Callback
     */
    void onSleepButtonClicked();
};

#endif // GLOBAL_UI_H