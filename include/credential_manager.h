#pragma once
#include <Arduino.h>
#include <LittleFS.h>
#include "logger.h"

class CredentialManager {
private:
    static const char* CREDENTIALS_FILE;
    static const String TAG;

public:
    struct WiFiCredentials {
        String ssid;
        String password;
        bool isValid;
        
        WiFiCredentials() : ssid(""), password(""), isValid(false) {}
        WiFiCredentials(const String& s, const String& p) : ssid(s), password(p), isValid(true) {}
    };

    static bool begin();
    static bool saveCredentials(const String& ssid, const String& password);
    static WiFiCredentials loadCredentials();
    static bool clearCredentials();
    static bool hasCredentials();
    static void printFileSystemInfo();
};