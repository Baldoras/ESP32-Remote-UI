/**
 * Globals.cpp
 * 
 * Globale Objekt-Instanzen
 * Alle Includes hier (nicht in Globals.h!)
 * 
 * ACHTUNG: PageManager benötigt Konstruktor-Parameter!
 * Wird in setup() initialisiert, hier nur Platzhalter
 */

#include "Globals.h"
#include "DisplayHandler.h"
#include "PageManager.h"
#include "BatteryMonitor.h"
#include "JoystickHandler.h"
#include "SDCardHandler.h"
#include "TouchManager.h"
#include "UserConfig.h"
#include "PowerManager.h"
#include "ESPNowManager.h"

// Globale Instanzen
DisplayHandler display;
BatteryMonitor battery;
JoystickHandler joystick;
SDCardHandler sdCard;
TouchManager touch;
UserConfig userConfig;
PowerManager powerMgr;
EspNowManager espNow;

// PageManager als Pointer (wird in setup() erstellt)
PageManager* pageManager = nullptr;