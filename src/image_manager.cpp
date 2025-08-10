#include "image_manager.h"

// Static members
const char* ImageManager::IMAGES_DIR = "/images";
const char* ImageManager::IMAGE_LIST_FILE = "/images/image_list.json";
const uint32_t ImageManager::MAX_IMAGE_SIZE = 50000; // 50KB max per image

ImageManager* ImageManager::instance = nullptr;
ImageManager* g_imageManager = nullptr;

ImageManager::ImageManager(DisplayManager* dm) : displayManager(dm) {
    instance = this;
    g_imageManager = this;
    currentTargetDisplay = 1; // Default to display 1
    
    // Determine display type from build flags
    #ifdef DISPLAY_TYPE_ST7789
        currentDisplayType = DisplayType::ST7789;
        displayWidth = 240;
        displayHeight = 240;
    #else
        currentDisplayType = DisplayType::ST7735;
        displayWidth = 160;
        displayHeight = 80;
    #endif
    
    // LOG_INFOF("IMAGES", "ImageManager initialized for %s (%dx%d)", 
    //             getDisplayTypeString().c_str(), displayWidth, displayHeight);
}

ImageManager::~ImageManager() {
    instance = nullptr;
    g_imageManager = nullptr;
}

bool ImageManager::begin() {
    if (!LittleFS.begin()) {
        // Log message removed for compilation
        return false;
    }
    
    // Create images directory if it doesn't exist
    if (!LittleFS.exists(IMAGES_DIR)) {
        if (!LittleFS.mkdir(IMAGES_DIR)) {
            // Log message removed for compilation
            return false;
        }
        // Log message removed for compilation
    }
    
    // Set up TJpg_Decoder
    TJpgDec.setJpgScale(1); // No scaling
    TJpgDec.setSwapBytes(true); // ESP32 is little endian
    TJpgDec.setCallback(tft_output);
    
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
        Serial.println("ERROR: Failed to decode JPEG for size check");
        return false;
    }
    
    Serial.printf("Image dimensions: %dx%d\n", w, h);
    Serial.printf("Required dimensions for %s: %s\n", 
                  currentDisplayType == DisplayType::ST7789 ? "ST7789" : "ST7735",
                  currentDisplayType == DisplayType::ST7789 ? "240x240" : "160x80 or 80x160");
    
    // Validate dimensions
    if (!validateImageDimensions(w, h)) {
        Serial.printf("ERROR: Invalid dimensions %dx%d\n", w, h);
        return false;
    }
    
    // Log message removed for compilation
    return true;
}

bool ImageManager::saveImage(const String& filename, uint8_t* data, size_t length) {
    String filepath = String(IMAGES_DIR) + "/" + filename;
    Serial.println("=== SAVE IMAGE DEBUG ===");
    Serial.println("Filepath: " + filepath);
    
    File file = LittleFS.open(filepath, "w");
    if (!file) {
        Serial.println("ERROR: Failed to open file for writing");
        return false;
    }
    Serial.println("File opened for writing");

    size_t written = file.write(data, length);
    file.close();
    Serial.println("Written bytes: " + String(written) + " / " + String(length));

    if (written != length) {
        Serial.println("ERROR: Write size mismatch, removing file");
        LittleFS.remove(filepath);
        return false;
    }

    // Verify file was saved
    if (LittleFS.exists(filepath)) {
        File testFile = LittleFS.open(filepath, "r");
        if (testFile) {
            Serial.println("File verified - size: " + String(testFile.size()));
            testFile.close();
        }
    } else {
        Serial.println("ERROR: File does not exist after save");
        return false;
    }

    // Create image info
    ImageInfo info;
    info.filename = filename;
    info.uploadTime = String(millis()); // Simple timestamp
    info.fileSize = length;
    info.isValid = true;

    // Get dimensions
    uint16_t w, h;
    if (TJpgDec.getJpgSize(&w, &h, data, length) == JDR_OK) {
        info.width = w;
        info.height = h;
        Serial.println("Image dimensions: " + String(w) + "x" + String(h));
    }

    saveImageInfo(info);
    updateImageList();

    Serial.println("=== SAVE COMPLETE ===");
    return true;
}

bool ImageManager::handleImageUpload(const String& filename, uint8_t* data, size_t length) {
    Serial.println("=== IMAGE UPLOAD DEBUG ===");
    Serial.println("Filename: " + filename);
    Serial.println("Length: " + String(length));
    
    // Validate the image
    if (!validateImageFile(filename, data, length)) {
        Serial.println("ERROR: Image validation failed");
        return false;
    }
    Serial.println("Image validation passed");
    
    // Check storage space
    if (!isStorageAvailable(length)) {
        Serial.println("ERROR: Storage not available");
        return false;
    }
    Serial.println("Storage available");
    
    // Save the image
    bool saveResult = saveImage(filename, data, length);
    Serial.println("Save result: " + String(saveResult ? "SUCCESS" : "FAILED"));
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
            Serial.printf("Cleared display %d for image %s with rotation 0\n", displayNum, filename.c_str());
        }
        // Keep display selected for TJpg drawing - don't deselect yet
    }
    
    // Decode and display (display should still be selected from above)
    Serial.printf("Starting TJpg decode for display %d\n", displayNum);
    bool success = (TJpgDec.drawJpg(0, 0, buffer, fileSize) == JDR_OK);
    Serial.printf("TJpg decode finished for display %d, result: %s\n", displayNum, success ? "SUCCESS" : "FAILED");
    
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
    
    // Debug output (only for first few pixels to avoid spam)
    static int pixelCount = 0;
    if (pixelCount < 5) {
        Serial.printf("Drawing pixels to display %d at (%d,%d) size %dx%d\n", 
                     instance->currentTargetDisplay, x, y, w, h);
        pixelCount++;
    }
    
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
    
    Serial.println("=== DIRECTORY LISTING DEBUG ===");
    Serial.println("Directory: " + String(IMAGES_DIR));
    
    File dir = LittleFS.open(IMAGES_DIR);
    if (!dir || !dir.isDirectory()) {
        Serial.println("ERROR: Directory not found or not a directory");
        json += "],\"count\":0,\"error\":\"Directory not found\"}";
        return json;
    }
    Serial.println("Directory opened successfully");
    
    bool first = true;
    int fileCount = 0;
    File file = dir.openNextFile();
    while (file) {
        if (!file.isDirectory()) {
            String filename = file.name();
            Serial.println("Found file: " + filename + " (size: " + String(file.size()) + ")");
            
            // Remove path prefix if present
            int lastSlash = filename.lastIndexOf('/');
            if (lastSlash >= 0) {
                filename = filename.substring(lastSlash + 1);
                Serial.println("Cleaned filename: " + filename);
            }
            
            if (filename.endsWith(".jpg") || filename.endsWith(".jpeg")) {
                Serial.println("Adding JPEG file to list: " + filename);
                if (!first) json += ",";
                
                json += "{";
                json += "\"filename\":\"" + filename + "\",";
                json += "\"size\":" + String(file.size()) + ",";
                json += "\"valid\":true";
                json += "}";
                
                first = false;
                fileCount++;
            } else {
                Serial.println("Skipping non-JPEG file: " + filename);
            }
        }
        file = dir.openNextFile();
    }
    
    Serial.println("Total JPEG files found: " + String(fileCount));
    Serial.println("=== DIRECTORY LISTING COMPLETE ===");
    
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
    info += "\"storageTotal\":" + String(LittleFS.totalBytes()) + ",";
    info += "\"storageUsed\":" + String(LittleFS.usedBytes()) + ",";
    info += "\"storageFree\":" + String(LittleFS.totalBytes() - LittleFS.usedBytes()) + ",";
    info += "\"maxImageSize\":" + String(MAX_IMAGE_SIZE);
    info += "}";
    return info;
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
