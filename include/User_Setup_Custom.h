#ifndef USER_SETUP_CUSTOM_H
#define USER_SETUP_CUSTOM_H

// This file is intentionally minimal since all settings 
// are defined in platformio.ini build_flags

// Just ensure USER_SETUP_LOADED is set (should come from build_flags)
#ifndef USER_SETUP_LOADED
#define USER_SETUP_LOADED 1
#endif

// Pin connections (these won't be redefined)
#define TFT_MOSI 23
#define TFT_SCLK 18

// Don't define TFT_CS - we handle manually
// #define TFT_CS   5

#endif // USER_SETUP_CUSTOM_H