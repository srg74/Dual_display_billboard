#include "display_manager.h"
#include "secrets.h"
#include "logger.h"

DisplayManager::DisplayManager() : initialized(false), brightness1(255), brightness2(255) {
}

bool DisplayManager::begin() {
    LOG_INFO("DISPLAY", "🎨 Initializing Display Manager with working config");
    
    initializeBacklight();
    initializeCS();
    initializeTFT();
    
    // Test both displays
    displayTest();
    
    initialized = true;
    LOG_INFO("DISPLAY", "✅ Display Manager initialized successfully");
    return true;
}

void DisplayManager::initializeBacklight() {
    LOG_INFO("DISPLAY", "🔧 Setting up backlight...");
    
    // Working project backlight setup
    ledcAttachPin(TFT_BACKLIGHT_PIN, 1); // channel 1
    ledcSetup(1, 5000, 8); // channel 1, 5 KHz, 8-bit
    ledcWrite(1, 255); // Full brightness
    
    LOG_INFO("DISPLAY", "✅ Backlight initialized");
}

void DisplayManager::initializeCS() {
    LOG_INFO("DISPLAY", "🔧 Setting up CS pins...");
    
    // CS pins setup (exact copy from working project)
    pinMode(firstScreenCS, OUTPUT);
    digitalWrite(firstScreenCS, HIGH);
    pinMode(secondScreenCS, OUTPUT);
    digitalWrite(secondScreenCS, HIGH);
    
    LOG_INFO("DISPLAY", "✅ CS pins configured");
}

void DisplayManager::initializeTFT() {
    LOG_INFO("DISPLAY", "🔧 Initializing TFT...");
    
    // TFT initialization (EXACT copy from working project)
    digitalWrite(firstScreenCS, LOW);   // Both CS LOW
    digitalWrite(secondScreenCS, LOW);
    tft.init();                         // Init with both selected
    digitalWrite(firstScreenCS, HIGH);  // Both CS HIGH
    digitalWrite(secondScreenCS, HIGH);
    
    LOG_INFO("DISPLAY", "✅ TFT initialized with dual CS method");
}

void DisplayManager::selectDisplay(int displayNum) {
    deselectAll();
    
    if (displayNum == 1) {
        digitalWrite(firstScreenCS, LOW);
        tft.setRotation(3);  // Working project uses rotation 3
    } else if (displayNum == 2) {
        digitalWrite(secondScreenCS, LOW);
        tft.setRotation(3);  // Working project uses rotation 3
    }
}

void DisplayManager::deselectAll() {
    digitalWrite(firstScreenCS, HIGH);
    digitalWrite(secondScreenCS, HIGH);
}

void DisplayManager::setBrightness(uint8_t brightness, int displayNum) {
    if (displayNum == 1 || displayNum == 0) {
        brightness1 = brightness;
    }
    if (displayNum == 2 || displayNum == 0) {
        brightness2 = brightness;
    }
    
    // Apply brightness (using working project's channel 1)
    uint8_t avgBrightness = (brightness1 + brightness2) / 2;
    ledcWrite(1, avgBrightness);
    
    LOG_INFOF("DISPLAY", "🔆 Brightness set - Display %d: %d", displayNum, brightness);
}

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

void DisplayManager::drawText(const char* text, int x, int y, uint16_t color, int displayNum) {
    selectDisplay(displayNum);
    tft.setTextColor(color);
    tft.setTextSize(1);
    tft.setCursor(x, y);
    tft.print(text);
    deselectAll();
}

void DisplayManager::displayTest() {
    LOG_INFO("DISPLAY", "🧪 Running display test...");
    
    // Test first screen (exact copy)
    selectDisplay(1);
    tft.fillScreen(TFT_RED);
    deselectAll();
    LOG_INFO("DISPLAY", "✅ First screen RED");
    
    // Test second screen (exact copy)
    selectDisplay(2);
    tft.fillScreen(TFT_GREEN);
    deselectAll();
    LOG_INFO("DISPLAY", "✅ Second screen GREEN");
}

void DisplayManager::showSystemInfo() {
    // Display 1: System info
    selectDisplay(1);
    tft.fillScreen(TFT_BLACK);
    tft.setTextColor(TFT_WHITE);
    tft.setTextSize(1);
    tft.setCursor(5, 5);
    tft.print("BILLBOARD");
    tft.setCursor(5, 20);
    tft.printf("Heap: %d", ESP.getFreeHeap());
    tft.setCursor(5, 35);
    tft.printf("Display: 1");
    deselectAll();
    
    // Display 2: System info
    selectDisplay(2);
    tft.fillScreen(TFT_BLUE);
    tft.setTextColor(TFT_WHITE);
    tft.setTextSize(1);
    tft.setCursor(5, 5);
    tft.print("BILLBOARD");
    tft.setCursor(5, 20);
    tft.printf("Heap: %d", ESP.getFreeHeap());
    tft.setCursor(5, 35);
    tft.printf("Display: 2");
    deselectAll();
    
    LOG_INFO("DISPLAY", "✅ System info displayed on both screens");
}

void DisplayManager::enableSecondDisplay(bool enable) {
    if (enable) {
        LOG_INFO("DISPLAY", "✅ Second display enabled");
    } else {
        selectDisplay(2);
        tft.fillScreen(TFT_BLACK);
        deselectAll();
        LOG_INFO("DISPLAY", "⚫ Second display disabled");
    }
}

void DisplayManager::alternateDisplays() {
    static bool useFirst = true;
    static unsigned long lastSwitch = 0;
    
    if (millis() - lastSwitch > 3000) {
        if (useFirst) {
            selectDisplay(1);
            tft.fillScreen(TFT_BLUE);
            deselectAll();
            LOG_INFO("DISPLAY", "🔵 First screen BLUE");
        } else {
            selectDisplay(2);
            tft.fillScreen(TFT_YELLOW);
            deselectAll();
            LOG_INFO("DISPLAY", "🟡 Second screen YELLOW");
        }
        
        useFirst = !useFirst;
        lastSwitch = millis();
    }
}