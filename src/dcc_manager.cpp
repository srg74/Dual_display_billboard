/**
 * @file dcc_manager.cpp
 * @brief Digital Command Control (DCC) interface implementation for model railway integration
 * 
 * This module implements a DCC accessory decoder that integrates the dual display billboard
 * with model railway DCC control systems. It provides seamless switching between gallery
 * slideshow and clock display modes based on DCC turnout commands.
 * 
 * Features:
 * - Standard DCC accessory decoder protocol compliance using NmraDcc library
 * - Configurable DCC address (1-2048) with address validation and filtering
 * - Configurable GPIO pin for DCC signal input with internal pull-up configuration
 * - Automatic display mode switching: Direction 0 = Gallery, Direction 1 = Clock
 * - Real-time configuration updates with automatic settings persistence
 * - Robust error handling and comprehensive logging for debugging
 * - Static callback integration required by NmraDcc C library constraints
 * 
 * Technical Implementation:
 * - Uses NmraDcc library for reliable DCC signal decoding and protocol compliance
 * - Implements singleton pattern for C callback integration requirements
 * - Provides immediate configuration persistence through SettingsManager
 * - Coordinates with SlideshowManager for seamless display mode transitions
 * 
 * @author Dual Display Billboard Project
 * @version 0.9
 * @date August 2025
 */

#include "dcc_manager.h"
#include "settings_manager.h"
#include "slideshow_manager.h"
#include "logger.h"

// ============================================================================
// STATIC MEMBER INITIALIZATION
// ============================================================================
const char* DCCManager::TAG = "DCC";           ///< Logging component identifier
DCCManager* DCCManager::instance = nullptr;    ///< Static instance for C callback integration

// ============================================================================
// CONSTRUCTOR AND LIFECYCLE MANAGEMENT
// ============================================================================

/**
 * @brief Constructor - initializes DCC manager with component dependencies
 * @param sm Pointer to SettingsManager for configuration persistence
 * @param slideshow Pointer to SlideshowManager for display mode control
 */
DCCManager::DCCManager(SettingsManager* sm, SlideshowManager* slideshow) {
    // Store component dependencies
    settingsManager = sm;
    slideshowManager = slideshow;
    
    // Initialize configuration with safe defaults
    enabled = false;        // DCC disabled by default for safety
    address = 101;          // Default DCC address (safe mid-range value)
    pin = 4;               // Default GPIO pin (commonly available)
    initialized = false;    // Not initialized until begin() is called
    
    // Initialize state management
    currentState = false;           // Default to gallery mode (deactivated state)
    lastCommandTime = 0;            // No commands received yet
    
    // Set static instance for C callback integration (required by NmraDcc library)
    instance = this;
}

/**
 * @brief Initialize DCC decoder with current configuration settings
 * @return true if initialization successful, false on failure
 */
bool DCCManager::begin() {
    // Validate settings manager dependency
    if (!settingsManager) {
        LOG_ERROR(TAG, "SettingsManager not available - cannot load DCC configuration");
        return false;
    }
    
    // Load current configuration from persistent settings
    enabled = settingsManager->isDCCEnabled();
    address = settingsManager->getDCCAddress();
    pin = settingsManager->getDCCPin();
    
    // Log initialization status for debugging
    LOG_INFOF(TAG, "DCC Manager initializing...");
    LOG_INFOF(TAG, "Enabled: %s", enabled ? "yes" : "no");
    LOG_INFOF(TAG, "Address: %d", address);
    LOG_INFOF(TAG, "GPIO Pin: %d", pin);
    
    // Start DCC decoder if enabled
    if (enabled) {
        restart();  // Initialize hardware and start processing
        return true;
    }
    
    // Report successful initialization even when disabled
    LOG_INFO(TAG, "DCC Manager initialized (disabled)");
    return true;
}

/**
 * @brief Process DCC signals in main application loop
 * 
 * Must be called regularly from main loop to process incoming DCC signals.
 * Only processes when DCC interface is enabled and properly initialized.
 */
void DCCManager::loop() {
    if (enabled && initialized) {
        dcc.process();  // Process incoming DCC signals through NmraDcc library
    }
}

// ============================================================================
// CONFIGURATION MANAGEMENT METHODS
// ============================================================================

/**
 * @brief Restart DCC decoder with current configuration
 * 
 * Stops current processing, reconfigures hardware, and restarts DCC signal processing.
 * Used when configuration changes or during initial setup.
 */
void DCCManager::restart() {
    // Stop processing if currently disabled
    if (!enabled) {
        if (initialized) {
            LOG_INFO(TAG, "Stopping DCC decoder");
            initialized = false;
        }
        return;
    }
    
    LOG_INFOF(TAG, "Starting DCC decoder on pin %d with address %d", pin, address);
    
    // Configure GPIO pin for DCC signal input with pull-up resistor
    pinMode(pin, INPUT_PULLUP);
    
    // Initialize NmraDcc library with manufacturer and version information
    dcc.pin(0, pin, 0);  // Pin 0 = DCC signal input, pin 1 = not used (0)
    dcc.init(MAN_ID_DIY, 1, FLAGS_AUTO_FACTORY_DEFAULT, 0);
    
    // Validate DCC address range for accessory decoders
    if (address >= 1 && address <= 2048) {
        // NmraDcc library expects 0-based addressing for accessories
    LOG_INFOF(TAG, "Setting DCC accessory address to %d", address);
        initialized = true;
    } else {
        LOG_ERRORF(TAG, "Invalid DCC address: %d (must be 1-2048)", address);
        return;
    }
    
    LOG_INFO(TAG, "DCC decoder started successfully");
}

/**
 * @brief Enable or disable DCC interface processing
 * @param enable true to enable DCC processing, false to disable
 */
void DCCManager::setEnabled(bool enable) {
    if (enabled != enable) {
        enabled = enable;
        
        // Persist configuration change
        if (settingsManager) {
            settingsManager->setDCCEnabled(enable);
        }
        
        // Apply configuration change immediately
        if (enable) {
            restart();  // Start DCC processing
        } else {
            initialized = false;
            LOG_INFO(TAG, "DCC decoder disabled");
        }
    }
}

/**
 * @brief Get current DCC interface enable state
 * @return true if DCC interface is enabled, false if disabled
 */
bool DCCManager::isEnabled() {
    return enabled;
}

/**
 * @brief Set DCC accessory decoder address
 * @param addr DCC address for this decoder (valid range: 1-2048)
 */
void DCCManager::setAddress(int addr) {
    // Validate address range and check for actual change
    if (addr >= 1 && addr <= 2048 && addr != address) {
        address = addr;
        
        // Persist configuration change
        if (settingsManager) {
            settingsManager->setDCCAddress(addr);
        }
        
    LOG_INFOF(TAG, "DCC address changed to %d", addr);
        
        // Restart decoder with new address if currently enabled
        if (enabled) {
            restart();
        }
    }
}

/**
 * @brief Get current DCC decoder address
 * @return Currently configured DCC address
 */
int DCCManager::getAddress() {
    return address;
}

/**
 * @brief Set GPIO pin for DCC signal input
 * @param pinNumber GPIO pin number for DCC signal (valid range: 0-39)
 */
void DCCManager::setPin(int pinNumber) {
    // Validate pin range and check for actual change
    if (pinNumber >= 0 && pinNumber <= 39 && pinNumber != pin) {
        pin = pinNumber;
        
        // Persist configuration change
        if (settingsManager) {
            settingsManager->setDCCPin(pinNumber);
        }
        
        LOG_INFOF(TAG, "DCC GPIO pin changed to %d", pin);
        
        // Restart decoder with new pin configuration if currently enabled
        if (enabled) {
            restart();
        }
    }
}

/**
 * @brief Get current DCC signal GPIO pin
 * @return Currently configured GPIO pin number
 */
int DCCManager::getPin() {
    return pin;
}

// ============================================================================
// STATE MANAGEMENT METHODS
// ============================================================================

/**
 * @brief Get current DCC command state
 * @return true if last command was activate (clock mode), false if deactivate (gallery mode)
 */
bool DCCManager::getCurrentState() {
    return currentState;
}

/**
 * @brief Manually set DCC state (for testing or manual control)
 * @param state true for activate/clock mode, false for deactivate/gallery mode
 */
void DCCManager::setState(bool state) {
    currentState = state;
    lastCommandTime = millis();  // Update command timestamp
    
    LOG_INFOF(TAG, "DCC state changed to: %s", state ? "activated" : "deactivated");
    
    // Trigger appropriate display mode based on state
    if (state) {
        switchToClock();    // Activated state = clock mode
    } else {
        switchToGallery();  // Deactivated state = gallery mode
    }
}

/**
 * @brief Check if DCC decoder is properly initialized
 * @return true if decoder is initialized and ready to process commands
 */
bool DCCManager::isInitialized() {
    return initialized;
}

// ============================================================================
// PRIVATE COMMAND PROCESSING METHODS
// ============================================================================

/**
 * @brief Process received DCC command and update system state
 * @param addr DCC address of received command
 * @param activate Command direction (true=thrown/clock, false=closed/gallery)
 */
void DCCManager::handleDCCCommand(uint16_t addr, bool activate) {
    LOG_INFOF(TAG, "DCC command received - Address: %d, Activate: %s", addr, activate ? "true" : "false");
    
    // Validate command is for our configured address
    if (addr == address) {
        setState(activate);  // Process command and update display mode
    } else {
    LOG_INFOF(TAG, "DCC command ignored (address %d != our address %d)", addr, address);
    }
}

/**
 * @brief Switch display system to gallery slideshow mode
 * 
 * Activates slideshow manager to begin image gallery display.
 * Called when DCC command direction is 0 (closed/straight).
 */
void DCCManager::switchToGallery() {
    LOG_INFO(TAG, "Switching to gallery mode");
    
    if (slideshowManager) {
        // Start slideshow to activate gallery mode
        slideshowManager->startSlideshow();
    LOG_INFO(TAG, "Gallery mode activated - slideshow started");
    } else {
        LOG_WARN(TAG, "SlideshowManager not available for gallery mode");
    }
}

/**
 * @brief Switch display system to clock display mode
 * 
 * Activates clock display through slideshow manager coordination.
 * Called when DCC command direction is 1 (thrown/diverging).
 */
void DCCManager::switchToClock() {
    LOG_INFO(TAG, "Switching to clock mode");
    
    if (slideshowManager) {
        // Force clock display mode
        slideshowManager->showClock();
    LOG_INFO(TAG, "Clock mode activated - clock displayed");
    } else {
        LOG_WARN(TAG, "SlideshowManager not available for clock mode");
    }
}

// ============================================================================
// STATIC CALLBACK METHODS - NMRADCC LIBRARY INTEGRATION
// ============================================================================

/**
 * @brief Static callback function for NmraDcc library turnout notifications
 * @param Addr DCC accessory address of received command
 * @param Direction Command direction (0=closed, 1=thrown)
 * @param OutputPower Command power state (not used in implementation)
 * 
 * Required static callback to interface with NmraDcc C library.
 * Routes DCC commands to appropriate instance method for processing.
 */
void DCCManager::notifyDccAccTurnoutOutput(uint16_t Addr, uint8_t Direction, uint8_t OutputPower) {
    // Validate instance exists and DCC is enabled and initialized
    if (instance && instance->isEnabled() && instance->isInitialized()) {
        // Convert DCC turnout command to our activate/deactivate logic
        bool activate = (Direction == 1);  // 1 = thrown/activated, 0 = closed/deactivated
        instance->handleDCCCommand(Addr, activate);
    }
}

// ============================================================================
// C LIBRARY INTEGRATION - EXTERNAL CALLBACK REGISTRATION
// ============================================================================

/**
 * @brief C-compatible callback function required by NmraDcc library
 * 
 * NmraDcc library requires a C-style callback function that cannot be a class member.
 * This external C function routes callbacks to our static class method.
 */
extern "C" {
    void notifyDccAccTurnoutOutput(uint16_t Addr, uint8_t Direction, uint8_t OutputPower) {
        DCCManager::notifyDccAccTurnoutOutput(Addr, Direction, OutputPower);
    }
}
