#include "time_manager.h"
#include "logger.h"
#include "config.h"
#include <time.h>
#include <sys/time.h>
#include <esp_sntp.h>

static const String TAG = "TIME";

// Settings file paths
const char* TimeManager::TIMEZONE_FILE = "/timezone.txt";
const char* TimeManager::CLOCK_LABEL_FILE = "/clock_label.txt";
const char* TimeManager::NTP_SERVER_FILE = "/ntp_server.txt";

TimeManager::TimeManager() : timeInitialized(false) {
    // Set defaults - will load from LittleFS in begin() method
    currentTimezone = DEFAULT_TIMEZONE;
    clockLabel = "Erfurt";
    customNTPServer1 = NTP_SERVER1;
    customNTPServer2 = NTP_SERVER2;
    customNTPServer3 = NTP_SERVER3;
}

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

void TimeManager::configureNTP() {
    LOG_INFO(TAG, "🌐 Configuring NTP servers...");
    
    // Configure NTP with custom or default servers
    const char* server1 = customNTPServer1.length() > 0 ? customNTPServer1.c_str() : NTP_SERVER1;
    const char* server2 = customNTPServer2.length() > 0 ? customNTPServer2.c_str() : NTP_SERVER2;
    const char* server3 = customNTPServer3.length() > 0 ? customNTPServer3.c_str() : NTP_SERVER3;
    
    LOG_INFOF(TAG, "📡 Using NTP servers: %s, %s, %s", server1, server2, server3);
    configTime(0, 0, server1, server2, server3);
    
    // Initialize SNTP service
    sntp_init();
    
    // Set the timezone
    setenv("TZ", currentTimezone.c_str(), 1);
    tzset();
    
    LOG_INFOF(TAG, "✅ NTP configured with timezone: %s", currentTimezone.c_str());
}

bool TimeManager::waitForTimeSync(int maxRetries) {
    LOG_INFO(TAG, "⏳ Waiting for time synchronization...");
    
    int retries = 0;
    while (time(nullptr) < 100000 && retries < maxRetries) {
        delay(500);
        retries++;
        LOG_DEBUGF(TAG, "⏳ Time sync attempt %d/%d", retries, maxRetries);
    }
    
    return time(nullptr) > 100000;
}

bool TimeManager::isTimeValid() {
    return timeInitialized && (time(nullptr) > 100000);
}

String TimeManager::getCurrentTime() {
    time_t now = time(nullptr);
    
    // Check if we have any reasonable time (not just 1970 epoch)
    if (now < 100000) {
        return "--:--";
    }
    
    struct tm timeinfo;
    localtime_r(&now, &timeinfo);
    
    char timeString[32];
    strftime(timeString, sizeof(timeString), "%H:%M", &timeinfo);
    return String(timeString);
}

String TimeManager::getCurrentDate() {
    if (!isTimeValid()) {
        return "----/--/--";
    }
    
    time_t now = time(nullptr);
    struct tm timeinfo;
    localtime_r(&now, &timeinfo);
    
    char dateString[32];
    strftime(dateString, sizeof(dateString), "%Y/%m/%d", &timeinfo);
    return String(dateString);
}

String TimeManager::getFormattedDateTime(const String& format) {
    if (!isTimeValid()) {
        return "Invalid time";
    }
    
    time_t now = time(nullptr);
    struct tm timeinfo;
    localtime_r(&now, &timeinfo);
    
    char buffer[128];
    strftime(buffer, sizeof(buffer), format.c_str(), &timeinfo);
    return String(buffer);
}

void TimeManager::setTimezone(const String& timezone) {
    currentTimezone = timezone;
    
    // Save to LittleFS
    if (saveTimezone(timezone)) {
        LOG_INFOF(TAG, "💾 Timezone saved to LittleFS: %s", timezone.c_str());
    } else {
        LOG_WARN(TAG, "⚠️ Failed to save timezone to LittleFS");
    }
    
    if (timeInitialized) {
        setenv("TZ", timezone.c_str(), 1);
        tzset();
        LOG_INFOF(TAG, "🌍 Timezone updated to: %s", timezone.c_str());
        LOG_INFOF(TAG, "📅 New time: %s", getCurrentTime().c_str());
    }
}

String TimeManager::getCurrentTimezone() {
    return currentTimezone;
}

String TimeManager::getTimezoneOptions() {
    String options = "";
    
    // Common timezones with proper TZ format
    String selected = (currentTimezone == "UTC0") ? " selected" : "";
    options += "<option value=\"UTC0\"" + selected + ">UTC (GMT+0)</option>";
    
    selected = (currentTimezone == "EST5EDT,M3.2.0,M11.1.0") ? " selected" : "";
    options += "<option value=\"EST5EDT,M3.2.0,M11.1.0\"" + selected + ">US Eastern</option>";
    
    selected = (currentTimezone == "CST6CDT,M3.2.0,M11.1.0") ? " selected" : "";
    options += "<option value=\"CST6CDT,M3.2.0,M11.1.0\"" + selected + ">US Central</option>";
    
    selected = (currentTimezone == "MST7MDT,M3.2.0,M11.1.0") ? " selected" : "";
    options += "<option value=\"MST7MDT,M3.2.0,M11.1.0\"" + selected + ">US Mountain</option>";
    
    selected = (currentTimezone == "PST8PDT,M3.2.0,M11.1.0") ? " selected" : "";
    options += "<option value=\"PST8PDT,M3.2.0,M11.1.0\"" + selected + ">US Pacific</option>";
    
    selected = (currentTimezone == "CET-1CEST,M3.5.0,M10.5.0/3") ? " selected" : "";
    options += "<option value=\"CET-1CEST,M3.5.0,M10.5.0/3\"" + selected + ">Central Europe</option>";
    
    selected = (currentTimezone == "GMT0BST,M3.5.0/1,M10.5.0") ? " selected" : "";
    options += "<option value=\"GMT0BST,M3.5.0/1,M10.5.0\"" + selected + ">UK</option>";
    
    selected = (currentTimezone == "JST-9") ? " selected" : "";
    options += "<option value=\"JST-9\"" + selected + ">Japan</option>";
    
    selected = (currentTimezone == "AEST-10AEDT,M10.1.0,M4.1.0/3") ? " selected" : "";
    options += "<option value=\"AEST-10AEDT,M10.1.0,M4.1.0/3\"" + selected + ">Australia Eastern</option>";
    
    return options;
}

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

String TimeManager::getClockLabel() {
    return clockLabel;
}

unsigned long TimeManager::getLastSyncTime() {
    // ESP32 doesn't provide direct access to last sync time
    // Return current time in milliseconds since epoch
    if (!isTimeValid()) return 0;
    
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec * 1000UL + tv.tv_usec / 1000UL;
}

bool TimeManager::needsResync() {
    if (!timeInitialized) return true;
    
    unsigned long currentTime = millis();
    unsigned long lastSync = getLastSyncTime();
    
    // Check if more than TIME_SYNC_INTERVAL has passed since last sync
    return (currentTime - lastSync) > TIME_SYNC_INTERVAL;
}

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

String TimeManager::getNTPServer1() {
    return customNTPServer1;
}

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

// Persistence methods implementation
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

String TimeManager::loadNTPServer() {
    if (!LittleFS.exists(NTP_SERVER_FILE)) {
        return "";
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
