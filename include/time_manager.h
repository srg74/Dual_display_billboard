#pragma once

#include <Arduino.h>

class TimeManager {
private:
    String currentTimezone;
    bool timeInitialized;
    String clockLabel;
    
    // Internal helper methods
    void configureNTP();
    bool waitForTimeSync(int maxRetries = 10);
    
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
    
    // Time sync status
    unsigned long getLastSyncTime();
    bool needsResync();
    void forceResync();
};
