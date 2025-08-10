#include "display_clock_manager.h"
#include "time_manager.h"
#include <math.h>

DisplayClockManager::DisplayClockManager(DisplayManager* dm, TimeManager* tm) {
    displayManager = dm;
    timeManager = tm;
    showClock = false;
    clockDisplayStart = 0;
    galleryStartTime = 0;
    firstScreenCS = -1;
    secondScreenCS = -1;
    enableSecondDisplay = true;
    clockLabel = "Munich";
}

bool DisplayClockManager::begin() {
    if (!displayManager || !timeManager) {
        return false;
    }
    // Clock manager doesn't need specific initialization
    return true;
}

void DisplayClockManager::begin(int firstCS, int secondCS) {
    firstScreenCS = firstCS;
    secondScreenCS = secondCS;
    showClock = false;
    clockDisplayStart = 0;
    galleryStartTime = 0;
    
    Serial.println("[ClockManager] Initialized with display pins");
}

void DisplayClockManager::resetGalleryTimer() {
    galleryStartTime = millis();
    showClock = false;
    Serial.println("[ClockManager] Gallery timer reset");
}

bool DisplayClockManager::shouldShowClock() {
    return !showClock && galleryStartTime > 0 && 
           millis() - galleryStartTime > GALLERY_INTERVAL;
}

bool DisplayClockManager::shouldHideClock() {
    return showClock && millis() - clockDisplayStart > CLOCK_DISPLAY_DURATION;
}

void DisplayClockManager::startClockDisplay() {
    showClock = true;
    clockDisplayStart = millis();
    Serial.println("[ClockManager] Clock display started");
}

void DisplayClockManager::stopClockDisplay() {
    showClock = false;
    Serial.println("[ClockManager] Clock display stopped");
}

bool DisplayClockManager::isClockDisplayActive() {
    return showClock;
}

void DisplayClockManager::displayAnalogClockOnBothTFTs(TFT_eSPI& tft) {
    Serial.println("[ClockManager] Displaying analog clock on both TFTs");
    
    // Display on first screen
    displayClockOnDisplay(tft, firstScreenCS);
    
    // Display on second screen if enabled
    if (enableSecondDisplay) {
        displayClockOnDisplay(tft, secondScreenCS);
    }
}

void DisplayClockManager::displayClockOnDisplay(TFT_eSPI& tft, int csPin) {
    if (csPin < 0) return;
    
    time_t now = time(nullptr);
    struct tm timeinfo;
    localtime_r(&now, &timeinfo);

    int centerX = 40; // Center X for 80px wide display
    int centerY = 80; // Center Y for 160px tall display  
    int radius = 39;  // Clock radius

    digitalWrite(csPin, LOW);
    tft.setRotation(0); // Use rotation 0 like the original
    tft.fillScreen(TFT_BLACK);

    // Draw clock label above the clock
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.setTextFont(2);
    int textWidth = tft.textWidth(clockLabel.c_str());
    tft.setCursor(centerX - textWidth / 2, 18); // LINE_HEIGHT = 18
    tft.print(clockLabel);

    // Draw clock face
    tft.drawCircle(centerX, centerY, radius, TFT_SKYBLUE);
    tft.drawCircle(centerX, centerY, 2, TFT_WHITE);

    // Draw hour marks
    for (int h = 0; h < 12; h++) {
        float angle = (h * 30 - 90) * DEG_TO_RAD;
        int x0 = centerX + cos(angle) * (radius - 8);
        int y0 = centerY + sin(angle) * (radius - 8);
        int x1 = centerX + cos(angle) * (radius - 2);
        int y1 = centerY + sin(angle) * (radius - 2);
        tft.drawLine(x0, y0, x1, y1, TFT_WHITE);
    }

    // Calculate angles for hands
    float minAngle = (timeinfo.tm_min * 6 + timeinfo.tm_sec * 0.1 - 90) * DEG_TO_RAD;
    float hourAngle = ((timeinfo.tm_hour % 12) * 30 + timeinfo.tm_min * 0.5 - 90) * DEG_TO_RAD;

    // Draw hour hand
    int hx = centerX + cos(hourAngle) * (radius * 0.5);
    int hy = centerY + sin(hourAngle) * (radius * 0.5);
    tft.drawLine(centerX, centerY, hx, hy, TFT_RED);

    // Draw minute hand
    int mx = centerX + cos(minAngle) * (radius * 0.8);
    int my = centerY + sin(minAngle) * (radius * 0.8);
    tft.drawLine(centerX, centerY, mx, my, TFT_RED);

    digitalWrite(csPin, HIGH);
}

void DisplayClockManager::setClockLabel(const String& label) {
    clockLabel = label;
    Serial.printf("[ClockManager] Clock label set to: %s\n", label.c_str());
}

String DisplayClockManager::getClockLabel() {
    return clockLabel;
}

void DisplayClockManager::setSecondDisplayEnabled(bool enabled) {
    enableSecondDisplay = enabled;
    Serial.printf("[ClockManager] Second display enabled: %s\n", enabled ? "true" : "false");
}

void DisplayClockManager::setDisplayPins(int firstCS, int secondCS) {
    firstScreenCS = firstCS;
    secondScreenCS = secondCS;
    Serial.printf("[ClockManager] Display pins set - First: %d, Second: %d\n", firstCS, secondCS);
}

unsigned long DisplayClockManager::getGalleryElapsedTime() {
    if (galleryStartTime == 0) return 0;
    return millis() - galleryStartTime;
}

unsigned long DisplayClockManager::getClockDisplayElapsedTime() {
    if (clockDisplayStart == 0) return 0;
    return millis() - clockDisplayStart;
}

void DisplayClockManager::displayClockOnBothDisplays() {
    if (!displayManager || !timeManager) {
        Serial.println("[ClockManager] DisplayManager or TimeManager not available");
        return;
    }
    
    // Get current time directly from system
    time_t now = time(nullptr);
    struct tm timeInfo;
    localtime_r(&now, &timeInfo);
    
    // Check if time is valid
    if (!timeManager->isTimeValid()) {
        Serial.println("[ClockManager] Time not synchronized for clock display");
        return;
    }
    
    // Display on first screen
    TFT_eSPI* tft1 = displayManager->getTFT(1);
    if (tft1) {
        displayManager->selectDisplayForImage(1);
        displayClockOnDisplay(*tft1, 1);
    }
    
    // Display on second screen if enabled
    TFT_eSPI* tft2 = displayManager->getTFT(2);
    if (tft2 && enableSecondDisplay) {
        displayManager->selectDisplayForImage(2);
        displayClockOnDisplay(*tft2, 2);
    }
}
