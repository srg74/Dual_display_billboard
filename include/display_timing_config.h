/**
 * @file display_timing_config.h
 * @brief Centralized timing configuration for display system
 * 
 * This header consolidates all timing constants used across the display
 * system to eliminate scattered magic numbers and provide single source
 * of truth for timing parameters.
 * 
 * Part of Display Manager refactoring to create standalone module.
 * 
 * @author Dual Display Billboard Team
 * @date 2024
 */

#ifndef DISPLAY_TIMING_CONFIG_H
#define DISPLAY_TIMING_CONFIG_H

// =============================================================================
// SPLASH SCREEN TIMING
// =============================================================================
/**
 * Duration to show connection success splash screen (milliseconds)
 * Used in main.cpp WiFi initialization sequence
 */
#define DISPLAY_SPLASH_DURATION_MS          4000

/**
 * Duration to show "switching to normal mode" message (milliseconds)
 * Used when transitioning from splash to normal operation
 */
#define DISPLAY_MODE_SWITCH_DURATION_MS     5000

// =============================================================================
// DISPLAY ALTERNATING TIMING
// =============================================================================
/**
 * Interval for alternating between displays in dual display mode (milliseconds)
 * Used by display manager for switching active display
 */
#define DISPLAY_ALTERNATING_INTERVAL_MS     3000

// =============================================================================
// SLIDESHOW DEFAULT TIMING
// =============================================================================
/**
 * Default interval between slideshow images (seconds)
 * Used as fallback when no user configuration exists
 */
#define SLIDESHOW_DEFAULT_INTERVAL_SEC      10

/**
 * Default interval between slideshow images (milliseconds)
 * Converted version for millis() comparisons
 */
#define SLIDESHOW_DEFAULT_INTERVAL_MS       (SLIDESHOW_DEFAULT_INTERVAL_SEC * 1000)

// =============================================================================
// SYSTEM HEARTBEAT TIMING
// =============================================================================
/**
 * Interval for system status logging (milliseconds)
 * Used for periodic system health reporting
 */
#define SYSTEM_HEARTBEAT_INTERVAL_MS        10000

// =============================================================================
// DISPLAY UPDATE TIMING
// =============================================================================
/**
 * Minimum interval between display updates (milliseconds)
 * Prevents excessive display refreshing
 */
#define DISPLAY_MIN_UPDATE_INTERVAL_MS      100

/**
 * Backlight PWM update interval (milliseconds)
 * For smooth brightness transitions
 */
#define BACKLIGHT_UPDATE_INTERVAL_MS        50

// =============================================================================
// TIMING VALIDATION HELPERS
// =============================================================================
/**
 * Validates that timing constants are reasonable
 * Returns true if all timing values are within expected ranges
 */
inline bool validateDisplayTimingConfig() {
    // Basic sanity checks
    if (DISPLAY_SPLASH_DURATION_MS < 1000 || DISPLAY_SPLASH_DURATION_MS > 10000) return false;
    if (DISPLAY_ALTERNATING_INTERVAL_MS < 1000 || DISPLAY_ALTERNATING_INTERVAL_MS > 30000) return false;
    if (SLIDESHOW_DEFAULT_INTERVAL_SEC < 1 || SLIDESHOW_DEFAULT_INTERVAL_SEC > 300) return false;
    
    return true;
}

#endif // DISPLAY_TIMING_CONFIG_H
