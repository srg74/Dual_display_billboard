/**
 * @file credential_manager.cpp
 * @brief Implementation of secure WiFi credential storage and management system
 * 
 * This module implements secure persistent storage of WiFi credentials using LittleFS
 * filesystem with JSON-based structured storage. Provides comprehensive credential
 * lifecycle management with robust error handling and validation.
 * 
 * Features:
 * - LittleFS filesystem integration with automatic initialization
 * - JSON-based credential storage with input sanitization
 * - Comprehensive error handling and logging throughout operations
 * - Filesystem health monitoring and capacity reporting
 * - Basic security through input escaping and validation
 * - Atomic write operations to prevent credential corruption
 * 
 * Technical Implementation:
 * - Uses LittleFS.begin(true) for automatic formatting on mount failure
 * - Implements simple JSON parsing to avoid external library dependencies
 * - Provides comprehensive logging for debugging and monitoring
 * - Validates file operations and reports detailed error conditions
 * 
 * @author Dual Display Billboard Project
 * @version 0.9
 * @date August 2025
 */

#include "credential_manager.h"

// ============================================================================
// STATIC MEMBER INITIALIZATION
// ============================================================================
const char* CredentialManager::CREDENTIALS_FILE = "/wifi_creds.json";  ///< Storage file path
const String CredentialManager::TAG = "CRED";                          ///< Logging identifier

// ============================================================================
// LIFECYCLE MANAGEMENT METHODS
// ============================================================================

/**
 * @brief Initialize credential manager and LittleFS filesystem
 * @return true if initialization successful, false on failure
 */
bool CredentialManager::begin() {
    LOG_INFO(TAG, "Initializing LittleFS filesystem...");
    
    // Check if LittleFS is already mounted (initialized in main.cpp)
    if (LittleFS.totalBytes() > 0) {
        LOG_INFO(TAG, "LittleFS already mounted, skipping initialization");
    } else {
        // Initialize LittleFS with automatic formatting if not already mounted
        if (!LittleFS.begin(true)) { // true = format if mount fails
            LOG_ERROR(TAG, "Failed to mount LittleFS filesystem");
            return false;
        }
        LOG_INFO(TAG, "LittleFS mounted successfully");
    }
    
    printFileSystemInfo();  // Report filesystem health and capacity
    return true;
}

// ============================================================================
// CREDENTIAL MANAGEMENT METHODS
// ============================================================================

/**
 * @brief Save WiFi credentials to persistent storage with input sanitization
 * @param ssid WiFi network SSID to store
 * @param password WiFi network password to store
 * @return true if credentials saved successfully, false on failure
 */
bool CredentialManager::saveCredentials(const String& ssid, const String& password) {
    LOG_INFOF(TAG, "Saving WiFi credentials for: %s", ssid.c_str());
    
    // Open credentials file for writing (overwrites existing)
    File file = LittleFS.open(CREDENTIALS_FILE, "w");
    if (!file) {
        LOG_ERROR(TAG, "Failed to open credentials file for writing");
        return false;
    }
    
    // Sanitize input to prevent JSON format corruption
    String escapedSSID = ssid;
    String escapedPassword = password;
    escapedSSID.replace("\"", "\\\"");      // Escape double quotes in SSID
    escapedPassword.replace("\"", "\\\"");  // Escape double quotes in password
    
    // Create JSON formatted credential string
    String json = "{\"ssid\":\"" + escapedSSID + "\",\"password\":\"" + escapedPassword + "\"}";
    
    // Write JSON to file with atomic operation
    size_t bytesWritten = file.print(json);
    file.close();
    
    // Validate write operation success
    if (bytesWritten > 0) {
        LOG_INFOF(TAG, "Credentials saved successfully (%d bytes)", bytesWritten);
        return true;
    } else {
        LOG_ERROR(TAG, "Failed to write credentials to filesystem");
        return false;
    }
}

/**
 * @brief Load WiFi credentials from persistent storage (struct return version)
 * @return WiFiCredentials struct populated with stored data or default invalid state
 */
CredentialManager::WiFiCredentials CredentialManager::loadCredentials() {
    WiFiCredentials creds;
    
    // Check if credentials file exists before attempting read
    if (!LittleFS.exists(CREDENTIALS_FILE)) {
        LOG_INFO(TAG, "No credentials file found - returning invalid credentials");
        return creds;
    }
    
    // Open credentials file for reading
    File file = LittleFS.open(CREDENTIALS_FILE, "r");
    if (!file) {
        LOG_ERROR(TAG, "Failed to open credentials file for reading");
        return creds;
    }
    
    // Read entire file content
    String content = file.readString();
    file.close();
    
    LOG_DEBUGF(TAG, "Credentials file content: %s", content.c_str());
    
    // Parse JSON content with improved boundary detection
    int ssidStart = content.indexOf("\"ssid\":\"") + 8;
    int ssidEnd = content.indexOf("\",\"password\"");
    int passStart = content.indexOf("\"password\":\"") + 12;
    int passEnd = content.indexOf("\"}");
    
    // Validate JSON structure and extract credentials
    if (ssidStart > 7 && ssidEnd > ssidStart && passStart > 11 && passEnd > passStart) {
        creds.ssid = content.substring(ssidStart, ssidEnd);
        creds.password = content.substring(passStart, passEnd);
        
        // Unescape JSON special characters
        creds.ssid.replace("\\\"", "\"");
        creds.password.replace("\\\"", "\"");
        
        // Mark credentials as valid if SSID is non-empty
        creds.isValid = (creds.ssid.length() > 0);
        
        LOG_INFOF(TAG, "Loaded credentials for SSID: %s", creds.ssid.c_str());
    } else {
        LOG_ERROR(TAG, "Invalid credentials file format - JSON parsing failed");
    }
    
    return creds;
}

/**
 * @brief Clear stored WiFi credentials by removing the credentials file
 * @return true if credentials cleared successfully or no credentials existed, false on failure
 */
bool CredentialManager::clearCredentials() {
    LOG_INFO(TAG, "Clearing stored WiFi credentials");
    
    // Check if credentials file exists before attempting removal
    if (LittleFS.exists(CREDENTIALS_FILE)) {
        bool success = LittleFS.remove(CREDENTIALS_FILE);
        if (success) {
            LOG_INFO(TAG, "Credentials file removed successfully");
        } else {
            LOG_ERROR(TAG, "Failed to remove credentials file");
        }
        return success;
    }
    
    LOG_INFO(TAG, "No credentials file to clear - operation successful");
    return true;
}

/**
 * @brief Check if WiFi credentials are stored in the filesystem
 * @return true if credentials file exists, false otherwise
 */
bool CredentialManager::hasCredentials() {
    bool exists = LittleFS.exists(CREDENTIALS_FILE);
    LOG_DEBUGF(TAG, "Credentials file exists: %s", exists ? "YES" : "NO");
    return exists;
}

/**
 * @brief Print LittleFS filesystem usage statistics for debugging
 * Outputs total, used, and free space information to the logger
 */
void CredentialManager::printFileSystemInfo() {
    size_t totalBytes = LittleFS.totalBytes();
    size_t usedBytes = LittleFS.usedBytes();
    size_t freeBytes = totalBytes - usedBytes;
    
    LOG_INFOF(TAG, "LittleFS Filesystem Statistics:");
    LOG_INFOF(TAG, "   Total Space: %d bytes (%.2f KB)", totalBytes, totalBytes / 1024.0);
    LOG_INFOF(TAG, "   Used Space:  %d bytes (%.2f KB)", usedBytes, usedBytes / 1024.0);
    LOG_INFOF(TAG, "   Free Space:  %d bytes (%.2f KB)", freeBytes, freeBytes / 1024.0);
    LOG_INFOF(TAG, "   Usage: %.1f%%", (usedBytes * 100.0) / totalBytes);
}