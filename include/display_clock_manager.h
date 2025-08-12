#pragma once
#include <Arduino.h>
#include <TFT_eSPI.h>
#include <time.h>
#include "clock_types.h"
#include "display_manager.h"
#include "time_manager.h"

// Clock display configuration constants
#define GALLERY_INTERVAL 30000         // 30 seconds - how long gallery runs before showing clock  
#define CLOCK_DISPLAY_DURATION 5000    // 5 seconds - how long clock is displayed

class DisplayClockManager {
private:
    DisplayManager* displayManager;
    TimeManager* timeManager;
    
    // Clock state variables
    bool showClock;
    unsigned long clockDisplayStart;
    unsigned long galleryStartTime;
    
    int firstScreenCS;
    int secondScreenCS;
    bool enableSecondDisplay;
    
    ClockFaceType currentClockFace;  // Current selected clock face

    // Clock face implementations (Phase 2)
    void displayAnalogClock(TFT_eSPI& tft);           // Classic analog
    void displayDigitalClock(TFT_eSPI& tft);          // Digital modern
    void displayMinimalistClock(TFT_eSPI& tft);       // Minimalist
    void displayModernSquareClock(TFT_eSPI& tft);     // Modern Square

    void displayClockOnDisplay(TFT_eSPI& tft, int csPin);

public:
    DisplayClockManager(DisplayManager* dm, TimeManager* tm);
    
    bool begin();
    void begin(int firstCS, int secondCS);
    void displayClockOnBothDisplays();
    void setClockLabel(const String& label);
    String getClockLabel();
    void setSecondDisplayEnabled(bool enabled);
    void setDisplayPins(int firstCS, int secondCS);
    
    // Gallery integration
    unsigned long getGalleryElapsedTime();
    unsigned long getClockDisplayElapsedTime();
    void resetGalleryTimer();
    bool shouldShowClock();
    bool shouldHideClock();
    void startClockDisplay();
    void stopClockDisplay();
    bool isClockDisplayActive();
    void displayAnalogClockOnBothTFTs(TFT_eSPI& tft);
    
    // Clock face management (Phase 2)
    void setClockFace(ClockFaceType faceType);
    ClockFaceType getClockFace() const;
    String getClockFaceName(ClockFaceType faceType) const;
};
