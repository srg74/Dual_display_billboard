#include "display_manager.h"
#include "display_timing_config.h"
#include "secrets.h"
#include "logger.h"
#include "splash_screen.h"

DisplayManager::DisplayManager() : initialized(false), brightness1(255), brightness2(255), 
                                   splashStartTime(0), splashActive(false), splashTimeoutMs(2000),
                                   portalSequenceActive(false) {
}

bool DisplayManager::begin() {
    LOG_INFO("DISPLAY", "🎨 Initializing Display Manager with working config");
    
    initializeBacklight();
    initializeCS();
    initializeTFT();
    
    // IMMEDIATE: Clear both displays to black before any other operations
    clearBothDisplaysToBlack();
    
    initialized = true;
    LOG_INFO("DISPLAY", "✅ Display Manager initialized successfully");
    return true;
}

void DisplayManager::initializeBacklight() {
    LOG_INFO("DISPLAY", "🔧 Setting up backlights...");
    
    // Backlight 1 setup (GPIO 22)
    ledcAttachPin(TFT_BACKLIGHT1_PIN, 1); // GPIO 22 → Channel 1
    ledcSetup(1, 5000, 8); // Channel 1, 5 KHz, 8-bit
    ledcWrite(1, 255); // Full brightness
    
    // Backlight 2 setup (GPIO 27)  
    ledcAttachPin(TFT_BACKLIGHT2_PIN, 2); // GPIO 27 → Channel 2
    ledcSetup(2, 5000, 8); // Channel 2, 5 KHz, 8-bit
    ledcWrite(2, 255); // Full brightness
    
    LOG_INFO("DISPLAY", "✅ Both backlights initialized");
}

void DisplayManager::initializeCS() {
    LOG_INFO("DISPLAY", "🔧 Setting up CS pins...");
    
    // CS pins setup (exact copy from working project)
    pinMode(firstScreenCS, OUTPUT);     // GPIO 5 = OUTPUT
    digitalWrite(firstScreenCS, HIGH);  // GPIO 5 = HIGH (deselected)
    pinMode(secondScreenCS, OUTPUT);    // GPIO 15 = OUTPUT
    digitalWrite(secondScreenCS, HIGH); // GPIO 15 = HIGH (deselected)

    LOG_INFO("DISPLAY", "✅ CS pins configured");
}

void DisplayManager::initializeTFT() {
    LOG_INFO("DISPLAY", "🔧 Initializing TFT...");
    
    // TFT initialization (EXACT copy from working project)
    digitalWrite(firstScreenCS, LOW);   // Both CS LOW
    digitalWrite(secondScreenCS, LOW);
    tft.init();                         // Init with both selected
    tft.fillScreen(TFT_BLACK);          // Immediately clear any init flash
    digitalWrite(firstScreenCS, HIGH);  // Both CS HIGH
    digitalWrite(secondScreenCS, HIGH);
    
    // Set correct rotation for both displays immediately after init
    selectDisplay(1);
    tft.setRotation(0);  // Ensure display 1 uses correct rotation
    tft.fillScreen(TFT_BLACK);  // Clear display 1
    selectDisplay(2); 
    tft.setRotation(0);  // Ensure display 2 uses correct rotation
    tft.fillScreen(TFT_BLACK);  // Clear display 2
    deselectAll();
    
    LOG_INFO("DISPLAY", "✅ TFT initialized with dual CS method");
}

void DisplayManager::selectDisplay(int displayNum) {
    // Default to text rotation for backward compatibility
    selectDisplayForText(displayNum);
}

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

void DisplayManager::deselectAll() {
    digitalWrite(firstScreenCS, HIGH);
    digitalWrite(secondScreenCS, HIGH);
}

void DisplayManager::clearBothDisplaysToBlack() {
    LOG_INFO("DISPLAY", "⚫ Clearing both displays to dark (no light) on startup");
    
    // Both displays: Clear to black content AND turn off brightness
    selectDisplay(1);
    tft.fillScreen(TFT_BLACK);
    setBrightness(0, 1);  // Turn off Display 1 brightness
    
    selectDisplay(2);
    tft.fillScreen(TFT_BLACK);
    setBrightness(0, 2);  // Turn off Display 2 brightness
    
    // Deselect all
    deselectAll();
    
    LOG_INFO("DISPLAY", "✅ Both displays cleared to dark (no light)");
}

void DisplayManager::setBrightness(uint8_t brightness, int displayNum) {
    if (displayNum == 1 || displayNum == 0) {
        brightness1 = brightness;
        ledcWrite(2, brightness); // Apply to backlight 1 (GPIO 27, Channel 2) - SWAPPED: Blue display is on Channel 2
        LOG_INFOF("DISPLAY", "🔆 Brightness set - Display 1: %d", brightness);
    }
    if (displayNum == 2 || displayNum == 0) {
        brightness2 = brightness;
        ledcWrite(1, brightness); // Apply to backlight 2 (GPIO 22, Channel 1) - SWAPPED: Yellow display is on Channel 1
        LOG_INFOF("DISPLAY", "🔆 Brightness set - Display 2: %d", brightness);
    }
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

// FIXED: Fast status display method - USES DISPLAY 1 FOR MESSAGES
void DisplayManager::showQuickStatus(const String& message, uint16_t color) {
    if (!initialized) return;
    
    // Display 1: Show status messages (FIXED)
    selectDisplay(1);  
    tft.fillScreen(color);
    tft.setTextColor(color == TFT_RED ? TFT_WHITE : TFT_BLACK);
    tft.setTextSize(1);
    tft.setTextDatum(MC_DATUM);
    tft.drawString(message, 80, 40, 2);
    
    // Display 2: Always keep dark (FIXED)
    selectDisplay(2);  
    tft.fillScreen(TFT_BLACK);
    setBrightness(0, 2);  
    
    deselectAll();
}

bool DisplayManager::isSplashActive() {
    return splashActive;
}

// Quick AP starting indicator
void DisplayManager::showAPStarting() {
    showQuickStatus("Starting AP...", TFT_ORANGE);
}

// Quick AP ready indicator  
void DisplayManager::showAPReady() {
    showQuickStatus("AP Ready!", TFT_BLUE);
}

// Quick connecting indicator
void DisplayManager::showConnecting() {
    showQuickStatus("Connecting...", TFT_YELLOW);
}

// FIXED: Portal info method - USES DISPLAY 1 FOR PORTAL INFO
void DisplayManager::showPortalInfo(const String& ssid, const String& ip, const String& status) {
    if (!initialized) {
        LOG_WARN("DISPLAY", "❌ Display not initialized - cannot show portal info");
        return;
    }
    
    LOG_INFO("DISPLAY", "📋 Showing portal information on display 1");  // FIXED
    
    // Display 1: Show portal info with GREEN background (FIXED)
    selectDisplay(1);  // FIXED: Changed to 1
    tft.fillScreen(TFT_GREEN);  // Green background
    tft.setTextColor(TFT_BLACK, TFT_GREEN);  // Black text on green background
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
    
    // Display 2: Keep dark (FIXED)
    selectDisplay(2);  
    tft.fillScreen(TFT_BLACK);
    setBrightness(0, 2);  
    
    deselectAll();
    
    LOG_INFO("DISPLAY", "✅ Portal info displayed - Screen 1: GREEN with text, Screen 2: dark");
}

// NEW: Show WiFi connection success message
void DisplayManager::showConnectionSuccess(const String& ip) {
    if (!initialized) {
        LOG_WARN("DISPLAY", "❌ Display not initialized - cannot show connection success");
        return;
    }
    
    LOG_INFO("DISPLAY", "📶 Showing WiFi connection success on display 1");
    
    // Display 1: Show connection success with BLUE background
    selectDisplay(1);
    tft.fillScreen(TFT_BLUE);  // Blue background
    tft.setTextColor(TFT_WHITE, TFT_BLUE);  // White text on blue background
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
    
    // Display 2: Keep dark
    selectDisplay(2);
    tft.fillScreen(TFT_BLACK);
    setBrightness(0, 2);
    
    deselectAll();
    
    LOG_INFOF("DISPLAY", "✅ Connection success displayed - IP: %s", ip.c_str());
}

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

void DisplayManager::drawColorBitmapRotated(int16_t x, int16_t y, const uint16_t *bitmap, 
                                          int16_t w, int16_t h, int displayNum) {
    selectDisplay(displayNum);
    
    // Rotate 270 degrees CW (or 90 degrees CCW) to fix upside down issue
    // For each pixel at (i,j) in original, draw at (h-1-j, i) in rotated
    for (int16_t j = 0; j < h; j++) {
        for (int16_t i = 0; i < w; i++) {
            int16_t pixelIndex = j * w + i;
            uint16_t color = pgm_read_word(&bitmap[pixelIndex]);
            
            // Calculate rotated position: 270 degrees CW (fixes upside down)
            int16_t rotatedX = x + (h - 1 - j);
            int16_t rotatedY = y + i;
            
            tft.drawPixel(rotatedX, rotatedY, color);
        }
    }
    
    deselectAll();
}

void DisplayManager::showSplashScreen(int displayNum, unsigned long timeoutMs) {
    if (displayNum == 0) {
        // Show on both displays
        showSplashScreen(1, timeoutMs);
        showSplashScreen(2, timeoutMs);
        return;
    }
    
    selectDisplayForImage(displayNum);  // Use image rotation for splash screen
    
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
    
    LOG_INFOF("DISPLAY", "🖼️ Rotated color splash screen displayed on display %d with full brightness (timeout: %lums)", displayNum, timeoutMs);
}

void DisplayManager::updateSplashScreen() {
    if (splashActive && (millis() - splashStartTime >= splashTimeoutMs)) {
        splashActive = false;
        
        // Check if we need to show portal info after splash
        if (portalSequenceActive) {
            portalSequenceActive = false;
            showPortalInfo(pendingSSID, pendingIP, pendingStatus);
            LOG_INFO("DISPLAY", "✅ Splash completed, showing portal info");
        } else {
            // Just clear screen
            fillScreen(TFT_BLACK);
            LOG_INFO("DISPLAY", "✅ Splash screen auto-cleared");
        }
    }
}

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

// TFT access for ImageManager
TFT_eSPI* DisplayManager::getTFT(int displayNum) {
    // Note: This returns the shared TFT instance
    // The display selection is handled by selectDisplay()
    if (displayNum >= 1 && displayNum <= 2) {
        return &tft;
    }
    return nullptr;
}

// Display type detection
String DisplayManager::getDisplayType() const {
    #ifdef DISPLAY_TYPE_ST7789
        return "ST7789";
    #else
        return "ST7735";
    #endif
}

uint16_t DisplayManager::getDisplayWidth() const {
    #ifdef DISPLAY_TYPE_ST7789
        return 240;
    #else
        return 160;
    #endif
}

uint16_t DisplayManager::getDisplayHeight() const {
    #ifdef DISPLAY_TYPE_ST7789
        return 240;
    #else
        return 80;
    #endif
}

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

