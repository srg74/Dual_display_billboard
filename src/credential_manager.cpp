#include "credential_manager.h"

const char* CredentialManager::CREDENTIALS_FILE = "/wifi_creds.json";
const String CredentialManager::TAG = "CRED";

bool CredentialManager::begin() {
    LOG_INFO(TAG, "🗂️ Initializing LittleFS...");
    
    if (!LittleFS.begin(true)) { // true = format if mount fails
        LOG_ERROR(TAG, "❌ Failed to mount LittleFS");
        return false;
    }
    
    LOG_INFO(TAG, "✅ LittleFS mounted successfully");
    printFileSystemInfo();
    return true;
}

bool CredentialManager::saveCredentials(const String& ssid, const String& password) {
    LOG_INFOF(TAG, "💾 Saving WiFi credentials for: %s", ssid.c_str());
    
    File file = LittleFS.open(CREDENTIALS_FILE, "w");
    if (!file) {
        LOG_ERROR(TAG, "❌ Failed to open credentials file for writing");
        return false;
    }
    
    // Simple JSON format - escape quotes if needed
    String escapedSSID = ssid;
    String escapedPassword = password;
    escapedSSID.replace("\"", "\\\"");
    escapedPassword.replace("\"", "\\\"");
    
    String json = "{\"ssid\":\"" + escapedSSID + "\",\"password\":\"" + escapedPassword + "\"}";
    size_t bytesWritten = file.print(json);
    file.close();
    
    if (bytesWritten > 0) {
        LOG_INFOF(TAG, "✅ Credentials saved successfully (%d bytes)", bytesWritten);
        return true;
    } else {
        LOG_ERROR(TAG, "❌ Failed to write credentials");
        return false;
    }
}

CredentialManager::WiFiCredentials CredentialManager::loadCredentials() {
    WiFiCredentials creds;
    
    if (!LittleFS.exists(CREDENTIALS_FILE)) {
        LOG_INFO(TAG, "📄 No credentials file found");
        return creds;
    }
    
    File file = LittleFS.open(CREDENTIALS_FILE, "r");
    if (!file) {
        LOG_ERROR(TAG, "❌ Failed to open credentials file for reading");
        return creds;
    }
    
    String content = file.readString();
    file.close();
    
    LOG_DEBUGF(TAG, "📄 Credentials file content: %s", content.c_str());
    
    // Simple JSON parsing (basic implementation)
    int ssidStart = content.indexOf("\"ssid\":\"") + 8;
    int ssidEnd = content.indexOf("\",\"password\"");
    int passStart = content.indexOf("\"password\":\"") + 12;
    int passEnd = content.indexOf("\"}");
    
    if (ssidStart > 7 && ssidEnd > ssidStart && passStart > 11 && passEnd > passStart) {
        creds.ssid = content.substring(ssidStart, ssidEnd);
        creds.password = content.substring(passStart, passEnd);
        creds.isValid = true;
        LOG_INFOF(TAG, "✅ Loaded credentials for SSID: %s", creds.ssid.c_str());
    } else {
        LOG_ERROR(TAG, "❌ Invalid credentials file format");
    }
    
    return creds;
}

bool CredentialManager::clearCredentials() {
    LOG_INFO(TAG, "🗑️ Clearing stored credentials");
    
    if (LittleFS.exists(CREDENTIALS_FILE)) {
        bool success = LittleFS.remove(CREDENTIALS_FILE);
        if (success) {
            LOG_INFO(TAG, "✅ Credentials cleared successfully");
        } else {
            LOG_ERROR(TAG, "❌ Failed to clear credentials");
        }
        return success;
    }
    
    LOG_INFO(TAG, "ℹ️ No credentials to clear");
    return true;
}

bool CredentialManager::hasCredentials() {
    bool exists = LittleFS.exists(CREDENTIALS_FILE);
    LOG_DEBUGF(TAG, "📄 Credentials file exists: %s", exists ? "YES" : "NO");
    return exists;
}

void CredentialManager::printFileSystemInfo() {
    size_t totalBytes = LittleFS.totalBytes();
    size_t usedBytes = LittleFS.usedBytes();
    
    LOG_INFOF(TAG, "📊 LittleFS - Total: %d bytes, Used: %d bytes, Free: %d bytes", 
              totalBytes, usedBytes, totalBytes - usedBytes);
}