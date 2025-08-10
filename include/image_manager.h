#pragma once

#include <Arduino.h>
#include <TJpg_Decoder.h>
#include <LittleFS.h>
#include <ESPAsyncWebServer.h>
#include "logger.h"
#include "display_manager.h"

struct ImageInfo {
    String filename;
    String uploadTime;
    uint32_t fileSize;
    bool isValid;
    uint16_t width;
    uint16_t height;
};

enum class DisplayType {
    ST7735,  // 160x80
    ST7789   // 240x240
};

class ImageManager {
private:
    static const char* IMAGES_DIR;
    static const char* IMAGE_LIST_FILE;
    static const uint32_t MAX_IMAGE_SIZE;
    
    DisplayManager* displayManager;
    DisplayType currentDisplayType;
    uint16_t displayWidth;
    uint16_t displayHeight;
    uint8_t currentTargetDisplay; // Track which display we're targeting for TJpg callback
    
    // JPEG decoding callback functions
    static bool tft_output(int16_t x, int16_t y, uint16_t w, uint16_t h, uint16_t* bitmap);
    static ImageManager* instance; // For static callback access
    
    // Internal methods
    bool validateImageDimensions(uint16_t width, uint16_t height);
    bool saveImageInfo(const ImageInfo& info);
    ImageInfo getImageInfo(const String& filename);
    void updateImageList();
    
public:
    ImageManager(DisplayManager* dm);
    ~ImageManager();
    
    // Display type management
    void setDisplayType(DisplayType type);
    DisplayType getDisplayType() const { return currentDisplayType; }
    String getDisplayTypeString() const;
    String getRequiredResolution() const;
    
    // Image upload and validation
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
    
    // Initialization
    bool begin();
    
    // Static callback helpers for TJpg_Decoder
};

// Static callback helpers for TJpg_Decoder
extern ImageManager* g_imageManager;
