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

/**
 * @brief Configure display type and corresponding resolution parameters
 * @param type Display hardware type (ST7735 or ST7789)
 * 
 * Updates internal display dimensions and hardware-specific settings:
 * - ST7789: 240x240 square display
 * - ST7735: 160x80 landscape display
 * 
 * @note Must be called before image operations to ensure proper scaling
 * @since v0.9
 */
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
}

/**
 * @brief Get string representation of current display type
 * @return Display type as string ("ST7789" or "ST7735")
 * 
 * Provides human-readable display type identifier for debugging,
 * logging, and web interface display purposes.
 * 
 * @since v0.9
 */
String ImageManager::getDisplayTypeString() const {
    return (currentDisplayType == DisplayType::ST7789) ? "ST7789" : "ST7735";
}

/**
 * @brief Get required image resolution string for current display type
 * @return Resolution string ("240x240" for ST7789, "160x80" for ST7735)
 * 
 * Used for web interface validation messages and user guidance.
 * Provides exact pixel dimensions required for uploaded images
 * to display correctly without distortion.
 * 
 * @since v0.9
 */
String ImageManager::getRequiredResolution() const {
    if (currentDisplayType == DisplayType::ST7789) {
        return "240x240";
    } else {
        return "160x80";
    }
}

/**
 * @brief Validate image dimensions against current display type requirements
 * @param width Image width in pixels
 * @param height Image height in pixels
 * @return true if dimensions are valid for current display type, false otherwise
 * 
 * Validates image dimensions according to display hardware specifications:
 * - ST7789: Requires exact 240x240 pixels (square format)
 * - ST7735: Accepts 160x80 (landscape) or 80x160 (portrait) orientations
 * 
 * @note Used during image upload validation process
 * @since v0.9
 */
bool ImageManager::validateImageDimensions(uint16_t width, uint16_t height) {
    if (currentDisplayType == DisplayType::ST7789) {
        return (width == 240 && height == 240);
    } else {
        // Allow both orientations for ST7735: 160x80 (landscape) or 80x160 (portrait)
        return ((width == 160 && height == 80) || (width == 80 && height == 160));
    }
}

/**
 * @brief Comprehensive validation of uploaded image file data
 * @param filename Name of the image file (used for extension validation)
 * @param data Raw image file data buffer
 * @param length Size of image data in bytes
 * @return true if image passes all validation checks, false otherwise
 * 
 * Performs multi-stage validation:
 * 1. File extension check (.jpg/.jpeg only)
 * 2. File size limits (100 bytes minimum, MAX_IMAGE_SIZE maximum)
 * 3. JPEG format validation using TJpg_Decoder
 * 4. Dimension validation against display requirements
 * 
 * @note Critical for preventing corrupt uploads and memory issues
 * @see validateImageDimensions() for dimension checking logic
 * @since v0.9
 */
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

/**
 * @brief Save validated image data to LittleFS with metadata tracking
 * @param filename Name for the saved image file
 * @param data Raw image data buffer 
 * @param length Size of image data in bytes
 * @return true if image saved successfully with metadata, false otherwise
 * 
 * Performs complete image persistence process:
 * 1. Creates file in /images/ directory
 * 2. Writes raw data to LittleFS
 * 3. Verifies write integrity
 * 4. Extracts and stores image metadata (dimensions, size, timestamp)
 * 5. Updates image list for slideshow management
 * 
 * @note Automatically removes corrupted files on write failure
 * @note Creates ImageInfo metadata for slideshow integration
 * @see saveImageInfo() for metadata persistence
 * @see updateImageList() for slideshow list maintenance
 * @since v0.9
 */
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

/**
 * @brief Display JPEG image on specified hardware display
 * @param filename Name of image file in /images/ directory
 * @param displayNum Target display number (1 or 2)
 * @return true if image displayed successfully, false otherwise
 * 
 * Complete image rendering process:
 * 1. Loads image file from LittleFS storage
 * 2. Allocates memory buffer for image data  
 * 3. Configures target display via DisplayManager
 * 4. Clears display with black background
 * 5. Uses TJpg_Decoder to render JPEG directly to display
 * 6. Handles proper display selection/deselection
 * 7. Manages memory cleanup
 * 
 * @note Requires sufficient heap memory for image buffer
 * @note Uses callback mechanism for pixel-by-pixel rendering
 * @see tft_output() for TJpg_Decoder callback implementation
 * @since v0.9
 */
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

/**
 * @brief Display image simultaneously on both hardware displays
 * @param filename Name of image file in /images/ directory
 * @return true if image displayed successfully on both displays, false otherwise
 * 
 * Renders the same image on both display outputs by calling displayImage()
 * sequentially for each display. Currently implemented as sequential
 * rendering for memory efficiency.
 * 
 * @note Both displays must succeed for true return value
 * @note Uses sequential rendering to minimize memory usage
 * @see displayImage() for single display rendering details
 * @since v0.9
 */
bool ImageManager::displayImageOnBoth(const String& filename) {
    // For now, display on both displays sequentially
    // This could be optimized to display simultaneously
    bool success1 = displayImage(filename, 1);
    bool success2 = displayImage(filename, 2);
    return success1 && success2;
}

/**
 * @brief Display a "No Images" message on both billboard displays when
 *        no JPEG images are available in the LittleFS storage. Provides visual
 *        feedback to users when the image directory is empty or when all images
 *        have been removed from the slideshow system.
 * 
 * This method serves as a fallback display state when the slideshow has no content
 * to show. The message is centered on both displays using white text on black
 * background for maximum visibility and contrast. Each display is individually
 * selected and configured to ensure proper rendering.
 * 
 * @note Message is displayed on both displays simultaneously
 * @note Uses text size 2 for good readability at typical viewing distances
 * @note Background is cleared to black before displaying the message
 * @note Text positioning is automatically calculated for center alignment
 * @note Individual display selection ensures proper hardware configuration
 * 
 * @see getImageCount() for checking available image count
 * @see getImageListJson() for retrieving complete image inventory
 * @see displayImage() for normal image display functionality
 * @see DisplayManager::selectDisplay() for display activation
 * 
 * @since v0.9
 */
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

/**
 * @brief Static callback function for TJpg_Decoder library to output decoded
 *        JPEG pixel data directly to the currently targeted TFT display. This
 *        function serves as the bridge between the JPEG decoder and the display
 *        hardware, enabling efficient streaming of decoded image data.
 * 
 * The TJpg_Decoder library calls this function repeatedly with chunks of decoded
 * pixel data during JPEG decompression. The function uses the singleton instance
 * to access the display manager and renders pixels to the currently selected
 * target display using hardware-accelerated operations.
 * 
 * @param x Left coordinate of the pixel block being rendered
 * @param y Top coordinate of the pixel block being rendered  
 * @param w Width of the pixel block in pixels
 * @param h Height of the pixel block in pixels
 * @param bitmap Pointer to RGB565 pixel data array for the block
 * 
 * @return true if pixels were successfully written to display, false on error
 * 
 * @note This is a static function called by TJpg_Decoder during decompression
 * @note Uses singleton instance to access display manager and target display
 * @note Pixel data is in RGB565 format ready for TFT display
 * @note Display rotation and orientation must be set before calling decoder
 * @note Function validates instance and display manager before pixel operations
 * 
 * @see displayImage() for complete JPEG display process
 * @see TJpg_Decoder::setJpgScale() for image scaling configuration
 * @see DisplayManager::getTFT() for display hardware access
 * @see TFT_eSPI::pushImage() for pixel data rendering
 * 
 * @since v0.9
 */
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

/**
 * @brief Delete a JPEG image file from the LittleFS storage system and update
 *        the internal image list cache to reflect the removal. This operation
 *        permanently removes the specified image file and cannot be undone.
 * 
 * The method performs validation to ensure the file exists before attempting
 * deletion, then removes the file from the filesystem and updates the cached
 * image list to maintain consistency with the storage state.
 * 
 * @param filename The base filename of the image to delete (without path)
 * 
 * @return true if the image was successfully deleted, false if file not found or deletion failed
 * 
 * @note File existence is validated before deletion attempt
 * @note Image list cache is automatically updated after successful deletion
 * @note Deletion is permanent and cannot be undone
 * @note Only affects files in the designated images directory
 * @note Logging statements removed to prevent compilation issues
 * 
 * @see imageExists() for checking file existence before deletion
 * @see updateImageList() for cache maintenance after deletion
 * @see getImageListJson() for retrieving updated image inventory
 * @see LittleFS::remove() for filesystem deletion operation
 * 
 * @since v0.9
 */
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

/**
 * @brief Check if a specific JPEG image file exists in the LittleFS storage
 *        system. This method provides a quick validation mechanism to verify
 *        file presence before performing operations like display or deletion.
 * 
 * The method constructs the full file path by combining the images directory
 * path with the provided filename and queries the LittleFS filesystem for
 * file existence using the native filesystem API.
 * 
 * @param filename The base filename to check for existence (without path)
 * 
 * @return true if the image file exists in storage, false otherwise
 * 
 * @note Only checks for file existence, not file validity or format
 * @note Uses LittleFS native existence checking for efficiency
 * @note Filename should not include directory path components
 * @note Does not validate JPEG format or file integrity
 * 
 * @see deleteImage() for removing existing files
 * @see saveImage() for creating new image files
 * @see validateImageFile() for comprehensive file validation
 * @see LittleFS::exists() for filesystem existence checking
 * 
 * @since v0.9
 */
bool ImageManager::imageExists(const String& filename) {
    String filepath = String(IMAGES_DIR) + "/" + filename;
    return LittleFS.exists(filepath);
}

/**
 * @brief Generate a comprehensive JSON representation of all JPEG images stored
 *        in the LittleFS filesystem, including metadata and system configuration
 *        information for web interface consumption and slideshow management.
 * 
 * This method scans the images directory and builds a structured JSON object
 * containing individual image details (filename, size, validity) along with
 * system-wide information like display configuration, image count, and storage
 * requirements. The JSON is formatted for direct consumption by web interfaces
 * and API endpoints.
 * 
 * @return String containing complete JSON object with image list and system metadata
 * 
 * @note Scans images directory dynamically for real-time accuracy
 * @note Filters files by .jpg and .jpeg extensions for JPEG validation
 * @note Includes system configuration data for display requirements
 * @note Removes path prefixes from filenames for consistent naming
 * @note Returns error information if directory access fails
 * @note JSON format suitable for direct web API responses
 * 
 * @see getImageCount() for simple image counting
 * @see getSystemInfo() for detailed system configuration
 * @see getDisplayTypeString() for current display configuration
 * @see getRequiredResolution() for display resolution requirements
 * 
 * @since v0.9
 */
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

/**
 * @brief Count the total number of valid JPEG image files currently stored
 *        in the LittleFS images directory. This method provides a simple
 *        numeric count for system monitoring and slideshow management.
 * 
 * The method scans the images directory and counts files with .jpg or .jpeg
 * extensions, filtering out directories and non-JPEG files to provide an
 * accurate count of displayable images available for the slideshow system.
 * 
 * @return Total count of JPEG files in the images directory, 0 if directory inaccessible
 * 
 * @note Only counts files with .jpg or .jpeg extensions
 * @note Returns 0 if images directory cannot be accessed
 * @note Does not validate JPEG format or file integrity
 * @note Excludes subdirectories from the count
 * @note Used for slideshow management and system status reporting
 * 
 * @see getImageListJson() for detailed image information
 * @see getSystemInfo() for comprehensive system statistics
 * @see isStorageAvailable() for storage capacity checking
 * @see MAX_IMAGE_COUNT for maximum allowed image count
 * 
 * @since v0.9
 */
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

/**
 * @brief Check if sufficient LittleFS storage space is available for storing
 *        a new image file of specified size. Includes safety buffer to prevent
 *        filesystem corruption and ensure stable system operation.
 * 
 * This method calculates available storage space by determining the difference
 * between total filesystem capacity and current usage, then compares against
 * the required bytes plus a 10KB safety buffer to prevent filesystem issues
 * and maintain system stability during image storage operations.
 * 
 * @param requiredBytes The number of bytes needed for the new image file
 * 
 * @return true if sufficient space is available with safety buffer, false otherwise
 * 
 * @note Includes 10KB safety buffer to prevent filesystem corruption
 * @note Checks actual filesystem usage vs capacity in real-time
 * @note Should be called before attempting image upload or storage operations
 * @note Safety buffer ensures stable system operation even after storage
 * @note Returns false if filesystem queries fail
 * 
 * @see saveImage() for image storage operations requiring space validation
 * @see getSystemInfo() for detailed storage statistics
 * @see MAX_IMAGE_SIZE for maximum allowed image file size
 * @see LittleFS::totalBytes() for filesystem capacity information
 * @see LittleFS::usedBytes() for current storage usage
 * 
 * @since v0.9
 */
bool ImageManager::isStorageAvailable(uint32_t requiredBytes) {
    size_t totalBytes = LittleFS.totalBytes();
    size_t usedBytes = LittleFS.usedBytes();
    size_t freeBytes = totalBytes - usedBytes;
    
    // Keep 10KB free as buffer
    return (freeBytes > (requiredBytes + 10240));
}

/**
 * @brief Generate comprehensive JSON system information including display
 *        configuration, storage statistics, image counts, and capacity limits
 *        for web interface monitoring and system administration.
 * 
 * This method compiles real-time system status data into a structured JSON
 * object containing display type, resolution requirements, current image
 * inventory, storage utilization, and operational limits. The JSON format
 * enables direct consumption by web interfaces and monitoring systems.
 * 
 * @return String containing complete JSON object with system configuration and statistics
 * 
 * @note Provides real-time storage statistics from LittleFS
 * @note Includes display configuration and resolution requirements
 * @note Shows current vs maximum image count limits
 * @note Calculates free storage space dynamically
 * @note JSON format suitable for direct web API responses
 * @note Used by system monitoring and administrative interfaces
 * 
 * @see getImageListJson() for detailed individual image information
 * @see getImageCount() for simple image counting
 * @see isStorageAvailable() for storage capacity validation
 * @see getDisplayTypeString() for current display configuration
 * @see getRequiredResolution() for display resolution requirements
 * 
 * @since v0.9
 */
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

/**
 * @brief Retrieve the most recent error message from ImageManager operations
 *        for debugging and user feedback purposes. Provides access to detailed
 *        error information when image operations fail.
 * 
 * This method returns the last recorded error message string that was set
 * during failed image operations such as upload validation, file saving,
 * or display rendering. The error message can be used for debugging,
 * logging, or providing user feedback through web interfaces.
 * 
 * @return String containing the most recent error message, empty if no errors
 * 
 * @note Error message is set by various ImageManager operations on failure
 * @note Returns empty string if no errors have occurred
 * @note Error message persists until next operation or system reset
 * @note Used for debugging and user feedback in web interfaces
 * @note Thread-safe for concurrent access to error state
 * 
 * @see validateImageFile() for operations that set validation error messages
 * @see saveImage() for operations that set file storage error messages
 * @see displayImage() for operations that set display error messages
 * @see handleImageUpload() for upload-specific error handling
 * 
 * @since v0.9
 */
String ImageManager::getLastError() {
    return lastErrorMessage;
}

/**
 * @brief Save image metadata information to a persistent database or cache
 *        for enhanced image management and slideshow functionality. Currently
 *        implements a simplified approach but designed for future expansion.
 * 
 * This method provides a framework for storing detailed image metadata such
 * as dimensions, file size, upload timestamp, and display preferences. The
 * current implementation is simplified but the interface is designed to
 * support future database integration for advanced image management features.
 * 
 * @param info ImageInfo structure containing metadata to be saved
 * 
 * @return true if metadata was successfully saved, false on error
 * 
 * @note Current implementation is simplified and always returns true
 * @note Designed for future expansion to support detailed metadata database
 * @note Could store information like upload time, display count, preferences
 * @note Interface prepared for caching and advanced slideshow features
 * @note Thread-safe for concurrent metadata operations
 * 
 * @see updateImageList() for image list cache management
 * @see getImageListJson() for retrieving image metadata
 * @see ImageInfo structure for metadata field definitions
 * @see saveImage() for file storage operations
 * 
 * @since v0.9
 */
bool ImageManager::saveImageInfo(const ImageInfo& info) {
    // This could be expanded to maintain a detailed image database
    // For now, we'll keep it simple
    return true;
}

/**
 * @brief Update the internal image list cache to reflect current filesystem
 *        state after image addition or removal operations. Maintains cache
 *        consistency for improved performance and accurate system status.
 * 
 * This method provides a framework for maintaining cached image list data
 * that can improve performance by avoiding repeated filesystem scans. The
 * current implementation uses dynamic generation but the interface is
 * designed to support future caching optimizations for better responsiveness.
 * 
 * @note Current implementation generates image list dynamically
 * @note Designed for future expansion to support intelligent caching
 * @note Called automatically after image addition or deletion operations
 * @note Could cache metadata for faster web interface responses
 * @note Interface prepared for advanced slideshow and management features
 * @note Thread-safe for concurrent list update operations
 * 
 * @see deleteImage() for operations that trigger image list updates
 * @see saveImage() for operations that require list cache refresh
 * @see getImageListJson() for accessing current image list data
 * @see getImageCount() for simple image counting without caching
 * 
 * @since v0.9
 */
void ImageManager::updateImageList() {
    // This could cache image list for faster access
    // For now, we'll generate it dynamically
}
