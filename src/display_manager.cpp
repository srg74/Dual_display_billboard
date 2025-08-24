#include "display_manager.h"
#include "display_timing_config.h"
#include "secrets.h"
#include "logger.h"
#include "splash_screen.h"
#include <SPI.h>

/**
 * @brief Constructor initializes DisplayManager with uninitialized hardware state.
 * Sets default brightness values and initialization status flag. Hardware
 * configuration and actual initialization must be performed by begin() execution.
 * 
 * @see begin() for complete hardware initialization sequence
 * @see initializeBacklight() for PWM brightness configuration
 * 
 * @since v0.9
 */
DisplayManager::DisplayManager() : initialized(false), brightness1(255), brightness2(255), 
                                   secondDisplayEnabled(true),  // Default to enabled for dual display builds
                                   splashStartTime(0), splashActive(false), splashTimeoutMs(2000),
                                   portalSequenceActive(false) {
}

/**
 * @brief 🚀 Initializes complete dual display hardware system
 * 
 * Performs comprehensive hardware initialization sequence for dual ST7735/ST7789 displays.
 * Configures PWM backlight control, chip select pins, TFT interface, and immediately
 * clears both displays to prevent visual artifacts during startup.
 * 
 * Initialization Sequence:
 * 1. 💡 Backlight PWM configuration (5kHz, 8-bit resolution)
 * 2. Chip select pin setup with safe deselect state
 * 3. TFT library initialization with dual CS method
 * 4. ⚫ Immediate display clearing to prevent startup flash
 * 
 * Hardware Requirements:
 * • ESP32/ESP32-S3 with sufficient GPIO pins
 * • Dual ST7735 or ST7789 TFT displays
 * • Independent backlight control circuits
 * • Proper SPI wiring with shared MOSI/MISO/SCK
 * 
 * @return true Always returns true for compatibility
 * 
 * @note Sets initialized flag to true upon successful completion
 * @note Both displays set to maximum brightness (255) by default
 * @warning Requires proper hardware connections as defined in display_hardware_config.h
 * 
 * @see initializeBacklight() for PWM backlight setup details
 * @see initializeCS() for chip select pin configuration
 * @see initializeTFT() for TFT library setup process
 */
bool DisplayManager::begin() {
    LOG_INFO("DISPLAY", "🎨 Initializing Display Manager with working config");
    
    initializeBacklight();
    initializeCS();
    initializeTFT();
    
    // IMMEDIATE: Clear both displays to black before any other operations
    clearBothDisplaysToBlack();
    
    initialized = true;
    LOG_INFO("DISPLAY", "Display Manager initialized successfully");
    return true;
}

/**
 * @brief Initializes PWM backlight control for both displays
 * 
 * Configures hardware PWM channels for independent backlight brightness control.
 * Each display has its own PWM channel with 5kHz frequency and 8-bit resolution
 * providing smooth brightness control from 0 to 255.
 * 
 * Hardware Configuration:
 * - Display 1: GPIO 22 on PWM Channel 1 (5kHz, 8-bit, full brightness)
 * - Display 2: GPIO 27 on PWM Channel 2 (5kHz, 8-bit, full brightness)
 * 
 * @note Both displays initialized to maximum brightness (255)
 * @note Uses ESP32 LEDC peripheral for PWM generation
 * @note Called automatically during begin() initialization
 * 
 * @see setBrightness() for runtime brightness control
 * @see begin() for complete initialization sequence
 * 
 * @since v0.9
 */
void DisplayManager::initializeBacklight() {
    LOG_INFO("DISPLAY", "Setting up backlights...");
    
#ifdef ESP32S3_MODE
    // ESP32S3: Use higher resolution PWM for better brightness control
    // Different pins but optimized PWM settings to match ESP32 brightness levels
    // Note: ESP32S3 also has text rendering differences requiring position adjustments
    // in TextUtils and DisplayClockManager for consistent Unicode character display
    ledcAttachPin(TFT_BACKLIGHT1_PIN, 3); // GPIO 7 → Channel 3
    ledcSetup(3, 5000, 10); // Channel 3, 5 KHz, 10-bit (higher resolution than ESP32)
    ledcWrite(3, 1023); // Full brightness (10-bit max = 1023)
    
    ledcAttachPin(TFT_BACKLIGHT2_PIN, 4); // GPIO 8 → Channel 4  
    ledcSetup(4, 5000, 10); // Channel 4, 5 KHz, 10-bit (higher resolution than ESP32)
    ledcWrite(4, 1023); // Full brightness (10-bit max = 1023)
    
    LOG_INFO("DISPLAY", "ESP32S3 backlights initialized (PWM channels 3,4, 10-bit, 5kHz - optimized for brightness)");
#else
    // ESP32 original setup (working) - keep PWM
    ledcAttachPin(TFT_BACKLIGHT1_PIN, 1); // GPIO 22 → Channel 1
    ledcSetup(1, 5000, 8); // Channel 1, 5 KHz, 8-bit
    ledcWrite(1, 255); // Full brightness
    
    ledcAttachPin(TFT_BACKLIGHT2_PIN, 2); // GPIO 27 → Channel 2
    ledcSetup(2, 5000, 8); // Channel 2, 5 KHz, 8-bit
    ledcWrite(2, 255); // Full brightness
    
    LOG_INFO("DISPLAY", "ESP32 backlights initialized (channels 1,2, 8-bit)");
#endif
}

/**
 * @brief Initializes chip select (CS) pins for dual display control
 * 
 * Configures GPIO pins for individual display selection in dual display setup.
 * Both pins are set to OUTPUT mode and initialized to HIGH (deselected state)
 * to ensure safe starting condition before any display operations.
 * 
 * Hardware Pin Mapping:
 * - First Display CS: GPIO 5 (firstScreenCS)
 * - Second Display CS: GPIO 15 (secondScreenCS)
 * 
 * @note Both CS pins start in deselected state (HIGH)
 * @note Called automatically during begin() initialization
 * @note Essential for preventing display conflicts in dual setup
 * 
 * @see selectDisplay() for runtime display activation
 * @see deselectAll() for deactivating all displays
 * @see begin() for complete initialization sequence
 * 
 * @since v0.9
 */
void DisplayManager::initializeCS() {
    LOG_INFO("DISPLAY", "Setting up CS pins...");
    
    // CS pins setup (exact copy from working project)
    pinMode(firstScreenCS, OUTPUT);     // GPIO 10/5 = OUTPUT
    digitalWrite(firstScreenCS, HIGH);  // GPIO 10/5 = HIGH (deselected)
    pinMode(secondScreenCS, OUTPUT);    // GPIO 21/15 = OUTPUT
    digitalWrite(secondScreenCS, HIGH); // GPIO 21/15 = HIGH (deselected)

    LOG_INFO("DISPLAY", "CS pins configured");
}

/**
 * @brief Initializes TFT library with dual display configuration
 * 
 * Performs TFT_eSPI library initialization using dual CS selection method.
 * Both displays are temporarily selected during initialization to ensure
 * proper hardware recognition, then immediately cleared and configured
 * with correct rotation settings.
 * 
 * Initialization Sequence:
 * 1. Select both displays (CS pins LOW)
 * 2. Initialize TFT library 
 * 3. Clear initialization flash artifacts
 * 4. Configure individual display rotations
 * 5. Clear both displays to black
 * 6. Deselect all displays
 * 
 * @note Uses dual CS method for reliable initialization
 * @note Immediately clears displays to prevent visual artifacts
 * @note Sets rotation 0 for both displays as default
 * @note Called automatically during begin() initialization
 * 
 * @see selectDisplay() for runtime display selection
 * @see begin() for complete initialization sequence
 * 
 * @since v0.9
 */
void DisplayManager::initializeTFT() {
    LOG_INFO("DISPLAY", "Initializing TFT...");
    
#ifdef ESP32S3_MODE
    // ESP32S3: Explicit SPI initialization since TFT_CS=-1
    SPI.begin(12, -1, 11, -1); // SCLK=12, MISO=-1, MOSI=11, SS=-1
    LOG_INFO("DISPLAY", "ESP32S3: SPI bus initialized");
#endif
    
    // TFT initialization (EXACT copy from working project)
    digitalWrite(firstScreenCS, LOW);   // Both CS LOW
    digitalWrite(secondScreenCS, LOW);
    tft.init();                         // Init with both selected
    tft.fillScreen(TFT_BLACK);          // Immediately clear any init flash
    digitalWrite(firstScreenCS, HIGH);  // Both CS HIGH
    digitalWrite(secondScreenCS, HIGH);
    
    // Set correct rotation for both displays immediately after init
    LOG_INFO("DISPLAY", "Configuring Display 1...");
    selectDisplay(1);
    tft.setRotation(0);  // Ensure display 1 uses correct rotation
    tft.fillScreen(TFT_BLACK);  // Clear display 1
    LOG_INFO("DISPLAY", "Display 1 configured");
    
    LOG_INFO("DISPLAY", "Configuring Display 2...");
    selectDisplay(2); 
    tft.setRotation(0);  // Ensure display 2 uses correct rotation
    tft.fillScreen(TFT_BLACK);  // Clear display 2
    LOG_INFO("DISPLAY", "Display 2 configured");
    
    deselectAll();
    
    LOG_INFO("DISPLAY", "TFT initialized with dual CS method");
}

/**
 * Selects the active display for operations
 * 
 * This method activates the specified display by controlling chip select (CS) lines
 * and setting the appropriate rotation for text display. Acts as a convenience
 * wrapper for backward compatibility, defaulting to text rotation.
 * 
 * @param displayNum The display number to select (1 or 2)
 * 
 * @warning Deselects all displays first, then selects the target display
 * @note 📝 Uses text rotation (DISPLAY_TEXT_ROTATION) as default
 * 
 * @see selectDisplayForText() for explicit text rotation
 * @see selectDisplayForImage() for image-specific rotation
 * @see deselectAll() for deactivating all displays
 * 
 * @since v0.9
 */
void DisplayManager::selectDisplay(int displayNum) {
    // Default to text rotation for backward compatibility
    selectDisplayForText(displayNum);
}

/**
 * 📝 Selects display with text-optimized rotation
 * 
 * Activates the specified display and configures it for optimal text rendering
 * by setting the text rotation (DISPLAY_TEXT_ROTATION). This method ensures
 * text appears properly oriented for reading.
 * 
 * @param displayNum The display number to activate (1 or 2)
 *                   - 1: First display (firstScreenCS)
 *                   - 2: Second display (secondScreenCS)
 * 
 * @warning Always deselects all displays before selecting target
 * @note Text rotation is configured via DISPLAY_TEXT_ROTATION constant
 * @note 🔄 Invalid display numbers are silently ignored
 * 
 * @see selectDisplayForImage() for image-specific rotation
 * @see deselectAll() for deactivating all displays
 * 
 * @since v0.9
 */
void DisplayManager::selectDisplayForText(int displayNum) {
    deselectAll();
    
    if (displayNum == 1) {
        digitalWrite(firstScreenCS, LOW);
        tft.setRotation(DISPLAY_TEXT_ROTATION);  // Use text rotation
    } else if (displayNum == 2) {
        digitalWrite(secondScreenCS, LOW);
        tft.setRotation(DISPLAY_TEXT_ROTATION);  // Use text rotation
    }
}

/**
 * @brief Selects display with image-optimized rotation
 * 
 * Activates the specified display and configures it for optimal image rendering
 * by setting the image rotation (DISPLAY_IMAGE_ROTATION). This method ensures
 * images are displayed in their correct orientation for viewing.
 * 
 * @param displayNum The display number to activate (1 or 2)
 *                   - 1: First display (firstScreenCS)
 *                   - 2: Second display (secondScreenCS)
 * 
 * @warning Always deselects all displays before selecting target
 * @note Image rotation is configured via DISPLAY_IMAGE_ROTATION constant
 * @note Invalid display numbers are silently ignored
 * 
 * @see selectDisplayForText() for text-specific rotation
 * @see deselectAll() for deactivating all displays
 * 
 * @since v0.9
 */
void DisplayManager::selectDisplayForImage(int displayNum) {
    deselectAll();
    
    if (displayNum == 1) {
        digitalWrite(firstScreenCS, LOW);
        tft.setRotation(DISPLAY_IMAGE_ROTATION);  // Use image rotation (0)
    } else if (displayNum == 2) {
        digitalWrite(secondScreenCS, LOW);
        tft.setRotation(DISPLAY_IMAGE_ROTATION);  // Use image rotation (0)
    }
}

/**
 * @brief Deactivates all displays by setting chip select lines HIGH
 * 
 * Sets both display chip select pins to HIGH state, effectively deselecting
 * all displays. This ensures no display is active before selecting a new one
 * and prevents potential conflicts between multiple displays.
 * 
 * @note Called automatically by selectDisplay methods before activation
 * @note Both firstScreenCS and secondScreenCS pins set to HIGH
 * 
 * @see selectDisplay() for display selection with automatic deselection
 * @see selectDisplayForText() for text-optimized display selection
 * @see selectDisplayForImage() for image-optimized display selection
 * 
 * @since v0.9
 */
void DisplayManager::deselectAll() {
    digitalWrite(firstScreenCS, HIGH);
    digitalWrite(secondScreenCS, HIGH);
}

/**
 * @brief Clears both displays to black and turns off backlight
 * 
 * Initializes both displays to a completely dark state by filling the screen
 * with black pixels and setting brightness to zero. This method is typically
 * called during system startup or reset to ensure displays start from a
 * known clean state.
 * 
 * The operation sequence:
 * 1. Activates display 1, fills with black, sets brightness to 0
 * 2. Activates display 2, fills with black, sets brightness to 0
 * 3. Deactivates all displays
 * 
 * @note Both screen content and backlight are cleared
 * @note Uses setBrightness(0) to turn off backlight completely
 * @note Ensures consistent startup state across both displays
 * 
 * @see selectDisplay() for individual display activation
 * @see setBrightness() for backlight control
 * @see deselectAll() for display deactivation
 * 
 * @since v0.9
 */
void DisplayManager::clearBothDisplaysToBlack() {
    LOG_INFO("DISPLAY", "⚫ Clearing both displays to black");
    
    // Clear display content to black
    selectDisplay(1);
    tft.fillScreen(TFT_BLACK);
    
    selectDisplay(2);
    tft.fillScreen(TFT_BLACK);
    
    // Deselect all
    deselectAll();

#ifdef ESP32S3_MODE
    // ESP32S3: Keep backlights ON with 10-bit PWM (1023 = max brightness)
    setBrightness(255, 1);  // Keep Display 1 brightness ON (will be scaled to 1023 internally)
    setBrightness(255, 2);  // Keep Display 2 brightness ON (will be scaled to 1023 internally)
    LOG_INFO("DISPLAY", "ESP32S3: Both displays cleared to black with backlights ON (10-bit PWM)");
#else
    // ESP32: Turn off brightness for complete darkness during startup
    setBrightness(0, 1);  // Turn off Display 1 brightness
    setBrightness(0, 2);  // Turn off Display 2 brightness
    LOG_INFO("DISPLAY", "ESP32: Both displays cleared to dark (no light)");
#endif
}

/**
 * @brief Sets backlight brightness for specified display(s)
 * 
 * Controls the PWM-driven backlight brightness for one or both displays.
 * Uses hardware PWM channels to provide smooth brightness control from
 * 0 (completely off) to 255 (maximum brightness). Platform-specific PWM:
 * - ESP32: 8-bit PWM (0-255 range, 5kHz)
 * - ESP32S3: 8-bit PWM (0-255 range, 5kHz) for consistent brightness
 * 
 * Channel mapping (hardware-specific):
 * - ESP32: Display 1=GPIO22/Ch1, Display 2=GPIO27/Ch2  
 * - ESP32S3: Display 1=GPIO7/Ch3, Display 2=GPIO8/Ch4
 * 
 * @param brightness PWM duty cycle value (0-255)
 *                   0 = backlight off, 255 = maximum brightness
 * @param displayNum Target display number
 *                   1 = first display only
 *                   2 = second display only  
 *                   0 = both displays
 * 
 * @note Channel assignment is swapped due to hardware wiring
 * @note Brightness values are stored in brightness1/brightness2 variables
 * @note Uses ledcWrite() for hardware PWM control
 * 
 * @see clearBothDisplaysToBlack() for setting brightness to 0
 * @see begin() for PWM channel initialization
 * 
 * @since v0.9
 */
void DisplayManager::setBrightness(uint8_t brightness, int displayNum) {
#ifdef ESP32S3_MODE
    // ESP32S3: Use 10-bit PWM resolution, scale 8-bit brightness (0-255) to 10-bit (0-1023)
    uint32_t scaledBrightness = (brightness * 1023) / 255;
    if (displayNum == 1 || displayNum == 0) {
        brightness1 = brightness;
        ledcWrite(3, scaledBrightness); // Apply to backlight 1 (GPIO 7, Channel 3)
        LOG_INFOF("DISPLAY", "ESP32S3 Backlight 1: %d/255 -> %d/1023 (%.1f%%)", brightness, scaledBrightness, (brightness / 255.0f) * 100.0f);
    }
    if (displayNum == 2 || displayNum == 0) {
        brightness2 = brightness;
        ledcWrite(4, scaledBrightness); // Apply to backlight 2 (GPIO 8, Channel 4)
        LOG_INFOF("DISPLAY", "ESP32S3 Backlight 2: %d/255 -> %d/1023 (%.1f%%)", brightness, scaledBrightness, (brightness / 255.0f) * 100.0f);
    }
#else
    // ESP32 original setup (working) - restore original channel mapping
    if (displayNum == 1 || displayNum == 0) {
        brightness1 = brightness;
        ledcWrite(2, brightness); // Apply to backlight 1 (GPIO 22, Channel 2) - Original working
        LOG_INFOF("DISPLAY", "🔆 Brightness set - Display 1: %d", brightness);
    }
    if (displayNum == 2 || displayNum == 0) {
        brightness2 = brightness;
        ledcWrite(1, brightness); // Apply to backlight 2 (GPIO 27, Channel 1) - Original working
        LOG_INFOF("DISPLAY", "🔆 Brightness set - Display 2: %d", brightness);
    }
#endif
}

/**
 * @brief Fills entire screen(s) with specified color
 * 
 * Clears the display buffer and fills it with a solid color. Can target
 * individual displays or both simultaneously. Automatically handles
 * display selection and deselection for safe multi-display operation.
 * 
 * @param color 16-bit RGB565 color value
 *              Common values: TFT_BLACK, TFT_WHITE, TFT_RED, TFT_GREEN, TFT_BLUE
 * @param displayNum Target display selection
 *                   0 = both displays (sequential operation)
 *                   1 = first display only
 *                   2 = second display only
 * 
 * @note For displayNum=0, displays are filled sequentially, not simultaneously
 * @note Uses selectDisplay() for safe chip select management
 * @note Automatically calls deselectAll() after operation
 * 
 * @see selectDisplay() for individual display activation
 * @see deselectAll() for display deactivation
 * @see clearBothDisplaysToBlack() for black fill with brightness control
 * 
 * @since v0.9
 */
void DisplayManager::fillScreen(uint16_t color, int displayNum) {
    if (displayNum == 0) {
        // Both displays
        selectDisplay(1);
        tft.fillScreen(color);
        selectDisplay(2);
        tft.fillScreen(color);
        deselectAll();
    } else {
        selectDisplay(displayNum);
        tft.fillScreen(color);
        deselectAll();
    }
}

/**
 * @brief Draws text on specified display at given coordinates
 * 
 * Renders text string at specified pixel coordinates using the TFT library.
 * Automatically handles display selection and deselection for safe operation.
 * Uses basic text rendering with standard size and specified color.
 * 
 * @param text Null-terminated string to display
 * @param x Horizontal pixel coordinate (0 = left edge)
 * @param y Vertical pixel coordinate (0 = top edge) 
 * @param color 16-bit RGB565 text color
 * @param displayNum Target display number (1 or 2)
 * 
 * @note Uses fixed text size of 1 for consistent rendering
 * @note Text may wrap or clip if coordinates exceed display bounds
 * @note Display is automatically selected and deselected
 * 
 * @see selectDisplay() for display activation
 * @see deselectAll() for display deactivation
 * @see fillScreen() for clearing display before text
 * 
 * @since v0.9
 */
void DisplayManager::drawText(const char* text, int x, int y, uint16_t color, int displayNum) {
    selectDisplay(displayNum);
    tft.setTextColor(color);
    tft.setTextSize(1);
    tft.setCursor(x, y);
    tft.print(text);
    deselectAll();
}

/**
 * @brief Enables or disables the second display
 * 
 * Controls the operational state of the second display. When disabled,
 * the display is cleared to black to provide visual feedback. When enabled,
 * the display becomes available for normal operations.
 * 
 * @param enable True to enable second display, false to disable
 * 
 * @note When disabled, second display is filled with black
 * @note Logging indicates current state change
 * @note Does not affect display hardware initialization
 * 
 * @see selectDisplay() for display activation
 * @see fillScreen() for display clearing
 * @see deselectAll() for display deactivation
 * 
 * @since v0.9
 */
void DisplayManager::enableSecondDisplay(bool enable) {
    secondDisplayEnabled = enable;  // Store the state
    
    if (enable) {
        setBrightness(255, 2);  // Enable brightness for Display 2
        LOG_INFO("DISPLAY", "Second display enabled with full brightness");
    } else {
        selectDisplay(2);
        tft.fillScreen(TFT_BLACK);
        deselectAll();
        setBrightness(0, 2);  // Disable brightness for Display 2
        LOG_INFO("DISPLAY", "⚫ Second display disabled (brightness = 0)");
    }
}

/**
 * @brief Alternates color display between two screens at timed intervals
 * 
 * Automatically switches between displays, filling the first with blue
 * and the second with yellow at regular intervals. Uses static variables
 * to maintain state between calls. Primarily used for testing or demo modes.
 * 
 * The alternation cycle:
 * 1. Display 1: Blue background
 * 2. Display 2: Yellow background
 * 3. Repeat after DISPLAY_ALTERNATING_INTERVAL_MS milliseconds
 * 
 * @note Uses static variables to track timing and current display
 * @note Logging is commented out to reduce log spam during continuous operation
 * @note Interval controlled by DISPLAY_ALTERNATING_INTERVAL_MS constant
 * 
 * @see selectDisplay() for display activation
 * @see fillScreen() for color fill operations
 * @see deselectAll() for display deactivation
 * 
 * @since v0.9
 */
void DisplayManager::alternateDisplays() {
    static bool useFirst = true;
    static unsigned long lastSwitch = 0;
    
    if (millis() - lastSwitch > DISPLAY_ALTERNATING_INTERVAL_MS) {
        if (useFirst) {
            selectDisplay(1);
            tft.fillScreen(TFT_BLUE);
            deselectAll();
            // LOG_INFO("DISPLAY", "🔵 First screen BLUE"); // Commented to reduce log spam
        } else {
            selectDisplay(2);
            tft.fillScreen(TFT_YELLOW);
            deselectAll();
            // LOG_INFO("DISPLAY", "🟡 Second screen YELLOW"); // Commented to reduce log spam
        }
        
        useFirst = !useFirst;
        lastSwitch = millis();
    }
}

/**
 * @brief Displays quick status message on first display
 * 
 * Shows a status message on display 1 with colored background while keeping
 * display 2 dark. Designed for system notifications, errors, or status updates.
 * Message is centered on screen with automatic text color selection for contrast.
 * 
 * Display behavior:
 * - Display 1: Shows message with background color
 * - Display 2: Always kept black with brightness 0
 * 
 * @param message Text string to display on screen
 * @param color Background color (RGB565), also affects text color selection
 * 
 * @note Text color automatically chosen: white on red background, black otherwise
 * @note Display 2 is always cleared and dimmed during status display
 * @note Returns immediately if DisplayManager not initialized
 * @note Uses centered text positioning at coordinates (80, 40)
 * 
 * @see selectDisplay() for display activation
 * @see setBrightness() for backlight control
 * @see deselectAll() for display deactivation
 * 
 * @since v0.9
 */
// FIXED: Fast status display method - USES DISPLAY 1 FOR MESSAGES
void DisplayManager::showQuickStatus(const String& message, uint16_t color) {
    if (!initialized) return;
    
    // Display 1: Show status messages
    selectDisplay(1);  
    tft.fillScreen(color);
    // Always use WHITE text for all status messages (matches user specifications)
    tft.setTextColor(TFT_WHITE);
    tft.setTextSize(1);
    tft.setTextDatum(MC_DATUM);
    tft.drawString(message, 80, 40, 2);
    
    deselectAll();
}

/**
 * @brief Returns the current splash screen activation state
 * 
 * Provides access to the internal splash screen state without modification.
 * Used by other components to determine if splash screen is currently active
 * and adjust behavior accordingly.
 * 
 * @return True if splash screen is currently active, false otherwise
 * 
 * @note This is a read-only state query method
 * @note Splash state is managed by other display methods
 * 
 * @see showSplashScreen() for splash activation
 * @see hideSplashScreen() for splash deactivation
 * 
 * @since v0.9
 */
bool DisplayManager::isSplashActive() {
    return splashActive;
}

/**
 * @brief Displays AP starting notification on primary display
 * 
 * Shows a quick status message indicating that Access Point mode is starting.
 * Uses orange background color to indicate transition state. Provides immediate
 * visual feedback during AP initialization process.
 * 
 * @note Uses showQuickStatus() with predefined message and color
 * @note Display 1 shows orange "Starting AP..." message
 * @note Display 2 remains dark during this status
 * 
 * @see showQuickStatus() for underlying display mechanism
 * @see showAPReady() for AP ready notification
 * 
 * @since v0.9
 */
// Quick AP starting indicator
void DisplayManager::showAPStarting() {
    showQuickStatus("Starting AP...", TFT_ORANGE);
}

/**
 * @brief Displays AP ready notification on primary display
 * 
 * Shows a quick status message confirming that Access Point mode is ready
 * and accepting connections. Uses blue background to indicate ready state.
 * Provides clear visual confirmation of successful AP initialization.
 * 
 * @note Uses showQuickStatus() with predefined message and color
 * @note Display 1 shows blue "AP Ready!" message
 * @note Display 2 remains dark during this status
 * 
 * @see showQuickStatus() for underlying display mechanism
 * @see showAPStarting() for AP starting notification
 * 
 * @since v0.9
 */
// Quick AP ready indicator  
void DisplayManager::showAPReady() {
    showQuickStatus("AP Ready!", TFT_BLUE);
}

/**
 * @brief Displays WiFi connection attempt notification
 * 
 * Shows a quick status message indicating active WiFi connection attempt.
 * Uses yellow background to indicate pending/in-progress state. Provides
 * visual feedback during network connection process.
 * 
 * @note Uses showQuickStatus() with predefined message and color
 * @note Display 1 shows yellow "Connecting..." message
 * @note Display 2 remains dark during this status
 * 
 * @see showQuickStatus() for underlying display mechanism
 * @see showConnectionSuccess() for successful connection notification
 * 
 * @since v0.9
 */
// Quick connecting indicator
void DisplayManager::showConnecting() {
    showQuickStatus("Connecting...", 0xFCC0);  // #ff9900 (orange) converted to RGB565
}

/**
 * @brief Displays comprehensive portal configuration information
 * 
 * Shows detailed portal information including SSID, IP address, and status
 * on the primary display with green background. Designed for configuration
 * portal mode when device acts as access point for initial setup.
 * 
 * Display layout uses optimized spacing for 160x80 pixel screen:
 * - Line 1: SSID (typically "Billboard-Portal")
 * - Line 2: IP address for portal access
 * - Line 3: Current status message
 * 
 * @param ssid Access point SSID name
 * @param ip IP address for portal access
 * @param status Current portal status message
 * 
 * @note Display 1 shows green background with black text
 * @note Display 2 remains dark with brightness set to 0
 * @note Uses left-aligned text positioning to prevent clipping
 * @note Returns immediately if DisplayManager not initialized
 * 
 * @see showQuickStatus() for simple status messages
 * @see showConnectionSuccess() for WiFi connection confirmation
 * @see showPortalSequence() for splash-to-portal transition
 * 
 * @since v0.9
 */
// FIXED: Portal info method - USES DISPLAY 1 FOR PORTAL INFO
void DisplayManager::showPortalInfo(const String& ssid, const String& ip, const String& status) {
    if (!initialized) {
        LOG_WARN("DISPLAY", "Display not initialized - cannot show portal info");
        return;
    }
    
    LOG_INFO("DISPLAY", "📋 Showing portal information on display 1");  // FIXED
    
    // Display 1: Show portal info with #00b33c background (brighter green)
    selectDisplay(1);  // FIXED: Changed to 1
    tft.fillScreen(0x058F);  // #00b33c (brighter green) converted to RGB565
    tft.setTextColor(TFT_WHITE, 0x058F);  // WHITE text on brighter green background
    tft.setTextSize(1);  // Text size 1
    
    // Use LEFT alignment to prevent clipping
    tft.setTextDatum(TL_DATUM); // Top Left alignment
    
    // Screen dimensions: 160x80 pixels
    // Optimized spacing for font size 2
    int startX = 8;      // Left margin
    int startY = 8;      // Top margin 
    int lineHeight = 20; // Space for font size 2
    
    // Line 1: SSID (Billboard-Portal)
    tft.drawString(ssid, startX, startY, 2);  // Font size 2
    
    // Line 2: IP address  
    tft.drawString(ip, startX, startY + lineHeight, 2);
    
    // Line 3: Status
    tft.drawString(status, startX, startY + (lineHeight * 2), 2);
    
    deselectAll();
    
    LOG_INFO("DISPLAY", "Portal info displayed - Screen 1: #00b33c background with WHITE text");
}

/**
 * @brief Displays WiFi connection success confirmation
 * 
 * Shows confirmation message for successful WiFi connection including
 * the assigned IP address. Uses blue background to indicate successful
 * connection state and provides essential network information.
 * 
 * Display layout for 80x160 pixel screen (rotated):
 * - Line 1: "Connected to Wi-Fi" confirmation
 * - Line 2: "IP: XXX.XXX.XXX.XXX" address display
 * 
 * @param ip The assigned IP address to display
 * 
 * @note Display 1 shows blue background with white text
 * @note Display 2 remains dark with brightness set to 0
 * @note Uses left-aligned text positioning for readability
 * @note Returns immediately if DisplayManager not initialized
 * 
 * @see showQuickStatus() for simple status messages
 * @see showPortalInfo() for portal configuration display
 * @see showConnecting() for connection attempt notification
 * 
 * @since v0.9
 */
// Show WiFi connection success message
void DisplayManager::showConnectionSuccess(const String& ip) {
    if (!initialized) {
        LOG_WARN("DISPLAY", "Display not initialized - cannot show connection success");
        return;
    }
    
    LOG_INFO("DISPLAY", "Showing WiFi connection success on display 1");
    
    // Display 1: Show connection success with #0000cc background (blue)
    selectDisplay(1);
    tft.fillScreen(0x001F);  // #0000cc (blue) converted to RGB565
    tft.setTextColor(TFT_WHITE, 0x001F);  // WHITE text on blue background
    tft.setTextSize(1);  // Text size 1
    
    // Use LEFT alignment (same as portal info)
    tft.setTextDatum(TL_DATUM); // Top Left alignment
    
    // Screen dimensions: 80x160 pixels (rotated)
    int startX = 8;      // Left margin
    int startY = 20;     // Top margin (centered vertically)
    int lineHeight = 20; // Space between lines
    
    // Line 1: "Connected to Wi-Fi"
    tft.drawString("Connected to Wi-Fi", startX, startY, 2);  // Font size 2
    
    // Line 2: "IP: XX.XX.XX.XX"
    String ipText = "IP: " + ip;
    tft.drawString(ipText, startX, startY + lineHeight, 2);  // Font size 2
    
    deselectAll();
    
    LOG_INFOF("DISPLAY", "Connection success displayed - IP: %s", ip.c_str());
}

/**
 * @brief Draws RGB565 color bitmap stored in PROGMEM
 * 
 * Renders a color bitmap stored in program memory (PROGMEM) pixel by pixel.
 * Each pixel is read using pgm_read_word() and drawn individually to support
 * bitmaps that exceed available RAM capacity.
 * 
 * @param x Top-left X coordinate for bitmap placement
 * @param y Top-left Y coordinate for bitmap placement
 * @param bitmap Pointer to RGB565 bitmap data in PROGMEM
 * @param w Bitmap width in pixels
 * @param h Bitmap height in pixels
 * @param displayNum Target display number (1 or 2)
 * 
 * @note Uses pgm_read_word() for PROGMEM access
 * @note Pixel-by-pixel drawing may be slow for large bitmaps
 * @note Display is automatically selected and deselected
 * @note Bitmap must be in RGB565 format
 * 
 * @see drawColorBitmapRotated() for rotated bitmap rendering
 * @see selectDisplay() for display activation
 * @see deselectAll() for display deactivation
 * 
 * @since v0.9
 */
void DisplayManager::drawColorBitmap(int16_t x, int16_t y, const uint16_t *bitmap, 
                                    int16_t w, int16_t h, int displayNum) {
    selectDisplay(displayNum);
    
    // Draw RGB565 color bitmap pixel by pixel
    for (int16_t j = 0; j < h; j++) {
        for (int16_t i = 0; i < w; i++) {
            int16_t pixelIndex = j * w + i;
            uint16_t color = pgm_read_word(&bitmap[pixelIndex]);
            tft.drawPixel(x + i, y + j, color);
        }
    }
    
    deselectAll();
}

/**
 * @brief Draws RGB565 color bitmap with 270-degree rotation
 * 
 * Renders a color bitmap with automatic 270-degree clockwise rotation
 * to correct orientation issues. Each pixel is transformed during rendering
 * to achieve the rotation effect without requiring pre-rotated bitmap data.
 * 
 * Rotation transformation:
 * - Original pixel at (i,j) becomes pixel at (h-1-j, i)
 * - Effectively rotates 270 degrees clockwise
 * - Corrects upside-down bitmap display issues
 * 
 * @param x Top-left X coordinate for rotated bitmap placement
 * @param y Top-left Y coordinate for rotated bitmap placement
 * @param bitmap Pointer to RGB565 bitmap data in PROGMEM
 * @param w Original bitmap width in pixels (before rotation)
 * @param h Original bitmap height in pixels (before rotation)
 * @param displayNum Target display number (1 or 2)
 * 
 * @note Final bitmap dimensions will be h×w after rotation
 * @note Uses pgm_read_word() for PROGMEM access
 * @note Display is automatically selected and deselected
 * @note Slower than non-rotated version due to coordinate transformation
 * 
 * @see drawColorBitmap() for non-rotated bitmap rendering
 * @see selectDisplay() for display activation
 * @see deselectAll() for display deactivation
 * 
 * @since v0.9
 */
void DisplayManager::drawColorBitmapRotated(int16_t x, int16_t y, const uint16_t *bitmap, 
                                          int16_t w, int16_t h, int displayNum) {
    selectDisplayForImage(displayNum);  // Uses DISPLAY_IMAGE_ROTATION = 0
    
    // Respect DISPLAY_IMAGE_ROTATION setting instead of hardcoded rotation
    if (DISPLAY_IMAGE_ROTATION == 0) {
        // No rotation: draw bitmap as-is (portrait 80x160)
        for (int16_t j = 0; j < h; j++) {
            for (int16_t i = 0; i < w; i++) {
                int16_t pixelIndex = j * w + i;
                uint16_t color = pgm_read_word(&bitmap[pixelIndex]);
                tft.drawPixel(x + i, y + j, color);
            }
        }
    } else {
        // Rotate 270 degrees CW for other rotation settings
        for (int16_t j = 0; j < h; j++) {
            for (int16_t i = 0; i < w; i++) {
                int16_t pixelIndex = j * w + i;
                uint16_t color = pgm_read_word(&bitmap[pixelIndex]);
                
                // Calculate rotated position: 270 degrees CW
                int16_t rotatedX = x + (h - 1 - j);
                int16_t rotatedY = y + i;
                
                tft.drawPixel(rotatedX, rotatedY, color);
            }
        }
    }
    
    deselectAll();
}

/**
 * @brief Displays splash screen on specified display(s) with timeout
 * 
 * Shows a rotated color bitmap splash screen with automatic brightness
 * control and timeout management. Can display on individual displays
 * or both simultaneously. Uses image rotation for optimal presentation.
 * 
 * Features:
 * - Automatic brightness boost to maximum (255) for visibility
 * - Black background clearing before splash display
 * - 270-degree rotation for correct orientation
 * - Configurable timeout with automatic state management
 * 
 * @param displayNum Target display selection
 *                   0 = both displays (recursive call)
 *                   1 = first display only
 *                   2 = second display only
 * @param timeoutMs Splash screen timeout in milliseconds
 * 
 * @note Sets splashActive flag and manages timing automatically
 * @note Uses selectDisplayForImage() for optimal image rotation
 * @note Splash bitmap must be defined as epd_bitmap_ constant
 * @note Portal sequence may follow after splash completion
 * 
 * @see drawColorBitmapRotated() for bitmap rendering
 * @see updateSplashScreen() for timeout management
 * @see showPortalSequence() for splash-to-portal transition
 * 
 * @since v0.9
 */
void DisplayManager::showSplashScreen(int displayNum, unsigned long timeoutMs) {
    if (displayNum == 0) {
        // Show on both displays (always show splash on both, regardless of settings)
        showSplashScreen(1, timeoutMs);
        showSplashScreen(2, timeoutMs);  // Always show splash on Display 2
        return;
    }
    
    LOG_INFOF("DISPLAY", "DEBUG: showSplashScreen called for display %d", displayNum);
    LOG_INFOF("DISPLAY", "DEBUG: firstScreenCS=%d, secondScreenCS=%d", firstScreenCS, secondScreenCS);
    
    selectDisplayForImage(displayNum);  // Use image rotation (0) for splash screen
    
    // SELF-CONTAINED: Ensure display has proper brightness for splash screen
    setBrightness(255, displayNum);  // Full brightness for splash screen visibility
    
    // Clear screen with black background
    fillScreen(TFT_BLACK, displayNum);
    
    // Calculate center position for rotated bitmap (80x160 becomes 160x80 after rotation)
    // The rotated bitmap will be 160x80, which perfectly fits the 160x80 display
    int16_t centerX = 0;  // Start at left edge
    int16_t centerY = 0;  // Start at top edge
    
    // Draw the rotated color bitmap
    drawColorBitmapRotated(centerX, centerY, epd_bitmap_, SPLASH_WIDTH, SPLASH_HEIGHT, displayNum);
    
    // Set splash timing
    splashStartTime = millis();
    splashActive = true;
    splashTimeoutMs = timeoutMs;
    
    LOG_INFOF("DISPLAY", "Splash screen displayed on display %d in portrait mode (rotation 0) with full brightness (timeout: %lums)", displayNum, timeoutMs);
}

/**
 * @brief Updates splash screen state and handles timeout transitions
 * 
 * Monitors splash screen timeout and performs appropriate transitions:
 * - Portal mode: Shows portal info on Display 1, clears Display 2 (unified behavior)
 * - Normal mode: Clears Display 1 only
 * 
 * This ensures consistent behavior across ESP32 and ESP32S3 platforms.
 * 
 * @note Display 2 is always disabled during portal mode for both platforms
 * @see showPortalInfo() for portal display details
 * 
 * @since v0.9
 */
void DisplayManager::updateSplashScreen() {
    if (splashActive && (millis() - splashStartTime >= splashTimeoutMs)) {
        splashActive = false;
        
        // ALWAYS disable Display 2 after splash, regardless of mode
        // Display 2 stays dark until user explicitly enables it via web UI
        fillScreen(TFT_BLACK, 2);
        setBrightness(0, 2);
        LOG_INFO("DISPLAY", "Splash complete: Display 2 disabled (stays dark until user enables via web UI)");
        
        // Check if we need to show portal info after splash
        if (portalSequenceActive) {
            portalSequenceActive = false;
            // Show portal info on Display 1
            showPortalInfo(pendingSSID, pendingIP, pendingStatus);
            LOG_INFO("DISPLAY", "Portal transition: Display 1 = portal info, Display 2 = disabled");
        } else {
            // Normal transition: clear Display 1 only
            fillScreen(TFT_BLACK, 1);
            LOG_INFO("DISPLAY", "Normal transition: Display 1 cleared, Display 2 disabled");
        }
    }
}

/**
 * @brief Initiates splash-to-portal sequence with automatic transition
 * 
 * Starts a timed sequence showing splash screen followed by portal
 * configuration information. Stores portal data for delayed display
 * after splash timeout completes.
 * 
 * Sequence timeline:
 * 1. Immediate: Shows splash screen on both displays
 * 2. After DISPLAY_SPLASH_DURATION_MS: Automatically shows portal info
 * 
 * @param ssid Access point SSID for portal display
 * @param ip IP address for portal access
 * @param status Current portal status message
 * 
 * @note Portal data is stored in pending variables for delayed display
 * @note Sets portalSequenceActive flag for updateSplashScreen()
 * @note Uses DISPLAY_SPLASH_DURATION_MS constant for timing
 * @note Splash displays on both screens (displayNum = 0)
 * 
 * @see showSplashScreen() for splash display
 * @see updateSplashScreen() for automatic transition handling
 * @see showPortalInfo() for portal information display
 * 
 * @since v0.9
 */
void DisplayManager::showPortalSequence(const String& ssid, const String& ip, const String& status) {
    // Store portal info for later display
    pendingSSID = ssid;
    pendingIP = ip;
    pendingStatus = status;
    portalSequenceActive = true;
    
    // Show splash screen for 4 seconds, then portal info will auto-display
    showSplashScreen(0, DISPLAY_SPLASH_DURATION_MS);  // 4 seconds on both displays
    
    LOG_INFO("DISPLAY", "🚀 Portal sequence started: 4s splash → portal info");
}

/**
 * @brief Provides TFT library access for external components
 * 
 * Returns a pointer to the internal TFT_eSPI instance for use by other
 * components like ImageManager. The returned instance is shared between
 * displays and requires proper display selection before use.
 * 
 * @param displayNum Display number for validation (1 or 2)
 * 
 * @return Pointer to TFT_eSPI instance if displayNum valid, nullptr otherwise
 * 
 * @note Caller must use selectDisplay() before TFT operations
 * @note Same TFT instance is shared between both displays
 * @note Display selection controls which screen receives commands
 * 
 * @see selectDisplay() for proper display activation
 * @see selectDisplayForImage() for image-specific operations
 * 
 * @since v0.9
 */
TFT_eSPI* DisplayManager::getTFT(int displayNum) {
    // Note: This returns the shared TFT instance
    // The display selection is handled by selectDisplay()
    if (displayNum >= 1 && displayNum <= 2) {
        return &tft;
    }
    return nullptr;
}

/**
 * @brief Returns the hardware display type string
 * 
 * Provides compile-time detection of the display hardware type based on
 * preprocessor definitions. Used for hardware-specific configuration
 * and debugging information.
 * 
 * @return "ST7789" if DISPLAY_TYPE_ST7789 defined, "ST7735" otherwise
 * 
 * @note Determined at compile time via preprocessor definitions
 * @note Used for hardware-specific optimizations and logging
 * 
 * @see getDisplayWidth() for display width detection
 * @see getDisplayHeight() for display height detection
 * 
 * @since v0.9
 */
String DisplayManager::getDisplayType() const {
    #ifdef DISPLAY_TYPE_ST7789
        return "ST7789";
    #else
        return "ST7735";
    #endif
}

/**
 * @brief Returns the display width in pixels
 * 
 * Provides compile-time detection of display width based on hardware type.
 * Different display types have different resolutions and this method
 * returns the appropriate width for the configured hardware.
 * 
 * @return 240 pixels for ST7789, 160 pixels for ST7735
 * 
 * @note Determined at compile time via preprocessor definitions
 * @note Used for image scaling and layout calculations
 * 
 * @see getDisplayHeight() for display height detection
 * @see getDisplayType() for hardware type identification
 * 
 * @since v0.9
 */
uint16_t DisplayManager::getDisplayWidth() const {
    #ifdef DISPLAY_TYPE_ST7789
        return 240;
    #else
        return 160;
    #endif
}

/**
 * @brief Returns the display height in pixels
 * 
 * Provides compile-time detection of display height based on hardware type.
 * Different display types have different resolutions and this method
 * returns the appropriate height for the configured hardware.
 * 
 * @return 240 pixels for ST7789, 80 pixels for ST7735
 * 
 * @note Determined at compile time via preprocessor definitions
 * @note Used for image scaling and layout calculations
 * 
 * @see getDisplayWidth() for display width detection
 * @see getDisplayType() for hardware type identification
 * 
 * @since v0.9
 */
uint16_t DisplayManager::getDisplayHeight() const {
    #ifdef DISPLAY_TYPE_ST7789
        return 240;
    #else
        return 80;
    #endif
}

/**
 * @brief Sets rotation for all displays and TFT instance
 * 
 * Configures the display rotation for both displays and the main TFT instance.
 * This method ensures consistent rotation across all display operations and
 * is particularly important for image operations that bypass display selection.
 * 
 * Rotation values:
 * - 0: No rotation (default)
 * - 1: 90 degrees clockwise
 * - 2: 180 degrees
 * - 3: 270 degrees clockwise
 * 
 * @param rotation Rotation value (0-3)
 * 
 * @note Returns immediately if DisplayManager not initialized
 * @note Sets rotation on main TFT instance first
 * @note Applies rotation to both individual displays
 * @note Automatically deselects displays after rotation setting
 * 
 * @see selectDisplay() for display activation
 * @see deselectAll() for display deactivation
 * 
 * @since v0.9
 */
void DisplayManager::setRotation(uint8_t rotation) {
    if (!initialized) return;
    
    // Set rotation on the main TFT instance (used by TJpg decoder)
    tft.setRotation(rotation);
    
    // Also ensure rotation is set for both display selections
    selectDisplay(1);
    tft.setRotation(rotation);
    selectDisplay(2);
    tft.setRotation(rotation);
    deselectAll();
    
    Serial.printf("Set rotation to %d for all displays\n", rotation);
}

