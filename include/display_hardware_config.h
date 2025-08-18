/**
 * @file display_hardware_config.h
 * @brief Centralized hardware pin configuration for dual TFT displays
 * 
 * This file consolidates all hardware-specific pin definitions for the
 * dual display billboard system. Pin assignments are FIXED by PCB design
 * and cannot be changed without hardware modification.
 * 
 * @author Dual Display Billboard Project
 * @version 1.0
 */

#ifndef DISPLAY_HARDWARE_CONFIG_H
#define DISPLAY_HARDWARE_CONFIG_H

/**
 * @brief Hardware pin definitions - platform specific
 * 
 * These pin assignments are FIXED by the PCB design and cannot be modified
 * without hardware changes. The configuration is selected based on the 
 * ESP32 variant being used.
 */
#if defined(ESP32S3_MODE) 
    // ESP32-S3 pinout: Optimized for development board layout
    // NOTE: GPIO 39 is I/O capable on ESP32-S3 (unlike ESP32 classic where it's input-only)
    #define DISPLAY_CS1_PIN      10     ///< Primary display CS pin (GPIO 10)
    #define DISPLAY_CS2_PIN      39     ///< Secondary display CS pin (GPIO 39) - I/O capable on ESP32S3
    #define DISPLAY_DC_PIN       14     ///< DC pin (GPIO 14) - shared between displays
    #define DISPLAY_BLK1_PIN     7      ///< Backlight 1 pin (GPIO 7)
    #define DISPLAY_BLK2_PIN     8      ///< Backlight 2 pin (GPIO 8)
    
#elif defined(ESP32DEV_MODE)
    // ESP32 pinout (original development board)
    #define DISPLAY_CS1_PIN      5      ///< Primary display CS pin (GPIO 5)
    #define DISPLAY_CS2_PIN      15     ///< Secondary display CS pin (GPIO 15)
    #define DISPLAY_DC_PIN       14     ///< DC pin (GPIO 14) - shared between displays
    #define DISPLAY_BLK1_PIN     22     ///< Backlight 1 pin (GPIO 22)
    #define DISPLAY_BLK2_PIN     27     ///< Backlight 2 pin (GPIO 27)
    
#else
    #error "No valid platform defined. Please define either ESP32S3_MODE or ESP32DEV_MODE"
#endif

/**
 * @brief Display configuration constants
 */
#define DISPLAY_COUNT            2      ///< Number of displays in the system
#define DISPLAY_PRIMARY          1      ///< Primary display number
#define DISPLAY_SECONDARY        2      ///< Secondary display number
#define DISPLAY_BOTH             0      ///< Both displays simultaneously

/**
 * @brief PWM configuration for backlight control
 */
#define BACKLIGHT_PWM_FREQ       5000   ///< PWM frequency in Hz
#define BACKLIGHT_PWM_RESOLUTION 8      ///< PWM resolution in bits (0-255)
#define BACKLIGHT_PWM_CHANNEL1   1      ///< PWM channel for display 1
#define BACKLIGHT_PWM_CHANNEL2   2      ///< PWM channel for display 2

/**
 * @brief Display rotation constants
 */
#define DISPLAY_TEXT_ROTATION    3      ///< Rotation for text displays (landscape)
#define DISPLAY_IMAGE_ROTATION   0      ///< Rotation for images (portrait)

#endif // DISPLAY_HARDWARE_CONFIG_H
