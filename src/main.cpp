#include <Arduino.h>

#ifdef TFT_TEST_ONLY
// Exact initialization from your working project
#include <TFT_eSPI.h>

#define TFT_BACKLIGHT_PIN 22
#define firstScreenCS 5
#define secondScreenCS 15

TFT_eSPI tft = TFT_eSPI();

void setup() {
    Serial.begin(115200);
    Serial.println("🎨 WORKING PROJECT INITIALIZATION");
    
    // Backlight setup (exact copy)
    ledcAttachPin(TFT_BACKLIGHT_PIN, 1); // channel 1
    ledcSetup(1, 5000, 8); // channel 1, 5 KHz, 8-bit
    ledcWrite(1, 255); // Full brightness
    Serial.println("✅ Backlight ON");

    // CS pins setup (exact copy from working project)
    pinMode(firstScreenCS, OUTPUT);
    digitalWrite(firstScreenCS, HIGH);
    pinMode(secondScreenCS, OUTPUT);
    digitalWrite(secondScreenCS, HIGH);
    Serial.println("✅ CS pins configured");

    // TFT initialization (EXACT copy from working project)
    digitalWrite(firstScreenCS, LOW);   // Both CS LOW
    digitalWrite(secondScreenCS, LOW);
    tft.init();                         // Init with both selected
    digitalWrite(firstScreenCS, HIGH);  // Both CS HIGH
    digitalWrite(secondScreenCS, HIGH);
    Serial.println("✅ TFT initialized with dual CS method");

    // Test first screen (exact copy)
    digitalWrite(firstScreenCS, LOW);
    tft.setRotation(3);  // Working project uses rotation 3!
    tft.fillScreen(TFT_RED);
    digitalWrite(firstScreenCS, HIGH);
    Serial.println("✅ First screen RED");

    // Test second screen (exact copy)
    digitalWrite(secondScreenCS, LOW);
    tft.setRotation(3);  // Working project uses rotation 3!
    tft.fillScreen(TFT_GREEN);
    digitalWrite(secondScreenCS, HIGH);
    Serial.println("✅ Second screen GREEN");
    
    Serial.println("🎉 SHOULD BE WORKING NOW!");
}

void loop() {
    static unsigned long lastSwitch = 0;
    static bool useFirst = true;
    
    if (millis() - lastSwitch > 3000) {
        if (useFirst) {
            digitalWrite(firstScreenCS, LOW);
            tft.setRotation(3);
            tft.fillScreen(TFT_BLUE);
            digitalWrite(firstScreenCS, HIGH);
            Serial.println("🔵 First screen BLUE");
        } else {
            digitalWrite(secondScreenCS, LOW);
            tft.setRotation(3);
            tft.fillScreen(TFT_YELLOW);
            digitalWrite(secondScreenCS, HIGH);
            Serial.println("🟡 Second screen YELLOW");
        }
        
        useFirst = !useFirst;
        lastSwitch = millis();
    }
    
    yield();
}

#elif defined(SAFE_MODE_ONLY)
// Ultra-safe mode - minimal functionality
unsigned long lastHeartbeat = 0;
unsigned long startupTime = 0;

const unsigned long HEARTBEAT_INTERVAL = 3000;
const unsigned long STARTUP_DELAY = 1000;

bool systemInitialized = false;
bool ledState = false;

void setup() {
    Serial.begin(115200);
    startupTime = millis();
    
    Serial.println("🚨 ULTRA-SAFE MODE - NO LIBRARIES");
    Serial.printf("Free Heap: %d bytes\n", ESP.getFreeHeap());
    Serial.printf("Chip Model: %s\n", ESP.getChipModel());
    Serial.printf("PSRAM Size: %d bytes (should be 0!)\n", ESP.getPsramSize());
    
    // Test basic GPIO only
    pinMode(22, OUTPUT); // BL1
    pinMode(27, OUTPUT); // BL2
    pinMode(5, OUTPUT);  // CS1
    pinMode(15, OUTPUT); // CS2
    
    digitalWrite(22, LOW);
    digitalWrite(27, LOW);
    digitalWrite(5, HIGH);
    digitalWrite(15, HIGH);
    
    Serial.println("✅ GPIO initialized safely");
}

void loop() {
    unsigned long currentTime = millis();
    
    if (!systemInitialized && (currentTime - startupTime >= STARTUP_DELAY)) {
        Serial.println("🔧 Ultra-safe mode initialization...");
        
        // Test PWM without any libraries
        ledcSetup(0, 5000, 8);
        ledcAttachPin(22, 0);
        ledcWrite(0, 128);
        
        ledcSetup(1, 5000, 8);
        ledcAttachPin(27, 1);
        ledcWrite(1, 128);
        
        Serial.println("✅ PWM backlight test successful");
        systemInitialized = true;
        lastHeartbeat = currentTime;
    }
    
    if (systemInitialized && (currentTime - lastHeartbeat >= HEARTBEAT_INTERVAL)) {
        ledState = !ledState;
        
        // Safe backlight test
        ledcWrite(0, ledState ? 255 : 64);
        ledcWrite(1, ledState ? 64 : 255);
        
        Serial.printf("💓 Ultra-safe heartbeat - Heap: %d, PSRAM: %d\n", 
                      ESP.getFreeHeap(), ESP.getPsramSize());
        lastHeartbeat = currentTime;
    }
    
    yield();
}

#else
// Full Billboard System
#include "logger.h"
#include "display_manager.h"
#include "config.h"     // Use timing constants from config.h

DisplayManager displayManager;

// Use constants from config.h instead of hardcoded values
unsigned long lastHeartbeat = 0;
unsigned long lastDisplayTest = 0;
unsigned long startupTime = 0;

bool systemInitialized = false;

void setup() {
    Logger::init();
    
    startupTime = millis();
    
    LOG_INFO("MAIN", "🚀 DUAL DISPLAY BILLBOARD SYSTEM");
    LOG_SYSTEM_INFO();
    
    LOG_INFO("MAIN", "🎯 System startup initiated");
}

void loop() {
    unsigned long currentTime = millis();
    
    // Use STARTUP_DELAY from config.h
    if (!systemInitialized && (currentTime - startupTime >= STARTUP_DELAY)) {
        LOG_INFO("MAIN", "🔧 Initializing billboard system...");
        
        LOG_MEMORY_INFO();
        
        if (displayManager.begin()) {
            LOG_INFO("MAIN", "✅ Display manager initialized");
            displayManager.showSystemInfo();
            displayManager.enableSecondDisplay(true);
        } else {
            LOG_ERROR("MAIN", "❌ Display manager failed");
        }
        
        systemInitialized = true;
        lastHeartbeat = currentTime;
        lastDisplayTest = currentTime;
        
        LOG_INFO("MAIN", "🎉 Billboard system ready!");
    }
    
    if (systemInitialized) {
        // Use HEARTBEAT_INTERVAL from config.h
        if (currentTime - lastHeartbeat >= HEARTBEAT_INTERVAL) {
            LOG_INFOF("MAIN", "💓 System heartbeat - Heap: %d bytes", ESP.getFreeHeap());
            lastHeartbeat = currentTime;
        }
        
        // Use DISPLAY_TEST_INTERVAL from config.h
        if (currentTime - lastDisplayTest >= DISPLAY_TEST_INTERVAL) {
            displayManager.alternateDisplays();
            lastDisplayTest = currentTime;
        }
    }
    
    yield();
}
#endif