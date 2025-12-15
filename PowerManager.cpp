/**
 * PowerManager.cpp
 * 
 * Implementation des Power-Managers
 */

#include "PowerManager.h"

PowerManager::PowerManager()
    : battery(nullptr)
    , display(nullptr)
    , initialized(false)
    , autoSleepEnabled(false)
    , autoSleepWakeSource(WakeSource::TOUCH)
    , autoSleepTimer(0)
    , fadeTimeMs(1000)
    , beforeSleepCallback(nullptr)
    , criticalWarningShown(false)
    , criticalWarningStart(0)
{
}

PowerManager::~PowerManager() {
}

bool PowerManager::begin(BatteryMonitor* batteryMon, DisplayHandler* displayHdlr) {
    if (!batteryMon || !displayHdlr) {
        DEBUG_PRINTLN("PowerManager: ❌ Ungültige Parameter!");
        return false;
    }
    
    battery = batteryMon;
    display = displayHdlr;
    
    DEBUG_PRINTLN("PowerManager: Initialisiere...");
    
    // Wake-Up Grund ausgeben
    String wakeupReason = getWakeupReason();
    DEBUG_PRINTF("PowerManager: Wake-Up Grund: %s\n", wakeupReason.c_str());
    
    initialized = true;
    
    DEBUG_PRINTLN("PowerManager: ✅ Initialisiert");
    
    return true;
}

void PowerManager::sleep(WakeSource wakeSource, uint32_t timerSeconds) {
    if (!initialized) {
        DEBUG_PRINTLN("PowerManager: ❌ Nicht initialisiert!");
        return;
    }
    
    DEBUG_PRINTLN("\n╔════════════════════════════════════════╗");
    DEBUG_PRINTLN("║       ENTERING SLEEP MODE              ║");
    DEBUG_PRINTLN("╚════════════════════════════════════════╝");
    
    // Before-Sleep Callback
    if (beforeSleepCallback != nullptr) {
        DEBUG_PRINTLN("PowerManager: Führe Before-Sleep Callback aus...");
        beforeSleepCallback();
    }
    
    // Backlight ausblenden
    if (display && fadeTimeMs > 0) {
        DEBUG_PRINTLN("PowerManager: Fade-Out Backlight...");
        fadeBacklight();
    }
    
    // Peripherie herunterfahren
    DEBUG_PRINTLN("PowerManager: Shutdown Peripherals...");
    shutdownPeripherals();
    
    // Wake-Up Quellen konfigurieren
    DEBUG_PRINTLN("PowerManager: Konfiguriere Wake-Up...");
    configureWakeup(wakeSource, timerSeconds);
    
    DEBUG_PRINTLN("PowerManager: ✅ Entering Deep-Sleep NOW!");
    delay(100);  // Zeit für letzte Serial-Ausgabe
    
    // Deep-Sleep!
    esp_deep_sleep_start();
}

void PowerManager::powerOff() {
    DEBUG_PRINTLN("PowerManager: ⚠️ PERMANENT POWER-OFF!");
    sleep(WakeSource::NONE, 0);
}

void PowerManager::restart() {
    DEBUG_PRINTLN("PowerManager: 🔄 RESTART!");
    
    // Before-Sleep Callback
    if (beforeSleepCallback != nullptr) {
        beforeSleepCallback();
    }
    
    delay(500);
    ESP.restart();
}

void PowerManager::setAutoSleepOnCritical(bool enabled, WakeSource wakeSource, uint32_t timerSeconds) {
    autoSleepEnabled = enabled;
    autoSleepWakeSource = wakeSource;
    autoSleepTimer = timerSeconds;
    
    DEBUG_PRINTF("PowerManager: Auto-Sleep bei Critical Battery: %s\n", 
                 enabled ? "AKTIVIERT" : "DEAKTIVIERT");
    
    if (enabled) {
        DEBUG_PRINTF("  Wake-Source: %d, Timer: %lus\n", 
                     static_cast<uint8_t>(wakeSource), timerSeconds);
        
        // BatteryMonitor Auto-Shutdown deaktivieren (Konflikt vermeiden)
        if (battery) {
            battery->setAutoShutdown(false);
            DEBUG_PRINTLN("  BatteryMonitor Auto-Shutdown deaktiviert (PowerManager übernimmt)");
        }
    }
}

void PowerManager::setBeforeSleepCallback(BeforeSleepCallback callback) {
    beforeSleepCallback = callback;
}

String PowerManager::getWakeupReason() {
    esp_sleep_wakeup_cause_t wakeup_reason = esp_sleep_get_wakeup_cause();
    
    switch (wakeup_reason) {
        case ESP_SLEEP_WAKEUP_EXT0:
            return "EXT0 (Touch IRQ)";
        case ESP_SLEEP_WAKEUP_EXT1:
            return "EXT1 (Multiple GPIOs)";
        case ESP_SLEEP_WAKEUP_TIMER:
            return "Timer";
        case ESP_SLEEP_WAKEUP_TOUCHPAD:
            return "Touchpad";
        case ESP_SLEEP_WAKEUP_ULP:
            return "ULP";
        case ESP_SLEEP_WAKEUP_GPIO:
            return "GPIO";
        case ESP_SLEEP_WAKEUP_UART:
            return "UART";
        case ESP_SLEEP_WAKEUP_WIFI:
            return "WiFi";
        case ESP_SLEEP_WAKEUP_COCPU:
            return "COCPU";
        case ESP_SLEEP_WAKEUP_COCPU_TRAP_TRIG:
            return "COCPU Trap";
        case ESP_SLEEP_WAKEUP_BT:
            return "Bluetooth";
        default:
            return "Power-On / Reset";
    }
}

void PowerManager::update() {
    if (!initialized || !autoSleepEnabled || !battery) {
        return;
    }
    
    // Prüfen ob Batterie kritisch
    if (battery->isCritical()) {
        if (!criticalWarningShown) {
            // Erste Warnung
            DEBUG_PRINTLN("\n⚠️⚠️⚠️ CRITICAL BATTERY - AUTO-SLEEP IN 5s! ⚠️⚠️⚠️");
            DEBUG_PRINTF("Spannung: %.2fV\n", battery->getVoltage());
            
            criticalWarningShown = true;
            criticalWarningStart = millis();
        } else {
            // Prüfen ob Warnung-Zeit abgelaufen
            if (millis() - criticalWarningStart >= CRITICAL_WARNING_DURATION) {
                DEBUG_PRINTLN("PowerManager: Auto-Sleep wird ausgelöst!");
                sleep(autoSleepWakeSource, autoSleepTimer);
            }
        }
    } else {
        // Batterie wieder OK → Warnung zurücksetzen
        if (criticalWarningShown) {
            criticalWarningShown = false;
            DEBUG_PRINTLN("PowerManager: Critical Battery Warnung zurückgesetzt");
        }
    }
}

// ═══════════════════════════════════════════════════════════════════════════
// PRIVATE METHODEN
// ═══════════════════════════════════════════════════════════════════════════

void PowerManager::shutdownPeripherals() {
    // Display komplett löschen (schwarz) vor Sleep
    if (display) {
        DEBUG_PRINTLN("  Display löschen...");
        TFT_eSPI& tft = display->getTft();
        tft.fillScreen(TFT_BLACK);  // Komplett schwarz
        delay(50);  // Kurz warten bis geschrieben
        
        display->setBacklightOn(false);
    }
    
    DEBUG_PRINTLN("  Peripherals shutdown complete");
}

void PowerManager::fadeBacklight() {
    if (!display) return;
    
    uint8_t brightness = 255;  // Aktuell (angenommen)
    uint32_t startTime = millis();
    
    while (millis() - startTime < fadeTimeMs) {
        float progress = (float)(millis() - startTime) / (float)fadeTimeMs;
        uint8_t newBrightness = 255 * (1.0 - progress);
        
        display->setBacklight(newBrightness);
        delay(10);
    }
    
    display->setBacklightOn(false);
}

void PowerManager::configureWakeup(WakeSource wakeSource, uint32_t timerSeconds) {
    // ALLE Wake-Up Quellen zurücksetzen
    esp_sleep_disable_wakeup_source(ESP_SLEEP_WAKEUP_ALL);
    
    switch (wakeSource) {
        case WakeSource::NONE:
            DEBUG_PRINTLN("  Wake-Up: NONE (Permanent Off)");
            // Keine Wake-Up Quellen → Permanent aus (wie BatteryMonitor::shutdown())
            break;
            
        case WakeSource::TOUCH:
            DEBUG_PRINTLN("  Wake-Up: Touch IRQ (GPIO6)");
            // EXT0: Single GPIO, LOW-Trigger
            esp_sleep_enable_ext0_wakeup((gpio_num_t)TOUCH_IRQ, 0);  // 0 = LOW
            break;
            
        case WakeSource::TIMER:
            DEBUG_PRINTF("  Wake-Up: Timer (%lu seconds)\n", timerSeconds);
            if (timerSeconds > 0) {
                esp_sleep_enable_timer_wakeup(timerSeconds * 1000000ULL);  // Mikrosekunden
            }
            break;
            
        case WakeSource::TOUCH_AND_TIMER:
            DEBUG_PRINTF("  Wake-Up: Touch + Timer (%lu seconds)\n", timerSeconds);
            esp_sleep_enable_ext0_wakeup((gpio_num_t)TOUCH_IRQ, 0);
            if (timerSeconds > 0) {
                esp_sleep_enable_timer_wakeup(timerSeconds * 1000000ULL);
            }
            break;
    }
}