#include "display_manager.h"
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

void DisplayManager::drawMonochromeBitmap(int16_t x, int16_t y, const uint8_t *bitmap, 
                                         int16_t w, int16_t h, uint16_t color, uint16_t bg, int displayNum) {
    selectDisplay(displayNum);
    
    for (int16_t j = 0; j < h; j++) {
        for (int16_t i = 0; i < w; i++) {
            int16_t byteIndex = j * ((w + 7) / 8) + i / 8;
            int16_t bitIndex = 7 - (i % 8);
            
            if (bitmap[byteIndex] & (1 << bitIndex)) {
                tft.drawPixel(x + i, y + j, color);
            } else {
                tft.drawPixel(x + i, y + j, bg);
            }
        }
    }
    
    deselectAll();
}

void DisplayManager::drawMonochromeBitmapRotated(int16_t x, int16_t y, const uint8_t *bitmap, 
                                               int16_t w, int16_t h, uint16_t color, uint16_t bg, int displayNum) {
    selectDisplay(displayNum);
    
    // Rotate 270 degrees CW (or 90 degrees CCW) to fix upside down issue
    // For each pixel at (i,j) in original, draw at (h-1-j, i) in rotated
    for (int16_t j = 0; j < h; j++) {
        for (int16_t i = 0; i < w; i++) {
            int16_t byteIndex = j * ((w + 7) / 8) + i / 8;
            int16_t bitIndex = 7 - (i % 8);
            
            // Calculate rotated position: 270 degrees CW (fixes upside down)
            int16_t rotatedX = x + (h - 1 - j);
            int16_t rotatedY = y + i;
            
            if (bitmap[byteIndex] & (1 << bitIndex)) {
                tft.drawPixel(rotatedX, rotatedY, color);
            } else {
                tft.drawPixel(rotatedX, rotatedY, bg);
            }
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
    
    selectDisplay(displayNum);
    
    // Clear screen with black background
    fillScreen(TFT_BLACK, displayNum);
    
    // Calculate center position for rotated bitmap (80x160 becomes 160x80 after rotation)
    // The rotated bitmap will be 160x80, which perfectly fits the 160x80 display
    int16_t centerX = 0;  // Start at left edge
    int16_t centerY = 0;  // Start at top edge
    
    // Draw the rotated bitmap (white pixels on black background)
    drawMonochromeBitmapRotated(centerX, centerY, epd_bitmap_, 80, 160, TFT_WHITE, TFT_BLACK, displayNum);
    
    // Set splash timing
    splashStartTime = millis();
    splashActive = true;
    splashTimeoutMs = timeoutMs;
    
    LOG_INFOF("DISPLAY", "🖼️ Rotated splash screen displayed on display %d (timeout: %lums)", displayNum, timeoutMs);
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
    
    // Show splash screen for 5 seconds, then portal info will auto-display
    showSplashScreen(0, 5000);  // 5 seconds on both displays
    
    LOG_INFO("DISPLAY", "🚀 Portal sequence started: 5s splash → portal info");
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

