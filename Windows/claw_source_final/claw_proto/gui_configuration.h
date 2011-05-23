
#ifndef GUI_CONFIGURATION_H
#define GUI_CONFIGURATION_H

#define GUI_BUILD_UPGRADE_WINDOW 1
#define GUI_BUILD_MAP_WINDOW 1
#define GUI_BUILD_LOG_WINDOW 1
#define GUI_BUILD_STORAGE_WINDOW 1
//#define GUI_BUILD_INGAME_GUI 1
#define GUI_BUILD_WEAPON_SELECTION_WINDOW 1

#ifdef PROJECT_CLAW_PROTO
// At least claw doesn't compile without this define after jukka's change.. survivor probably doesn't either,
// but I leave this ifdeffed so that I won't break anything for sure -harri
#	define GUI_BUILD_INGAME_GUI_TABS
#endif

#endif

