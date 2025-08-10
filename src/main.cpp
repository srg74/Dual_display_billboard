#include <Arduino.h>

#ifdef ROTATION_TEST_MODE
// Simple rotation test with minimal WiFi
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <TFT_eSPI.h>
#include <LittleFS.h>
#include <TJpg_Decoder.h>
#include "display_manager.h"
#include "credential_manager.h"

AsyncWebServer server(80);
DisplayManager displayManager;
TFT_eSPI* currentDisplayTft = nullptr;  // Global pointer for TJpg callback

// TJpg decoder callback function
bool tftOutput(int16_t x, int16_t y, uint16_t w, uint16_t h, uint16_t* bitmap) {
    if (currentDisplayTft && x < currentDisplayTft->width() && y < currentDisplayTft->height()) {
        currentDisplayTft->pushImage(x, y, w, h, bitmap);
    }
    return true;
}

#define TFT_BACKLIGHT_PIN 22

void setup() {
    Serial.begin(115200);
    Serial.println("🔄 ROTATION TEST MODE");
    
    // Initialize display
    if (!displayManager.begin()) {
        Serial.println("❌ DisplayManager failed");
        return;
    }
    Serial.println("✅ DisplayManager ready");
    
    // Initialize LittleFS for images
    if (!LittleFS.begin()) {
        Serial.println("❌ LittleFS failed");
        return;
    }
    Serial.println("✅ LittleFS ready");
    
    // Try to connect using saved credentials
    if (CredentialManager::hasCredentials()) {
        CredentialManager::WiFiCredentials creds = CredentialManager::loadCredentials();
        if (creds.isValid) {
            Serial.println("🔗 Using saved WiFi credentials");
            WiFi.begin(creds.ssid.c_str(), creds.password.c_str());
        }
    } else {
        Serial.println("⚠️ No saved credentials, starting AP");
        WiFi.softAP("RotationTest", "test123456");
        Serial.println("AP IP: " + WiFi.softAPIP().toString());
    }
    
    // Wait for connection
    Serial.print("Connecting to WiFi");
    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 30) {
        delay(500);
        Serial.print(".");
        attempts++;
    }
    
    if (WiFi.status() == WL_CONNECTED) {
        Serial.println();
        Serial.println("✅ WiFi connected!");
        Serial.println("IP: " + WiFi.localIP().toString());
    } else {
        Serial.println();
        Serial.println("❌ WiFi failed - starting AP mode");
        WiFi.softAP("RotationTest", "test123456");
        Serial.println("AP IP: " + WiFi.softAPIP().toString());
    }
    
    // Setup web routes
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
        String html = "<html><body>";
        html += "<h1>Rotation Tester</h1>";
        html += "<p>Device IP: " + WiFi.localIP().toString() + "</p>";
        html += "<button onclick=\"test(0)\" style=\"margin:10px;padding:20px;font-size:24px;\">ROT 0</button><br>";
        html += "<button onclick=\"test(1)\" style=\"margin:10px;padding:20px;font-size:24px;\">ROT 1</button><br>";
        html += "<button onclick=\"test(2)\" style=\"margin:10px;padding:20px;font-size:24px;\">ROT 2</button><br>";
        html += "<button onclick=\"test(3)\" style=\"margin:10px;padding:20px;font-size:24px;\">ROT 3</button><br>";
        html += "<div id=\"result\" style=\"margin:20px;font-size:18px;\"></div>";
        html += "<script>";
        html += "function test(r) {";
        html += "  document.getElementById('result').innerHTML = 'Testing rotation ' + r + '...';";
        html += "  fetch('/test-rotation?r=' + r)";
        html += "    .then(response => response.text())";
        html += "    .then(data => document.getElementById('result').innerHTML = data);";
        html += "}";
        html += "</script></body></html>";
        request->send(200, "text/html", html);
    });
    
    // Add debug endpoint to list files
    server.on("/list-files", HTTP_GET, [](AsyncWebServerRequest *request){
        String fileList = "Files in LittleFS:\n";
        File root = LittleFS.open("/");
        File file;
        int fileCount = 0;
        
        while ((file = root.openNextFile())) {
            String fileName = file.name();
            size_t fileSize = file.size();
            fileList += String(fileCount) + ": " + fileName + " (" + String(fileSize) + " bytes)\n";
            fileCount++;
            file.close();
        }
        root.close();
        
        if (fileCount == 0) {
            fileList += "No files found!\n";
        }
        
        request->send(200, "text/plain", fileList);
    });

    server.on("/test-rotation", HTTP_GET, [](AsyncWebServerRequest *request){
        if (request->hasParam("r")) {
            int rotation = request->getParam("r")->value().toInt();
            Serial.println("Testing rotation: " + String(rotation));
            
            if (rotation >= 0 && rotation <= 3) {
                // Find first available image in LittleFS
                File root = LittleFS.open("/");
                File file;
                String imagePath = "";
                int fileCount = 0;
                
                Serial.println("Searching for JPEG files in LittleFS...");
                while ((file = root.openNextFile())) {
                    String fileName = file.name();
                    Serial.println("Found file: " + fileName + " (size: " + String(file.size()) + ")");
                    fileCount++;
                    
                    // Check for JPEG files (case insensitive)
                    String lowerName = fileName;
                    lowerName.toLowerCase();
                    if (lowerName.endsWith(".jpg") || lowerName.endsWith(".jpeg")) {
                        imagePath = fileName.startsWith("/") ? fileName : "/" + fileName;
                        Serial.println("Selected JPEG: " + imagePath);
                        file.close();
                        break;
                    }
                    file.close();
                }
                root.close();
                
                Serial.println("Total files found: " + String(fileCount));
                Serial.println("Selected image path: " + imagePath);
                
                // Test rotation on display 1
                displayManager.selectDisplay(1);
                TFT_eSPI* tft = displayManager.getTFT(1);
                if (tft) {
                    tft->setRotation(rotation);
                    tft->fillScreen(TFT_BLACK);
                    
                    if (imagePath.length() > 0) {
                        Serial.println("Found image: " + imagePath);
                        
                        // Set up TJpg decoder callback
                        currentDisplayTft = tft;
                        TJpgDec.setCallback(tftOutput);
                        
                        // Decode and display the image
                        uint8_t result = TJpgDec.drawFsJpg(0, 0, imagePath.c_str());
                        Serial.println("TJpg decode result: " + String(result));
                    } else {
                        // Draw a test pattern that clearly shows orientation
                        Serial.println("No image found, drawing test pattern");
                        
                        // Clear screen
                        tft->fillScreen(TFT_BLACK);
                        
                        // Draw orientation indicators
                        // TOP: White bar
                        tft->fillRect(0, 0, tft->width(), 20, TFT_WHITE);
                        tft->setTextColor(TFT_BLACK, TFT_WHITE);
                        tft->setTextSize(2);
                        tft->drawString("TOP", 10, 2, 2);
                        
                        // BOTTOM: Red bar  
                        tft->fillRect(0, tft->height()-20, tft->width(), 20, TFT_RED);
                        tft->setTextColor(TFT_WHITE, TFT_RED);
                        tft->drawString("BOT", 10, tft->height()-18, 2);
                        
                        // LEFT: Green bar
                        tft->fillRect(0, 20, 20, tft->height()-40, TFT_GREEN);
                        tft->setTextColor(TFT_BLACK, TFT_GREEN);
                        tft->drawString("L", 5, tft->height()/2-8, 2);
                        
                        // RIGHT: Blue bar
                        tft->fillRect(tft->width()-20, 20, 20, tft->height()-40, TFT_BLUE);
                        tft->setTextColor(TFT_WHITE, TFT_BLUE);
                        tft->drawString("R", tft->width()-15, tft->height()/2-8, 2);
                        
                        // Center: Arrow pointing UP
                        int centerX = tft->width() / 2;
                        int centerY = tft->height() / 2;
                        
                        // Draw arrow pointing up
                        tft->fillTriangle(centerX, centerY-30, centerX-20, centerY, centerX+20, centerY, TFT_YELLOW);
                        tft->fillRect(centerX-8, centerY, 16, 30, TFT_YELLOW);
                        
                        tft->setTextColor(TFT_BLACK, TFT_YELLOW);
                        tft->drawString("UP", centerX-15, centerY+5, 2);
                    }
                    
                    // Add rotation indicator in a corner
                    tft->setTextColor(TFT_WHITE, TFT_BLACK);
                    tft->setTextSize(2);
                    String label = "R" + String(rotation);
                    tft->drawString(label, 5, 25, 2);
                    
                    Serial.println("Display 1 - Rotation " + String(rotation) + " applied");
                }
                displayManager.deselectAll();
                
                if (imagePath.length() > 0) {
                    request->send(200, "text/plain", "Rotation " + String(rotation) + " applied with image " + imagePath + "! Check display 1.");
                } else {
                    request->send(200, "text/plain", "Rotation " + String(rotation) + " applied with test pattern! Check display 1. Look for: TOP(white), BOT(red), L(green), R(blue), UP(yellow arrow).");
                }
            } else {
                request->send(400, "text/plain", "Invalid rotation: " + String(rotation));
            }
        } else {
            request->send(400, "text/plain", "Missing rotation parameter");
        }
    });
    
    server.begin();
    Serial.println("✅ Web server started");
    Serial.println("Open: http://" + WiFi.localIP().toString());
}

void loop() {
    delay(1000);
}

#elif defined(TFT_TEST_ONLY)
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
#include "config.h"

// Create instances
DisplayManager displayManager;
AsyncWebServer server(80);          // ADD: Web server

// Configure TCP settings to reduce timeout errors
void configureTCPSettings() {
    // More conservative TCP settings for better stability
    WiFi.setTxPower(WIFI_POWER_15dBm);  // Further reduce power
    
    // Give more time for ESP32 to handle requests
    delay(10);
}

TimeManager timeManager;            // ADD: Time manager
SettingsManager settingsManager;    // ADD: Settings manager
ImageManager imageManager(&displayManager);  // ADD: Image manager
DisplayClockManager clockManager(&displayManager, &timeManager);  // ADD: Clock manager
SlideshowManager slideshowManager(&imageManager, &settingsManager, &clockManager);  // ADD: Slideshow manager
WiFiManager wifiManager(&server, &timeManager, &settingsManager, &displayManager, &imageManager, &slideshowManager);   // ADD: WiFi manager with all components
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
        
        // Give more time for network processing
        delay(10);
        yield();
    }
    
    // Essential yield for ESP32 with small delay to reduce load
    delay(5);
    yield();
}
#endif