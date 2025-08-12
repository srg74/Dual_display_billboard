#ifndef DISPLAY_MANAGER_H
#define DISPLAY_MANAGER_H

#include <Arduino.h>
#include <TFT_eSPI.h>

class DisplayManager {
private:
    TFT_eSPI tft;
    bool initialized;
    uint8_t brightness1, brightness2;
    
    // Rotation settings for different content types
    static const uint8_t TEXT_ROTATION = 3;   // Rotation for text displays
    static const uint8_t IMAGE_ROTATION = 0;  // Rotation for images (confirmed working)
    
    // Splash screen timing
    unsigned long splashStartTime;
    bool splashActive;
    unsigned long splashTimeoutMs;
    
    // Portal sequence state
    bool portalSequenceActive;
    String pendingSSID, pendingIP, pendingStatus;
    
    // Pin definitions - ESP32 vs ESP32-S3
    #if defined(ESP32S3_MODE) 
        // ESP32-S3 pinout: CS1=10, CS2=39, BL1=7, BL2=8, DC=14
        static const int firstScreenCS = 10;     // CS1 = GPIO 10 ✅  
        static const int secondScreenCS = 39;    // CS2 = GPIO 39 ✅
        static const int TFT_DC_PIN = 14;        // DC = GPIO 14 (shared) ✅
        static const int TFT_BACKLIGHT1_PIN = 7; // BLK1 = GPIO 7 ✅
        static const int TFT_BACKLIGHT2_PIN = 8; // BLK2 = GPIO 8 ✅
    #elif defined(ESP32DEV_MODE)
        // ESP32 pinout (original)
        static const int firstScreenCS = 5;      // CS1 = GPIO 5 ✅
        static const int secondScreenCS = 15;    // CS2 = GPIO 15 ✅
        static const int TFT_DC_PIN = 14;        // DC = GPIO 14 (shared) ✅
        static const int TFT_BACKLIGHT1_PIN = 22; // BLK1 = GPIO 22 ✅
        static const int TFT_BACKLIGHT2_PIN = 27; // BLK2 = GPIO 27 ✅
    #endif
    
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
    void setBrightness(uint8_t brightness, int displayNum = 0);
    void fillScreen(uint16_t color, int displayNum = 0);
    void drawText(const char* text, int x, int y, uint16_t color, int displayNum);
    
    // Bitmap drawing functions
    void drawMonochromeBitmap(int16_t x, int16_t y, const uint8_t *bitmap, 
                             int16_t w, int16_t h, uint16_t color, uint16_t bg, int displayNum = 0);
    void drawMonochromeBitmapRotated(int16_t x, int16_t y, const uint8_t *bitmap, 
                                    int16_t w, int16_t h, uint16_t color, uint16_t bg, int displayNum = 0);
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