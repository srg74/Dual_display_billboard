#pragma once

#include <Arduino.h>
#include <LittleFS.h>
#include <TJpg_Decoder.h>
#include "display_manager.h"

class RotationTester {
private:
    DisplayManager* displayManager;
    String getFirstAvailableImage();
    
public:
    RotationTester(DisplayManager* dm);
    ~RotationTester();
    
    bool begin();
    bool testRotation(uint8_t rotation, uint8_t displayNum = 1);
    String getWebInterface();
};