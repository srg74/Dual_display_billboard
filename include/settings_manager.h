#pragma once
#include <Arduino.h>
#include <LittleFS.h>
#include "logger.h"

class SettingsManager {
private:
    // Settings file paths
    static const char* SECOND_DISPLAY_FILE;
    static const char* DCC_ENABLED_FILE;
    static const char* IMAGE_INTERVAL_FILE;
    static const char* IMAGE_ENABLED_FILE;
    static const char* BRIGHTNESS_FILE;
    static const char* CLOCK_ENABLED_FILE;
    
    // Current settings values
    bool secondDisplayEnabled;
    bool dccEnabled;
    int imageInterval;
    bool imageEnabled;
    int brightness;
    bool clockEnabled;
    
    // Private helper methods
    bool saveBoolean(const char* filename, bool value);
    bool loadBoolean(const char* filename, bool defaultValue);
    bool saveInteger(const char* filename, int value);
    int loadInteger(const char* filename, int defaultValue);

public:
    SettingsManager();
    
    // Initialization
    bool begin();
    
    // Second Display settings
    void setSecondDisplayEnabled(bool enabled);
    bool isSecondDisplayEnabled();
    
    // DCC settings
    void setDCCEnabled(bool enabled);
    bool isDCCEnabled();
    
    // Image settings
    void setImageInterval(int seconds);
    int getImageInterval();
    void setImageEnabled(bool enabled);
    bool isImageEnabled();
    
    // Brightness settings
    void setBrightness(int value);
    int getBrightness();
    
    // Clock settings
    void setClockEnabled(bool enabled);
    bool isClockEnabled();
    
    // Utility
    void printSettings();
    void resetToDefaults();
};
