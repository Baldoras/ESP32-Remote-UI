/**
 * PowerManager.h
 * 
 * Power-Management für ESP32-S3
 * 
 * Features:
 * - Deep-Sleep mit Wake-Up via Touch/Timer
 * - Auto-Sleep bei kritischer Batterie
 * - Backlight Fade-Out
 * - Before-Sleep Callback
 * - Wake-Up Reason Detection
 */

#ifndef POWER_MANAGER_H
#define POWER_MANAGER_H

#include <Arduino.h>
#include <esp_sleep.h>
#include "setupConf.h"
#include "BatteryMonitor.h"
#include "DisplayHandler.h"

// Wake-Up Quellen
enum class WakeSource : uint8_t {
    NONE = 0,           // Permanent Power-Off
    TOUCH,              // Wake via Touch (GPIO6)
    TIMER,              // Wake via Timer
    TOUCH_AND_TIMER     // Beide Quellen
};

// Before-Sleep Callback
typedef void (*BeforeSleepCallback)();

class PowerManager {
public:
    /**
     * Konstruktor
     */
    PowerManager();
    
    /**
     * Destruktor
     */
    ~PowerManager();

    /**
     * Power-Manager initialisieren
     * @param batteryMon BatteryMonitor Pointer
     * @param displayHdlr DisplayHandler Pointer
     * @return true bei Erfolg
     */
    bool begin(BatteryMonitor* batteryMon, DisplayHandler* displayHdlr);

    /**
     * Deep-Sleep aktivieren
     * @param wakeSource Wake-Up Quelle
     * @param timerSeconds Timer in Sekunden (nur bei TIMER/TOUCH_AND_TIMER)
     */
    void sleep(WakeSource wakeSource = WakeSource::TOUCH, uint32_t timerSeconds = 0);

    /**
     * Permanent ausschalten (kein Wake-Up)
     */
    void powerOff();

    /**
     * Soft-Reset (Neustart)
     */
    void restart();

    /**
     * Auto-Sleep bei kritischer Batterie konfigurieren
     * @param enabled Aktiviert?
     * @param wakeSource Wake-Up Quelle für Auto-Sleep
     * @param timerSeconds Timer für Auto-Sleep (optional)
     */
    void setAutoSleepOnCritical(bool enabled, WakeSource wakeSource = WakeSource::TOUCH, uint32_t timerSeconds = 0);

    /**
     * Before-Sleep Callback setzen
     * @param callback Funktion die vor Sleep aufgerufen wird
     */
    void setBeforeSleepCallback(BeforeSleepCallback callback);

    /**
     * Fade-Out Zeit für Backlight setzen
     * @param ms Millisekunden (0 = sofort aus)
     */
    void setFadeTime(uint32_t ms) { fadeTimeMs = ms; }

    /**
     * Wake-Up Grund abrufen (nach Neustart)
     * @return Wake-Up Grund als String
     */
    String getWakeupReason();

    /**
     * Update-Schleife (in loop() aufrufen!)
     * Prüft Auto-Sleep Bedingungen
     */
    void update();

    /**
     * Ist Auto-Sleep aktiv?
     */
    bool isAutoSleepEnabled() const { return autoSleepEnabled; }

private:
    BatteryMonitor* battery;
    DisplayHandler* display;
    
    bool initialized;
    bool autoSleepEnabled;
    WakeSource autoSleepWakeSource;
    uint32_t autoSleepTimer;
    uint32_t fadeTimeMs;
    
    BeforeSleepCallback beforeSleepCallback;
    
    // Critical Battery Warnung
    bool criticalWarningShown;
    unsigned long criticalWarningStart;
    static const uint32_t CRITICAL_WARNING_DURATION = 5000;  // 5 Sekunden Warnung
    
    /**
     * Display und Peripherie herunterfahren
     */
    void shutdownPeripherals();
    
    /**
     * Backlight sanft ausblenden
     */
    void fadeBacklight();
    
    /**
     * Wake-Up Quellen konfigurieren
     */
    void configureWakeup(WakeSource wakeSource, uint32_t timerSeconds);
};

#endif // POWER_MANAGER_H