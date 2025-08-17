/**
 * @file time_manager.cpp
 * @brief Professional time management implementation for ESP32 dual display billboard
 * 
 * Implements comprehensive time synchronization, timezone management, and NTP server
 * configuration with persistent storage. Provides reliable time services for clock
 * display functionality across the dual display system with automatic fallback
 * and error recovery mechanisms.
 * 
 * Key Features:
 * - Multi-server NTP synchronization with configurable fallbacks
 * - POSIX timezone support with DST handling
 * - Persistent configuration storage via LittleFS
 * - Automatic time validation and resync capabilities
 * - Custom clock labeling for location identification
 * - Non-blocking time synchronization with yield support
 * 
 * @author Dual Display Billboard Project
 * @version 0.9
 * @date August 2025
 */

#include "time_manager.h"
#include "timezone_config.h"
#include "logger.h"
#include "config.h"
#include <time.h>
#include <sys/time.h>
#include <esp_sntp.h>

static const String TAG = "TIME";

// Persistent storage file paths for configuration
const char* TimeManager::TIMEZONE_FILE = "/timezone.txt";
const char* TimeManager::CLOCK_LABEL_FILE = "/clock_label.txt";
const char* TimeManager::NTP_SERVER_FILE = "/ntp_server.txt";

/**
 * @brief Constructor initializes time manager with default configuration
 * 
 * Sets up default values for timezone, clock label, and NTP servers.
 * Actual configuration loading from persistent storage occurs in begin().
 */

TimeManager::TimeManager() : timeInitialized(false) {
    // Initialize with default values - persistent settings loaded in begin()
    currentTimezone = DEFAULT_TIMEZONE;
    clockLabel = "Erfurt";                    // Default location label
    customNTPServer1 = NTP_SERVER1;           // Primary NTP server from config
    customNTPServer2 = NTP_SERVER2;           // Secondary NTP server from config  
    customNTPServer3 = NTP_SERVER3;           // Tertiary NTP server from config
}

/**
 * @brief Initializes time manager and establishes NTP synchronization
 * @return true if initialization and time sync successful, false otherwise
 * 
 * Initialization sequence:
 * 1. Check for already initialized state
 * 2. Load persistent configuration from LittleFS
 * 3. Configure NTP servers and timezone
 * 4. Wait for time synchronization
 * 5. Validate and confirm successful initialization
 */

bool TimeManager::begin() {
    LOG_INFO(TAG, "🕐 Initializing Time Manager...");
    
    if (timeInitialized) {
        LOG_INFO(TAG, "✅ Time Manager already initialized");
        return true;
    }
    
    // Load settings from LittleFS (now that it's mounted)
    String savedTimezone = loadTimezone();
    if (!savedTimezone.isEmpty()) {
        currentTimezone = savedTimezone;
        LOG_INFOF(TAG, "📅 Loaded timezone from LittleFS: %s", currentTimezone.c_str());
    } else {
        LOG_INFOF(TAG, "📅 Using default timezone: %s", currentTimezone.c_str());
    }
    
    String savedClockLabel = loadClockLabel();
    if (!savedClockLabel.isEmpty()) {
        clockLabel = savedClockLabel;
        LOG_INFOF(TAG, "🏷️ Loaded clock label from LittleFS: %s", clockLabel.c_str());
    } else {
        LOG_INFOF(TAG, "🏷️ Using default clock label: %s", clockLabel.c_str());
    }
    
    String savedNTPServer = loadNTPServer();
    if (!savedNTPServer.isEmpty()) {
        customNTPServer1 = savedNTPServer;
        LOG_INFOF(TAG, "🌐 Loaded NTP server from LittleFS: %s", customNTPServer1.c_str());
    } else {
        LOG_INFOF(TAG, "🌐 Using default NTP server: %s", customNTPServer1.c_str());
    }
    
    configureNTP();
    
    if (waitForTimeSync()) {
        timeInitialized = true;
        LOG_INFO(TAG, "✅ Time Manager initialized successfully");
        LOG_INFOF(TAG, "📅 Current time: %s", getCurrentTime().c_str());
        return true;
    } else {
        LOG_ERROR(TAG, "❌ Time synchronization failed");
        return false;
    }
}

/**
 * @brief Configures NTP servers and timezone settings
 * 
 * Sets up SNTP (Simple Network Time Protocol) service with configured
 * servers and applies the current timezone. Uses fallback to default
 * servers if custom servers are not configured.
 * 
 * Configuration process:
 * 1. Select custom or default NTP servers
 * 2. Initialize ESP32 configTime with server list
 * 3. Start SNTP service
 * 4. Apply timezone settings with DST rules
 */

void TimeManager::configureNTP() {
    LOG_INFO(TAG, "🌐 Configuring NTP servers...");
    
    // Select custom or default NTP servers based on configuration
    const char* server1 = customNTPServer1.length() > 0 ? customNTPServer1.c_str() : NTP_SERVER1;
    const char* server2 = customNTPServer2.length() > 0 ? customNTPServer2.c_str() : NTP_SERVER2;
    const char* server3 = customNTPServer3.length() > 0 ? customNTPServer3.c_str() : NTP_SERVER3;
    
    LOG_INFOF(TAG, "📡 Using NTP servers: %s, %s, %s", server1, server2, server3);
    
    // Configure ESP32 time system with multiple NTP servers for redundancy
    configTime(0, 0, server1, server2, server3);
    
    // Initialize SNTP (Simple Network Time Protocol) service
    sntp_init();
    
    // Apply timezone with daylight saving time rules
    setenv("TZ", currentTimezone.c_str(), 1);
    tzset();
    
    LOG_INFOF(TAG, "✅ NTP configured with timezone: %s", currentTimezone.c_str());
}

/**
 * @brief Waits for NTP time synchronization with non-blocking approach
 * @param maxRetries Maximum number of sync attempts (default: 10)
 * @return true if time synchronization successful, false if timeout
 * 
 * Uses cooperative multitasking approach with yield() and microsecond delays
 * instead of blocking delay() to maintain system responsiveness during sync.
 */

bool TimeManager::waitForTimeSync(int maxRetries) {
    LOG_INFO(TAG, "⏳ Waiting for time synchronization...");
    
    int retries = 0;
    time_t currentTime;
    while ((currentTime = time(nullptr)) < 100000 && retries < maxRetries) {
        yield(); // Non-blocking yield instead of delay
        delayMicroseconds(1000000); // 1000ms (1 second) for better sync chances
        retries++;
        LOG_INFOF(TAG, "⏳ Time sync attempt %d/%d (current time: %ld)", retries, maxRetries, currentTime);
    }
    
    if (currentTime > 100000) {
        LOG_INFOF(TAG, "✅ Time synchronized successfully! Current epoch: %ld", currentTime);
    } else {
        LOG_ERRORF(TAG, "❌ Time sync failed after %d attempts (current time: %ld)", retries, currentTime);
    }
    
    return currentTime > 100000;
}

/**
 * @brief Validates current time synchronization status
 * @return true if time is synchronized and valid, false otherwise
 * 
 * Checks both initialization flag and time validity to ensure
 * the system has properly synchronized time from NTP servers.
 */
bool TimeManager::isTimeValid() {
    return timeInitialized && (time(nullptr) > 100000);
}

/**
 * @brief Gets current time in HH:MM format
 * @return Formatted time string or "--:--" if time invalid
 * 
 * Returns human-readable time format for display purposes.
 * Automatically handles timezone conversion and DST rules.
 */
String TimeManager::getCurrentTime() {
    time_t now = time(nullptr);
    
    // Validate time is reasonable (not 1970 epoch)
    if (now < 100000) {
        return "--:--";
    }
    
    struct tm timeinfo;
    localtime_r(&now, &timeinfo);  // Apply timezone conversion
    
    char timeString[32];
    strftime(timeString, sizeof(timeString), "%H:%M", &timeinfo);
    return String(timeString);
}

/**
 * @brief Gets current date in YYYY/MM/DD format
 * @return Formatted date string or "----/--/--" if time invalid
 * 
 * Provides date information with timezone-aware conversion
 * for consistent date display across the system.
 */

String TimeManager::getCurrentDate() {
    if (!isTimeValid()) {
        return "----/--/--";
    }
    
    time_t now = time(nullptr);
    struct tm timeinfo;
    localtime_r(&now, &timeinfo);  // Apply timezone conversion
    
    char dateString[32];
    strftime(dateString, sizeof(dateString), "%Y/%m/%d", &timeinfo);
    return String(dateString);
}

/**
 * @brief Gets formatted date/time with custom format string
 * @param format strftime-compatible format string (default: "%Y-%m-%d %H:%M:%S")
 * @return Formatted date/time string or "Invalid time" if sync failed
 * 
 * Provides flexible time formatting using standard strftime format specifiers.
 * Automatically applies timezone conversion and validates time synchronization.
 */
String TimeManager::getFormattedDateTime(const String& format) {
    if (!isTimeValid()) {
        return "Invalid time";
    }
    
    time_t now = time(nullptr);
    struct tm timeinfo;
    localtime_r(&now, &timeinfo);  // Apply timezone conversion
    
    char buffer[128];
    strftime(buffer, sizeof(buffer), format.c_str(), &timeinfo);
    return String(buffer);
}

/**
 * @brief Sets timezone using POSIX TZ format with persistent storage
 * @param timezone POSIX timezone string (e.g., "CET-1CEST,M3.5.0,M10.5.0/3")
 * 
 * Updates the system timezone and saves the setting to LittleFS for persistence.
 * If time is already initialized, immediately applies the new timezone setting.
 * Timezone strings follow POSIX TZ format with daylight saving time rules.
 */
void TimeManager::setTimezone(const String& timezone) {
    currentTimezone = timezone;
    
    // Persist timezone setting to LittleFS
    if (saveTimezone(timezone)) {
        LOG_INFOF(TAG, "💾 Timezone saved to LittleFS: %s", timezone.c_str());
    } else {
        LOG_WARN(TAG, "⚠️ Failed to save timezone to LittleFS");
    }
    
    // Apply timezone immediately if time system is initialized
    if (timeInitialized) {
        setenv("TZ", timezone.c_str(), 1);
        tzset();  // Apply timezone change to system
        LOG_INFOF(TAG, "🌍 Timezone updated to: %s", timezone.c_str());
        LOG_INFOF(TAG, "📅 New time: %s", getCurrentTime().c_str());
    }
}

/**
 * @brief Gets current configured timezone
 * @return Current timezone string in POSIX TZ format
 */
String TimeManager::getCurrentTimezone() {
    return currentTimezone;
}

/**
 * @brief Generates HTML option elements for timezone selection
 * @return HTML options string with current timezone pre-selected
 * 
 * Creates a complete set of HTML option elements for common timezones
 * with proper POSIX TZ format strings and DST rules. The current
 * timezone is automatically marked as selected.
 */

String TimeManager::getTimezoneOptions() {
    String options = "";
    
    // Generate options from timezone configuration file
    for (size_t i = 0; i < TIMEZONE_OPTIONS_COUNT; i++) {
        const TimezoneOption& tz = TIMEZONE_OPTIONS[i];
        String selected = (currentTimezone == tz.posixTZ) ? " selected" : "";
        options += "<option value=\"" + String(tz.posixTZ) + "\"" + selected + ">" + String(tz.displayName) + "</option>";
    }
    
    return options;
}

/**
 * @brief Sets a custom display label for the clock
 * @details Stores the custom label in LittleFS persistence. The label
 *          is used for display purposes and persists across device reboots.
 * @param label The custom label text to assign to the clock
 */
void TimeManager::setClockLabel(const String& label) {
    clockLabel = label;
    
    // Save to LittleFS
    if (saveClockLabel(label)) {
        LOG_INFOF(TAG, "💾 Clock label saved to LittleFS: %s", label.c_str());
    } else {
        LOG_WARN(TAG, "⚠️ Failed to save clock label to LittleFS");
    }
    
    LOG_INFOF(TAG, "🏷️ Clock label set to: %s", label.c_str());
}

/**
 * @brief Gets the current clock display label
 * @return The custom label string currently assigned to the clock
 */
String TimeManager::getClockLabel() {
    return clockLabel;
}

/**
 * @brief Gets current time as struct tm for precise calculations
 * @param timeinfo Pointer to struct tm to fill with current time
 * @return true if time is valid and structure filled, false otherwise
 * 
 * Provides centralized access to time structure for display calculations
 * while maintaining proper timezone conversion and validation.
 */
bool TimeManager::getCurrentTimeStruct(struct tm* timeinfo) {
    if (!isTimeValid() || !timeinfo) {
        return false;
    }
    
    time_t now = time(nullptr);
    localtime_r(&now, timeinfo);  // Apply timezone conversion
    return true;
}

/**
 * @brief Forces immediate time resynchronization with NTP servers
 * 
 * Stops current SNTP service and reinitializes with configured servers.
 * Useful for recovering from time sync failures or applying new NTP settings.
 */

void TimeManager::forceResync() {
    LOG_INFO(TAG, "🔄 Forcing time resync...");
    
    // Stop and restart SNTP service
    sntp_stop();
    configureNTP();
    
    if (waitForTimeSync(5)) {
        LOG_INFO(TAG, "✅ Time resync successful");
        LOG_INFOF(TAG, "📅 Updated time: %s", getCurrentTime().c_str());
    } else {
        LOG_WARN(TAG, "⚠️ Time resync failed");
    }
}

/**
 * @brief Configures custom NTP servers for time synchronization
 * @details Sets up to three custom NTP servers and saves the primary server to
 *          LittleFS for persistence. Automatically applies new configuration if
 *          time system is already initialized.
 * @param server1 Primary NTP server address (required)
 * @param server2 Secondary NTP server address (optional)
 * @param server3 Tertiary NTP server address (optional)
 */
void TimeManager::setNTPServer(const String& server1, const String& server2, const String& server3) {
    LOG_INFOF(TAG, "🌐 Setting custom NTP server: %s", server1.c_str());
    
    customNTPServer1 = server1;
    if (server2.length() > 0) {
        customNTPServer2 = server2;
    }
    if (server3.length() > 0) {
        customNTPServer3 = server3;
    }
    
    // Save to LittleFS
    if (saveNTPServer(server1)) {
        LOG_INFOF(TAG, "💾 NTP server saved to LittleFS: %s", server1.c_str());
    } else {
        LOG_WARN(TAG, "⚠️ Failed to save NTP server to LittleFS");
    }
    
    // Reconfigure NTP with new servers
    if (timeInitialized) {
        sntp_stop();
        configureNTP();
        if (waitForTimeSync(5)) {
            LOG_INFO(TAG, "✅ NTP server updated successfully");
        } else {
            LOG_WARN(TAG, "⚠️ Failed to sync with new NTP server");
        }
    }
}

/**
 * @brief Gets the primary NTP server address
 * @return The currently configured primary NTP server string
 */
String TimeManager::getNTPServer1() {
    return customNTPServer1;
}

/**
 * @brief Resets NTP configuration to default servers
 * @details Restores the default pool.ntp.org servers and applies the
 *          configuration immediately if time system is initialized.
 *          Automatically attempts time synchronization with restored servers.
 */
void TimeManager::resetToDefaultNTP() {
    LOG_INFO(TAG, "🔄 Resetting to default NTP servers");
    customNTPServer1 = NTP_SERVER1;
    customNTPServer2 = NTP_SERVER2;
    customNTPServer3 = NTP_SERVER3;
    
    if (timeInitialized) {
        sntp_stop();
        configureNTP();
        if (waitForTimeSync(5)) {
            LOG_INFO(TAG, "✅ Default NTP servers restored");
        }
    }
}

/**
 * @brief Saves timezone configuration to LittleFS persistence
 * @param timezone POSIX timezone string to save
 * @return true if saved successfully, false on file system error
 */
bool TimeManager::saveTimezone(const String& timezone) {
    File file = LittleFS.open(TIMEZONE_FILE, "w");
    if (!file) {
        LOG_ERROR(TAG, "❌ Failed to open timezone file for writing");
        return false;
    }
    
    file.print(timezone);
    file.close();
    return true;
}

/**
 * @brief Loads timezone configuration from LittleFS persistence
 * @return Saved timezone string, or empty string if not found or error
 */
String TimeManager::loadTimezone() {
    if (!LittleFS.exists(TIMEZONE_FILE)) {
        return "";
    }
    
    File file = LittleFS.open(TIMEZONE_FILE, "r");
    if (!file) {
        LOG_ERROR(TAG, "❌ Failed to open timezone file for reading");
        return "";
    }
    
    String timezone = file.readString();
    file.close();
    timezone.trim();
    return timezone;
}

/**
 * @brief Saves clock label to LittleFS persistence
 * @param label Clock label string to save
 * @return true if saved successfully, false on file system error
 */
bool TimeManager::saveClockLabel(const String& label) {
    File file = LittleFS.open(CLOCK_LABEL_FILE, "w");
    if (!file) {
        LOG_ERROR(TAG, "❌ Failed to open clock label file for writing");
        return false;
    }
    
    file.print(label);
    file.close();
    return true;
}

/**
 * @brief Loads clock label from LittleFS persistence
 * @return Saved clock label string, or empty string if not found or error
 */
String TimeManager::loadClockLabel() {
    if (!LittleFS.exists(CLOCK_LABEL_FILE)) {
        return "";
    }
    
    File file = LittleFS.open(CLOCK_LABEL_FILE, "r");
    if (!file) {
        LOG_ERROR(TAG, "❌ Failed to open clock label file for reading");
        return "";
    }
    
    String label = file.readString();
    file.close();
    label.trim();
    return label;
}

/**
 * @brief Saves NTP server configuration to LittleFS persistence
 * @param server NTP server address string to save
 * @return true if saved successfully, false on file system error
 */
bool TimeManager::saveNTPServer(const String& server) {
    File file = LittleFS.open(NTP_SERVER_FILE, "w");
    if (!file) {
        LOG_ERROR(TAG, "❌ Failed to open NTP server file for writing");
        return false;
    }
    
    file.print(server);
    file.close();
    return true;
}

/**
 * @brief Loads NTP server configuration from LittleFS persistence
 * @return Saved NTP server address string, or empty string if not found or error
 */
String TimeManager::loadNTPServer() {
    // Create default file if it doesn't exist to prevent VFS errors
    if (!LittleFS.exists(NTP_SERVER_FILE)) {
        File file = LittleFS.open(NTP_SERVER_FILE, "w");
        if (file) {
            file.print("pool.ntp.org");  // Default NTP server
            file.close();
        }
    }
    
    File file = LittleFS.open(NTP_SERVER_FILE, "r");
    if (!file) {
        LOG_ERROR(TAG, "❌ Failed to open NTP server file for reading");
        return "";
    }
    
    String server = file.readString();
    file.close();
    server.trim();
    return server;
}
