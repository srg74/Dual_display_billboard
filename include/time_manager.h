/**
 * @file time_manager.h
 * @brief Professional time management system for ESP32 dual display billboard
 * 
 * Provides comprehensive time synchronization, timezone management, and NTP server
 * configuration with persistent storage capabilities. Handles time display formatting,
 * automatic synchronization, and user-configurable clock labels for the dual display system.
 * 
 * Features:
 * - NTP-based time synchronization with configurable servers
 * - Timezone management with persistent storage
 * - Custom clock label support
 * - Automatic time validation and resync capabilities
 * - LittleFS-based configuration persistence
 * 
 * @author Dual Display Billboard Project
 * @version 2.0
 * @date August 2025
 */

#pragma once

#include <Arduino.h>
#include <LittleFS.h>
#include "timezone_config.h"

/**
 * @class TimeManager
 * @brief Advanced time management system with NTP synchronization and timezone support
 * 
 * Manages time synchronization, timezone configuration, and time display formatting
 * for the ESP32-based dual display billboard system. Provides persistent storage
 * for user preferences and automatic time synchronization with configurable NTP servers.
 */

class TimeManager {
private:
    // Time configuration state
    String currentTimezone;          ///< Current timezone in POSIX TZ format
    bool timeInitialized;           ///< Flag indicating successful time synchronization
    String clockLabel;              ///< Custom label for clock display
    
    // NTP server configuration
    String customNTPServer1;        ///< Primary NTP server
    String customNTPServer2;        ///< Secondary NTP server  
    String customNTPServer3;        ///< Tertiary NTP server
    
    // Persistent storage file paths
    static const char* TIMEZONE_FILE;     ///< LittleFS path for timezone storage
    static const char* CLOCK_LABEL_FILE;  ///< LittleFS path for clock label storage
    static const char* NTP_SERVER_FILE;   ///< LittleFS path for NTP server storage
    
    // Internal time synchronization methods
    void configureNTP();
    bool waitForTimeSync(int maxRetries = 20);
    
    // Persistent storage methods
    bool saveTimezone(const String& timezone);
    String loadTimezone();
    bool saveClockLabel(const String& label);
    String loadClockLabel();
    bool saveNTPServer(const String& server);
    String loadNTPServer();
    
public:
    /**
     * @brief Constructs time manager with default configuration
     */
    TimeManager();
    
    /**
     * @brief Initializes time manager and establishes NTP synchronization
     * @return true if initialization and time sync successful, false otherwise
     */
    bool begin();
    
    /**
     * @brief Validates current time synchronization status
     * @return true if time is synchronized and valid, false otherwise
     */
    bool isTimeValid();
    
    /**
     * @brief Gets current time in HH:MM format
     * @return Formatted time string or "--:--" if time invalid
     */
    String getCurrentTime();
    
    /**
     * @brief Gets current date in YYYY/MM/DD format
     * @return Formatted date string or "----/--/--" if time invalid
     */
    String getCurrentDate();
    
    /**
     * @brief Gets formatted date/time with custom format string
     * @param format strftime-compatible format string (default: "%Y-%m-%d %H:%M:%S")
     * @return Formatted date/time string or "Invalid time" if sync failed
     */
    String getFormattedDateTime(const String& format = "%Y-%m-%d %H:%M:%S");
    
    /**
     * @brief Sets timezone using POSIX TZ format with persistent storage
     * @param timezone POSIX timezone string (e.g., "CET-1CEST,M3.5.0,M10.5.0/3")
     */
    void setTimezone(const String& timezone);
    
    /**
     * @brief Gets current configured timezone
     * @return Current timezone string in POSIX TZ format
     */
    String getCurrentTimezone();
    
    /**
     * @brief Generates HTML option elements for timezone selection
     * @return HTML options string with current timezone pre-selected
     * 
     * Creates HTML option elements from timezone definitions in timezone_config.h.
     * Users can customize available timezones by modifying that configuration file.
     */
    String getTimezoneOptions();
    
    /**
     * @brief Sets custom clock label with persistent storage
     * @param label Custom text label to display with clock
     */
    void setClockLabel(const String& label);
    
    /**
     * @brief Gets current clock label
     * @return Current clock label string
     */
    String getClockLabel();
    
    /**
     * @brief Gets current time as struct tm for precise calculations
     * @param timeinfo Pointer to struct tm to fill with current time
     * @return true if time is valid and structure filled, false otherwise
     * 
     * Provides centralized access to time structure for display calculations
     * while maintaining proper timezone conversion and validation.
     */
    bool getCurrentTimeStruct(struct tm* timeinfo);
    
    /**
     * @brief Configures custom NTP servers with persistent storage
     * @param server1 Primary NTP server hostname or IP
     * @param server2 Optional secondary NTP server
     * @param server3 Optional tertiary NTP server
     */
    void setNTPServer(const String& server1, const String& server2 = "", const String& server3 = "");
    
    /**
     * @brief Gets primary NTP server configuration
     * @return Primary NTP server hostname or IP
     */
    String getNTPServer1();
    
    /**
     * @brief Resets NTP configuration to default servers
     */
    void resetToDefaultNTP();
    
    /**
     * @brief Forces immediate time resynchronization with NTP servers
     */
    void forceResync();
};
