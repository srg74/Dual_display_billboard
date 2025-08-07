#ifndef DISPLAY_MANAGER_H
#define DISPLAY_MANAGER_H

#include <Arduino.h>
#include <TFT_eSPI.h>

class DisplayManager {
private:
    TFT_eSPI tft;
    bool initialized;
    
    // Pin definitions from working config
    static const int TFT_BACKLIGHT_PIN = 22;
    static const int firstScreenCS = 5;
    static const int secondScreenCS = 15;
    
    // Current brightness levels
    uint8_t brightness1;
    uint8_t brightness2;
    
    void initializeBacklight();
    void initializeCS();
    void initializeTFT();

public:
    DisplayManager();
    bool begin();
    
    // Display control
    void selectDisplay(int displayNum);
    void deselectAll();
    void setBrightness(uint8_t brightness, int displayNum = 0);
    
    // Drawing functions
    void fillScreen(uint16_t color, int displayNum = 0);
    void drawText(const char* text, int x, int y, uint16_t color = TFT_WHITE, int displayNum = 0);
    void displayTest();
    void showSystemInfo();
    
    // Dual display functions
    void enableSecondDisplay(bool enable);
    void alternateDisplays();
    
    bool isInitialized() const { return initialized; }
};

#endif // DISPLAY_MANAGER_H