/**
 * @file secrets.h
 * @brief Security credentials and authentication constants for ESP32 dual display billboard
 * 
 * Contains hardcoded security credentials for system authentication and access control.
 * This file defines default passwords and authentication tokens used by various
 * system components for initial setup and administrative access.
 * 
 * **SECURITY CONSIDERATIONS** 
 * 
 * IMPORTANT: This file contains sensitive security information and should be:
 * - Modified with unique passwords before deployment
 * - Excluded from version control in production environments
 * - Protected with appropriate file system permissions
 * - Never shared or exposed in public repositories
 * 
 * Default credentials are intended for development and initial setup only.
 * Production deployments MUST change these values to prevent unauthorized access.
 * 
 * Credential Usage:
 * - PORTAL_PASSWORD: Access to Billboard-Portal WiFi AP (192.168.4.1)
 * - OTA_USERNAME/PASSWORD: Over-the-Air firmware update authentication
 * 
 * Security Best Practices:
 * 1. Change all default passwords before production use
 * 2. Use strong, unique passwords with mixed case, numbers, and symbols
 * 3. Regularly rotate credentials in production environments
 * 4. Implement additional authentication layers where possible
 * 5. Monitor system logs for unauthorized access attempts
 * 
 * @author ESP32 Billboard Project
 * @date 2025
 * @version 0.9
 * @since v0.9
 * 
 * @warning DEFAULT CREDENTIALS - CHANGE BEFORE PRODUCTION USE
 */

#pragma once

/**
 * @brief Default password for Billboard-Portal WiFi access point
 * 
 * Password required to connect to the "Billboard-Portal" WiFi hotspot
 * created by the device when operating in access point mode. Used for
 * initial configuration and troubleshooting access.
 * 
 * Default: "billboard321"
 * 
 * @warning Change this password before production deployment
 * @note Access point operates on 192.168.4.1 with this credential
 * @see WiFiManager for access point implementation
 */
#define PORTAL_PASSWORD "billboard321"

/**
 * @brief Username for OTA (Over-The-Air) firmware update interface
 * 
 * HTTP Basic Authentication username required for accessing the
 * firmware update interface. Used to prevent unauthorized firmware
 * modifications and system tampering.
 * 
 * Default: "admin"
 * 
 * @warning Change this username before production deployment
 * @note Used in conjunction with OTA_PASSWORD for authentication
 * @see WiFiManager OTA implementation for usage
 */
#define OTA_USERNAME "admin"

/**
 * @brief Password for OTA (Over-The-Air) firmware update interface
 * 
 * HTTP Basic Authentication password required for accessing the
 * firmware update interface. Provides security layer preventing
 * unauthorized firmware modifications.
 * 
 * Default: "Billboard987$"
 * 
 * @warning Change this password before production deployment
 * @note Must be used with OTA_USERNAME for successful authentication
 * @see WiFiManager OTA endpoints for implementation details
 */
#define OTA_PASSWORD "Billboard987$"
