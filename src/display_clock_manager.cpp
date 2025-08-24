/**
 * @file display_clock_manager.cpp
 * @brief Advanced clock display system with multiple face styles and dual display support
 * 
 * This module provides sophisticated clock rendering capabilities for the dual display
 * billboard system. It manages multiple clock face styles, synchronizes displays,
 * and handles the alternation between gallery mode and clock display.
 * 
 * Features:
 * - Multiple clock face styles (analog, digital, minimalist, modern square)
 * - Dual display synchronization with independent control
 * - Adaptive label positioning for different display sizes (ST7735/ST7789)
 * - Gallery/clock alternation with configurable intervals
 * - Dynamic label updates with time synchronization
 * - Hardware-optimized rendering with size-aware layout
 * - Clean sans-serif typography for professional appearance
 * 
 * @author Dual Display Billboard Project
 * @version 0.9
 * @date August 2025
 */

#include "display_clock_manager.h"
#include "time_manager.h"
#include "text_utils.h"
#include <math.h>

/**
 * @brief Constructor - initializes display clock manager with required dependencies
 * @param dm DisplayManager instance for TFT control
 * @param tm TimeManager instance for time synchronization
 */
DisplayClockManager::DisplayClockManager(DisplayManager* dm, TimeManager* tm) {
    displayManager = dm;
    timeManager = tm;
    
    // Initialize hardware configuration
    firstScreenCS = -1;
    secondScreenCS = -1;
    enableSecondDisplay = true;
    
    // Set default clock face style
    currentClockFace = CLOCK_MODERN_SQUARE;
}

/**
 * @brief Initialize clock manager (basic initialization)
 * @return true if initialization successful, false if dependencies missing
 */
bool DisplayClockManager::begin() {
    if (!displayManager || !timeManager) {
        return false;
    }
    return true;
}

/**
 * @brief Initialize clock manager with specific display pin configuration
 * @param firstCS Chip select pin for primary display
 * @param secondCS Chip select pin for secondary display
 */
void DisplayClockManager::begin(int firstCS, int secondCS) {
    firstScreenCS = firstCS;
    secondScreenCS = secondCS;
    
    Serial.println("[ClockManager] Initialized with display pins");
}

/**
 * @brief Renders clock display on specified hardware display
 * @param tft TFT_eSPI instance for rendering
 * @param csPin Chip select pin for target display
 * 
 * Complete clock rendering pipeline:
 * 1. Display selection via CS pin control
 * 2. Screen preparation and clearing
 * 3. Adaptive label positioning based on display size
 * 4. Clock face rendering based on current style
 * 5. Display deselection
 * 
 * Label positioning adapts to display size:
 * - Small displays (≤80px width): Center at X=40
 * - Large displays (>80px width): Center at X=120
 */
void DisplayClockManager::displayClockOnDisplay(TFT_eSPI& tft, int csPin) {
    if (csPin < 0) return;
    
    // Select target display via chip select
    digitalWrite(csPin, LOW);
    tft.setRotation(0);           // Portrait orientation for optimal clock display
    tft.fillScreen(TFT_BLACK);    // Clear previous content
    
    // Render clock label with Unicode support and proper centering
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    String rawLabel = timeManager ? timeManager->getClockLabel() : "Clock";
    String currentLabel = TextUtils::toDisplayText(rawLabel);  // Process international characters
    
    // Calculate adaptive center position based on display width
    int textWidth = TextUtils::getUnicodeTextWidth(tft, currentLabel);
    bool isSmallDisplay = (tft.width() <= 80);
    int centerX = isSmallDisplay ? 40 : 120;  // 80px displays: center=40, 240px displays: center=120
    int labelY = 20;  // Fixed Y position for consistent appearance
    
    // Draw Unicode text with proper character rendering
    int labelX = centerX - textWidth / 2;
    TextUtils::drawUnicodeText(tft, currentLabel, labelX, labelY, TFT_WHITE);

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
            displayModernSquareClock(tft);
            break;
        default:
            displayAnalogClock(tft); // Fallback to analog
            break;
    }

    digitalWrite(csPin, HIGH);
}

/**
 * @brief Renders classic analog clock face with circular design
 * @param tft TFT_eSPI instance for rendering
 * 
 * Features:
 * - Circular clock face with sky blue border
 * - White hour markers at all 12 positions
 * - Red hour and minute hands with smooth movement
 * - White center dot
 * - Optimized for 80x160 displays
 */
void DisplayClockManager::displayAnalogClock(TFT_eSPI& tft) {
    // Get current time
    time_t now = time(nullptr);
    struct tm timeinfo;
    localtime_r(&now, &timeinfo);
    
    // Clock face geometry for 80x160 display
    int centerX = 40, centerY = 80;
    int radius = 35;
    
    // Draw outer circle and center dot
    tft.drawCircle(centerX, centerY, radius, TFT_SKYBLUE);
    tft.drawCircle(centerX, centerY, 2, TFT_WHITE);

    // Draw hour markers at all 12 positions
    for (int h = 0; h < 12; h++) {
        float angle = (h * 30 - 90) * DEG_TO_RAD;
        int x0 = centerX + cos(angle) * (radius - 8);
        int y0 = centerY + sin(angle) * (radius - 8);
        int x1 = centerX + cos(angle) * (radius - 2);
        int y1 = centerY + sin(angle) * (radius - 2);
        tft.drawLine(x0, y0, x1, y1, TFT_WHITE);
    }

    // Calculate hand angles with smooth seconds for minute hand
    float minAngle = (timeinfo.tm_min * 6 + timeinfo.tm_sec * 0.1 - 90) * DEG_TO_RAD;
    float hourAngle = ((timeinfo.tm_hour % 12) * 30 + timeinfo.tm_min * 0.5 - 90) * DEG_TO_RAD;

    // Draw hour hand (shorter, red)
    int hx = centerX + cos(hourAngle) * (radius * 0.5);
    int hy = centerY + sin(hourAngle) * (radius * 0.5);
    tft.drawLine(centerX, centerY, hx, hy, TFT_RED);

    // Draw minute hand (longer, red)
    int mx = centerX + cos(minAngle) * (radius * 0.8);
    int my = centerY + sin(minAngle) * (radius * 0.8);
    tft.drawLine(centerX, centerY, mx, my, TFT_RED);
}

/**
 * @brief Renders modern digital clock with large cyan text
 * @param tft TFT_eSPI instance for rendering
 * 
 * Features:
 * - Large font size for excellent readability
 * - Cyan color for modern appearance
 * - HH:MM format (24-hour time)
 * - Centered positioning
 */
void DisplayClockManager::displayDigitalClock(TFT_eSPI& tft) {
    // Get current time
    time_t now = time(nullptr);
    struct tm timeinfo;
    localtime_r(&now, &timeinfo);
    
    // Modern digital display styling
    tft.setTextColor(TFT_CYAN, TFT_BLACK);
    tft.setTextFont(4);
    
    // Format time as HH:MM
    char timeStr[10];
    snprintf(timeStr, sizeof(timeStr), "%02d:%02d", timeinfo.tm_hour, timeinfo.tm_min);
    
    // Center the time display
    int textWidth = tft.textWidth(timeStr);
    tft.setCursor(40 - textWidth / 2, 60);
    tft.print(timeStr);
}

/**
 * @brief Renders minimalist clock with clean, simple design
 * @param tft TFT_eSPI instance for rendering
 * 
 * Features:
 * - Minimal visual elements for clean appearance
 * - White text on black background
 * - Single decorative line under time
 * - Centered time display
 */
void DisplayClockManager::displayMinimalistClock(TFT_eSPI& tft) {
    // Get current time
    time_t now = time(nullptr);
    struct tm timeinfo;
    localtime_r(&now, &timeinfo);
    
    // Clean, minimal styling
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.setTextFont(4);
    
    // Format and display time
    char timeStr[10];
    snprintf(timeStr, sizeof(timeStr), "%02d:%02d", timeinfo.tm_hour, timeinfo.tm_min);
    
    int textWidth = tft.textWidth(timeStr);
    tft.setCursor(40 - textWidth / 2, 70);
    tft.print(timeStr);
    
    // Add single decorative line
    tft.drawLine(20, 95, 60, 95, TFT_WHITE);
}

/**
 * @brief Renders modern square analog clock with adaptive display size support
 * @param tft TFT_eSPI instance for rendering
 * 
 * Features:
 * - Rounded square border design for modern aesthetic
 * - Adaptive sizing for both 80x160 and 240x240 displays
 * - Blue analog hands with red center dot
 * - Quarter-hour markers (12, 3, 6, 9) are longer than others
 * - Time synchronization validation with fallback display
 * - Graceful error handling for coordinate and math errors
 */
void DisplayClockManager::displayModernSquareClock(TFT_eSPI& tft) {
    // Obtain current system time
    time_t now = time(nullptr);
    struct tm timeinfo;
    localtime_r(&now, &timeinfo);
    
    // Validate time synchronization before rendering
    if (now < 1000000000) { // Check for reasonable timestamp (post-2001)
        String clockLabel = getClockLabel();
        tft.setTextColor(TFT_WHITE, TFT_BLACK);
        tft.setTextFont(4);
        int textWidth = tft.textWidth(clockLabel.c_str());
        
        // Adaptive positioning for different display sizes
        bool isSmallDisplay = (tft.width() <= 80);
        int centerX = isSmallDisplay ? 40 : 120;
        int textX = centerX - textWidth / 2;
        int textY = isSmallDisplay ? 25 : 30;
        
        tft.setCursor(textX, textY);
        tft.print(clockLabel);
        tft.setCursor(isSmallDisplay ? 5 : 80, isSmallDisplay ? 80 : 120);
        tft.print("No Time Sync");
        return;
    }
    
    // Calculate display dimensions and positioning
    bool isSmallDisplay = (tft.width() <= 80);
    int centerX = isSmallDisplay ? 40 : 120;    // Display center X coordinate
    int centerY = isSmallDisplay ? 80 : 120;    // Display center Y coordinate  
    int clockRadius = isSmallDisplay ? 35 : 80; // Clock radius scaled to display
    
    // Validate calculated coordinates
    if (centerX < 0 || centerX > tft.width() || centerY < 0 || centerY > tft.height() || clockRadius <= 0) {
        String clockLabel = getClockLabel();
        tft.setTextColor(TFT_WHITE, TFT_BLACK);
        tft.setTextFont(2);
        int textWidth = tft.textWidth(clockLabel.c_str());
        int textX = centerX - textWidth / 2;
        tft.setCursor(textX, isSmallDisplay ? 25 : 30);
        tft.print(clockLabel);
        tft.setCursor(isSmallDisplay ? 5 : 80, isSmallDisplay ? 80 : 120);
        tft.print("Coord Error");
        return;
    }
    
    // Draw rounded square frame with double border for depth
    tft.drawRoundRect(centerX - clockRadius, centerY - clockRadius, 
                      clockRadius * 2, clockRadius * 2, 8, TFT_WHITE);
    tft.drawRoundRect(centerX - clockRadius + 1, centerY - clockRadius + 1, 
                      clockRadius * 2 - 2, clockRadius * 2 - 2, 7, TFT_WHITE);
    
    // Draw hour markers with emphasis on quarter hours
    for (int i = 0; i < 12; i++) {
        float angle = (i * 30 - 90) * PI / 180;  // Convert to radians, start from 12 o'clock
        
        // Quarter-hour markers (12, 3, 6, 9) are longer
        int markerLength = (i % 3 == 0) ? (isSmallDisplay ? 8 : 12) : (isSmallDisplay ? 4 : 6);
        int outerX = centerX + (clockRadius - 5) * cos(angle);
        int outerY = centerY + (clockRadius - 5) * sin(angle);
        int innerX = centerX + (clockRadius - 5 - markerLength) * cos(angle);
        int innerY = centerY + (clockRadius - 5 - markerLength) * sin(angle);
        
        tft.drawLine(innerX, innerY, outerX, outerY, TFT_WHITE);
    }
    
    // Calculate hand angles for smooth movement
    float hourAngle = ((timeinfo.tm_hour % 12) * 30 + timeinfo.tm_min * 0.5 - 90) * PI / 180;
    float minuteAngle = (timeinfo.tm_min * 6 - 90) * PI / 180;
    
    // Validate calculated angles
    if (isnan(hourAngle) || isnan(minuteAngle) || isinf(hourAngle) || isinf(minuteAngle)) {
        String clockLabel = getClockLabel();
        tft.setTextColor(TFT_WHITE, TFT_BLACK);
        tft.setTextFont(2);
        int textWidth = tft.textWidth(clockLabel.c_str());
        int textX = centerX - textWidth / 2;
        tft.setCursor(textX, isSmallDisplay ? 25 : 30);
        tft.print(clockLabel);
        
        // Display time as fallback
        char timeStr[10];
        snprintf(timeStr, sizeof(timeStr), "%02d:%02d", timeinfo.tm_hour, timeinfo.tm_min);
        tft.setCursor(isSmallDisplay ? 20 : 80, isSmallDisplay ? 80 : 120);
        tft.print(timeStr);
        tft.setCursor(isSmallDisplay ? 5 : 60, isSmallDisplay ? 100 : 140);
        tft.print("Math Error");
        return;
    }
    
    // Draw minute hand (longer, thin, blue)
    int minuteHandLength = isSmallDisplay ? 25 : 55;
    int minuteX = centerX + minuteHandLength * cos(minuteAngle);
    int minuteY = centerY + minuteHandLength * sin(minuteAngle);
    tft.drawLine(centerX, centerY, minuteX, minuteY, TFT_BLUE);
    
    // Draw hour hand (shorter, thick, blue)
    int hourHandLength = isSmallDisplay ? 18 : 40;
    int hourX = centerX + hourHandLength * cos(hourAngle);
    int hourY = centerY + hourHandLength * sin(hourAngle);
    
    // Make hour hand thicker with parallel line
    tft.drawLine(centerX, centerY, hourX, hourY, TFT_BLUE);
    tft.drawLine(centerX + 1, centerY, hourX + 1, hourY, TFT_BLUE);
    
    // Draw center dot
    int centerDotRadius = isSmallDisplay ? 3 : 5;
    tft.fillCircle(centerX, centerY, centerDotRadius, TFT_RED);
}

/**
 * @brief Set custom clock label text
 * @param label Custom label text to display above clock
 */
void DisplayClockManager::setClockLabel(const String& label) {
    if (timeManager) {
        timeManager->setClockLabel(label);
        Serial.printf("[ClockManager] Clock label set to: %s\n", label.c_str());
    }
}

/**
 * @brief Get current clock label text
 * @return Current clock label or "Clock" if not set
 */
String DisplayClockManager::getClockLabel() {
    return timeManager ? timeManager->getClockLabel() : "Clock";
}

/**
 * @brief Enable or disable second display
 * @param enabled true to enable second display, false to disable
 */
void DisplayClockManager::setSecondDisplayEnabled(bool enabled) {
    enableSecondDisplay = enabled;
    Serial.printf("[ClockManager] Second display enabled: %s\n", enabled ? "true" : "false");
}

/**
 * @brief Configure display pin assignments
 * @param firstCS Chip select pin for primary display
 * @param secondCS Chip select pin for secondary display
 */
void DisplayClockManager::setDisplayPins(int firstCS, int secondCS) {
    firstScreenCS = firstCS;
    secondScreenCS = secondCS;
    Serial.printf("[ClockManager] Display pins set - First: %d, Second: %d\n", firstCS, secondCS);
}

/**
 * @brief Display clock on both screens simultaneously
 * 
 * Coordinates rendering on primary and secondary displays with proper
 * display manager integration for consistent timing and synchronization.
 */
void DisplayClockManager::displayClockOnBothDisplays() {
    if (!displayManager || !timeManager) {
        Serial.println("[ClockManager] DisplayManager or TimeManager not available");
        return;
    }
    
    // Get current time for consistent display across both screens
    time_t now = time(nullptr);
    struct tm timeInfo;
    localtime_r(&now, &timeInfo);
    
    // Render on primary display
    TFT_eSPI* tft1 = displayManager->getTFT(1);
    if (tft1) {
        displayManager->selectDisplayForImage(1);
        displayClockOnDisplay(*tft1, 1);
    }
    
    // Render on secondary display if enabled
    TFT_eSPI* tft2 = displayManager->getTFT(2);
    if (tft2 && enableSecondDisplay) {
        displayManager->selectDisplayForImage(2);
        displayClockOnDisplay(*tft2, 2);
    }
}

/**
 * @brief Set active clock face style
 * @param faceType Clock face type from ClockFaceType enumeration
 */
void DisplayClockManager::setClockFace(ClockFaceType faceType) {
    currentClockFace = faceType;
    Serial.printf("[ClockManager] Clock face changed to: %s\n", getClockFaceName(faceType).c_str());
}

/**
 * @brief Get current active clock face type
 * @return Current ClockFaceType enumeration value
 */
ClockFaceType DisplayClockManager::getClockFace() const {
    return currentClockFace;
}

/**
 * @brief Get human-readable name for clock face type
 * @param faceType Clock face type to get name for
 * @return User-friendly name string
 */
String DisplayClockManager::getClockFaceName(ClockFaceType faceType) const {
    switch (faceType) {
        case CLOCK_CLASSIC_ANALOG: return "Classic Analog";
        case CLOCK_DIGITAL_MODERN: return "Digital Modern";
        case CLOCK_MINIMALIST: return "Minimalist";
        case CLOCK_MODERN_SQUARE: return "Modern Square";
        default: return "Unknown";
    }
}
