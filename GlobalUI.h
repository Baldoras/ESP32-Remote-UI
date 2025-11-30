/**
 * GlobalUI.h
 * 
 * Zentrale UI-Verwaltung - Header/Footer/Battery
 * 
 * WICHTIG: Nur EINE Instanz pro Projekt!
 * Verhindert Memory-Corruption durch Duplikate
 */

#ifndef GLOBAL_UI_H
#define GLOBAL_UI_H

#include <Arduino.h>
#include <TFT_eSPI.h>
#include "config.h"
#include "UIManager.h"
#include "UILabel.h"
#include "UIButton.h"
#include "BatteryMonitor.h"

// Forward Declaration
class UIPageManager;

/**
 * GlobalUI - Zentrale Verwaltung für Header/Footer/Battery
 */
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
     * GlobalUI initialisieren
     * @param ui UIManager Pointer
     * @param tft TFT_eSPI Pointer
     * @param battery BatteryMonitor Pointer
     * @return true bei Erfolg
     */
    bool init(UIManager* ui, TFT_eSPI* tft, BatteryMonitor* battery);

    /**
     * Seiten-Titel setzen (wird bei Page-Wechsel aufgerufen)
     * @param title Neuer Titel
     */
    void setPageTitle(const char* title);

    /**
     * Zurück-Button anzeigen/verstecken
     * @param show true = anzeigen, false = verstecken
     * @param pageManager PageManager Pointer (für Navigation)
     * @param targetPageId Ziel-Seite beim Klick
     */
    void showBackButton(bool show, UIPageManager* pageManager = nullptr, int targetPageId = -1);

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
     * Content-Bereich löschen (zwischen Header/Footer)
     */
    void clearContentArea();

    /**
     * UIManager abrufen
     */
    UIManager* getUIManager() { return ui; }

    /**
     * TFT abrufen
     */
    TFT_eSPI* getTFT() { return tft; }

    /**
     * Battery-Prozent abrufen
     */
    uint8_t getBatteryPercent();

    /**
     * Battery-Spannung abrufen
     */
    float getBatteryVoltage();

    /**
     * Header-Label abrufen (für direkten Zugriff)
     */
    UILabel* getHeaderLabel() { return headerLabel; }

    /**
     * Footer-Label abrufen (für direkten Zugriff)
     */
    UILabel* getFooterLabel() { return footerLabel; }

private:
    UIManager* ui;              // UI-Manager
    TFT_eSPI* tft;              // Display
    BatteryMonitor* battery;    // Battery-Monitor
    
    // UI-Elemente (nur hier erstellt!)
    UILabel* headerLabel;       // Header Label
    UILabel* footerLabel;       // Footer Label
    UILabel* batteryLabel;      // Battery-Prozent Text
    UIButton* backButton;       // Zurück-Button (optional)
    
    bool initialized;           // Init-Flag
    unsigned long lastBatteryUpdate;  // Letzte Battery-Aktualisierung
    
    /**
     * Header-Elemente erstellen
     */
    void createHeaderElements();
    
    /**
     * Footer-Elemente erstellen
     */
    void createFooterElements();
    
    /**
     * Battery-Icon zeichnen
     */
    void drawBatteryIcon(int16_t x, int16_t y, uint8_t percent, bool charging = false);
};

#endif // GLOBAL_UI_H
