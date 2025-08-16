/**
 * @file timezone_config.h
 * @brief User-configurable timezone definitions for the dual display billboard system
 * 
 * This header contains timezone configuration data that users can easily modify
 * to add or remove timezone options without touching the core TimeManager code.
 * 
 * Each timezone entry consists of:
 * - POSIX TZ format string (technical identifier)
 * - Human-readable display name
 * 
 * POSIX TZ Format Reference:
 * - Standard format: STD[offset[DST[offset],start[/time],end[/time]]]
 * - Example: "EST5EDT,M3.2.0,M11.1.0" = Eastern Time with DST
 * - Offset: Hours behind UTC (positive for west of Greenwich)
 * - DST rules: M3.2.0 = 2nd Sunday in March, M11.1.0 = 1st Sunday in November
 * 
 * Users can add custom timezones by following the existing pattern.
 * 
 * @author Dual Display Billboard Project
 * @version 1.0
 * @date August 2025
 */

#ifndef TIMEZONE_CONFIG_H
#define TIMEZONE_CONFIG_H

#include <Arduino.h>

/**
 * @struct TimezoneOption
 * @brief Structure defining a timezone option for the web interface
 */
struct TimezoneOption {
    const char* posixTZ;        ///< POSIX TZ format string
    const char* displayName;    ///< Human-readable timezone name
};

/**
 * @brief Array of available timezone options
 * 
 * Users can modify this array to add or remove timezone options.
 * Each entry consists of a POSIX TZ string and a display name.
 * 
 * To add a new timezone:
 * 1. Find the correct POSIX TZ format for your location
 * 2. Add a new entry following the existing pattern
 * 3. Recompile and upload the firmware
 * 
 * Common POSIX TZ format examples:
 * - UTC: "UTC0"
 * - US Eastern: "EST5EDT,M3.2.0,M11.1.0"
 * - Central Europe: "CET-1CEST,M3.5.0,M10.5.0/3"
 * - Japan: "JST-9" (no DST)
 * - Australia Eastern: "AEST-10AEDT,M10.1.0,M4.1.0/3"
 */
static const TimezoneOption TIMEZONE_OPTIONS[] = {
    {"UTC0", "UTC (GMT+0)"},
    {"EST5EDT,M3.2.0,M11.1.0", "US Eastern"},
    {"CST6CDT,M3.2.0,M11.1.0", "US Central"},
    {"MST7MDT,M3.2.0,M11.1.0", "US Mountain"},
    {"PST8PDT,M3.2.0,M11.1.0", "US Pacific"},
    {"CET-1CEST,M3.5.0,M10.5.0/3", "Central Europe"},
    {"GMT0BST,M3.5.0/1,M10.5.0", "UK"},
    {"JST-9", "Japan"},
    {"AEST-10AEDT,M10.1.0,M4.1.0/3", "Australia Eastern"},
    
    // ========================================================================
    // USER CUSTOMIZATION AREA
    // ========================================================================
    // Add your custom timezones below this line following the same pattern:
    // {"POSIX_TZ_STRING", "Display Name"},
    //
    // Examples for common additional timezones:
    // {"IST-5:30", "India Standard Time"},
    // {"CST-8", "China Standard Time"},
    // {"MSK-3", "Moscow Time"},
    // {"BRT3BRST,M10.3.0/0,M2.3.0/0", "Brazil Eastern"},
    // {"NZST-12NZDT,M9.5.0,M4.1.0/3", "New Zealand"},
    //
    // Uncomment and modify the examples above or add your own entries
    // ========================================================================
};

/**
 * @brief Number of timezone options in the array
 */
static const size_t TIMEZONE_OPTIONS_COUNT = sizeof(TIMEZONE_OPTIONS) / sizeof(TIMEZONE_OPTIONS[0]);

#endif // TIMEZONE_CONFIG_H
