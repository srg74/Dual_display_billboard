#ifndef DISPLAY_MANAGER_H
#define DISPLAY_MANAGER_H

#include <Arduino.h>
#include <TFT_eSPI.h>

class DisplayManager {
private:
    TFT_eSPI tft;
    bool initialized;
    uint8_t brightness1, brightness2;
    
    // CORRECT Pin definitions 
    static const int firstScreenCS = 5;      // CS1 = GPIO 5 ✅
    static const int secondScreenCS = 15;    // CS2 = GPIO 15 ✅
    static const int TFT_DC_PIN = 14;        // DC = GPIO 14 (shared) ✅
    static const int TFT_BACKLIGHT1_PIN = 22; // BLK1 = GPIO 22 ✅
    static const int TFT_BACKLIGHT2_PIN = 27; // BLK2 = GPIO 27 ✅
    
    void initializeBacklight();
    void initializeCS();
    void initializeTFT();

public:
    DisplayManager();
    bool begin();
    
    void selectDisplay(int displayNum);
    void deselectAll();
    void setBrightness(uint8_t brightness, int displayNum = 0);
    void fillScreen(uint16_t color, int displayNum = 0);
    void drawText(const char* text, int x, int y, uint16_t color, int displayNum);
    
    void enableSecondDisplay(bool enable);
    void alternateDisplays();
    
    // Portal display methods
    void showPortalInfo(const String& ssid, const String& ip, const String& status);
    void showQuickStatus(const String& message, uint16_t color);  // NEW: Fast status display
    void showAPStarting();  // NEW: Quick AP starting indicator
    void showAPReady();     // NEW: Quick AP ready indicator
    void showConnectionSuccess(const String& ip);
};

#endif // DISPLAY_MANAGER_H