/**
 * @file image_manager.h
 * @brief Manages JPEG image storage, validation, and display for dual TFT displays
 * 
 * This class provides comprehensive image management functionality including:
 * - JPEG image upload with size and dimension validation
 * - File system storage management with configurable limits
 * - Multi-display rendering with automatic display selection
 * - Image metadata tracking and retrieval
 * - Display type awareness (ST7735 vs ST7789)
 * 
 * @author Dual Display Billboard Project
 * @version 2.0
 */

#pragma once

#include <Arduino.h>
#include <TJpg_Decoder.h>
#include <LittleFS.h>
#include <ESPAsyncWebServer.h>
#include "logger.h"
#include "display_manager.h"

/**
 * @brief Image metadata structure
 * 
 * Contains essential information about stored images including
 * file properties, validation status, and upload tracking.
 */
struct ImageInfo {
    String filename;    ///< Original filename with extension
    String uploadTime;  ///< Upload timestamp (milliseconds since boot)
    uint32_t fileSize;  ///< File size in bytes
    bool isValid;       ///< Image validation status
    uint16_t width;     ///< Image width in pixels
    uint16_t height;    ///< Image height in pixels
};

/**
 * @brief Supported display types
 * 
 * Defines the different TFT display types with their native resolutions.
 * This affects image validation and display rendering.
 */
enum class DisplayType {
    ST7735,  ///< 160x80 or 80x160 pixels (portrait/landscape)
    ST7789   ///< 240x240 pixels (square)
};

/**
 * @brief Comprehensive image management system
 * 
 * Handles all aspects of image storage and display for the billboard system.
 * Integrates with DisplayManager for hardware control and provides web API
 * endpoints for image upload and management.
 * 
 * Key features:
 * - Automatic image count limiting (MAX_IMAGE_COUNT)
 * - Size validation (MAX_IMAGE_SIZE)
 * - Dimension validation per display type
 * - LittleFS storage management
 * - TJpg_Decoder integration for efficient rendering
 * - Detailed error reporting
 */
class ImageManager {
private:
    // Storage configuration constants
    static const char* IMAGES_DIR;        ///< Directory path for image storage
    static const char* IMAGE_LIST_FILE;   ///< Metadata file for image tracking
    static const uint32_t MAX_IMAGE_SIZE; ///< Maximum file size (50KB)
    static const uint32_t MAX_IMAGE_COUNT;///< Maximum number of images (10)
    
    // Hardware integration
    DisplayManager* displayManager;        ///< Reference to display hardware manager
    DisplayType currentDisplayType;        ///< Current display type (ST7735/ST7789)
    uint16_t displayWidth;                 ///< Display width in pixels
    uint16_t displayHeight;                ///< Display height in pixels
    uint8_t currentTargetDisplay;          ///< Active display for TJpg callback routing
    String lastErrorMessage;               ///< Detailed error message storage
    
    // Static callback support for TJpg_Decoder
    static ImageManager* instance;         ///< Singleton instance for static callbacks
    
    /**
     * @brief TJpg_Decoder output callback
     * @param x X coordinate for pixel block
     * @param y Y coordinate for pixel block  
     * @param w Width of pixel block
     * @param h Height of pixel block
     * @param bitmap RGB565 pixel data
     * @return true if pixels written successfully
     */
    static bool tft_output(int16_t x, int16_t y, uint16_t w, uint16_t h, uint16_t* bitmap);
    
    // Internal validation and utility methods
    /**
     * @brief Validates image dimensions against display capabilities
     * @param width Image width in pixels
     * @param height Image height in pixels
     * @return true if dimensions are acceptable for current display type
     */
    bool validateImageDimensions(uint16_t width, uint16_t height);
    
    /**
     * @brief Saves image metadata to persistent storage
     * @param info Image information structure
     * @return true if metadata saved successfully
     */
    bool saveImageInfo(const ImageInfo& info);
    
    /**
     * @brief Retrieves image metadata from storage
     * @param filename Image filename
     * @return ImageInfo structure with file details
     */
    ImageInfo getImageInfo(const String& filename);
    
    /**
     * @brief Updates the master image list file
     */
    void updateImageList();
    
public:
    /**
     * @brief Constructs ImageManager with display hardware integration
     * @param dm Pointer to DisplayManager instance for hardware control
     */
    ImageManager(DisplayManager* dm);
    
    /**
     * @brief Destructor - cleans up singleton references
     */
    ~ImageManager();
    
    // Display type management
    /**
     * @brief Sets the display type for validation and rendering
     * @param type ST7735 or ST7789 display type
     */
    void setDisplayType(DisplayType type);
    
    /**
     * @brief Gets current display type
     * @return Currently configured display type
     */
    DisplayType getDisplayType() const { return currentDisplayType; }
    
    /**
     * @brief Gets human-readable display type string
     * @return "ST7735" or "ST7789"
     */
    String getDisplayTypeString() const;
    
    /**
     * @brief Gets required resolution string for current display type
     * @return Resolution description for validation feedback
     */
    String getRequiredResolution() const;
    
    // Image upload and validation
    /**
     * @brief Handles complete image upload with validation and storage
     * @param filename Original filename including extension
     * @param data Raw JPEG file data
     * @param length Data size in bytes
     * @return true if image uploaded and validated successfully
     * 
     * Performs comprehensive validation:
     * - Image count limit check (MAX_IMAGE_COUNT)
     * - File size validation (MAX_IMAGE_SIZE)
     * - JPEG format validation
     * - Dimension validation for display type
     * - Storage space verification
     */
    bool handleImageUpload(const String& filename, uint8_t* data, size_t length);
    bool validateImageFile(const String& filename, uint8_t* data, size_t length);
    
    // Image management
    bool saveImage(const String& filename, uint8_t* data, size_t length);
    bool deleteImage(const String& filename);
    bool imageExists(const String& filename);
    
    // Image display
    bool displayImage(const String& filename, uint8_t displayNum = 1);
    bool displayImageOnBoth(const String& filename);
    void showNoImagesMessage(); // NEW: Show "No Images" message on displays
    
    // Image listing and info
    String getImageListJson();
    ImageInfo getImageDetails(const String& filename);
    uint32_t getImageCount();
    uint32_t getTotalImagesSize();
    
    // System info
    String getSystemInfo();
    bool isStorageAvailable(uint32_t requiredBytes);
    String getLastError();  // Get detailed error message
    
    // Initialization
    bool begin();
    
    // Static callback helpers for TJpg_Decoder
};

// Static callback helpers for TJpg_Decoder
extern ImageManager* g_imageManager;
