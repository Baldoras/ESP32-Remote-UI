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
#include "include/LogHandler.h"
#include "include/TouchManager.h"
#include "include/UserConfig.h"
#include "include/PowerManager.h"
#include "include/ESPNowRemoteController.h"

// Globale Instanzen
DisplayHandler display;
BatteryMonitor battery;
JoystickHandler joystick;
SDCardHandler sdCard;
LogHandler logger(nullptr, LOG_INFO);  // Startet ohne SD, nur Serial, Level INFO
TouchManager touch;
UserConfig userConfig;
PowerManager powerMgr;
ESPNowRemoteController espNow;

// PageManager als Pointer (wird in setup() erstellt)
PageManager* pageManager = nullptr;