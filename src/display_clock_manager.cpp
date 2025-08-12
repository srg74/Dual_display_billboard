/**
 * @file display_clock_manager.cpp
 * @brief Advanced clock display system with multiple face styles and dual display support
 * 
 * Provides sophisticated clock rendering with:
 * - Multiple clock face styles (analog, digital, minimalist, modern square)
 * - Dual display synchronization
 * - Gallery/clock alternation timing
 * - Time-based label updates
 * - Hardware-optimized rendering
 */

#include "display_clock_manager.h"
#include "time_manager.h"
#include <math.h>

DisplayClockManager::DisplayClockManager(DisplayManager* dm, TimeManager* tm) {
    displayManager = dm;
    timeManager = tm;
    
    // Initialize timing state
    showClock = false;
    clockDisplayStart = 0;
    galleryStartTime = 0;
    
    // Initialize display hardware references
    firstScreenCS = -1;
    secondScreenCS = -1;
    enableSecondDisplay = true;
    
    // Set default clock face to most visually appealing option
    currentClockFace = CLOCK_MODERN_SQUARE;
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

/**
 * @brief Renders clock display on specified hardware display
 * @param tft TFT_eSPI instance for rendering
 * @param csPin Chip select pin for target display
 * 
 * Complete clock rendering pipeline:
 * 1. Display selection via CS pin control
 * 2. Screen preparation and clearing
 * 3. Time-based label rendering
 * 4. Clock face rendering based on current style
 * 5. Display deselection
 */
void DisplayClockManager::displayClockOnDisplay(TFT_eSPI& tft, int csPin) {
    if (csPin < 0) return;
    
    // Select target display via chip select
    digitalWrite(csPin, LOW);
    tft.setRotation(0);           // Portrait orientation for optimal clock display
    tft.fillScreen(TFT_BLACK);    // Clear previous content
    
    // Render dynamic time-based label
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.setTextFont(1);
    String currentLabel = timeManager ? timeManager->getClockLabel() : "Clock";
    
    // Center the label horizontally
    int textWidth = tft.textWidth(currentLabel.c_str());
    tft.setCursor(40 - textWidth / 2, 18);
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
        case CLOCK_MODERN_SQUARE:
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

/**
 * @brief Renders modern square analog clock face with rounded border
 * @param tft TFT_eSPI instance for rendering
 * 
 * Features:
 * - Rounded square border design
 * - Color-coded clock hands (blue minute/hour, red center)
 * - Hour markers with emphasis on quarters (12, 3, 6, 9)
 * - Time synchronization validation
 * - Graceful fallback for invalid time states
 * - Optimized rendering for 160x80 displays
 */
void DisplayClockManager::displayColorfulClock(TFT_eSPI& tft) {
    // Obtain current system time
    time_t now = time(nullptr);
    struct tm timeinfo;
    localtime_r(&now, &timeinfo);
    
    // Validate time synchronization (avoid displaying incorrect time)
    if (now < 1000000000) { // Check for reasonable timestamp (post-2001)
        // Display time sync status instead of incorrect clock
        tft.fillScreen(TFT_BLACK);
        String clockLabel = getClockLabel();
        tft.setTextFont(1);
        tft.setTextSize(1);
        tft.setTextColor(TFT_WHITE, TFT_BLACK);
        int textWidth = clockLabel.length() * 6;
        int textX = (80 - textWidth) / 2;
        tft.setCursor(textX, 25);
        tft.print(clockLabel);
        tft.setCursor(5, 80);
        tft.print("No Time Sync");
        return;
    }
    
    // REMOVED: Minute throttling that was causing Display 2 to skip rendering
    // The static lastMinute variable was shared between both displays, causing
    // Display 2 to return early when Display 1 had already updated for the same minute
    
    // Screen already cleared by displayClockOnDisplay(), no need to clear again
    // tft.fillScreen(TFT_BLACK);
    
    // Define clock dimensions and center
    int centerX = 40;  // 80/2
    int centerY = 80;  // 160/2
    int clockRadius = 35;
    
    // Bounds checking for coordinates
    if (centerX < 0 || centerX > 80 || centerY < 0 || centerY > 160 || clockRadius <= 0) {
        // Fallback: just show label if coordinates are invalid
        String clockLabel = getClockLabel();
        tft.setTextFont(1);
        tft.setTextSize(1);
        tft.setTextColor(TFT_WHITE, TFT_BLACK);
        int textWidth = clockLabel.length() * 6;
        int textX = (80 - textWidth) / 2;
        tft.setCursor(textX, 25);
        tft.print(clockLabel);
        tft.setCursor(5, 80);
        tft.print("Coord Error");
        return;
    }
    
    // Draw rounded square frame (white border) - optimized single call
    tft.drawRoundRect(centerX - clockRadius, centerY - clockRadius, 
                      clockRadius * 2, clockRadius * 2, 8, TFT_WHITE);
    tft.drawRoundRect(centerX - clockRadius + 1, centerY - clockRadius + 1, 
                      clockRadius * 2 - 2, clockRadius * 2 - 2, 7, TFT_WHITE);
    
    // Draw hour markers (simplified for performance)
    for (int i = 0; i < 12; i++) {
        float angle = (i * 30 - 90) * PI / 180;  // Convert to radians, start from 12 o'clock
        
        // Calculate marker positions
        int markerLength = (i % 3 == 0) ? 8 : 4;  // Longer markers for 12, 3, 6, 9
        int outerX = centerX + (clockRadius - 5) * cos(angle);
        int outerY = centerY + (clockRadius - 5) * sin(angle);
        int innerX = centerX + (clockRadius - 5 - markerLength) * cos(angle);
        int innerY = centerY + (clockRadius - 5 - markerLength) * sin(angle);
        
        // Draw marker line (single call per marker)
        tft.drawLine(innerX, innerY, outerX, outerY, TFT_WHITE);
        // Skip extra thickness for main markers to improve performance
    }
    
    // Calculate hand angles
    float hourAngle = ((timeinfo.tm_hour % 12) * 30 + timeinfo.tm_min * 0.5 - 90) * PI / 180;
    float minuteAngle = (timeinfo.tm_min * 6 - 90) * PI / 180;
    
    // Debug: Check if angles are valid
    if (isnan(hourAngle) || isnan(minuteAngle) || isinf(hourAngle) || isinf(minuteAngle)) {
        // Fallback: just show label and time if angles are invalid
        String clockLabel = getClockLabel();
        tft.setTextFont(1);
        tft.setTextSize(1);
        tft.setTextColor(TFT_WHITE, TFT_BLACK);
        int textWidth = clockLabel.length() * 6;
        int textX = (80 - textWidth) / 2;
        tft.setCursor(textX, 25);
        tft.print(clockLabel);
        
        // Show time as fallback
        char timeStr[10];
        snprintf(timeStr, sizeof(timeStr), "%02d:%02d", timeinfo.tm_hour, timeinfo.tm_min);
        tft.setCursor(20, 80);
        tft.print(timeStr);
        tft.setCursor(5, 100);
        tft.print("Math Error");
        return;
    }
    
    // Draw minute hand (longer, thin, blue)
    int minuteHandLength = 25;
    int minuteX = centerX + minuteHandLength * cos(minuteAngle);
    int minuteY = centerY + minuteHandLength * sin(minuteAngle);
    tft.drawLine(centerX, centerY, minuteX, minuteY, TFT_BLUE);
    
    // Draw hour hand (shorter, thick, blue) - simplified for performance
    int hourHandLength = 18;
    int hourX = centerX + hourHandLength * cos(hourAngle);
    int hourY = centerY + hourHandLength * sin(hourAngle);
    
    // Make hour hand thicker with just 2 lines instead of 4
    tft.drawLine(centerX, centerY, hourX, hourY, TFT_BLUE);
    tft.drawLine(centerX + 1, centerY, hourX + 1, hourY, TFT_BLUE);
    
    // Draw red center dot
    tft.fillCircle(centerX, centerY, 3, TFT_RED);
    
    // Label is already rendered by displayClockOnDisplay(), no need to render again
    
    // No sprite push needed
    // clockSprite.pushSprite(0, 0);
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
        case CLOCK_MODERN_SQUARE: return "Modern Square";
        default: return "Unknown";
    }
}
