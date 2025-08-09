#pragma once

#include <Arduino.h>
#include <LittleFS.h>

class TimeManager {
private:
    String currentTimezone;
    bool timeInitialized;
    String clockLabel;
    String customNTPServer1;
    String customNTPServer2;
    String customNTPServer3;
    
    // Settings file paths
    static const char* TIMEZONE_FILE;
    static const char* CLOCK_LABEL_FILE;
    static const char* NTP_SERVER_FILE;
    
    // Internal helper methods
    void configureNTP();
    bool waitForTimeSync(int maxRetries = 10);
    
    // Persistence methods
    bool saveTimezone(const String& timezone);
    String loadTimezone();
    bool saveClockLabel(const String& label);
    String loadClockLabel();
    bool saveNTPServer(const String& server);
    String loadNTPServer();
    
public:
    TimeManager();
    
    // Core functionality
    bool begin();
    bool isTimeValid();
    
    // Time management
    String getCurrentTime();
    String getCurrentDate();
    String getFormattedDateTime(const String& format = "%Y-%m-%d %H:%M:%S");
    
    // Timezone management
    void setTimezone(const String& timezone);
    String getCurrentTimezone();
    String getTimezoneOptions();
    
    // Clock label management
    void setClockLabel(const String& label);
    String getClockLabel();
    
    // NTP server management
    void setNTPServer(const String& server1, const String& server2 = "", const String& server3 = "");
    String getNTPServer1();
    void resetToDefaultNTP();
    
    // Time sync status
    unsigned long getLastSyncTime();
    bool needsResync();
    void forceResync();
};
