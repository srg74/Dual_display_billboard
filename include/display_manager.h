/**
 * @file display_manager.h
 * @brief Hardware abst    // Hardware pin definitions - platform specific
    #if defined(ESP32S3_MODE) 
        // ESP32-S3 pinout: Optimized for development board layout
        static const int firstScreenCS = 10;     ///< Primary display CS pin (GPIO 10)
        static const int secondScreenCS = 39;    ///< Secondary display CS pin (GPIO 39)
        static const int TFT_DC_PIN = 14;        // DC = GPIO 14 (shared) ✅
        static const int TFT_BACKLIGHT1_PIN = 7; // BLK1 = GPIO 7
        static const int TFT_BACKLIGHT2_PIN = 8; // BLK2 = GPIO 8
    #elif defined(ESP32DEV_MODE)
        // ESP32 pinout (original)
        static const int firstScreenCS = 5;      // CS1 = GPIO 5 ✅
        static const int secondScreenCS = 15;    // CS2 = GPIO 15 ✅
        static const int TFT_DC_PIN = 14;        // DC = GPIO 14 (shared) ✅
        static const int TFT_BACKLIGHT1_PIN = 22; // BLK1 = GPIO 22 ✅
        static const int TFT_BACKLIGHT2_PIN = 27; // BLK2 = GPIO 27 ✅
    #endifr dual TFT display control
 * 
 * Provides unified interface for managing dual ST7735 displays with independent
 * chip select control, brightness management, and rotation handling.
 * 
 * Key features:
 * - Dual display support with independent control
 * - Hardware-specific pin mapping (ESP32 vs ESP32-S3)
 * - Brightness control via PWM
 * - Content-aware rotation settings
 * - Splash screen and status display management
 * - Portal sequence visualization
 * 
 * @author Dual Display Billboard Project
 * @version 2.0
 */

#ifndef DISPLAY_MANAGER_H
#define DISPLAY_MANAGER_H

#include <Arduino.h>
#include <TFT_eSPI.h>
#include "display_hardware_config.h"

/**
 * @brief Dual TFT display hardware controller
 * 
 * Manages hardware interfacing for dual ST7735 displays with:
 * - Independent chip select (CS) pin control
 * - PWM brightness control for each display
 * - Content-specific rotation management
 * - Synchronized display operations
 * - Hardware abstraction for different ESP32 variants
 */
class DisplayManager {
private:
    TFT_eSPI tft;                    ///< TFT_eSPI library instance
    bool initialized;                ///< Initialization status flag
    uint8_t brightness1, brightness2;///< Brightness levels (0-255)
    
    // Splash screen management
    unsigned long splashStartTime;   ///< Splash screen start timestamp
    bool splashActive;               ///< Splash screen active status
    unsigned long splashTimeoutMs;   ///< Splash timeout duration
    
    // Portal sequence visualization
    bool portalSequenceActive;       ///< Portal connection sequence status
    String pendingSSID, pendingIP, pendingStatus; ///< Connection details for display
    
    // Hardware pin references (defined in display_hardware_config.h)
    static const int firstScreenCS = DISPLAY_CS1_PIN;      ///< Primary display CS pin
    static const int secondScreenCS = DISPLAY_CS2_PIN;     ///< Secondary display CS pin
    static const int TFT_DC_PIN = DISPLAY_DC_PIN;          ///< DC pin (shared)
    static const int TFT_BACKLIGHT1_PIN = DISPLAY_BLK1_PIN; ///< Backlight 1 pin
    static const int TFT_BACKLIGHT2_PIN = DISPLAY_BLK2_PIN; ///< Backlight 2 pin
    
    void initializeBacklight();
    void initializeCS();
    void initializeTFT();

public:
    DisplayManager();
    bool begin();
    
    void selectDisplay(int displayNum);
    void selectDisplayForText(int displayNum);   // Select display with text rotation
    void selectDisplayForImage(int displayNum);  // Select display with image rotation
    void deselectAll();
    void clearBothDisplaysToBlack();             // Clear both displays to black on startup
    void setBrightness(uint8_t brightness, int displayNum = 0);
    void fillScreen(uint16_t color, int displayNum = 0);
    void drawText(const char* text, int x, int y, uint16_t color, int displayNum);
    
    // Color bitmap drawing functions
    void drawColorBitmap(int16_t x, int16_t y, const uint16_t *bitmap, 
                        int16_t w, int16_t h, int displayNum = 0);
    void drawColorBitmapRotated(int16_t x, int16_t y, const uint16_t *bitmap, 
                               int16_t w, int16_t h, int displayNum = 0);
    void showSplashScreen(int displayNum = 0, unsigned long timeoutMs = 2000);
    void updateSplashScreen();  // Call this in main loop
    bool isSplashActive();
    
    void enableSecondDisplay(bool enable);
    void alternateDisplays();
    
    // Portal display methods
    void showPortalInfo(const String& ssid, const String& ip, const String& status);
    void showPortalSequence(const String& ssid, const String& ip, const String& status);  // NEW: Splash + Portal
    void showQuickStatus(const String& message, uint16_t color);  // NEW: Fast status display
    void showAPStarting();  // NEW: Quick AP starting indicator
    void showAPReady();     // NEW: Quick AP ready indicator
    void showConnecting();  // NEW: Show connecting status during WiFi connection
    void showConnectionSuccess(const String& ip);
    
    // TFT access for ImageManager
    TFT_eSPI* getTFT(int displayNum = 1);
    void setRotation(uint8_t rotation);  // Set rotation for all displays
    
    // Display type detection
    String getDisplayType() const;
    uint16_t getDisplayWidth() const;
    uint16_t getDisplayHeight() const;
};

#endif // DISPLAY_MANAGER_H