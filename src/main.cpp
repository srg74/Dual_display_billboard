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
// Full Billboard System with WiFi + Display Integration
#include "logger.h"
#include "display_manager.h"
#include "wifi_manager.h"           // ADD: WiFi management
#include "credential_manager.h"     // ADD: Credential storage
#include "time_manager.h"           // ADD: Time management
#include "settings_manager.h"       // ADD: Settings management
#include "image_manager.h"          // ADD: Image management
#include "slideshow_manager.h"      // ADD: Slideshow management
#include "display_clock_manager.h"  // ADD: Clock management
#include "dcc_manager.h"            // ADD: DCC management
#include "config.h"

// Create instances
DisplayManager displayManager;
AsyncWebServer server(80);          // ADD: Web server

// Configure TCP settings to reduce timeout errors
void configureTCPSettings() {
    // More conservative TCP settings for better stability
    WiFi.setTxPower(WIFI_POWER_15dBm);  // Further reduce power
    
    // Use yield instead of blocking delay
    yield();
}

TimeManager timeManager;            // ADD: Time manager
SettingsManager settingsManager;    // ADD: Settings manager
ImageManager imageManager(&displayManager);  // ADD: Image manager
DisplayClockManager clockManager(&displayManager, &timeManager);  // ADD: Clock manager
SlideshowManager slideshowManager(&imageManager, &settingsManager, &clockManager);  // ADD: Slideshow manager
DCCManager dccManager(&settingsManager, &slideshowManager);  // ADD: DCC manager
WiFiManager wifiManager(&server, &timeManager, &settingsManager, &displayManager, &imageManager, &slideshowManager, &dccManager);   // ADD: WiFi manager with all components
CredentialManager credentialManager; // ADD: Credential manager

// Timing variables using config.h constants
unsigned long lastHeartbeat = 0;
unsigned long startupTime = 0;

// State management
bool systemInitialized = false;
bool wifiInitialized = false;
bool displayInitialized = false;
bool timeInitialized = false;

void setup() {
    // Initialize logging first
    Logger::init();
    
    startupTime = millis();
    
    LOG_INFO("MAIN", "🚀 DUAL DISPLAY BILLBOARD SYSTEM v2.0");
    LOG_SYSTEM_INFO();
    
    LOG_INFO("MAIN", "🎯 System startup initiated");
    
    // NOTE: WiFi manager will set up appropriate routes based on mode
    // Do not set up routes here to avoid conflicts
}

void loop() {
    unsigned long currentTime = millis();
    
    // Non-blocking startup sequence
    if (!systemInitialized && (currentTime - startupTime >= STARTUP_DELAY)) {
        LOG_INFO("MAIN", "🔧 Initializing integrated billboard system...");
        
        LOG_MEMORY_INFO();
        
        // Step 1: Initialize display system
        if (!displayInitialized) {
            LOG_INFO("MAIN", "📺 Initializing display subsystem...");
            if (displayManager.begin()) {
                LOG_INFO("MAIN", "✅ Display manager initialized");
                // displayManager.showSystemInfo(); // Commented out to skip startup info
                displayInitialized = true;
            } else {
                LOG_ERROR("MAIN", "❌ Display manager failed");
                return; // Don't continue if displays fail
            }
        }
        
        // Step 2: Initialize WiFi system
        if (!wifiInitialized && displayInitialized) {
            LOG_INFO("MAIN", "🌐 Initializing WiFi subsystem...");
            
            // Initialize credential manager
            if (credentialManager.begin()) {
                LOG_INFO("MAIN", "✅ Credential manager initialized");
            } else {
                LOG_ERROR("MAIN", "❌ Credential manager failed");
            }
            
            // Initialize settings manager
            if (settingsManager.begin()) {
                LOG_INFO("MAIN", "✅ Settings manager initialized");
            } else {
                LOG_ERROR("MAIN", "❌ Settings manager failed");
            }
            
            // Initialize image manager
            if (imageManager.begin()) {
                LOG_INFO("MAIN", "✅ Image manager initialized");
                
                // Initialize clock manager
                if (clockManager.begin()) {
                    LOG_INFO("MAIN", "✅ Clock manager initialized");
                } else {
                    LOG_ERROR("MAIN", "❌ Clock manager failed");
                }
                
                // Initialize slideshow manager
                if (slideshowManager.begin()) {
                    LOG_INFO("MAIN", "✅ Slideshow manager initialized");
                } else {
                    LOG_ERROR("MAIN", "❌ Slideshow manager failed");
                }
                
                // Initialize DCC manager
                if (dccManager.begin()) {
                    LOG_INFO("MAIN", "✅ DCC manager initialized");
                } else {
                    LOG_ERROR("MAIN", "❌ DCC manager failed");
                }
            } else {
                LOG_ERROR("MAIN", "❌ Image manager failed");
            }
            
            // Initialize WiFi manager from credentials
            if (wifiManager.initializeFromCredentials()) {
                LOG_INFO("MAIN", "✅ WiFi manager initialized");
                wifiInitialized = true;
                // Configure TCP settings to reduce timeout errors
                configureTCPSettings();
                LOG_INFO("MAIN", "🔧 TCP settings configured");
            } else {
                LOG_INFO("MAIN", "ℹ️  WiFi starting in setup mode");
                wifiInitialized = true; // Still continue in AP mode
                // Configure TCP settings to reduce timeout errors
                configureTCPSettings();
                LOG_INFO("MAIN", "🔧 TCP settings configured");
            }
        }
        
        // Step 3: Initialize time system (only in normal mode)
        if (!timeInitialized && wifiInitialized && 
            wifiManager.getCurrentMode() == WiFiManager::MODE_NORMAL) {
            LOG_INFO("MAIN", "🕐 Initializing time subsystem...");
            
            if (timeManager.begin()) {
                LOG_INFO("MAIN", "✅ Time manager initialized");
                timeInitialized = true;
            } else {
                LOG_WARN("MAIN", "⚠️ Time manager initialization failed");
                timeInitialized = true; // Continue without time sync
            }
        }
        
        // Step 4: System ready
        if (displayInitialized && wifiInitialized) {
            // Enable second display based on saved setting
            displayManager.enableSecondDisplay(settingsManager.isSecondDisplayEnabled());
            
            // Set initial brightness based on saved setting
            uint8_t savedBrightness = settingsManager.getBrightness();
            bool secondDisplayEnabled = settingsManager.isSecondDisplayEnabled();
            if (secondDisplayEnabled) {
                // Both displays enabled
                displayManager.setBrightness(savedBrightness, 0); // 0 = both displays
                LOG_INFOF("MAIN", "🔆 Initial brightness set to %d for both displays", savedBrightness);
            } else {
                // Only first display enabled
                displayManager.setBrightness(savedBrightness, 1); // 1 = first display only
                displayManager.setBrightness(0, 2); // Turn off second display backlight
                LOG_INFOF("MAIN", "🔆 Initial brightness set to %d for first display only", savedBrightness);
            }
            
            systemInitialized = true;
            lastHeartbeat = currentTime;
            
            LOG_INFO("MAIN", "🎉 Integrated billboard system ready!");
            LOG_INFOF("MAIN", "📱 WiFi Mode: %s", 
                     wifiManager.getCurrentMode() == WiFiManager::MODE_NORMAL ? "Normal" : "Setup");
        }
    }
    
    // Main operation loop - only run if fully initialized
    if (systemInitialized) {
        // Handle splash screen transitions (non-blocking)
        displayManager.updateSplashScreen();
        
        // WiFi management (non-blocking)
        wifiManager.checkConnectionStatus();
        wifiManager.checkGpio0FactoryReset();
        wifiManager.checkScheduledRestart();
        wifiManager.checkPortalModeSwitch();
        wifiManager.checkConnectionSuccessDisplay();  // NEW: Add this line
        
        // DCC signal processing (non-blocking)
        dccManager.loop();
        
        // Image slideshow management
        if (settingsManager.isImageEnabled() && 
            wifiManager.getCurrentMode() == WiFiManager::MODE_NORMAL && 
            !wifiManager.isShowingConnectionSuccess()) {
            
            // Start slideshow if not active and images are enabled
            // But only retry if enough time has passed since last "no images" check
            if (!slideshowManager.isSlideshowActive() && slideshowManager.shouldRetrySlideshow()) {
                slideshowManager.startSlideshow();
            }
            
            // Update slideshow
            slideshowManager.updateSlideshow();
        } else {
            // Stop slideshow if conditions are not met
            if (slideshowManager.isSlideshowActive()) {
                slideshowManager.stopSlideshow();
            }
            
            // FIXED: Only alternate displays when not in setup mode AND not showing connection success AND images disabled
            if (wifiManager.getCurrentMode() == WiFiManager::MODE_NORMAL && 
                !wifiManager.isShowingConnectionSuccess()) {
                displayManager.alternateDisplays();
            }
        }
        // In SETUP mode OR showing connection success, keep current display
        
        // Essential yield for ESP32 responsiveness
        yield();
        
        // Cooperative multitasking - yield more frequently for web server
        static unsigned long lastMainLoopYield = 0;
        if (millis() - lastMainLoopYield >= 10) {
            lastMainLoopYield = millis();
            vTaskDelay(1 / portTICK_PERIOD_MS); // 1ms FreeRTOS delay
        }
    }
    
    // Always yield to prevent watchdog issues
    yield();
}
#endif