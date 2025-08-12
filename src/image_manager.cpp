/**
 * @file image_manager.cpp
 * @brief Implementation of comprehensive image management system
 * 
 * Handles JPEG image upload, validation, storage, and display rendering
 * for dual TFT display billboard system with memory management and
 * hardware-specific optimizations.
 */

#include "image_manager.h"

// Static configuration constants
const char* ImageManager::IMAGES_DIR = "/images";
const char* ImageManager::IMAGE_LIST_FILE = "/images/image_list.json";
const uint32_t ImageManager::MAX_IMAGE_SIZE = 50000; // 50KB per image - optimized for ESP32 memory
const uint32_t ImageManager::MAX_IMAGE_COUNT = 10;   // Production limit for stable operation

// Singleton pattern support for TJpg_Decoder callbacks
ImageManager* ImageManager::instance = nullptr;
ImageManager* g_imageManager = nullptr;

ImageManager::ImageManager(DisplayManager* dm) : displayManager(dm) {
    // Initialize singleton references for static callback support
    instance = this;
    g_imageManager = this;
    currentTargetDisplay = 1; // Default to primary display
    
    // Auto-detect display type from build-time configuration
    #ifdef DISPLAY_TYPE_ST7789
        currentDisplayType = DisplayType::ST7789;
        displayWidth = 240;
        displayHeight = 240;
    #else
        currentDisplayType = DisplayType::ST7735;
        displayWidth = 160;
        displayHeight = 80;
    #endif
}

ImageManager::~ImageManager() {
    // Clean up singleton references
    instance = nullptr;
    g_imageManager = nullptr;
}

/**
 * @brief Initializes the image management system
 * @return true if initialization successful, false otherwise
 * 
 * Sets up:
 * - LittleFS file system
 * - Image storage directory structure
 * - TJpg_Decoder library configuration
 * - Hardware-specific display settings
 */
bool ImageManager::begin() {
    // Initialize LittleFS for persistent storage
    if (!LittleFS.begin()) {
        return false;
    }
    
    // Ensure image directory exists
    if (!LittleFS.exists(IMAGES_DIR)) {
        if (!LittleFS.mkdir(IMAGES_DIR)) {
            return false;
        }
    }
    
    // Configure TJpg_Decoder for optimal ESP32 performance
    TJpgDec.setJpgScale(1);        // Full resolution rendering
    TJpgDec.setSwapBytes(true);    // ESP32 endianness compatibility
    TJpgDec.setCallback(tft_output); // Route pixels to our display handler
    
    // Log message removed for compilation
    return true;
}

void ImageManager::setDisplayType(DisplayType type) {
    currentDisplayType = type;
    
    if (type == DisplayType::ST7789) {
        displayWidth = 240;
        displayHeight = 240;
    } else {
        displayWidth = 160;
        displayHeight = 80;
    }
    
    // Log message removed for compilation
}String ImageManager::getDisplayTypeString() const {
    return (currentDisplayType == DisplayType::ST7789) ? "ST7789" : "ST7735";
}

String ImageManager::getRequiredResolution() const {
    if (currentDisplayType == DisplayType::ST7789) {
        return "240x240";
    } else {
        return "160x80";
    }
}

bool ImageManager::validateImageDimensions(uint16_t width, uint16_t height) {
    if (currentDisplayType == DisplayType::ST7789) {
        return (width == 240 && height == 240);
    } else {
        // Allow both orientations for ST7735: 160x80 (landscape) or 80x160 (portrait)
        return ((width == 160 && height == 80) || (width == 80 && height == 160));
    }
}

bool ImageManager::validateImageFile(const String& filename, uint8_t* data, size_t length) {
    // Check file extension
    String lowerFilename = filename;
    lowerFilename.toLowerCase();
    if (!lowerFilename.endsWith(".jpg") && !lowerFilename.endsWith(".jpeg")) {
        // Log message removed for compilation
        return false;
    }
    
    // Check file size
    if (length > MAX_IMAGE_SIZE) {
        // Log message removed for compilation
        return false;
    }
    
    if (length < 100) {
        // Log message removed for compilation
        return false;
    }
    
    // Decode JPEG to check dimensions
    uint16_t w, h;
    if (TJpgDec.getJpgSize(&w, &h, data, length) != JDR_OK) {
        return false;
    }
    
    // Validate dimensions
    if (!validateImageDimensions(w, h)) {
        return false;
    }
    
    // Log message removed for compilation
    return true;
}

bool ImageManager::saveImage(const String& filename, uint8_t* data, size_t length) {
    String filepath = String(IMAGES_DIR) + "/" + filename;
    
    File file = LittleFS.open(filepath, "w");
    if (!file) {
        return false;
    }

    size_t written = file.write(data, length);
    file.close();

    if (written != length) {
        LittleFS.remove(filepath);
        return false;
    }

    // Verify file was saved
    if (!LittleFS.exists(filepath)) {
        return false;
    }

    // Create image info
    ImageInfo info;
    info.filename = filename;
    info.uploadTime = String(millis()); // Simple timestamp
    info.fileSize = length;
    info.isValid = true;

    // Get dimensions
    // Extract image dimensions if possible
    uint16_t w, h;
    if (TJpgDec.getJpgSize(&w, &h, data, length) == JDR_OK) {
        info.width = w;
        info.height = h;
    }

    // Update persistent metadata
    saveImageInfo(info);
    updateImageList();

    return true;
}

/**
 * @brief Primary image upload handler with comprehensive validation
 * @param filename Original filename with extension
 * @param data Raw JPEG file data
 * @param length File size in bytes
 * @return true if upload successful, false with detailed error in lastErrorMessage
 * 
 * Validation pipeline:
 * 1. Image count limit enforcement (MAX_IMAGE_COUNT)
 * 2. File format and structure validation
 * 3. Dimension compatibility check
 * 4. Storage space verification
 * 5. Atomic file save operation
 */
bool ImageManager::handleImageUpload(const String& filename, uint8_t* data, size_t length) {
    // Enforce maximum image count for memory management
    uint32_t currentCount = getImageCount();
    if (currentCount >= MAX_IMAGE_COUNT) {
        lastErrorMessage = "Maximum image limit reached (" + String(currentCount) + "/" + String(MAX_IMAGE_COUNT) + " images). Please delete some images first.";
        return false;
    }
    
    // Comprehensive JPEG validation and dimension checking
    if (!validateImageFile(filename, data, length)) {
        lastErrorMessage = "Image validation failed - invalid format or dimensions";
        return false;
    }
    
    // Check storage space
    if (!isStorageAvailable(length)) {
        lastErrorMessage = "Insufficient storage space available";
        return false;
    }
    
    // Save the image
    bool saveResult = saveImage(filename, data, length);
    if (!saveResult) {
        lastErrorMessage = "Failed to save image to storage";
    } else {
        lastErrorMessage = ""; // Clear error on success
    }
    return saveResult;
}

bool ImageManager::displayImage(const String& filename, uint8_t displayNum) {
    String filepath = String(IMAGES_DIR) + "/" + filename;
    
    if (!LittleFS.exists(filepath)) {
        // Log message removed for compilation
        return false;
    }
    
    File file = LittleFS.open(filepath, "r");
    if (!file) {
        // Log message removed for compilation
        return false;
    }
    
    size_t fileSize = file.size();
    uint8_t* buffer = (uint8_t*)malloc(fileSize);
    if (!buffer) {
        // Log message removed for compilation
        file.close();
        return false;
    }
    
    file.read(buffer, fileSize);
    file.close();
    
    // Set target display for callback
    currentTargetDisplay = displayNum;
    
    // Clear the screen before drawing the image
    if (displayManager) {
        // Set rotation for all displays and TJpg decoder
        displayManager->setRotation(0);  // Correct rotation for proper orientation
        
        displayManager->selectDisplayForImage(displayNum);  // Use image rotation
        TFT_eSPI* tft = displayManager->getTFT(displayNum);
        if (tft) {
            tft->fillScreen(TFT_BLACK);  // Clear screen with black background
        }
        // Keep display selected for TJpg drawing - don't deselect yet
    }
    
    // Decode and display (display should still be selected from above)
    bool success = (TJpgDec.drawJpg(0, 0, buffer, fileSize) == JDR_OK);
    
    // Now deselect the display
    if (displayManager) {
        displayManager->deselectAll();
    }
    
    free(buffer);
    
    if (success) {
        // Log message removed for compilation
    } else {
        // Log message removed for compilation
    }
    
    return success;
}

bool ImageManager::displayImageOnBoth(const String& filename) {
    // For now, display on both displays sequentially
    // This could be optimized to display simultaneously
    bool success1 = displayImage(filename, 1);
    bool success2 = displayImage(filename, 2);
    return success1 && success2;
}

void ImageManager::showNoImagesMessage() {
    if (!displayManager) {
        return;
    }
    
    // Get both TFT displays
    TFT_eSPI* tft1 = displayManager->getTFT(1);
    TFT_eSPI* tft2 = displayManager->getTFT(2);
    
    // Display "No Images" message on both screens
    if (tft1) {
        displayManager->selectDisplay(1);
        tft1->fillScreen(TFT_BLACK);
        tft1->setTextColor(TFT_WHITE, TFT_BLACK);
        tft1->setTextSize(2);
        
        // Center the text on display
        String message = "No Images";
        int16_t x = (displayWidth - (message.length() * 12)) / 2; // Approximate character width
        int16_t y = (displayHeight - 16) / 2; // Approximate character height
        
        tft1->setCursor(x, y);
        tft1->print(message);
    }
    
    if (tft2) {
        displayManager->selectDisplay(2);
        tft2->fillScreen(TFT_BLACK);
        tft2->setTextColor(TFT_WHITE, TFT_BLACK);
        tft2->setTextSize(2);
        
        // Center the text on display
        String message = "No Images";
        int16_t x = (displayWidth - (message.length() * 12)) / 2;
        int16_t y = (displayHeight - 16) / 2;
        
        tft2->setCursor(x, y);
        tft2->print(message);
    }
}

// Static callback for TJpg_Decoder
bool ImageManager::tft_output(int16_t x, int16_t y, uint16_t w, uint16_t h, uint16_t* bitmap) {
    if (!instance || !instance->displayManager) {
        return false;
    }
    
    // Use getTFT with the target display number
    // Note: We need to know which display we're targeting
    TFT_eSPI* tft = instance->displayManager->getTFT(instance->currentTargetDisplay);
    if (!tft) {
        return false;
    }
    
    // Push the pixels to the display (rotation should already be set)
    tft->pushImage(x, y, w, h, bitmap);
    
    return true;
}

bool ImageManager::deleteImage(const String& filename) {
    String filepath = String(IMAGES_DIR) + "/" + filename;
    
    if (!LittleFS.exists(filepath)) {
        // Log message removed for compilation
        return false;
    }
    
    if (LittleFS.remove(filepath)) {
        updateImageList();
        // Log message removed for compilation
        return true;
    } else {
        // Log message removed for compilation
        return false;
    }
}

bool ImageManager::imageExists(const String& filename) {
    String filepath = String(IMAGES_DIR) + "/" + filename;
    return LittleFS.exists(filepath);
}

String ImageManager::getImageListJson() {
    String json = "{\"images\":[";
    
    File dir = LittleFS.open(IMAGES_DIR);
    if (!dir || !dir.isDirectory()) {
        json += "],\"count\":0,\"error\":\"Directory not found\"}";
        return json;
    }
    
    bool first = true;
    int fileCount = 0;
    File file = dir.openNextFile();
    while (file) {
        if (!file.isDirectory()) {
            String filename = file.name();
            
            // Remove path prefix if present
            int lastSlash = filename.lastIndexOf('/');
            if (lastSlash >= 0) {
                filename = filename.substring(lastSlash + 1);
            }
            
            if (filename.endsWith(".jpg") || filename.endsWith(".jpeg")) {
                if (!first) json += ",";
                
                json += "{";
                json += "\"filename\":\"" + filename + "\",";
                json += "\"size\":" + String(file.size()) + ",";
                json += "\"valid\":true";
                json += "}";
                
                first = false;
                fileCount++;
            }
        }
        file = dir.openNextFile();
    }
    
    json += "],\"count\":" + String(fileCount);
    json += ",\"displayType\":\"" + getDisplayTypeString() + "\",";
    json += "\"requiredResolution\":\"" + getRequiredResolution() + "\"}";
    
    return json;
}

uint32_t ImageManager::getImageCount() {
    uint32_t count = 0;
    
    File dir = LittleFS.open(IMAGES_DIR);
    if (!dir || !dir.isDirectory()) {
        return 0;
    }
    
    File file = dir.openNextFile();
    while (file) {
        if (!file.isDirectory()) {
            String filename = file.name();
            if (filename.endsWith(".jpg") || filename.endsWith(".jpeg")) {
                count++;
            }
        }
        file = dir.openNextFile();
    }
    
    return count;
}

bool ImageManager::isStorageAvailable(uint32_t requiredBytes) {
    size_t totalBytes = LittleFS.totalBytes();
    size_t usedBytes = LittleFS.usedBytes();
    size_t freeBytes = totalBytes - usedBytes;
    
    // Keep 10KB free as buffer
    return (freeBytes > (requiredBytes + 10240));
}

String ImageManager::getSystemInfo() {
    String info = "{";
    info += "\"displayType\":\"" + getDisplayTypeString() + "\",";
    info += "\"resolution\":\"" + getRequiredResolution() + "\",";
    info += "\"imageCount\":" + String(getImageCount()) + ",";
    info += "\"maxImageCount\":" + String(MAX_IMAGE_COUNT) + ",";
    info += "\"storageTotal\":" + String(LittleFS.totalBytes()) + ",";
    info += "\"storageUsed\":" + String(LittleFS.usedBytes()) + ",";
    info += "\"storageFree\":" + String(LittleFS.totalBytes() - LittleFS.usedBytes()) + ",";
    info += "\"maxImageSize\":" + String(MAX_IMAGE_SIZE);
    info += "}";
    return info;
}

String ImageManager::getLastError() {
    return lastErrorMessage;
}

bool ImageManager::saveImageInfo(const ImageInfo& info) {
    // This could be expanded to maintain a detailed image database
    // For now, we'll keep it simple
    return true;
}

void ImageManager::updateImageList() {
    // This could cache image list for faster access
    // For now, we'll generate it dynamically
}
