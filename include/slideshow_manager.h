/**
 * @file slideshow_manager.h
 * @brief Advanced image slideshow system with clock integration for dual display billboard
 * 
 * Provides comprehensive slideshow functionality with smooth transitions, clock overlays,
 * and intelligent content management for the ESP32 dual display billboard system.
 * Integrates seamlessly with image storage, display control, and time management.
 * 
 * Key Features:
 * - Automatic image discovery from LittleFS filesystem
 * - Configurable slideshow timing and transitions
 * - Clock overlay integration with multiple display modes
 * - Dual display synchronization for independent content
 * - Smart image rotation and aspect ratio handling
 * - Memory-efficient JPEG processing via TJpg_Decoder
 * - Dynamic content updates without restart
 * - Display mode switching (Gallery/Clock/Mixed modes)
 * 
 * Slideshow Modes:
 * - Gallery Mode: Full-screen image display with transitions
 * - Clock Mode: Time display with optional background images
 * - Mixed Mode: Alternating between images and clock faces
 * - Custom Timing: User-configurable display intervals
 * 
 * Display Integration:
 * - Automatic display type detection (ST7735/ST7789)
 * - Independent content control for dual displays
 * - Brightness synchronization with display settings
 * - Rotation handling for optimal image presentation
 * 
 * Content Management:
 * - Real-time image directory scanning
 * - Automatic slideshow updates on content changes
 * - Image validation and error handling
 * - Memory usage optimization for embedded systems
 * 
 * @author ESP32 Billboard Project
 * @date 2025
 * @version 0.9
 * @since v0.9
 */

#pragma once

#include <Arduino.h>
#include <vector>
#include <map>
#include <LittleFS.h>
#include "logger.h"
#include "image_manager.h"
#include "settings_manager.h"
#include "display_clock_manager.h"

/**
 * @brief Advanced image slideshow system with clock integration
 * 
 * Comprehensive slideshow manager that coordinates image display, clock overlays,
 * and content transitions for the dual display billboard system. Provides
 * intelligent content management with configurable timing and display modes.
 * 
 * The SlideshowManager operates as the central coordinator between image storage,
 * display hardware, clock functionality, and user settings to deliver smooth
 * slideshow experiences with professional presentation quality.
 * 
 * Key Capabilities:
 * - Automatic image discovery and playlist generation
 * - Smooth transitions between images and clock displays
 * - Memory-efficient content loading and caching
 * - Real-time configuration updates without interruption
 * - Error recovery and content validation
 */
class SlideshowManager {
private:
    ImageManager* imageManager;
    SettingsManager* settingsManager;
    DisplayClockManager* clockManager;
    
    // Slideshow state
    bool slideshowActive;
    unsigned long lastImageChange;
    unsigned long lastNoImagesCheck;  // NEW: Track timing for "no images" state
    int currentImageIndex;
    std::vector<String> enabledImages;
    std::map<String, bool> imageEnabledStates; // Track enabled/disabled state
    bool showingClock; // Track if currently showing clock
    
    // Internal methods
    void loadEnabledImages();
    bool isImageEnabled(const String& filename);
    void showNextImage();
    void showNoImagesMessage();
    
public:
    SlideshowManager(ImageManager* im, SettingsManager* sm, DisplayClockManager* cm);
    ~SlideshowManager();
    
    // Slideshow control
    void startSlideshow();
    void stopSlideshow();
    void updateSlideshow();
    void restartSlideshow(); // Reload images and restart
    
    // State queries
    bool isSlideshowActive() const { return slideshowActive; }
    bool shouldRetrySlideshow() const; // NEW: Check if enough time passed to retry when no images
    int getCurrentImageIndex() const { return currentImageIndex; }
    int getEnabledImageCount() const { return enabledImages.size(); }
    String getCurrentImageName() const;
    
    // Clock/gallery switching for DCC control
    void showClock();
    
    // Configuration
    void refreshImageList(); // Call when images are added/deleted/enabled/disabled
    void updateImageEnabledState(const String& filename, bool enabled);
    void loadImageStatesFromStorage();
    std::map<String, bool> getImageEnabledStates() const; // NEW: Get all enabled states
    
    // Initialization
    bool begin();
};
