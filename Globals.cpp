/**
 * Globals.cpp
 * 
 * Globale Objekt-Instanzen
 * Alle Includes hier (nicht in Globals.h!)
 * 
 * ACHTUNG: PageManager benötigt Konstruktor-Parameter!
 * Wird in setup() initialisiert, hier nur Platzhalter
 */

#include "include/Globals.h"
#include "include/DisplayHandler.h"
#include "include/PageManager.h"
#include "include/BatteryMonitor.h"
#include "include/JoystickHandler.h"
#include "include/SDCardHandler.h"
#include "include/TouchManager.h"
#include "include/UserConfig.h"
#include "include/PowerManager.h"
#include "include/ESPNowManager.h"

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