#include "dcc_manager.h"
#include "settings_manager.h"
#include "slideshow_manager.h"
#include "logger.h"

const char* DCCManager::TAG = "DCC";
DCCManager* DCCManager::instance = nullptr;

DCCManager::DCCManager(SettingsManager* sm, SlideshowManager* slideshow) {
    settingsManager = sm;
    slideshowManager = slideshow;
    enabled = false;
    address = 101;
    pin = 4;
    initialized = false;
    currentState = false;
    lastCommandTime = 0;
    
    // Set static instance for callbacks
    instance = this;
}

bool DCCManager::begin() {
    if (!settingsManager) {
        LOG_ERROR(TAG, "❌ SettingsManager not available");
        return false;
    }
    
    // Load settings
    enabled = settingsManager->isDCCEnabled();
    address = settingsManager->getDCCAddress();
    pin = settingsManager->getDCCPin();
    
    LOG_INFOF(TAG, "🚂 DCC Manager initializing...");
    LOG_INFOF(TAG, "🚂 Enabled: %s", enabled ? "yes" : "no");
    LOG_INFOF(TAG, "🚂 Address: %d", address);
    LOG_INFOF(TAG, "🚂 GPIO Pin: %d", pin);
    
    if (enabled) {
        restart();
        return true;
    }
    
    LOG_INFO(TAG, "✅ DCC Manager initialized (disabled)");
    return true;
}

void DCCManager::loop() {
    if (enabled && initialized) {
        dcc.process();
    }
}

void DCCManager::restart() {
    if (!enabled) {
        if (initialized) {
            LOG_INFO(TAG, "🚂 Stopping DCC decoder");
            initialized = false;
        }
        return;
    }
    
    LOG_INFOF(TAG, "🚂 Starting DCC decoder on pin %d with address %d", pin, address);
    
    // Configure DCC pin
    pinMode(pin, INPUT_PULLUP);
    
    // Setup DCC decoder
    dcc.pin(0, pin, 0); // Pin 0 = DCC signal, pin 1 = not used
    dcc.init(MAN_ID_DIY, 1, FLAGS_AUTO_FACTORY_DEFAULT, 0);
    
    // Set the accessory decoder address
    if (address >= 1 && address <= 2048) {
        // NmraDcc library expects 0-based addressing for accessories
        LOG_INFOF(TAG, "🚂 Setting DCC accessory address to %d", address);
        initialized = true;
    } else {
        LOG_ERRORF(TAG, "❌ Invalid DCC address: %d (must be 1-2048)", address);
        return;
    }
    
    LOG_INFO(TAG, "✅ DCC decoder started successfully");
}

void DCCManager::setEnabled(bool enable) {
    if (enabled != enable) {
        enabled = enable;
        if (settingsManager) {
            settingsManager->setDCCEnabled(enable);
        }
        
        if (enable) {
            restart();
        } else {
            initialized = false;
            LOG_INFO(TAG, "🚂 DCC decoder disabled");
        }
    }
}

bool DCCManager::isEnabled() {
    return enabled;
}

void DCCManager::setAddress(int addr) {
    if (addr >= 1 && addr <= 2048 && addr != address) {
        address = addr;
        if (settingsManager) {
            settingsManager->setDCCAddress(addr);
        }
        LOG_INFOF(TAG, "🚂 DCC address changed to %d", addr);
        
        if (enabled) {
            restart();
        }
    }
}

int DCCManager::getAddress() {
    return address;
}

void DCCManager::setPin(int pinNumber) {
    if (pinNumber >= 0 && pinNumber <= 39 && pinNumber != pin) {
        pin = pinNumber;
        if (settingsManager) {
            settingsManager->setDCCPin(pinNumber);
        }
        LOG_INFOF(TAG, "🚂 DCC GPIO pin changed to %d", pin);
        
        if (enabled) {
            restart();
        }
    }
}

int DCCManager::getPin() {
    return pin;
}

bool DCCManager::getCurrentState() {
    return currentState;
}

void DCCManager::setState(bool state) {
    currentState = state;
    lastCommandTime = millis();
    
    LOG_INFOF(TAG, "🚂 DCC state changed to: %s", state ? "activated" : "deactivated");
    
    if (state) {
        switchToClock();
    } else {
        switchToGallery();
    }
}

bool DCCManager::isInitialized() {
    return initialized;
}

void DCCManager::handleDCCCommand(uint16_t addr, bool activate) {
    LOG_INFOF(TAG, "🚂 DCC command received - Address: %d, Activate: %s", addr, activate ? "true" : "false");
    
    if (addr == address) {
        setState(activate);
    } else {
        LOG_INFOF(TAG, "🚂 DCC command ignored (address %d != our address %d)", addr, address);
    }
}

void DCCManager::switchToGallery() {
    LOG_INFO(TAG, "🚂 Switching to gallery mode");
    
    if (slideshowManager) {
        // Force the slideshow to start (gallery mode)
        slideshowManager->startSlideshow();
        LOG_INFO(TAG, "🚂 Gallery mode activated - slideshow started");
    } else {
        LOG_WARN(TAG, "⚠️ SlideshowManager not available for gallery mode");
    }
}

void DCCManager::switchToClock() {
    LOG_INFO(TAG, "🚂 Switching to clock mode");
    
    if (slideshowManager) {
        // Force clock display
        slideshowManager->showClock();
        LOG_INFO(TAG, "🚂 Clock mode activated - clock displayed");
    } else {
        LOG_WARN(TAG, "⚠️ SlideshowManager not available for clock mode");
    }
}

// Static callback function for NmraDcc library
void DCCManager::notifyDccAccTurnoutOutput(uint16_t Addr, uint8_t Direction, uint8_t OutputPower) {
    if (instance && instance->isEnabled() && instance->isInitialized()) {
        // Convert DCC turnout command to our activate/deactivate logic
        bool activate = (Direction == 1); // 1 = thrown/activated, 0 = closed/deactivated
        instance->handleDCCCommand(Addr, activate);
    }
}

// Register the callback with NmraDcc library
// This needs to be done outside the class
extern "C" {
    void notifyDccAccTurnoutOutput(uint16_t Addr, uint8_t Direction, uint8_t OutputPower) {
        DCCManager::notifyDccAccTurnoutOutput(Addr, Direction, OutputPower);
    }
}
