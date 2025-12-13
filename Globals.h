/**
 * Globals.h
 * 
 * Globale Objekt-Definitionen
 * 
 * WICHTIG: Nur Forward Declarations hier!
 * Includes nur in Globals.cpp
 */

#pragma once

// Forward Declarations (keine Includes!)
class DisplayHandler;
class UIPageManager;
class BatteryMonitor;
class JoystickHandler;
class SDCardHandler;
class TouchManager;
class UserConfig;
class PowerManager;

// Globale Objekte (extern)
extern DisplayHandler display;
extern UIPageManager pageManager;
extern BatteryMonitor battery;
extern JoystickHandler joystick;
extern SDCardHandler sdCard;
extern TouchManager touch;
extern UserConfig userConfig;
extern PowerManager powerMgr;

// Seiten-IDs
enum PageID {
    PAGE_HOME = 0,
    PAGE_REMOTE = 1,
    PAGE_CONNECTION = 2,
    PAGE_SETTINGS = 3,
    PAGE_INFO = 4
};