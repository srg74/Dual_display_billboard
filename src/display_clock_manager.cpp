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
    currentClockFace = CLOCK_CLASSIC_ANALOG;  // Default to classic analog
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
    
    digitalWrite(csPin, LOW);
    tft.setRotation(0); // Use rotation 0 like the original
    tft.fillScreen(TFT_BLACK);

    // Draw clock label above the clock
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.setTextFont(2);
    String currentLabel = timeManager ? timeManager->getClockLabel() : "Clock";
    int textWidth = tft.textWidth(currentLabel.c_str());
    tft.setCursor(40 - textWidth / 2, 18); // Center the label
    tft.print(currentLabel);

    // Display the selected clock face type
    switch (currentClockFace) {
        case CLOCK_CLASSIC_ANALOG:
            displayAnalogClock(tft);
            break;
        case CLOCK_DIGITAL_MODERN:
            displayDigitalClock(tft);
            break;
        case CLOCK_MINIMALIST:
            displayMinimalistClock(tft);
            break;
        case CLOCK_COLORFUL:
            displayColorfulClock(tft);
            break;
        default:
            displayAnalogClock(tft); // Fallback to analog
            break;
    }

    digitalWrite(csPin, HIGH);
}

// Clock face implementations (Phase 2)
void DisplayClockManager::displayAnalogClock(TFT_eSPI& tft) {
    // Get current time
    time_t now = time(nullptr);
    struct tm timeinfo;
    localtime_r(&now, &timeinfo);
    
    // Clock face dimensions
    int centerX = 40, centerY = 80;
    int radius = 35;
    
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
}

void DisplayClockManager::displayDigitalClock(TFT_eSPI& tft) {
    // Get current time
    time_t now = time(nullptr);
    struct tm timeinfo;
    localtime_r(&now, &timeinfo);
    
    // Modern digital display with large font
    tft.setTextColor(TFT_CYAN, TFT_BLACK);
    tft.setTextFont(4);
    
    // Format time as HH:MM
    char timeStr[10];
    snprintf(timeStr, sizeof(timeStr), "%02d:%02d", timeinfo.tm_hour, timeinfo.tm_min);
    
    // Center the time
    int textWidth = tft.textWidth(timeStr);
    tft.setCursor(40 - textWidth / 2, 60);
    tft.print(timeStr);
}

void DisplayClockManager::displayMinimalistClock(TFT_eSPI& tft) {
    // Get current time
    time_t now = time(nullptr);
    struct tm timeinfo;
    localtime_r(&now, &timeinfo);
    
    // Simple, clean design with minimal elements
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.setTextFont(4);
    
    // Just the time, centered
    char timeStr[10];
    snprintf(timeStr, sizeof(timeStr), "%02d:%02d", timeinfo.tm_hour, timeinfo.tm_min);
    
    int textWidth = tft.textWidth(timeStr);
    tft.setCursor(40 - textWidth / 2, 70);
    tft.print(timeStr);
    
    // Single line under the time
    tft.drawLine(20, 95, 60, 95, TFT_WHITE);
}

void DisplayClockManager::displayColorfulClock(TFT_eSPI& tft) {
    // Get current time
    time_t now = time(nullptr);
    struct tm timeinfo;
    localtime_r(&now, &timeinfo);
    
    // Colorful gradient background effect
    for (int i = 0; i < 160; i++) {
        uint16_t color = tft.color565(i/2, (160-i)/2, 100);
        tft.drawFastHLine(0, i, 80, color);
    }
    
    // Multi-colored time display
    tft.setTextFont(4);
    
    // Hour in red
    char hourStr[5];
    snprintf(hourStr, sizeof(hourStr), "%02d", timeinfo.tm_hour);
    tft.setTextColor(TFT_RED, TFT_BLACK);
    tft.setCursor(15, 60);
    tft.print(hourStr);
    
    // Colon in white
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.setCursor(35, 60);
    tft.print(":");
    
    // Minutes in green
    char minStr[5];
    snprintf(minStr, sizeof(minStr), "%02d", timeinfo.tm_min);
    tft.setTextColor(TFT_GREEN, TFT_BLACK);
    tft.setCursor(45, 60);
    tft.print(minStr);
}

void DisplayClockManager::setClockLabel(const String& label) {
    if (timeManager) {
        timeManager->setClockLabel(label);
        Serial.printf("[ClockManager] Clock label set to: %s\n", label.c_str());
    }
}

String DisplayClockManager::getClockLabel() {
    return timeManager ? timeManager->getClockLabel() : "Clock";
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

// Clock face management methods (Phase 2)
void DisplayClockManager::setClockFace(ClockFaceType faceType) {
    currentClockFace = faceType;
    Serial.printf("[ClockManager] Clock face changed to: %s\n", getClockFaceName(faceType).c_str());
}

ClockFaceType DisplayClockManager::getClockFace() const {
    return currentClockFace;
}

String DisplayClockManager::getClockFaceName(ClockFaceType faceType) const {
    switch (faceType) {
        case CLOCK_CLASSIC_ANALOG: return "Classic Analog";
        case CLOCK_DIGITAL_MODERN: return "Digital Modern";
        case CLOCK_MINIMALIST: return "Minimalist";
        case CLOCK_COLORFUL: return "Colorful";
        default: return "Unknown";
    }
}
