#pragma once

#include <Arduino.h>
#include <TFT_eSPI.h>
#include <time.h>
#include "display_manager.h"
#include "time_manager.h"

// Clock display configuration constants (matching external project)
#define GALLERY_INTERVAL 30000         // 30 seconds - how long gallery runs before showing clock  
#define CLOCK_DISPLAY_DURATION 5000    // 5 seconds - how long clock is displayed
#define LINE_HEIGHT 18                 // Line height for display text

class DisplayClockManager {
private:
    // Manager dependencies
    DisplayManager* displayManager;
    TimeManager* timeManager;
    
    // Clock state variables
    bool showClock;
    unsigned long clockDisplayStart;
    unsigned long galleryStartTime;
    
    // Display pins
    int firstScreenCS;
    int secondScreenCS;
    
    // Settings
    bool enableSecondDisplay;
    String clockLabel;
    
    // Internal helper method
    void displayClockOnDisplay(TFT_eSPI& tft, int csPin);
    
public:
    DisplayClockManager(DisplayManager* dm, TimeManager* tm);
    
    // Initialization
    bool begin();
    void begin(int firstCS, int secondCS);
    
    // Clock state management
    void resetGalleryTimer();
    bool shouldShowClock();
    bool shouldHideClock();
    void startClockDisplay();
    void stopClockDisplay();
    bool isClockDisplayActive();
    
    // Clock display functions
    void displayAnalogClockOnBothTFTs(TFT_eSPI& tft);
    void displayClockOnBothDisplays(); // New method for DisplayManager integration
    
    // Settings management
    void setClockLabel(const String& label);
    String getClockLabel();
    void setSecondDisplayEnabled(bool enabled);
    void setDisplayPins(int firstCS, int secondCS);
    
    // Timing functions
    unsigned long getGalleryElapsedTime();
    unsigned long getClockDisplayElapsedTime();
    
    // Clock intervals
    static unsigned long getGalleryInterval() { return GALLERY_INTERVAL; }
    static unsigned long getClockDisplayDuration() { return CLOCK_DISPLAY_DURATION; }
};

// Global instance declaration
extern DisplayClockManager clockManager;
