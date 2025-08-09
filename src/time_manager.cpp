#include "time_manager.h"
#include "logger.h"
#include "config.h"
#include <time.h>
#include <sys/time.h>
#include <esp_sntp.h>

static const String TAG = "TIME";

TimeManager::TimeManager() : currentTimezone(DEFAULT_TIMEZONE), timeInitialized(false), clockLabel("Clock") {
}

bool TimeManager::begin() {
    LOG_INFO(TAG, "🕐 Initializing Time Manager...");
    
    if (timeInitialized) {
        LOG_INFO(TAG, "✅ Time Manager already initialized");
        return true;
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
    
    // Configure NTP with multiple servers for redundancy
    configTime(0, 0, NTP_SERVER1, NTP_SERVER2, NTP_SERVER3);
    
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
    if (!isTimeValid()) {
        return "--:--";
    }
    
    time_t now = time(nullptr);
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
    options += "<option value=\"UTC0\">UTC (GMT+0)</option>";
    options += "<option value=\"EST5EDT,M3.2.0,M11.1.0\">US Eastern</option>";
    options += "<option value=\"CST6CDT,M3.2.0,M11.1.0\">US Central</option>";
    options += "<option value=\"MST7MDT,M3.2.0,M11.1.0\">US Mountain</option>";
    options += "<option value=\"PST8PDT,M3.2.0,M11.1.0\">US Pacific</option>";
    options += "<option value=\"CET-1CEST,M3.5.0,M10.5.0/3\">Central Europe</option>";
    options += "<option value=\"GMT0BST,M3.5.0/1,M10.5.0\">UK</option>";
    options += "<option value=\"JST-9\">Japan</option>";
    options += "<option value=\"AEST-10AEDT,M10.1.0,M4.1.0/3\">Australia Eastern</option>";
    
    return options;
}

void TimeManager::setClockLabel(const String& label) {
    clockLabel = label;
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
