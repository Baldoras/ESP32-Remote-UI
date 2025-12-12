/*
*
* Global object definition
*
*/

#pragma once

#include "DisplayHandler.h"
#include "UIPageManager.h"
#include "BatteryMonitor.h"
#include "JoystickHandler.h"
#include "SDCardHandler.h"
#include "TouchManager.h"
#include "PowerManager.h"
#include "config.h"

extern DisplayHandler display;
extern UIPageManager pageManager;
extern BatteryMonitor battery;
extern JoystickHandler joystick;
extern SDCardHandler sdCard;
extern TouchManager touch;
extern PowerManager powerMgr;

// Seiten-IDs
enum PageID {
    PAGE_HOME = 0,
    PAGE_REMOTE = 1,
    PAGE_CONNECTION = 2,
    PAGE_SETTINGS = 3,
    PAGE_INFO = 4
};