/**
 * @file version.h
 * @brief Version Information and Build Details for ESP32 Dual Display Billboard
 * @version 0.9
 * @author Dual Display Billboard Team  
 * @date 2025-08-16
 * 
 * This header provides standardized access to version information automatically
 * generated during the build process. Version information includes:
 * 
 * • Base version from platformio.ini (e.g., v0.9)
 * • Build numbers in YYMMDDx format (e.g., 2508160)
 * • Full version strings (e.g., v0.9-build.2508160)
 * • Build timestamps and environment details
 * 
 * Usage:
 * ```cpp
 * #include "version.h"
 * LOG_INFO("System", "Firmware: " + String(getFirmwareVersion()));
 * LOG_INFO("System", "Build: " + String(getBuildNumber()));
 * ```
 */

#ifndef VERSION_H
#define VERSION_H

#include <Arduino.h>

/**
 * @brief  Version Information Namespace
 * 
 * Provides centralized access to all firmware version and build information.
 * All version data is automatically generated during build time.
 */
namespace Version {
    
    /**
     * @brief Get firmware version string
     * @return Base version string (e.g., "v0.9")
     * @note Generated from current_version in platformio.ini
     */
    inline const String getFirmwareVersion() {
        #ifdef FIRMWARE_VERSION
            return String(FIRMWARE_VERSION);
        #else
            return "v0.9";  // Fallback if not defined
        #endif
    }
    
    /**
     * @brief Get full firmware version with build number
     * @return Full version string (e.g., "v0.9-build.2508160")
     * @note Includes base version + build number
     */
    inline const String getFirmwareVersionFull() {
        #ifdef FIRMWARE_VERSION_FULL
            return String(FIRMWARE_VERSION_FULL);
        #else
            return getFirmwareVersion() + "-build.unknown";
        #endif
    }
    
    /**
     * @brief Get build number in YYMMDDx format
     * @return Build number string (e.g., "2508160")
     * @note x = daily build counter (0-9)
     */
    inline const String getBuildNumber() {
        #ifdef BUILD_NUMBER
            return String(BUILD_NUMBER);
        #else
            return "unknown";
        #endif
    }
    
    /**
     * @brief Get build date in YYMMDD format
     * @return Build date string (e.g., "250816")
     */
    inline const String getBuildDate() {
        #ifdef BUILD_DATE
            return String(BUILD_DATE);
        #else
            return "000000";
        #endif
    }
    
    /**
     * @brief Get build timestamp
     * @return Build timestamp string (e.g., "2025-08-16 14:30:25")
     */
    inline const String getBuildTimestamp() {
        #ifdef BUILD_TIMESTAMP
            return String(BUILD_TIMESTAMP);
        #else
            return "Unknown";
        #endif
    }
    
    /**
     * @brief Get build environment name
     * @return Environment string (e.g., "esp32dev-st7735-debug")
     */
    inline const String getBuildEnvironment() {
        #ifdef BUILD_ENVIRONMENT
            return String(BUILD_ENVIRONMENT);
        #else
            return "unknown";
        #endif
    }
    
    /**
     * @brief Get daily build count
     * @return Daily build counter (0-9)
     */
    inline int getDailyBuildCount() {
        #ifdef DAILY_BUILD_COUNT
            return DAILY_BUILD_COUNT;
        #else
            return 0;
        #endif
    }
    
    /**
     * @brief Get comprehensive version information as JSON string
     * @return JSON formatted version information
     * 
     * Example output:
     * ```json
     * {
     *   "version": "v0.9",
     *   "fullVersion": "v0.9-build.2508160",
     *   "buildNumber": "2508160",
     *   "buildDate": "250816",
     *   "buildTimestamp": "2025-08-16 14:30:25",
     *   "environment": "esp32dev-st7735-debug",
     *   "dailyBuild": 0
     * }
     * ```
     */
    inline const String getVersionInfoJson() {
        String json = "{";
        json += "\"version\":\"" + getFirmwareVersion() + "\",";
        json += "\"fullVersion\":\"" + getFirmwareVersionFull() + "\",";
        json += "\"buildNumber\":\"" + getBuildNumber() + "\",";
        json += "\"buildDate\":\"" + getBuildDate() + "\",";
        json += "\"buildTimestamp\":\"" + getBuildTimestamp() + "\",";
        json += "\"environment\":\"" + getBuildEnvironment() + "\",";
        json += "\"dailyBuild\":" + String(getDailyBuildCount());
        json += "}";
        return json;
    }
    
    /**
     * @brief Print version information to Serial
     * @param detailed Include detailed build information
     */
    inline void printVersionInfo(bool detailed = false) {
        Serial.println("===  Firmware Version Information ===");
        Serial.println("Version: " + getFirmwareVersion());
        Serial.println("Full Version: " + getFirmwareVersionFull());
        Serial.println("Build Number: " + getBuildNumber());
        
        if (detailed) {
            Serial.println("Build Date: " + getBuildDate());
            Serial.println("Build Timestamp: " + getBuildTimestamp());
            Serial.println("Environment: " + getBuildEnvironment());
            Serial.println("Daily Build: #" + String(getDailyBuildCount()));
        }
        
        Serial.println("=========================================");
    }
}

/**
 * @brief Legacy compatibility macros
 * @deprecated Use Version namespace functions instead
 */
#define VERSION_STRING Version::getFirmwareVersion()
#define BUILD_VERSION Version::getFirmwareVersionFull()

#endif // VERSION_H
