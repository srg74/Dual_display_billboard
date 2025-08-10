#ifndef DCC_MANAGER_H
#define DCC_MANAGER_H

#include <Arduino.h>
#include <NmraDcc.h>

class SettingsManager;
class SlideshowManager;

class DCCManager {
private:
    static const char* TAG;
    
    // DCC library instance
    NmraDcc dcc;
    
    // Configuration
    bool enabled;
    int address;
    int pin;
    bool initialized;
    
    // State management
    bool currentState; // true = activated, false = deactivated
    unsigned long lastCommandTime;
    
    // Component dependencies
    SettingsManager* settingsManager;
    SlideshowManager* slideshowManager;
    
    static DCCManager* instance; // Static instance for callbacks
    
    // Internal methods
    void handleDCCCommand(uint16_t addr, bool activate);
    void switchToGallery();
    void switchToClock();

public:
    DCCManager(SettingsManager* sm, SlideshowManager* slideshow);
    
    bool begin();
    void loop();
    
    // DCC callback functions (must be static for NmraDcc library)
    static void notifyDccAccTurnoutOutput(uint16_t Addr, uint8_t Direction, uint8_t OutputPower);
    
    // Configuration methods
    void setEnabled(bool enable);
    bool isEnabled();
    void setAddress(int addr);
    int getAddress();
    void setPin(int pinNumber);
    int getPin();
    
    // State methods
    bool getCurrentState();
    void setState(bool state);
    
    // Utility methods
    bool isInitialized();
    void restart(); // Restart DCC with new settings
};

#endif // DCC_MANAGER_H
