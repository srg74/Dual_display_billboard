/**
 * @file dcc_manager.h
 * @brief Digital Command Control (DCC) interface for model railway integration
 * 
 * This module provides DCC accessory decoder functionality for the dual display billboard,
 * enabling control from model railway DCC systems. Integrates with the NmraDcc library
 * to decode DCC commands and automatically switch between gallery slideshow and clock modes.
 * 
 * Key Features:
 * - NmraDcc library integration for standard DCC protocol compliance
 * - Configurable DCC address (1-2048) for accessory decoder identification
 * - Configurable GPIO pin for DCC signal input with pull-up configuration
 * - Automatic slideshow/clock mode switching based on DCC turnout commands
 * - Real-time configuration updates with settings persistence
 * - Static callback integration required by NmraDcc C library
 * - Command validation and address filtering for reliable operation
 * 
 * DCC Command Mapping:
 * - Direction 0 (Closed): Switch to gallery slideshow mode
 * - Direction 1 (Thrown): Switch to clock display mode
 * 
 * @author Dual Display Billboard Project
 * @version 1.0
 * @date August 2025
 */

#pragma once
#include <Arduino.h>
#include <NmraDcc.h>

// Forward declarations to avoid circular dependencies
class SettingsManager;
class SlideshowManager;

/**
 * @class DCCManager
 * @brief Digital Command Control accessory decoder for model railway integration
 * 
 * Implements a DCC accessory decoder that responds to turnout commands to control
 * display modes. Provides seamless integration between model railway DCC systems
 * and the dual display billboard functionality.
 * 
 * The manager operates as a standard DCC accessory decoder, listening for commands
 * on a configurable address. When commands are received, it automatically switches
 * the display system between gallery slideshow mode and clock display mode.
 * 
 * Technical Implementation:
 * - Uses NmraDcc library for robust DCC signal decoding
 * - Configurable GPIO pin with internal pull-up for reliable signal reception
 * - Static callback integration to meet NmraDcc C library requirements
 * - Address validation ensures commands target the correct decoder
 * - State management with command timestamp tracking
 */
class DCCManager {
private:
    // ============================================================================
    // PRIVATE CONSTANTS AND CONFIGURATION
    // ============================================================================
    static const char* TAG;              ///< Logging tag for component identification
    
    // ============================================================================
    // PRIVATE MEMBER VARIABLES - HARDWARE AND STATE
    // ============================================================================
    NmraDcc dcc;                         ///< NmraDcc library instance for signal decoding
    
    // Configuration state
    bool enabled;                        ///< DCC interface enable state
    int address;                         ///< DCC accessory decoder address (1-2048)
    int pin;                             ///< GPIO pin number for DCC signal input
    bool initialized;                    ///< Initialization state of DCC decoder
    
    // Runtime state management
    bool currentState;                   ///< Current decoder state (true=activated/clock, false=deactivated/gallery)
    unsigned long lastCommandTime;       ///< Timestamp of last received DCC command
    
    // ============================================================================
    // PRIVATE MEMBER VARIABLES - COMPONENT DEPENDENCIES
    // ============================================================================
    SettingsManager* settingsManager;   ///< Reference to configuration persistence manager
    SlideshowManager* slideshowManager; ///< Reference to slideshow control manager
    
    static DCCManager* instance;         ///< Static instance pointer for C callback integration
    
    // ============================================================================
    // PRIVATE METHODS - INTERNAL COMMAND PROCESSING
    // ============================================================================
    
    /**
     * @brief Processes received DCC command and updates system state
     * @param addr DCC address of received command
     * @param activate Command direction (true=thrown/clock, false=closed/gallery)
     * 
     * Validates the command address against the configured decoder address
     * and triggers appropriate mode switching if the address matches.
     */
    void handleDCCCommand(uint16_t addr, bool activate);
    
    /**
     * @brief Switches display system to gallery slideshow mode
     * 
     * Activates the slideshow manager to begin image gallery display mode.
     * Called when DCC command direction is 0 (closed/straight).
     */
    void switchToGallery();
    
    /**
     * @brief Switches display system to clock display mode
     * 
     * Activates clock display through slideshow manager coordination.
     * Called when DCC command direction is 1 (thrown/diverging).
     */
    void switchToClock();

public:
    // ============================================================================
    // PUBLIC METHODS - LIFECYCLE MANAGEMENT
    // ============================================================================
    
    /**
     * @brief Constructs DCC manager with required component dependencies
     * @param sm Pointer to SettingsManager instance for configuration persistence
     * @param slideshow Pointer to SlideshowManager instance for display mode control
     * 
     * Initializes the DCC manager with default configuration values and
     * establishes component dependencies for configuration and display control.
     */
    DCCManager(SettingsManager* sm, SlideshowManager* slideshow);
    
    /**
     * @brief Initializes DCC decoder with current configuration
     * @return true if initialization successful, false on failure
     * 
     * Loads configuration from settings manager, configures GPIO pin,
     * initializes NmraDcc library, and starts DCC signal processing if enabled.
     */
    bool begin();
    
    /**
     * @brief Processes DCC signals in main application loop
     * 
     * Must be called regularly from main loop to process incoming DCC signals.
     * Only processes signals when DCC interface is enabled and initialized.
     */
    void loop();
    
    /**
     * @brief Restarts DCC decoder with current configuration
     * 
     * Stops current DCC processing, reconfigures hardware with current settings,
     * and restarts DCC signal processing. Used when configuration changes.
     */
    void restart();
    
    // ============================================================================
    // PUBLIC METHODS - DCC LIBRARY INTEGRATION
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
    static void notifyDccAccTurnoutOutput(uint16_t Addr, uint8_t Direction, uint8_t OutputPower);
    
    // ============================================================================
    // PUBLIC METHODS - CONFIGURATION MANAGEMENT
    // ============================================================================
    
    /**
     * @brief Enable or disable DCC interface processing
     * @param enable true to enable DCC processing, false to disable
     * 
     * Controls whether the DCC interface actively processes incoming signals.
     * Configuration is automatically persisted through settings manager.
     * When disabled, stops DCC signal processing and releases hardware resources.
     */
    void setEnabled(bool enable);
    
    /**
     * @brief Get current DCC interface enable state
     * @return true if DCC interface is enabled, false if disabled
     */
    bool isEnabled();
    
    /**
     * @brief Set DCC accessory decoder address
     * @param addr DCC address for this decoder (valid range: 1-2048)
     * 
     * Sets the DCC address this decoder will respond to. Only commands
     * targeting this specific address will trigger mode switching.
     * Configuration is automatically persisted and DCC decoder restarted.
     */
    void setAddress(int addr);
    
    /**
     * @brief Get current DCC decoder address
     * @return Currently configured DCC address
     */
    int getAddress();
    
    /**
     * @brief Set GPIO pin for DCC signal input
     * @param pinNumber GPIO pin number for DCC signal (valid range: 0-39)
     * 
     * Configures which GPIO pin receives DCC signals. Pin is automatically
     * configured with internal pull-up resistor for reliable signal reception.
     * Configuration is automatically persisted and DCC decoder restarted.
     */
    void setPin(int pinNumber);
    
    /**
     * @brief Get current DCC signal GPIO pin
     * @return Currently configured GPIO pin number
     */
    int getPin();
    
    // ============================================================================
    // PUBLIC METHODS - STATE MANAGEMENT
    // ============================================================================
    
    /**
     * @brief Get current DCC command state
     * @return true if last command was activate (clock mode), false if deactivate (gallery mode)
     * 
     * Returns the state from the most recently processed DCC command.
     * Useful for monitoring current display mode from external components.
     */
    bool getCurrentState();
    
    /**
     * @brief Manually set DCC state (for testing or manual control)
     * @param state true for activate/clock mode, false for deactivate/gallery mode
     * 
     * Allows manual state changes for testing purposes or external control.
     * Updates timestamp and triggers appropriate display mode switching.
     */
    void setState(bool state);
    
    // ============================================================================
    // PUBLIC METHODS - STATUS AND DIAGNOSTICS
    // ============================================================================
    
    /**
     * @brief Check if DCC decoder is properly initialized
     * @return true if decoder is initialized and ready to process commands
     * 
     * Indicates whether the DCC decoder hardware and software initialization
     * completed successfully and is ready to process incoming DCC signals.
     */
    bool isInitialized();
};

#pragma once
