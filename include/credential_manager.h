/**
 * @file credential_manager.h
 * @brief Secure WiFi credential storage and management system for ESP32 dual display billboard
 * 
 * This module provides secure persistent storage and management of WiFi credentials
 * using LittleFS filesystem. Implements JSON-based storage with basic security measures
 * for the dual display billboard system's network connectivity.
 * 
 * Key Features:
 * - Secure LittleFS-based credential persistence
 * - JSON format for structured credential storage
 * - Automatic filesystem initialization and formatting
 * - Input validation and sanitization
 * - Comprehensive logging and error handling
 * - File system health monitoring and diagnostics
 * 
 * Security Considerations:
 * - Credentials are stored on-device in LittleFS partition
 * - Basic JSON escaping to prevent format corruption
 * - No encryption implemented (relies on device physical security)
 * - Cleartext storage suitable for non-critical applications
 * 
 * @author Dual Display Billboard Project
 * @version 1.0
 * @date August 2025
 */

#pragma once
#include <Arduino.h>
#include <LittleFS.h>
#include "logger.h"

/**
 * @class CredentialManager
 * @brief Secure WiFi credential storage and management system
 * 
 * Manages persistent storage of WiFi credentials using LittleFS filesystem with
 * JSON-based structured storage. Provides comprehensive credential lifecycle
 * management including storage, retrieval, validation, and secure deletion.
 * 
 * The manager implements a simple but robust approach to credential persistence
 * suitable for embedded applications where device physical security is assumed.
 * Credentials are stored in JSON format with basic input sanitization and
 * validation to prevent filesystem corruption.
 */
class CredentialManager {
private:
    // ============================================================================
    // PRIVATE CONSTANTS AND CONFIGURATION
    // ============================================================================
    static const char* CREDENTIALS_FILE;    ///< LittleFS path for credential storage
    static const String TAG;                ///< Logging component identifier

public:
    // ============================================================================
    // PUBLIC DATA STRUCTURES
    // ============================================================================
    
    /**
     * @struct WiFiCredentials
     * @brief Container for WiFi network credentials with validation state
     * 
     * Encapsulates WiFi network credentials with built-in validation state
     * to ensure credential integrity and provide clear error handling.
     */
    struct WiFiCredentials {
        String ssid;        ///< WiFi network SSID (Service Set Identifier)
        String password;    ///< WiFi network password/passphrase
        bool isValid;       ///< Validation flag indicating credential integrity
        
        /**
         * @brief Default constructor creating invalid credentials
         * 
         * Initializes empty credentials with isValid set to false,
         * providing a clear invalid state for error conditions.
         */
        WiFiCredentials() : ssid(""), password(""), isValid(false) {}
        
        /**
         * @brief Constructor with credentials creating valid credentials
         * @param s WiFi network SSID
         * @param p WiFi network password
         * 
         * Creates valid credentials with provided SSID and password,
         * automatically setting isValid to true for successful construction.
         */
        WiFiCredentials(const String& s, const String& p) : ssid(s), password(p), isValid(true) {}
    };

    // ============================================================================
    // PUBLIC METHODS - LIFECYCLE MANAGEMENT
    // ============================================================================
    
    /**
     * @brief Initialize credential manager and filesystem
     * @return true if initialization successful, false on failure
     * 
     * Initializes LittleFS filesystem with automatic formatting if mount fails.
     * Performs filesystem health checks and reports storage capacity information.
     * Must be called before any other credential operations.
     */
    static bool begin();
    
    // ============================================================================
    // PUBLIC METHODS - CREDENTIAL MANAGEMENT
    // ============================================================================
    
    /**
     * @brief Save WiFi credentials to persistent storage
     * @param ssid WiFi network SSID to store
     * @param password WiFi network password to store
     * @return true if credentials saved successfully, false on failure
     * 
     * Stores WiFi credentials in JSON format with input sanitization.
     * Automatically escapes special characters to prevent JSON corruption.
     * Overwrites any existing credentials with new values.
     */
    static bool saveCredentials(const String& ssid, const String& password);
    
    /**
     * @brief Load WiFi credentials from persistent storage
     * @return WiFiCredentials structure with loaded data or invalid state
     * 
     * Loads and parses stored WiFi credentials from LittleFS storage.
     * Returns WiFiCredentials with isValid=false if no credentials exist
     * or if parsing fails due to corruption or format errors.
     */
    static WiFiCredentials loadCredentials();
    
    /**
     * @brief Clear stored WiFi credentials
     * @return true if credentials cleared successfully, false on failure
     * 
     * Permanently removes stored WiFi credentials from filesystem.
     * Returns true even if no credentials were stored (successful no-op).
     * Use for factory reset or credential security purposes.
     */
    static bool clearCredentials();
    
    // ============================================================================
    // PUBLIC METHODS - STATUS AND DIAGNOSTICS
    // ============================================================================
    
    /**
     * @brief Check if credentials are currently stored
     * @return true if credentials file exists, false otherwise
     * 
     * Quick check for credential existence without loading or parsing.
     * Useful for conditional logic and user interface state management.
     */
    static bool hasCredentials();
    
    /**
     * @brief Print filesystem capacity and usage information
     * 
     * Outputs comprehensive LittleFS filesystem statistics including
     * total capacity, used space, and available free space.
     * Useful for debugging and system health monitoring.
     */
    static void printFileSystemInfo();
};