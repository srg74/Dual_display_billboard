/**
 * @file settings_manager.h
 * @brief Comprehensive configuration persistence and management system for ESP32 dual display billboard
 * 
 * This module provides persistent storage and management of all system configuration settings
 * using LittleFS. Handles display, DCC, image slideshow, brightness, and clock settings with
 * automatic saving, loading, validation, and immediate hardware application.
 * 
 * Key Features:
 * - Persistent storage using LittleFS filesystem
 * - Automatic configuration loading on startup
 * - Input validation and safe value clamping
 * - Default value fallbacks for missing settings
 * - Real-time configuration updates with immediate persistence
 * - Integrated DisplayManager support for immediate brightness hardware application
 * - Settings reset and diagnostic utilities
 * 
 * @author ESP32 Billboard System
 * @date 2025
 * @version 1.1.0
 */

#pragma once
#include <Arduino.h>
#include <LittleFS.h>
#include "logger.h"
#include "clock_types.h"

// Forward declaration to avoid circular dependency
class DisplayManager;

/**
 * @class SettingsManager
 * @brief Comprehensive configuration persistence and management system with hardware integration
 * 
 * Manages all system configuration settings with persistent storage using LittleFS.
 * Provides type-safe accessors, automatic validation, real-time persistence, and
 * immediate hardware application for all configurable aspects of the dual display billboard system.
 * 
 * Enhanced Features:
 * - Optional DisplayManager integration for immediate brightness hardware control
 * - Automatic brightness application based on second display enable state
 * - Clean separation of configuration storage and hardware control responsibilities
 */
class SettingsManager {
private:
    // ============================================================================
    // PRIVATE CONSTANTS AND FILE PATHS
    // ============================================================================
    
    /// @brief Persistent storage file paths for individual settings
    static const char* SECOND_DISPLAY_FILE;    ///< Second display enable state file
    static const char* DCC_ENABLED_FILE;       ///< DCC interface enable state file
    static const char* DCC_ADDRESS_FILE;       ///< DCC address configuration file
    static const char* DCC_PIN_FILE;           ///< DCC GPIO pin configuration file
    static const char* IMAGE_INTERVAL_FILE;    ///< Image slideshow interval file
    static const char* IMAGE_ENABLED_FILE;     ///< Image display enable state file
    static const char* BRIGHTNESS_FILE;        ///< Display brightness setting file
    static const char* CLOCK_ENABLED_FILE;     ///< Clock display enable state file
    static const char* CLOCK_FACE_FILE;        ///< Clock face type configuration file
    
    // ============================================================================
    // PRIVATE MEMBER VARIABLES - CURRENT SETTINGS CACHE
    // ============================================================================
    
    bool secondDisplayEnabled;    ///< Current second display enable state
    bool dccEnabled;             ///< Current DCC interface enable state
    int dccAddress;              ///< Current DCC address (1-10239)
    int dccPin;                  ///< Current DCC GPIO pin number
    int imageInterval;           ///< Current image interval in seconds
    bool imageEnabled;           ///< Current image slideshow enable state
    int brightness;              ///< Current display brightness (0-255)
    bool clockEnabled;           ///< Current clock display enable state
    ClockFaceType clockFace;     ///< Current clock face type selection
    
    // Display hardware integration for immediate brightness application
    DisplayManager* displayManager; ///< Optional DisplayManager reference for immediate hardware updates
    
    // ============================================================================
    // PRIVATE HELPER METHODS - FILE I/O OPERATIONS
    // ============================================================================
    
    /**
     * @brief Save boolean value to LittleFS file
     * @param filename Target file path for storage
     * @param value Boolean value to save
     * @return true if save successful, false otherwise
     */
    bool saveBoolean(const char* filename, bool value);
    
    /**
     * @brief Load boolean value from LittleFS file with fallback default
     * @param filename Source file path for loading
     * @param defaultValue Default value if file doesn't exist or read fails
     * @return Loaded boolean value or default if unavailable
     */
    bool loadBoolean(const char* filename, bool defaultValue);
    
    /**
     * @brief Save integer value to LittleFS file
     * @param filename Target file path for storage
     * @param value Integer value to save
     * @return true if save successful, false otherwise
     */
    bool saveInteger(const char* filename, int value);
    
    /**
     * @brief Load integer value from LittleFS file with fallback default
     * @param filename Source file path for loading
     * @param defaultValue Default value if file doesn't exist or read fails
     * @return Loaded integer value or default if unavailable
     */
    int loadInteger(const char* filename, int defaultValue);

public:
    // ============================================================================
    // CONSTRUCTOR AND INITIALIZATION
    // ============================================================================
    
    /**
     * @brief Constructor - initializes settings with default values
     * 
     * Sets up default configuration values for all settings. Actual values
     * will be loaded from persistent storage during begin() initialization.
     */
    SettingsManager();
    
    /**
     * @brief Initialize settings manager and load all configurations from storage
     * 
     * Loads all settings from LittleFS persistent storage. If any setting file
     * doesn't exist, uses the default value and creates the file for future use.
     * Should be called once during system startup after LittleFS initialization.
     * 
     * @return true if initialization successful, false if critical error occurred
     */
    bool begin();
    
    /**
     * @brief Set DisplayManager reference for immediate brightness hardware application
     * @param dm Pointer to DisplayManager instance for hardware control integration
     * 
     * Optional integration allowing SettingsManager to immediately apply brightness
     * changes to hardware when setBrightness() is called. This eliminates the need
     * for separate configuration storage and hardware application calls.
     */
    void setDisplayManager(DisplayManager* dm);
    
    // ============================================================================
    // SECOND DISPLAY CONFIGURATION
    // ============================================================================
    
    /**
     * @brief Enable or disable the second display
     * @param enabled true to enable second display, false to disable
     * 
     * Immediately saves the setting to persistent storage. When disabled,
     * the second display will not be initialized or updated.
     */
    void setSecondDisplayEnabled(bool enabled);
    
    /**
     * @brief Get current second display enable state
     * @return true if second display is enabled, false if disabled
     */
    bool isSecondDisplayEnabled();
    
    // ============================================================================
    // DCC (DIGITAL COMMAND CONTROL) CONFIGURATION
    // ============================================================================
    
    /**
     * @brief Enable or disable DCC interface functionality
     * @param enabled true to enable DCC interface, false to disable
     * 
     * Controls whether the system listens for DCC commands. When enabled,
     * the system will monitor the configured GPIO pin for DCC signals.
     */
    void setDCCEnabled(bool enabled);
    
    /**
     * @brief Get current DCC interface enable state
     * @return true if DCC interface is enabled, false if disabled
     */
    bool isDCCEnabled();
    
    /**
     * @brief Set DCC decoder address for command filtering
     * @param address DCC address (typically 1-10239, standard range 1-127)
     * 
     * Sets the DCC address that this device will respond to. Only DCC
     * commands sent to this address will be processed by the system.
     */
    void setDCCAddress(int address);
    
    /**
     * @brief Get current DCC decoder address
     * @return Current DCC address setting
     */
    int getDCCAddress();
    
    /**
     * @brief Set GPIO pin for DCC signal input
     * @param pin GPIO pin number for DCC signal reception
     * 
     * Configures which GPIO pin will be used to receive DCC signals.
     * Should be a pin capable of interrupt-driven input detection.
     */
    void setDCCPin(int pin);
    
    /**
     * @brief Get current DCC GPIO pin setting
     * @return Current DCC GPIO pin number
     */
    int getDCCPin();
    
    // ============================================================================
    // IMAGE SLIDESHOW CONFIGURATION
    // ============================================================================
    
    /**
     * @brief Set image slideshow interval
     * @param seconds Time in seconds between image transitions
     * 
     * Controls how long each image is displayed before transitioning to
     * the next image in the slideshow sequence.
     */
    void setImageInterval(int seconds);
    
    /**
     * @brief Get current image slideshow interval
     * @return Current interval in seconds between image changes
     */
    int getImageInterval();
    
    /**
     * @brief Enable or disable image slideshow functionality
     * @param enabled true to enable image slideshow, false to disable
     * 
     * When disabled, the system will not display images and may show
     * alternative content like clocks or static displays.
     */
    void setImageEnabled(bool enabled);
    
    /**
     * @brief Get current image slideshow enable state
     * @return true if image slideshow is enabled, false if disabled
     */
    bool isImageEnabled();
    
    // ============================================================================
    // DISPLAY BRIGHTNESS CONFIGURATION
    // ============================================================================
    
    /**
     * @brief Set display brightness level with immediate hardware application
     * @param value Brightness level (0-255, where 0=minimum, 255=maximum)
     * 
     * Controls the overall brightness of both displays. Values are automatically
     * clamped to the valid range of 0-255. Setting is persisted immediately to
     * storage and, if DisplayManager is available, applied immediately to hardware
     * respecting the current second display enable setting.
     */
    void setBrightness(int value);
    
    /**
     * @brief Get current display brightness level
     * @return Current brightness setting (0-255)
     */
    int getBrightness();
    
    // ============================================================================
    // CLOCK DISPLAY CONFIGURATION
    // ============================================================================
    
    /**
     * @brief Enable or disable clock display functionality
     * @param enabled true to enable clock display, false to disable
     * 
     * When enabled, clock faces can be displayed instead of or alongside
     * the image slideshow, depending on system configuration.
     */
    void setClockEnabled(bool enabled);
    
    /**
     * @brief Get current clock display enable state
     * @return true if clock display is enabled, false if disabled
     */
    bool isClockEnabled();
    
    /**
     * @brief Set clock face display type
     * @param faceType Clock face type from ClockFaceType enumeration
     * 
     * Selects which style of clock face to display when clock mode is active.
     * Available types defined in clock_types.h header file.
     */
    void setClockFace(ClockFaceType faceType);
    
    /**
     * @brief Get current clock face type setting
     * @return Current clock face type selection
     */
    ClockFaceType getClockFace();
    
    // ============================================================================
    // UTILITY AND DIAGNOSTIC METHODS
    // ============================================================================
    
    /**
     * @brief Print complete settings summary to logger
     * 
     * Outputs all current setting values to the logging system for
     * diagnostic and debugging purposes. Useful for system status checks.
     */
    void printSettings();
    
    /**
     * @brief Reset all settings to factory default values
     * 
     * Restores all configuration settings to their default values and
     * saves them to persistent storage. Use with caution as this will
     * overwrite all user customizations.
     */
    void resetToDefaults();
    
    /**
     * @brief Get settings summary as JSON string for web interface
     * @return JSON formatted string containing all current settings
     * 
     * Provides a structured JSON representation of all settings suitable
     * for web interface display or API responses.
     */
    String getSettingsJson();
    
    /**
     * @brief Validate and clamp setting values to acceptable ranges
     * 
     * Performs validation on all current settings and corrects any values
     * that are outside acceptable ranges. Useful for data integrity checks.
     * 
     * @return Number of settings that were corrected
     */
    int validateAndCorrectSettings();
    
    /**
     * @brief Check if all settings files exist in persistent storage
     * @return true if all setting files exist, false if any are missing
     * 
     * Diagnostic method to verify the integrity of persistent storage
     * and identify missing configuration files.
     */
    bool areAllSettingsFilesPersistent();
};

// ============================================================================
// CONVENIENCE MACROS FOR EASY SETTINGS ACCESS
// ============================================================================

/// @brief Quick access to settings manager instance (assumes global instance named 'settingsManager')
#define SETTINGS_SECOND_DISPLAY() settingsManager.isSecondDisplayEnabled()
#define SETTINGS_DCC_ENABLED() settingsManager.isDCCEnabled()
#define SETTINGS_DCC_ADDRESS() settingsManager.getDCCAddress()
#define SETTINGS_DCC_PIN() settingsManager.getDCCPin()
#define SETTINGS_IMAGE_INTERVAL() settingsManager.getImageInterval()
#define SETTINGS_IMAGE_ENABLED() settingsManager.isImageEnabled()
#define SETTINGS_BRIGHTNESS() settingsManager.getBrightness()
#define SETTINGS_CLOCK_ENABLED() settingsManager.isClockEnabled()
#define SETTINGS_CLOCK_FACE() settingsManager.getClockFace()
#define SETTINGS_PRINT() settingsManager.printSettings()
#define SETTINGS_RESET() settingsManager.resetToDefaults()
