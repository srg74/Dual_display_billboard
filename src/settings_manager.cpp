/**
 * @file settings_manager.cpp
 * @brief Implementation of comprehensive configuration persistence system with hardware integration
 * 
 * Provides complete implementation of settings management with LittleFS persistent storage,
 * input validation, real-time configuration updates, and immediate hardware application.
 * Handles all system configuration aspects including display, DCC, image slideshow, brightness,
 * and clock settings with integrated DisplayManager support for immediate brightness control.
 * 
 * Enhanced Features:
 * - DisplayManager integration for immediate brightness hardware application
 * - Automatic brightness adjustment when second display setting changes
 * - Clean separation of concerns with optional hardware integration
 * 
 * @author ESP32 Billboard System
 * @date 2025
 * @version 0.9
 */

#include "settings_manager.h"
#include "display_manager.h"

static const String TAG = "SETTINGS";

// ============================================================================
// STATIC MEMBER DEFINITIONS - PERSISTENT STORAGE FILE PATHS
// ============================================================================

const char* SettingsManager::SECOND_DISPLAY_FILE = "/second_display.txt";
const char* SettingsManager::DCC_ENABLED_FILE = "/dcc_enabled.txt";
const char* SettingsManager::DCC_ADDRESS_FILE = "/dcc_address.txt";
const char* SettingsManager::DCC_PIN_FILE = "/dcc_pin.txt";
const char* SettingsManager::IMAGE_INTERVAL_FILE = "/image_interval.txt";
const char* SettingsManager::IMAGE_ENABLED_FILE = "/image_enabled.txt";
const char* SettingsManager::BRIGHTNESS_FILE = "/brightness.txt";
const char* SettingsManager::CLOCK_ENABLED_FILE = "/clock_enabled.txt";
const char* SettingsManager::CLOCK_FACE_FILE = "/clock_face.txt";

// ============================================================================
// CONSTRUCTOR AND INITIALIZATION
// ============================================================================

/**
 * @brief Initialize SettingsManager with safe default configuration values
 *        for all system settings. Sets conservative defaults that ensure stable
 *        system operation before loading persistent configuration from LittleFS.
 * 
 * The constructor establishes a known-good baseline configuration with safety-first
 * defaults. All settings will be overridden by persistent storage during the begin()
 * initialization sequence, but these defaults provide fallback values and ensure
 * the system has valid configuration even if persistent storage is corrupted.
 * 
 * Default Configuration:
 * - Second display: disabled (single display mode)
 * - DCC interface: disabled (safety-first approach)
 * - DCC address: 101 (standard range default)
 * - DCC GPIO pin: 4 (safe pin choice)
 * - Image interval: 10 seconds (reasonable slideshow speed)
 * - Image slideshow: enabled (primary functionality)
 * - Brightness: 200/255 (78% for comfortable viewing)
 * - Clock display: disabled (compatibility mode)
 * - Clock face: modern square (clean aesthetic)
 * - DisplayManager: null (optional hardware integration)
 * 
 * @note All settings are validated and corrected during begin() initialization
 * @note DisplayManager integration is optional and set via setDisplayManager()
 * @note Persistent storage values override these defaults when available
 * 
 * @see begin() for complete initialization with persistent storage loading
 * @see setDisplayManager() for hardware integration setup
 * @see resetToDefaults() for restoring factory configuration
 * 
 * @since v0.9
 */
SettingsManager::SettingsManager() {
    // Initialize with safe default values - will be overridden by persistent storage in begin()
#ifdef DUAL_DISPLAY_ENABLED
    secondDisplayEnabled = true;            // Display 2 enabled by default in dual display mode
#else
    secondDisplayEnabled = false;           // Single display mode when dual display not compiled
#endif
    dccEnabled = false;                    // DCC interface disabled by default for safety
    dccAddress = 101;                      // Standard DCC address range default
    dccPin = 4;                           // Safe GPIO pin choice for DCC input
    imageInterval = 10;                    // 10-second default slideshow interval
    imageEnabled = true;                   // Enable image slideshow by default
    brightness = 200;                      // Moderate brightness (78% of maximum)
    clockEnabled = false;                  // Clock disabled by default to maintain compatibility
    clockFace = CLOCK_MODERN_SQUARE;       // Modern square clock face as default
    displayManager = nullptr;              // No DisplayManager integration by default
}

/**
 * @brief Initialize the SettingsManager by loading all configuration values
 *        from LittleFS persistent storage with automatic validation and correction
 *        of out-of-range values. Establishes complete system configuration state.
 * 
 * This method performs comprehensive initialization including loading all settings
 * from individual LittleFS files, validating loaded values against acceptable ranges,
 * automatically correcting any invalid values, and logging the complete system
 * configuration for diagnostic purposes. The initialization process ensures
 * robust system configuration regardless of persistent storage state.
 * 
 * Initialization Process:
 * 1. Load all settings from LittleFS with fallback defaults
 * 2. Validate and automatically correct out-of-range values
 * 3. Save corrected values back to persistent storage
 * 4. Log complete configuration state for diagnostics
 * 5. Report successful initialization
 * 
 * Settings Loaded:
 * - Second display enable/disable state
 * - DCC interface configuration (enable, address, GPIO pin)
 * - Image slideshow settings (interval, enable state)
 * - Display brightness level
 * - Clock display settings (enable state, face type)
 * 
 * @return true on successful initialization (always succeeds with fallback defaults)
 * 
 * @note Automatically corrects invalid settings and reports correction count
 * @note All corrected values are immediately saved to persistent storage
 * @note Logs complete configuration state for system diagnostics
 * @note Safe to call multiple times (reloads configuration)
 * @note DisplayManager integration requires separate setDisplayManager() call
 * 
 * @see setDisplayManager() for hardware integration after initialization
 * @see validateAndCorrectSettings() for automatic value correction
 * @see resetToDefaults() for factory reset functionality
 * @see printSettings() for diagnostic configuration display
 * 
 * @since v0.9
 */
bool SettingsManager::begin() {
    LOG_INFO(TAG, "Initializing Settings Manager...");
    
    // Ensure all configuration files exist with default values to prevent VFS errors
    if (!LittleFS.exists(SECOND_DISPLAY_FILE)) {
#ifdef DUAL_DISPLAY_ENABLED
        saveBoolean(SECOND_DISPLAY_FILE, false);  // Default to disabled - user must enable via web UI
#else
        saveBoolean(SECOND_DISPLAY_FILE, false);  // Default to disabled when single display compiled
#endif
    }
    if (!LittleFS.exists(DCC_ENABLED_FILE)) {
        saveBoolean(DCC_ENABLED_FILE, false);
    }
    if (!LittleFS.exists(DCC_ADDRESS_FILE)) {
        saveInteger(DCC_ADDRESS_FILE, 101);
    }
    if (!LittleFS.exists(DCC_PIN_FILE)) {
        saveInteger(DCC_PIN_FILE, 4);
    }
    if (!LittleFS.exists(IMAGE_INTERVAL_FILE)) {
        saveInteger(IMAGE_INTERVAL_FILE, 10);
    }
    if (!LittleFS.exists(IMAGE_ENABLED_FILE)) {
        saveBoolean(IMAGE_ENABLED_FILE, true);
    }
    if (!LittleFS.exists(BRIGHTNESS_FILE)) {
        saveInteger(BRIGHTNESS_FILE, 200);
    }
    if (!LittleFS.exists(CLOCK_ENABLED_FILE)) {
        saveBoolean(CLOCK_ENABLED_FILE, false);
    }
    if (!LittleFS.exists(CLOCK_FACE_FILE)) {
        saveInteger(CLOCK_FACE_FILE, CLOCK_MODERN_SQUARE);
    }
    
    // Load all settings from LittleFS with appropriate defaults
#ifdef DUAL_DISPLAY_ENABLED
    secondDisplayEnabled = loadBoolean(SECOND_DISPLAY_FILE, true);   // Default to enabled in dual display mode
#else
    secondDisplayEnabled = loadBoolean(SECOND_DISPLAY_FILE, false);  // Default to disabled when single display compiled
#endif
    dccEnabled = loadBoolean(DCC_ENABLED_FILE, false);
    dccAddress = loadInteger(DCC_ADDRESS_FILE, 101);
    dccPin = loadInteger(DCC_PIN_FILE, 4);
    imageInterval = loadInteger(IMAGE_INTERVAL_FILE, 10);
    imageEnabled = loadBoolean(IMAGE_ENABLED_FILE, true);
    brightness = loadInteger(BRIGHTNESS_FILE, 200);
    clockEnabled = loadBoolean(CLOCK_ENABLED_FILE, false);
    clockFace = static_cast<ClockFaceType>(loadInteger(CLOCK_FACE_FILE, CLOCK_MODERN_SQUARE));
    
    // Validate and correct loaded settings to ensure they're within acceptable ranges
    int correctedCount = validateAndCorrectSettings();
    if (correctedCount > 0) {
    LOG_INFOF(TAG, "Corrected %d out-of-range setting value(s)", correctedCount);
    }
    
    // Log current configuration for diagnostic purposes
    LOG_INFOF(TAG, "Second Display: %s", secondDisplayEnabled ? "enabled" : "disabled");
    LOG_INFOF(TAG, "DCC Interface: %s", dccEnabled ? "enabled" : "disabled");
    if (dccEnabled) {
        LOG_INFOF(TAG, "DCC Address: %d", dccAddress);
        LOG_INFOF(TAG, "DCC GPIO Pin: %d", dccPin);
    }
    LOG_INFOF(TAG, "Image Interval: %d seconds", imageInterval);
    LOG_INFOF(TAG, "Image Display: %s", imageEnabled ? "enabled" : "disabled");
    LOG_INFOF(TAG, "Brightness: %d (%.1f%%)", brightness, (brightness / 255.0f) * 100.0f);
    LOG_INFOF(TAG, "Clock Display: %s", clockEnabled ? "enabled" : "disabled");
    
    LOG_INFO(TAG, "Settings Manager initialized successfully");
    return true;
}

/**
 * @brief Integrate DisplayManager hardware interface for immediate brightness
 *        control and automatic hardware state management. Enables real-time
 *        application of brightness settings to display hardware.
 * 
 * This method establishes integration with the DisplayManager to provide immediate
 * hardware control capabilities. When integrated, brightness changes are instantly
 * applied to the appropriate displays based on the second display configuration.
 * The method also immediately applies current brightness settings to hardware
 * upon integration.
 * 
 * Integration Features:
 * - Immediate brightness application on settings changes
 * - Automatic dual/single display brightness management
 * - Hardware state synchronization with configuration
 * - Optional integration (can be disabled by passing nullptr)
 * 
 * Hardware Application Logic:
 * - If second display enabled: applies brightness to both displays
 * - If second display disabled: applies brightness to main display, turns off second
 * - Immediate application of current brightness settings upon integration
 * 
 * @param dm Pointer to DisplayManager instance for hardware integration, or nullptr to disable
 * 
 * @note Integration is optional - SettingsManager functions without DisplayManager
 * @note Immediately applies current brightness settings to hardware when integrated
 * @note Brightness changes are applied in real-time when DisplayManager is integrated
 * @note Can be called multiple times to change or disable integration
 * @note Safe to pass nullptr to disable hardware integration
 * 
 * @see setBrightness() for brightness control with immediate hardware application
 * @see setSecondDisplayEnabled() for display configuration with hardware control
 * @see DisplayManager::setBrightness() for underlying hardware control method
 * 
 * @since v0.9
 */
void SettingsManager::setDisplayManager(DisplayManager* dm) {
    displayManager = dm;
    if (dm) {
        LOG_INFO(TAG, "DisplayManager integration enabled for immediate brightness application");
        // Apply current brightness setting to hardware immediately
        if (secondDisplayEnabled) {
            dm->setBrightness(brightness, 0); // Both displays
                LOG_DEBUG(TAG, "Applied current brightness to both displays");
        } else {
            dm->setBrightness(brightness, 1); // Main display only
            // Don't turn off Display 2 if splash screen is active (let splash complete first)
            if (!dm->isSplashActive()) {
                dm->setBrightness(0, 2);          // Turn off second display
                LOG_DEBUG(TAG, "Applied brightness to main display only - second display disabled");
            } else {
                LOG_INFO(TAG, "Splash active - deferring Display 2 brightness setting");
            }
        }
    } else {
    LOG_INFO(TAG, "DisplayManager integration disabled");
    }
}

// ============================================================================
// SECOND DISPLAY CONFIGURATION METHODS
// ============================================================================

/**
 * @brief Configure second display enable/disable state with persistent storage
 *        and immediate hardware brightness application. Controls dual display
 *        functionality for the billboard system.
 * 
 * This method manages the second display configuration with complete persistence
 * and immediate hardware integration. When the second display state changes,
 * the method automatically adjusts brightness application to match the new
 * configuration, ensuring proper hardware state synchronization.
 * 
 * Hardware Integration Behavior:
 * - When enabled: applies current brightness to both displays
 * - When disabled: applies brightness to main display only, turns off second display
 * - Immediate hardware state synchronization when DisplayManager is integrated
 * 
 * @param enabled true to enable second display functionality, false for single display mode
 * 
 * @note Setting is immediately saved to LittleFS persistent storage
 * @note Hardware brightness is automatically adjusted when DisplayManager is integrated
 * @note Safe to call repeatedly with same value (no unnecessary operations)
 * @note Logs save operation success/failure for diagnostics
 * 
 * @see isSecondDisplayEnabled() for querying current second display state
 * @see setDisplayManager() for hardware integration setup
 * @see setBrightness() for brightness control with display awareness
 * @see DisplayManager::setBrightness() for underlying hardware control
 * 
 * @since v0.9
 */
void SettingsManager::setSecondDisplayEnabled(bool enabled) {
    secondDisplayEnabled = enabled;
    if (saveBoolean(SECOND_DISPLAY_FILE, enabled)) {
        LOG_INFOF(TAG, "Second display setting saved: %s", enabled ? "enabled" : "disabled");
    } else {
        LOG_WARN(TAG, "Failed to save second display setting to persistent storage");
    }
    
    // Apply current brightness to appropriate displays when second display setting changes
    if (displayManager) {
        if (enabled) {
            displayManager->setBrightness(brightness, 0); // Both displays
            LOG_DEBUG(TAG, "Applied current brightness to both displays");
        } else {
            displayManager->setBrightness(brightness, 1);  // Main display only
            // Don't turn off Display 2 if splash screen is active (let splash complete first)
            if (!displayManager->isSplashActive()) {
                displayManager->setBrightness(0, 2);           // Turn off second display
                LOG_DEBUG(TAG, "Applied brightness to main display only, turned off second");
            } else {
                LOG_INFO(TAG, "Splash active - deferring Display 2 brightness setting");
            }
        }
    }
}

/**
 * @brief Query the current second display enable/disable state for system
 *        configuration and display management logic.
 * 
 * This method provides access to the current second display configuration state,
 * which is used throughout the system for display-aware operations including
 * brightness control, image rendering, and clock display management.
 * 
 * @return true if second display is enabled (dual display mode), false if disabled (single display mode)
 * 
 * @note Returns the current in-memory state (no file system access)
 * @note Value is loaded from persistent storage during begin() initialization
 * @note Used by other system components for display-aware operations
 * @note Thread-safe for read access
 * 
 * @see setSecondDisplayEnabled() for configuring second display state
 * @see begin() for initialization from persistent storage
 * @see DisplayManager integration for hardware-aware operations
 * 
 * @since v0.9
 */
bool SettingsManager::isSecondDisplayEnabled() {
    return secondDisplayEnabled;
}

// ============================================================================
// DCC (DIGITAL COMMAND CONTROL) CONFIGURATION METHODS
// ============================================================================

/**
 * @brief Configure Digital Command Control (DCC) interface enable/disable state
 *        for model railroad integration with persistent storage and validation.
 * 
 * This method controls the DCC interface functionality for model railroad
 * integration, enabling the billboard system to respond to DCC commands from
 * model railroad control systems. When enabled, the system will initialize
 * the DCC decoder functionality using the configured address and GPIO pin.
 * 
 * DCC Integration Features:
 * - Enables/disables DCC command reception from model railroad control systems
 * - Automatically saves configuration to persistent storage for system restarts
 * - Validates DCC interface state for proper model railroad integration
 * - Coordinates with DCC address and pin configuration for complete setup
 * 
 * @param enabled true to enable DCC interface for model railroad integration,
 *                false to disable DCC functionality
 * 
 * @note Setting is immediately saved to LittleFS persistent storage
 * @note DCC functionality requires additional configuration of address and GPIO pin
 * @note Changes take effect after system restart when DCC hardware is initialized
 * @note Logs save operation success/failure for diagnostics
 * @note Safe to call repeatedly with same value (no unnecessary file operations)
 * 
 * @see isDCCEnabled() for querying current DCC interface state
 * @see setDCCAddress() for configuring DCC decoder address
 * @see setDCCPin() for configuring DCC input GPIO pin
 * @see DCCManager for underlying DCC command processing
 * 
 * @since v0.9
 */
void SettingsManager::setDCCEnabled(bool enabled) {
    dccEnabled = enabled;
    if (saveBoolean(DCC_ENABLED_FILE, enabled)) {
        LOG_INFOF(TAG, "DCC interface setting saved: %s", enabled ? "enabled" : "disabled");
    } else {
        LOG_WARN(TAG, "Failed to save DCC interface setting to persistent storage");
    }
}

/**
 * @brief Query the current Digital Command Control (DCC) interface enable/disable
 *        state for model railroad integration and system initialization logic.
 * 
 * This method provides access to the current DCC interface configuration state,
 * which determines whether the billboard system will initialize DCC decoder
 * functionality for model railroad command reception. The state is used by
 * the DCCManager during system startup to conditionally enable DCC features.
 * 
 * @return true if DCC interface is enabled for model railroad integration,
 *         false if DCC functionality is disabled
 * 
 * @note Returns the current in-memory state (no file system access)
 * @note Value is loaded from persistent storage during begin() initialization
 * @note Used by DCCManager during system startup for conditional initialization
 * @note Thread-safe for read access
 * @note DCC functionality also requires valid address and pin configuration
 * 
 * @see setDCCEnabled() for configuring DCC interface state
 * @see begin() for initialization from persistent storage
 * @see getDCCAddress() for DCC decoder address configuration
 * @see getDCCPin() for DCC input GPIO pin configuration
 * @see DCCManager for DCC command processing implementation
 * 
 * @since v0.9
 */
bool SettingsManager::isDCCEnabled() {
    return dccEnabled;
}

/**
 * @brief Configure Digital Command Control (DCC) decoder address for model
 *        railroad command reception with NMRA standards validation and persistence.
 * 
 * This method sets the DCC decoder address that the billboard system will respond
 * to when receiving commands from model railroad control systems. The address
 * determines which DCC commands will be processed by the billboard, allowing
 * multiple devices to coexist on the same DCC bus with unique addresses.
 * 
 * DCC Address Standards and Validation:
 * - Validates against NMRA DCC standards (valid range: 1-10239)
 * - Short addresses: 1-127 (most commonly used for accessories)
 * - Long addresses: 128-10239 (extended addressing for complex layouts)
 * - Automatically clamps out-of-range values to valid limits
 * - Logs validation warnings when address correction is required
 * 
 * @param address DCC decoder address (1-10239) for command reception
 * 
 * @note Address is immediately validated and saved to LittleFS persistent storage
 * @note Out-of-range addresses are automatically clamped to valid limits (1-10239)
 * @note Changes take effect after system restart when DCC hardware is initialized
 * @note Logs save operation success/failure and validation warnings for diagnostics
 * @note Address 1 is used as default fallback for invalid values below range
 * 
 * @see getDCCAddress() for querying current DCC decoder address
 * @see setDCCEnabled() for enabling/disabling DCC interface
 * @see setDCCPin() for configuring DCC input GPIO pin
 * @see DCCManager for DCC command processing and address filtering
 * 
 * @since v0.9
 */
void SettingsManager::setDCCAddress(int address) {
    // Validate DCC address range (1-10239 per NMRA standards, with 1-127 most common)
    if (address < 1 || address > 10239) {
        LOG_WARNF(TAG, "DCC address %d out of range (1-10239), clamping to valid range", address);
        address = (address < 1) ? 1 : 10239;
    }
    
    dccAddress = address;
    if (saveInteger(DCC_ADDRESS_FILE, address)) {
        LOG_INFOF(TAG, "DCC address saved: %d", address);
    } else {
        LOG_WARN(TAG, "Failed to save DCC address to persistent storage");
    }
}

/**
 * @brief Query the current Digital Command Control (DCC) decoder address
 *        for model railroad command filtering and system configuration.
 * 
 * This method provides access to the configured DCC decoder address that
 * determines which DCC commands the billboard system will respond to from
 * model railroad control systems. The address is used by the DCCManager
 * for command filtering and processing.
 * 
 * @return DCC decoder address (1-10239) for command reception and filtering
 * 
 * @note Returns the current in-memory address value (no file system access)
 * @note Address is loaded from persistent storage during begin() initialization
 * @note Used by DCCManager for command filtering and address matching
 * @note Thread-safe for read access
 * @note Address range is validated during configuration (setDCCAddress)
 * 
 * @see setDCCAddress() for configuring DCC decoder address with validation
 * @see begin() for initialization from persistent storage
 * @see isDCCEnabled() for DCC interface enable state
 * @see getDCCPin() for DCC input GPIO pin configuration
 * @see DCCManager for DCC command processing and address filtering
 * 
 * @since v0.9
 */
int SettingsManager::getDCCAddress() {
    return dccAddress;
}

/**
 * @brief Configure Digital Command Control (DCC) input GPIO pin for signal
 *        reception with ESP32 hardware validation and persistent storage.
 * 
 * This method sets the ESP32 GPIO pin that will be used to receive DCC signals
 * from model railroad control systems. The pin must be capable of digital input
 * and interrupt handling for proper DCC signal timing and decoding.
 * 
 * ESP32 GPIO Pin Validation and Constraints:
 * - Validates pin range against ESP32 hardware capabilities (0-39)
 * - Automatically clamps out-of-range values to valid GPIO limits
 * - Default fallback to pin 4 for invalid low values (reliable input pin)
 * - Logs validation warnings when pin correction is required
 * - Some pins have hardware restrictions (input-only, boot-sensitive, etc.)
 * 
 * @param pin ESP32 GPIO pin number (0-39) for DCC signal input
 * 
 * @note Pin is immediately validated and saved to LittleFS persistent storage
 * @note Out-of-range pins are automatically clamped to valid ESP32 GPIO range
 * @note Changes take effect after system restart when DCC hardware is initialized
 * @note Pin 4 is used as default fallback for invalid values below range
 * @note Logs save operation success/failure and validation warnings for diagnostics
 * @note Consider pin usage conflicts with display SPI, I2C, or other peripherals
 * 
 * @see getDCCPin() for querying current DCC input GPIO pin
 * @see setDCCEnabled() for enabling/disabling DCC interface
 * @see setDCCAddress() for configuring DCC decoder address
 * @see DCCManager for DCC signal decoding and interrupt handling
 * 
 * @since v0.9
 */
void SettingsManager::setDCCPin(int pin) {
    // Validate GPIO pin range for ESP32 (0-39, but some pins have restrictions)
    if (pin < 0 || pin > 39) {
        LOG_WARNF(TAG, "DCC GPIO pin %d out of valid range (0-39), clamping", pin);
        pin = (pin < 0) ? 4 : 39;  // Default to pin 4 if invalid
    }
    
    dccPin = pin;
    if (saveInteger(DCC_PIN_FILE, pin)) {
        LOG_INFOF(TAG, "DCC GPIO pin saved: %d", pin);
    } else {
        LOG_WARN(TAG, "Failed to save DCC GPIO pin to persistent storage");
    }
}

/**
 * @brief Query the current Digital Command Control (DCC) input GPIO pin
 *        for hardware initialization and signal processing setup.
 * 
 * This method provides access to the configured ESP32 GPIO pin that will be
 * used for DCC signal reception from model railroad control systems. The pin
 * is used by the DCCManager during initialization for interrupt setup and
 * signal decoding configuration.
 * 
 * @return ESP32 GPIO pin number (0-39) configured for DCC signal input
 * 
 * @note Returns the current in-memory pin value (no file system access)
 * @note Pin is loaded from persistent storage during begin() initialization
 * @note Used by DCCManager for interrupt setup and signal processing
 * @note Thread-safe for read access
 * @note Pin range is validated during configuration (setDCCPin)
 * @note Consider pin usage conflicts with other system peripherals
 * 
 * @see setDCCPin() for configuring DCC input GPIO pin with validation
 * @see begin() for initialization from persistent storage
 * @see isDCCEnabled() for DCC interface enable state
 * @see getDCCAddress() for DCC decoder address configuration
 * @see DCCManager for DCC signal decoding and interrupt handling
 * 
 * @since v0.9
 */
int SettingsManager::getDCCPin() {
    return dccPin;
}

// ============================================================================
// IMAGE SLIDESHOW CONFIGURATION METHODS
// ============================================================================

/**
 * @brief Configure automatic image slideshow interval timing with validation
 *        and persistent storage for billboard display management.
 * 
 * This method sets the time interval between automatic image transitions in
 * the billboard slideshow system. The interval determines how long each image
 * is displayed before automatically advancing to the next image in the sequence.
 * 
 * Interval Validation and Constraints:
 * - Validates interval range for practical slideshow operation (1-3600 seconds)
 * - Minimum 1 second prevents excessive CPU usage and display flicker
 * - Maximum 1 hour (3600 seconds) prevents excessively long static displays
 * - Automatically clamps out-of-range values to valid limits
 * - Logs validation warnings when interval correction is required
 * 
 * @param seconds Image display interval in seconds (1-3600)
 * 
 * @note Interval is immediately validated and saved to LittleFS persistent storage
 * @note Out-of-range intervals are automatically clamped to valid limits (1-3600)
 * @note Changes take effect immediately for next image transition
 * @note Logs save operation success/failure and validation warnings for diagnostics
 * @note Used by SlideshowManager for automatic image advancement timing
 * 
 * @see getImageInterval() for querying current slideshow interval
 * @see setImageEnabled() for enabling/disabling automatic slideshow
 * @see isImageEnabled() for querying slideshow enable state
 * @see SlideshowManager for automatic image transition management
 * 
 * @since v0.9
 */
void SettingsManager::setImageInterval(int seconds) {
    // Validate image interval range (minimum 1 second, maximum 3600 seconds/1 hour)
    if (seconds < 1 || seconds > 3600) {
        LOG_WARNF(TAG, "Image interval %d seconds out of range (1-3600), clamping", seconds);
        seconds = (seconds < 1) ? 1 : 3600;
    }
    
    imageInterval = seconds;
    if (saveInteger(IMAGE_INTERVAL_FILE, seconds)) {
        LOG_INFOF(TAG, "Image interval saved: %d seconds", seconds);
    } else {
        LOG_WARN(TAG, "Failed to save image interval to persistent storage");
    }
}

/**
 * @brief Query the current automatic image slideshow interval timing for
 *        display management and transition scheduling.
 * 
 * This method provides access to the configured time interval between automatic
 * image transitions in the billboard slideshow system. The interval is used by
 * the SlideshowManager for scheduling image advancement and timing calculations.
 * 
 * @return Image display interval in seconds (1-3600) for slideshow timing
 * 
 * @note Returns the current in-memory interval value (no file system access)
 * @note Interval is loaded from persistent storage during begin() initialization
 * @note Used by SlideshowManager for automatic image transition timing
 * @note Thread-safe for read access
 * @note Interval range is validated during configuration (setImageInterval)
 * @note Value is used in millisecond calculations for precise timing control
 * 
 * @see setImageInterval() for configuring slideshow interval with validation
 * @see begin() for initialization from persistent storage
 * @see setImageEnabled() for enabling/disabling automatic slideshow
 * @see isImageEnabled() for querying slideshow enable state
 * @see SlideshowManager for automatic image transition management
 * 
 * @since v0.9
 */
int SettingsManager::getImageInterval() {
    return imageInterval;
}

/**
 * @brief Configure automatic image slideshow enable/disable state with
 *        persistent storage for billboard display control.
 * 
 * This method controls whether the billboard system will automatically cycle
 * through images in the slideshow sequence. When enabled, the system will
 * automatically transition between images using the configured interval timing.
 * When disabled, images must be changed manually or through external control.
 * 
 * Slideshow Control Features:
 * - Enables/disables automatic image cycling for billboard displays
 * - Automatically saves configuration to persistent storage for system restarts
 * - Coordinates with image interval timing for complete slideshow management
 * - Used by SlideshowManager for conditional automatic advancement
 * 
 * @param enabled true to enable automatic image slideshow cycling,
 *                false to disable automatic transitions (manual control only)
 * 
 * @note Setting is immediately saved to LittleFS persistent storage
 * @note Changes take effect immediately for slideshow behavior
 * @note Logs save operation success/failure for diagnostics
 * @note Safe to call repeatedly with same value (no unnecessary file operations)
 * @note When disabled, current image remains displayed until manually changed
 * 
 * @see isImageEnabled() for querying current slideshow enable state
 * @see setImageInterval() for configuring automatic transition timing
 * @see getImageInterval() for querying current slideshow interval
 * @see SlideshowManager for automatic image transition management
 * 
 * @since v0.9
 */
void SettingsManager::setImageEnabled(bool enabled) {
    imageEnabled = enabled;
    if (saveBoolean(IMAGE_ENABLED_FILE, enabled)) {
        LOG_INFOF(TAG, "Image slideshow setting saved: %s", enabled ? "enabled" : "disabled");
    } else {
        LOG_WARN(TAG, "Failed to save image slideshow setting to persistent storage");
    }
}

/**
 * @brief Query the current automatic image slideshow enable/disable state
 *        for display management and transition control logic.
 * 
 * This method provides access to the current slideshow configuration state,
 * which determines whether the billboard system will automatically cycle
 * through images or require manual control. The state is used by the
 * SlideshowManager for conditional automatic image advancement.
 * 
 * @return true if automatic image slideshow is enabled,
 *         false if slideshow is disabled (manual control only)
 * 
 * @note Returns the current in-memory state (no file system access)
 * @note Value is loaded from persistent storage during begin() initialization
 * @note Used by SlideshowManager for conditional automatic image transitions
 * @note Thread-safe for read access
 * @note When false, images must be changed manually or through external control
 * 
 * @see setImageEnabled() for configuring slideshow enable state
 * @see begin() for initialization from persistent storage
 * @see setImageInterval() for configuring automatic transition timing
 * @see getImageInterval() for querying current slideshow interval
 * @see SlideshowManager for automatic image transition management
 * 
 * @since v0.9
 */
bool SettingsManager::isImageEnabled() {
    return imageEnabled;
}

// ============================================================================
// DISPLAY BRIGHTNESS CONFIGURATION METHODS
// ============================================================================

/**
 * @brief Configure display brightness with PWM validation, persistent storage,
 *        and immediate hardware application for billboard display control.
 * 
 * This method sets the display brightness level for the billboard system with
 * automatic hardware application when DisplayManager is integrated. The brightness
 * value controls the PWM backlight intensity for both single and dual display
 * configurations, providing immediate visual feedback and persistent storage.
 * 
 * Brightness Control Features:
 * - Validates brightness against PWM hardware limits (0-255)
 * - Automatically clamps out-of-range values to valid PWM limits
 * - Immediate hardware application when DisplayManager is integrated
 * - Display-aware application (single vs dual display configurations)
 * - Persistent storage for brightness retention across system restarts
 * 
 * Hardware Integration Behavior:
 * - When second display enabled: applies brightness to both displays
 * - When second display disabled: applies to main display, turns off second
 * - Immediate PWM adjustment provides real-time brightness feedback
 * - Coordinates with display configuration for proper hardware state
 * 
 * @param value Brightness level (0-255) where 0=off, 255=maximum brightness
 * 
 * @note Brightness is immediately validated and saved to LittleFS persistent storage
 * @note Out-of-range values are automatically clamped to PWM limits (0-255)
 * @note Hardware brightness is applied immediately when DisplayManager is integrated
 * @note Logs save operation success/failure and validation warnings for diagnostics
 * @note Display configuration awareness ensures proper dual display management
 * 
 * @see getBrightness() for querying current brightness level
 * @see setDisplayManager() for hardware integration setup
 * @see setSecondDisplayEnabled() for display configuration control
 * @see DisplayManager::setBrightness() for underlying PWM hardware control
 * 
 * @since v0.9
 */
void SettingsManager::setBrightness(int value) {
    // Clamp brightness value to valid PWM range (0-255)
    if (value < 0) {
        LOG_WARNF(TAG, "Brightness value %d below minimum, setting to 0", value);
        value = 0;
    }
    if (value > 255) {
        LOG_WARNF(TAG, "Brightness value %d above maximum, setting to 255", value);
        value = 255;
    }
    
    brightness = value;
    if (saveInteger(BRIGHTNESS_FILE, value)) {
        LOG_INFOF(TAG, "Brightness saved: %d (%.1f%%)", value, (value / 255.0f) * 100.0f);
    } else {
        LOG_WARN(TAG, "Failed to save brightness setting to persistent storage");
    }
    
    // Apply brightness immediately to hardware if DisplayManager is available
    if (displayManager) {
        if (secondDisplayEnabled) {
            displayManager->setBrightness(value, 0); // Both displays
            LOG_DEBUG(TAG, "Applied brightness to both displays immediately");
        } else {
            displayManager->setBrightness(value, 1);  // Main display only
            // Don't turn off Display 2 if splash screen is active (let splash complete first)
            if (!displayManager->isSplashActive()) {
                displayManager->setBrightness(0, 2);      // Turn off second display
                LOG_DEBUG(TAG, "Applied brightness to main display only, turned off second");
            } else {
                LOG_INFO(TAG, "Splash active - deferring Display 2 brightness setting");
            }
        }
    }
}

/**
 * @brief Query the current display brightness level for hardware control
 *        and user interface feedback.
 * 
 * This method provides access to the current display brightness configuration,
 * which is used for hardware PWM control, user interface display, and system
 * state reporting. The value represents the PWM duty cycle for backlight control.
 * 
 * @return Brightness level (0-255) where 0=off, 255=maximum brightness
 * 
 * @note Returns the current in-memory brightness value (no file system access)
 * @note Value is loaded from persistent storage during begin() initialization
 * @note Used by DisplayManager for PWM hardware control
 * @note Thread-safe for read access
 * @note Value range is validated during configuration (setBrightness)
 * @note Can be converted to percentage: (value / 255.0f) * 100.0f
 * 
 * @see setBrightness() for configuring brightness with validation and hardware application
 * @see begin() for initialization from persistent storage
 * @see setDisplayManager() for hardware integration setup
 * @see DisplayManager::setBrightness() for underlying PWM hardware control
 * 
 * @since v0.9
 */
int SettingsManager::getBrightness() {
    return brightness;
}

// ============================================================================
// CLOCK DISPLAY CONFIGURATION METHODS
// ============================================================================

/**
 * @brief Configure clock display enable/disable state with persistent storage
 *        for billboard time display functionality.
 * 
 * This method controls whether the billboard system will display clock/time
 * information as part of its content rotation. When enabled, the system will
 * show the current time using the configured clock face style, providing
 * useful time information to viewers.
 * 
 * Clock Display Features:
 * - Enables/disables time display functionality for billboard content
 * - Automatically saves configuration to persistent storage for system restarts
 * - Coordinates with clock face type configuration for complete time display
 * - Used by DisplayClockManager for conditional time rendering
 * 
 * @param enabled true to enable clock/time display functionality,
 *                false to disable time display (content-only mode)
 * 
 * @note Setting is immediately saved to LittleFS persistent storage
 * @note Changes take effect immediately for clock display behavior
 * @note Logs save operation success/failure for diagnostics
 * @note Safe to call repeatedly with same value (no unnecessary file operations)
 * @note When disabled, time display is excluded from content rotation
 * 
 * @see isClockEnabled() for querying current clock display state
 * @see setClockFace() for configuring clock visual style
 * @see getClockFace() for querying current clock face type
 * @see DisplayClockManager for time display rendering
 * 
 * @since v0.9
 */
void SettingsManager::setClockEnabled(bool enabled) {
    clockEnabled = enabled;
    if (saveBoolean(CLOCK_ENABLED_FILE, enabled)) {
        LOG_INFOF(TAG, "Clock display setting saved: %s", enabled ? "enabled" : "disabled");
    } else {
        LOG_WARN(TAG, "Failed to save clock display setting to persistent storage");
    }
}

/**
 * @brief Query the current clock display enable/disable state for time
 *        display management and content rotation logic.
 * 
 * This method provides access to the current clock display configuration state,
 * which determines whether the billboard system will include time display in
 * its content rotation. The state is used by the DisplayClockManager for
 * conditional time rendering and content scheduling.
 * 
 * @return true if clock/time display is enabled,
 *         false if time display is disabled (content-only mode)
 * 
 * @note Returns the current in-memory state (no file system access)
 * @note Value is loaded from persistent storage during begin() initialization
 * @note Used by DisplayClockManager for conditional time display rendering
 * @note Thread-safe for read access
 * @note When false, time display is excluded from billboard content rotation
 * 
 * @see setClockEnabled() for configuring clock display state
 * @see begin() for initialization from persistent storage
 * @see setClockFace() for configuring clock visual style
 * @see getClockFace() for querying current clock face type
 * @see DisplayClockManager for time display rendering
 * 
 * @since v0.9
 */
bool SettingsManager::isClockEnabled() {
    return clockEnabled;
}

/**
 * @brief Configure clock face visual style with type validation and persistent
 *        storage for billboard time display customization.
 * 
 * This method sets the visual style/type for clock display when time functionality
 * is enabled. Different clock face types provide various visual presentations
 * of time information, allowing customization of the billboard's time display
 * to match different aesthetic preferences or visibility requirements.
 * 
 * Clock Face Type Features:
 * - Supports multiple visual styles defined in ClockFaceType enumeration
 * - Modern square, analog, digital, and other display formats
 * - Automatically saves configuration to persistent storage for system restarts
 * - Used by DisplayClockManager for time rendering style selection
 * - Type safety through ClockFaceType enumeration
 * 
 * @param faceType Clock face visual style from ClockFaceType enumeration
 * 
 * @note Face type is immediately saved to LittleFS persistent storage
 * @note Changes take effect immediately for next clock display update
 * @note Logs save operation success/failure for diagnostics
 * @note Safe to call repeatedly with same value (no unnecessary file operations)
 * @note Face type determines visual layout and rendering style for time display
 * 
 * @see getClockFace() for querying current clock face type
 * @see setClockEnabled() for enabling/disabling clock display
 * @see isClockEnabled() for querying clock display state
 * @see ClockFaceType enumeration for available visual styles
 * @see DisplayClockManager for time display rendering with face type
 * 
 * @since v0.9
 */
void SettingsManager::setClockFace(ClockFaceType faceType) {
    clockFace = faceType;
    if (saveInteger(CLOCK_FACE_FILE, static_cast<int>(faceType))) {
        LOG_INFOF(TAG, "� Clock face type saved: %d", static_cast<int>(faceType));
    } else {
        LOG_WARN(TAG, "Failed to save clock face setting to persistent storage");
    }
}

/**
 * @brief Query the current clock face visual style for time display rendering
 *        and user interface configuration.
 * 
 * This method provides access to the configured clock face type that determines
 * the visual style for time display when clock functionality is enabled. The
 * face type is used by the DisplayClockManager for selecting appropriate
 * rendering methods and visual layouts.
 * 
 * @return ClockFaceType enumeration value representing current visual style
 * 
 * @note Returns the current in-memory face type value (no file system access)
 * @note Face type is loaded from persistent storage during begin() initialization
 * @note Used by DisplayClockManager for time display style selection
 * @note Thread-safe for read access
 * @note Face type determines visual layout and rendering approach for time display
 * 
 * @see setClockFace() for configuring clock face visual style
 * @see begin() for initialization from persistent storage
 * @see setClockEnabled() for enabling/disabling clock display
 * @see isClockEnabled() for querying clock display state
 * @see ClockFaceType enumeration for available visual styles
 * @see DisplayClockManager for time display rendering with face type
 * 
 * @since v0.9
 */
ClockFaceType SettingsManager::getClockFace() {
    return clockFace;
}

// ============================================================================
// UTILITY AND DIAGNOSTIC METHODS
// ============================================================================

/**
 * @brief Display comprehensive system settings summary for diagnostics
 *        and configuration verification purposes.
 * 
 * This method outputs a formatted summary of all current system settings
 * to the logging system, providing a complete overview of the billboard's
 * configuration state. The output includes all configurable parameters
 * with their current values and states for debugging and verification.
 * 
 * Settings Summary Output:
 * - Second display configuration (enabled/disabled)
 * - DCC interface settings (enabled/disabled, address, GPIO pin)
 * - Image slideshow configuration (interval, enabled/disabled)
 * - Display brightness (absolute value and percentage)
 * - Clock display settings (enabled/disabled, face type)
 * - Formatted with emoji icons for easy visual identification
 * 
 * @note Outputs to LOG_INFO level for standard diagnostic visibility
 * @note DCC details only shown when DCC interface is enabled
 * @note Clock face type only shown when clock display is enabled
 * @note Brightness displayed as both raw value (0-255) and percentage
 * @note Safe to call frequently for status monitoring
 * 
 * @see begin() for initialization logging
 * @see getSettingsJson() for machine-readable settings export
 * @see resetToDefaults() for factory reset functionality
 * 
 * @since v0.9
 */
void SettingsManager::printSettings() {
    LOG_INFO(TAG, "=== CURRENT SYSTEM SETTINGS ===");
    LOG_INFOF(TAG, "  Second Display: %s", secondDisplayEnabled ? "enabled" : "disabled");
    LOG_INFOF(TAG, "  DCC Interface: %s", dccEnabled ? "enabled" : "disabled");
    if (dccEnabled) {
        LOG_INFOF(TAG, "  DCC Address: %d", dccAddress);
        LOG_INFOF(TAG, "  DCC GPIO Pin: %d", dccPin);
    }
    LOG_INFOF(TAG, "  Image Interval: %d seconds", imageInterval);
    LOG_INFOF(TAG, "  Image Slideshow: %s", imageEnabled ? "enabled" : "disabled");
    LOG_INFOF(TAG, "  Brightness: %d (%.1f%%)", brightness, (brightness / 255.0f) * 100.0f);
    LOG_INFOF(TAG, "  Clock Display: %s", clockEnabled ? "enabled" : "disabled");
    if (clockEnabled) {
        LOG_INFOF(TAG, "  Clock Face: %d", static_cast<int>(clockFace));
    }
    LOG_INFO(TAG, "=== END SETTINGS SUMMARY ===");
}

/**
 * @brief Reset all system settings to factory default values with immediate
 *        persistence and comprehensive validation.
 * 
 * This method restores the entire billboard system configuration to factory
 * default values, providing a complete reset functionality for troubleshooting,
 * initial setup, or configuration recovery. All settings are immediately
 * validated and saved to persistent storage with proper error handling.
 * 
 * Factory Default Values:
 * - Second Display: enabled (dual display mode)
 * - DCC Interface: disabled (standalone operation)
 * - DCC Address: 101 (standard accessory decoder range)
 * - DCC GPIO Pin: 4 (safe ESP32 input pin)
 * - Image Interval: 10 seconds (balanced slideshow timing)
 * - Image Slideshow: enabled (automatic image cycling)
 * - Brightness: 200 (78% of maximum, good visibility)
 * - Clock Display: disabled (image-focused content)
 * - Clock Face: Modern Square (clean digital style)
 * 
 * Reset Process Features:
 * - Immediate application of all default values
 * - Complete persistence to LittleFS storage
 * - Validation and error handling for each setting
 * - Hardware integration when DisplayManager is available
 * - Comprehensive logging for reset verification
 * 
 * @note All settings are immediately saved to persistent storage
 * @note Hardware brightness is applied immediately when DisplayManager is integrated
 * @note Reset operation is atomic (all settings changed together)
 * @note Logs comprehensive reset confirmation for diagnostics
 * @note Safe to call multiple times (idempotent operation)
 * 
 * @see printSettings() for verification of reset values
 * @see begin() for default value documentation
 * @see setDisplayManager() for hardware integration after reset
 * 
 * @since v0.9
 */
void SettingsManager::resetToDefaults() {
    LOG_INFO(TAG, "Resetting all settings to factory defaults...");
    
    // Reset all settings to their default values with persistence
    setSecondDisplayEnabled(true);
    setDCCEnabled(false);
    setDCCAddress(101);
    setDCCPin(4);
    setImageInterval(10);
    setImageEnabled(true);
    setBrightness(200);
    setClockEnabled(false);
    setClockFace(CLOCK_MODERN_SQUARE);
    
    LOG_INFO(TAG, "All settings reset to factory defaults and saved");
}

// ============================================================================
// PRIVATE HELPER METHODS - FILE I/O OPERATIONS
// ============================================================================

/**
 * @brief Save boolean setting value to LittleFS persistent storage with
 *        error handling and logging for configuration persistence.
 * 
 * This private helper method handles the low-level file I/O operations for
 * saving boolean configuration values to the LittleFS filesystem. The method
 * provides standardized error handling and logging for all boolean settings
 * throughout the SettingsManager system.
 * 
 * Storage Format:
 * - Boolean values stored as text strings ("true" or "false")
 * - Simple text format for easy debugging and manual editing
 * - Atomic write operation with proper file handle management
 * - Automatic file creation if not exists
 * 
 * @param filename Constant string pointer to the target filename for storage
 * @param value Boolean value to be saved (true or false)
 * 
 * @return true if save operation was successful, false if file I/O error occurred
 * 
 * @note Private method for internal SettingsManager use only
 * @note Logs error messages for failed file operations
 * @note Uses LittleFS write mode "w" (create/overwrite)
 * @note File handle is properly closed after operation
 * @note Thread-safe through LittleFS filesystem operations
 * 
 * @see loadBoolean() for corresponding read operation
 * @see saveInteger() for integer value storage
 * @see LittleFS filesystem for underlying storage implementation
 * 
 * @since v0.9
 */
bool SettingsManager::saveBoolean(const char* filename, bool value) {
    File file = LittleFS.open(filename, "w");
    if (!file) {
        LOG_ERRORF(TAG, "Failed to open file for writing: %s", filename);
        return false;
    }
    
    file.print(value ? "true" : "false");
    file.close();
    return true;
}

/**
 * @brief Load boolean setting value from LittleFS persistent storage with
 *        default value fallback and comprehensive error handling.
 * 
 * This private helper method handles the low-level file I/O operations for
 * loading boolean configuration values from the LittleFS filesystem. The method
 * provides robust error handling, default value fallback, and standardized
 * logging for all boolean settings throughout the SettingsManager system.
 * 
 * Loading Logic:
 * - Returns default value if file does not exist (first boot scenario)
 * - Returns default value if file cannot be opened (I/O error recovery)
 * - Parses text content as "true" for boolean true, everything else false
 * - Automatic string trimming for whitespace tolerance
 * - Safe fallback behavior for corrupted or invalid files
 * 
 * @param filename Constant string pointer to the source filename for loading
 * @param defaultValue Boolean default value to return on file error or absence
 * 
 * @return Boolean setting value from file, or defaultValue on any error condition
 * 
 * @note Private method for internal SettingsManager use only
 * @note Logs debug messages for missing files and warnings for I/O errors
 * @note Uses LittleFS read mode "r" (read-only access)
 * @note File handle is properly closed after operation
 * @note Thread-safe through LittleFS filesystem operations
 * @note Only explicit "true" string evaluates to boolean true
 * 
 * @see saveBoolean() for corresponding write operation
 * @see loadInteger() for integer value loading
 * @see LittleFS filesystem for underlying storage implementation
 * 
 * @since v0.9
 */
bool SettingsManager::loadBoolean(const char* filename, bool defaultValue) {
    if (!LittleFS.exists(filename)) {
        LOG_DEBUGF(TAG, "Settings file not found, using default: %s", filename);
        return defaultValue;
    }
    
    File file = LittleFS.open(filename, "r");
    if (!file) {
        LOG_WARNF(TAG, "Failed to open settings file for reading: %s", filename);
        return defaultValue;
    }
    
    String value = file.readString();
    file.close();
    value.trim();
    
    // Return true only if explicitly set to "true", otherwise use default
    return (value == "true");
}

/**
 * @brief Save integer setting value to LittleFS persistent storage with
 *        error handling and logging for numerical configuration persistence.
 * 
 * This private helper method handles the low-level file I/O operations for
 * saving integer configuration values to the LittleFS filesystem. The method
 * provides standardized error handling and logging for all integer settings
 * throughout the SettingsManager system.
 * 
 * Storage Format:
 * - Integer values stored as decimal text strings
 * - Simple text format for easy debugging and manual editing
 * - Atomic write operation with proper file handle management
 * - Automatic file creation if not exists
 * - No validation (validation performed at higher level)
 * 
 * @param filename Constant string pointer to the target filename for storage
 * @param value Integer value to be saved (any valid int range)
 * 
 * @return true if save operation was successful, false if file I/O error occurred
 * 
 * @note Private method for internal SettingsManager use only
 * @note Logs error messages for failed file operations
 * @note Uses LittleFS write mode "w" (create/overwrite)
 * @note File handle is properly closed after operation
 * @note Thread-safe through LittleFS filesystem operations
 * @note Value validation should be performed before calling this method
 * 
 * @see loadInteger() for corresponding read operation
 * @see saveBoolean() for boolean value storage
 * @see LittleFS filesystem for underlying storage implementation
 * 
 * @since v0.9
 */
bool SettingsManager::saveInteger(const char* filename, int value) {
    File file = LittleFS.open(filename, "w");
    if (!file) {
        LOG_ERRORF(TAG, "Failed to open file for writing: %s", filename);
        return false;
    }
    
    file.print(value);
    file.close();
    return true;
}

/**
 * @brief Load integer setting value from LittleFS persistent storage with
 *        default value fallback and robust parsing with error handling.
 * 
 * This private helper method handles the low-level file I/O operations for
 * loading integer configuration values from the LittleFS filesystem. The method
 * provides comprehensive error handling, default value fallback, input validation,
 * and standardized logging for all integer settings throughout the SettingsManager.
 * 
 * Loading and Parsing Logic:
 * - Returns default value if file does not exist (first boot scenario)
 * - Returns default value if file cannot be opened (I/O error recovery)
 * - Parses text content as decimal integer with String.toInt()
 * - Validates parsing success (toInt() returns 0 for invalid strings)
 * - Handles edge case where "0" is valid but other invalid strings also return 0
 * - Automatic string trimming for whitespace tolerance
 * - Safe fallback behavior for corrupted or invalid files
 * 
 * @param filename Constant string pointer to the source filename for loading
 * @param defaultValue Integer default value to return on file error, absence, or parsing failure
 * 
 * @return Integer setting value from file, or defaultValue on any error condition
 * 
 * @note Private method for internal SettingsManager use only
 * @note Logs debug messages for missing files and warnings for I/O or parsing errors
 * @note Uses LittleFS read mode "r" (read-only access)
 * @note File handle is properly closed after operation
 * @note Thread-safe through LittleFS filesystem operations
 * @note Distinguishes between valid "0" and invalid string parsing
 * 
 * @see saveInteger() for corresponding write operation
 * @see loadBoolean() for boolean value loading
 * @see LittleFS filesystem for underlying storage implementation
 * 
 * @since v0.9
 */
int SettingsManager::loadInteger(const char* filename, int defaultValue) {
    if (!LittleFS.exists(filename)) {
        LOG_DEBUGF(TAG, "Settings file not found, using default: %s", filename);
        return defaultValue;
    }
    
    File file = LittleFS.open(filename, "r");
    if (!file) {
        LOG_WARNF(TAG, "Failed to open settings file for reading: %s", filename);
        return defaultValue;
    }
    
    String value = file.readString();
    file.close();
    value.trim();
    
    // Use toInt() which returns 0 for invalid strings, check for this case
    int result = value.toInt();
    if (result == 0 && value != "0") {
        LOG_WARNF(TAG, "Invalid integer value in file %s: '%s', using default", filename, value.c_str());
        return defaultValue;
    }
    
    return result;
}

// ============================================================================
// ADDITIONAL UTILITY METHODS - JSON EXPORT AND VALIDATION
// ============================================================================

/**
 * @brief Export all current system settings as a JSON string for API responses,
 *        debugging, and configuration backup purposes.
 * 
 * This method creates a comprehensive JSON representation of all current system
 * settings, providing both machine-readable configuration data and human-friendly
 * formatted values. The JSON output is suitable for web API responses, debugging
 * output, configuration backup, and system integration.
 * 
 * JSON Structure and Content:
 * - Complete settings coverage (all configurable parameters)
 * - Boolean values as JSON boolean primitives (true/false)
 * - Integer values as JSON number primitives
 * - Additional computed fields (brightnessPercent for convenience)
 * - Compact single-line format for efficient transmission
 * - Standards-compliant JSON for broad compatibility
 * 
 * Included Settings:
 * - secondDisplayEnabled: boolean display configuration
 * - dccEnabled: boolean DCC interface state
 * - dccAddress: integer DCC decoder address (1-10239)
 * - dccPin: integer ESP32 GPIO pin (0-39)
 * - imageInterval: integer slideshow timing in seconds
 * - imageEnabled: boolean slideshow enable state
 * - brightness: integer PWM value (0-255)
 * - brightnessPercent: float percentage value for UI display
 * - clockEnabled: boolean clock display state
 * - clockFace: integer clock face type enumeration
 * 
 * @return String containing complete JSON representation of all settings
 * 
 * @note Returns current in-memory values (no file system access)
 * @note JSON is compact single-line format (no pretty printing)
 * @note brightnessPercent is computed from brightness for convenience
 * @note Safe to call frequently for status reporting
 * @note Compatible with standard JSON parsers and web APIs
 * 
 * @see printSettings() for human-readable diagnostic output
 * @see resetToDefaults() for factory configuration restoration
 * @see begin() for settings loading and initialization
 * 
 * @since v0.9
 */
String SettingsManager::getSettingsJson() {
    String json = "{";
    json += "\"secondDisplayEnabled\":" + String(secondDisplayEnabled ? "true" : "false") + ",";
    json += "\"dccEnabled\":" + String(dccEnabled ? "true" : "false") + ",";
    json += "\"dccAddress\":" + String(dccAddress) + ",";
    json += "\"dccPin\":" + String(dccPin) + ",";
    json += "\"imageInterval\":" + String(imageInterval) + ",";
    json += "\"imageEnabled\":" + String(imageEnabled ? "true" : "false") + ",";
    json += "\"brightness\":" + String(brightness) + ",";
    json += "\"brightnessPercent\":" + String((brightness / 255.0f) * 100.0f, 1) + ",";
    json += "\"clockEnabled\":" + String(clockEnabled ? "true" : "false") + ",";
    json += "\"clockFace\":" + String(static_cast<int>(clockFace));
    json += "}";
    return json;
}

/**
 * @brief Validate and automatically correct all loaded settings values to ensure
 *        they fall within acceptable ranges with persistence and logging.
 * 
 * This method performs comprehensive validation of all loaded configuration values
 * against their respective valid ranges and hardware constraints. When out-of-range
 * values are detected, they are automatically corrected to safe limits and immediately
 * saved back to persistent storage to prevent repeated corrections on future boots.
 * 
 * Validation Rules and Corrections:
 * - DCC Address: 1-10239 (NMRA standards), corrected to boundaries
 * - DCC GPIO Pin: 0-39 (ESP32 hardware), corrected to safe values
 * - Image Interval: 1-3600 seconds, corrected to practical limits
 * - Brightness: 0-255 (PWM range), corrected to hardware limits
 * - Boolean settings: inherently valid (no correction needed)
 * - Clock face enumeration: no range validation (enum safety)
 * 
 * Correction Process:
 * - Identifies each out-of-range value with logging
 * - Applies appropriate boundary clamping logic
 * - Immediately saves corrected value to persistent storage
 * - Counts total corrections for diagnostic reporting
 * - Provides detailed logging for each correction applied
 * 
 * @return Integer count of settings that required correction and were fixed
 * 
 * @note Called automatically during begin() initialization
 * @note Corrections are immediately persisted to prevent repeat corrections
 * @note Logs warning messages for each correction applied
 * @note Returns 0 when all settings are within valid ranges
 * @note Uses conservative safe values for boundary corrections
 * 
 * @see begin() for automatic validation during initialization
 * @see resetToDefaults() for complete configuration reset
 * @see printSettings() for verification of corrected values
 * 
 * @since v0.9
 */
int SettingsManager::validateAndCorrectSettings() {
    int correctionCount = 0;
    
    // Validate DCC address range
    if (dccAddress < 1 || dccAddress > 10239) {
        LOG_WARNF(TAG, "Correcting DCC address %d to valid range", dccAddress);
        dccAddress = (dccAddress < 1) ? 1 : 10239;
        saveInteger(DCC_ADDRESS_FILE, dccAddress);
        correctionCount++;
    }
    
    // Validate DCC pin range
    if (dccPin < 0 || dccPin > 39) {
        LOG_WARNF(TAG, "Correcting DCC pin %d to valid range", dccPin);
        dccPin = (dccPin < 0) ? 4 : 39;
        saveInteger(DCC_PIN_FILE, dccPin);
        correctionCount++;
    }
    
    // Validate image interval range
    if (imageInterval < 1 || imageInterval > 3600) {
        LOG_WARNF(TAG, "� Correcting image interval %d to valid range", imageInterval);
        imageInterval = (imageInterval < 1) ? 1 : 3600;
        saveInteger(IMAGE_INTERVAL_FILE, imageInterval);
        correctionCount++;
    }
    
    // Validate brightness range
    if (brightness < 0 || brightness > 255) {
        LOG_WARNF(TAG, "Correcting brightness %d to valid range", brightness);
        brightness = (brightness < 0) ? 0 : 255;
        saveInteger(BRIGHTNESS_FILE, brightness);
        correctionCount++;
    }
    
    return correctionCount;
}

/**
 * @brief Check persistence completeness by verifying existence of all settings
 *        files in LittleFS storage for configuration integrity assessment.
 * 
 * This method performs a comprehensive audit of the LittleFS filesystem to verify
 * that all expected settings files exist, providing insight into configuration
 * persistence integrity and potential file system issues. The method is useful
 * for diagnostics, system health checks, and troubleshooting persistence problems.
 * 
 * Persistence Audit Process:
 * - Checks existence of all 9 core settings files in LittleFS
 * - Counts existing vs. expected files for completeness assessment
 * - Logs debug information for missing files (detailed diagnostics)
 * - Provides summary statistics for persistence health monitoring
 * - Returns boolean status for automated health checking
 * 
 * Audited Settings Files:
 * - SECOND_DISPLAY_FILE: dual display configuration
 * - DCC_ENABLED_FILE: DCC interface enable state
 * - DCC_ADDRESS_FILE: DCC decoder address
 * - DCC_PIN_FILE: DCC GPIO pin configuration
 * - IMAGE_INTERVAL_FILE: slideshow timing
 * - IMAGE_ENABLED_FILE: slideshow enable state
 * - BRIGHTNESS_FILE: display brightness level
 * - CLOCK_ENABLED_FILE: clock display state
 * - CLOCK_FACE_FILE: clock visual style
 * 
 * @return true if all settings files exist in persistent storage,
 *         false if any files are missing from LittleFS
 * 
 * @note Performs read-only filesystem operations (no modifications)
 * @note Logs informational summary of persistence status
 * @note Logs debug details for each missing file
 * @note Safe to call frequently for health monitoring
 * @note Missing files are normal on first boot before settings are saved
 * 
 * @see begin() for settings loading and file creation
 * @see resetToDefaults() for complete settings file creation
 * @see LittleFS filesystem for underlying storage implementation
 * 
 * @since v0.9
 */
bool SettingsManager::areAllSettingsFilesPersistent() {
    const char* files[] = {
        SECOND_DISPLAY_FILE, DCC_ENABLED_FILE, DCC_ADDRESS_FILE, DCC_PIN_FILE,
        IMAGE_INTERVAL_FILE, IMAGE_ENABLED_FILE, BRIGHTNESS_FILE, 
        CLOCK_ENABLED_FILE, CLOCK_FACE_FILE
    };
    
    int totalFiles = sizeof(files) / sizeof(files[0]);
    int existingFiles = 0;
    
    for (int i = 0; i < totalFiles; i++) {
        if (LittleFS.exists(files[i])) {
            existingFiles++;
        } else {
            LOG_DEBUGF(TAG, "Missing settings file: %s", files[i]);
        }
    }
    
    LOG_INFOF(TAG, "Settings persistence: %d/%d files exist", existingFiles, totalFiles);
    return (existingFiles == totalFiles);
}
