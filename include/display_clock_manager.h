/**
 * @file display_clock_manager.h
 * @brief Professional clock display system with multiple face styles for dual display billboard
 * 
 * Provides sophisticated clock rendering capabilities with multiple face styles,
 * dual display synchronization, customizable layouts, and seamless gallery integration
 * for the ESP32-based dual display billboard system.
 * 
 * Key Features:
 * - Multiple clock face styles (Classic Analog, Digital Modern, Minimalist, Modern Square)
 * - Dual display synchronization with independent content control
 * - Real-time update system with configurable refresh intervals
 * - Customizable labels and timezone-aware time display
 * - Smooth transitions between clock and gallery modes
 * - Memory-efficient rendering optimized for embedded systems
 * - Integration with settings persistence and time management
 * 
 * Clock Face Types:
 * - Classic Analog: Traditional circular clock with hour/minute hands
 * - Digital Modern: Large digital time display with date information
 * - Minimalist: Clean, simple design with essential time information
 * - Modern Square: Contemporary square layout with enhanced readability
 * 
 * Display Integration:
 * - Automatic display type detection (ST7735/ST7789)
 * - Independent clock content for each display
 * - Brightness synchronization with display settings
 * - Rotation handling for optimal clock presentation
 * - Efficient buffer management for smooth updates
 * 
 * Time Management:
 * - Integration with NTP time synchronization
 * - Timezone-aware display with automatic DST handling
 * - Customizable time format (12/24 hour) and date display
 * - User-configurable clock labels for personalization
 * 
 * Performance Optimization:
 * - Selective update regions to minimize display writes
 * - Frame rate control for smooth animation without flicker
 * - Memory pooling for graphic buffers
 * - Optimized drawing routines for embedded performance
 * 
 * @author Dual Display Billboard Project
 * @version 2.0
 * @date August 2025
 * @since v0.9
 */

#pragma once
#include <Arduino.h>
#include <TFT_eSPI.h>
#include <time.h>
#include "clock_types.h"
#include "display_manager.h"
#include "time_manager.h"

/**
 * @class DisplayClockManager
 * @brief Advanced clock display system with multiple face styles
 * 
 * Manages clock rendering across dual displays with support for:
 * - Multiple clock face styles (analog, digital, minimalist, modern square)
 * - Dual display synchronization
 * - Dynamic sizing for different display types
 * - Professional typography
 */

class DisplayClockManager {
private:
    DisplayManager* displayManager;
    TimeManager* timeManager;
    
    int firstScreenCS;
    int secondScreenCS;
    bool enableSecondDisplay;
    
    ClockFaceType currentClockFace;  // Current selected clock face

    // Internal clock rendering functions
    void displayAnalogClock(TFT_eSPI& tft);
    void displayDigitalClock(TFT_eSPI& tft);
    void displayMinimalistClock(TFT_eSPI& tft);
    void displayModernSquareClock(TFT_eSPI& tft);
    void displayClockOnDisplay(TFT_eSPI& tft, int csPin);

public:
    /**
     * @brief Constructs the clock manager
     * @param dm Pointer to the display manager instance
     * @param tm Pointer to the time manager instance
     */
    DisplayClockManager(DisplayManager* dm, TimeManager* tm);
    
    /**
     * @brief Initializes the clock manager system
     * @return true if initialization successful, false otherwise
     */
    bool begin();
    
    /**
     * @brief Initialize clock manager with specific display pin configuration
     * @param firstCS Chip select pin for primary display
     * @param secondCS Chip select pin for secondary display
     */
    void begin(int firstCS, int secondCS);
    
    /**
     * @brief Displays clock on both configured displays
     */
    void displayClockOnBothDisplays();
    
    /**
     * @brief Sets custom label for clock display
     * @param label Custom text label to display
     */
    void setClockLabel(const String& label);
    
    /**
     * @brief Gets current clock label
     * @return Current clock label string
     */
    String getClockLabel();
    
    /**
     * @brief Enables or disables second display
     * @param enabled true to enable second display, false to disable
     */
    void setSecondDisplayEnabled(bool enabled);
    
    /**
     * @brief Configures display chip select pins
     * @param firstCS First display CS pin
     * @param secondCS Second display CS pin
     */
    void setDisplayPins(int firstCS, int secondCS);
    
    // Clock face selection and management
    void setClockFace(ClockFaceType faceType);
    ClockFaceType getClockFace() const;
    String getClockFaceName(ClockFaceType faceType) const;
};
