#include "settings_manager.h"

static const String TAG = "SETTINGS";

// Settings file paths
const char* SettingsManager::SECOND_DISPLAY_FILE = "/second_display.txt";
const char* SettingsManager::DCC_ENABLED_FILE = "/dcc_enabled.txt";
const char* SettingsManager::IMAGE_INTERVAL_FILE = "/image_interval.txt";
const char* SettingsManager::IMAGE_ENABLED_FILE = "/image_enabled.txt";
const char* SettingsManager::BRIGHTNESS_FILE = "/brightness.txt";
const char* SettingsManager::CLOCK_ENABLED_FILE = "/clock_enabled.txt";
const char* SettingsManager::CLOCK_FACE_FILE = "/clock_face.txt";

SettingsManager::SettingsManager() {
    // Set defaults - will be loaded in begin()
    secondDisplayEnabled = true;
    dccEnabled = false;
    imageInterval = 10; // Default image interval 10 (seconds)
    imageEnabled = true;
    brightness = 200; // Default brightness value (0-255)
    clockEnabled = false; // Default clock disabled to maintain existing behavior
    clockFace = CLOCK_CLASSIC_ANALOG; // Default to classic analog
}

bool SettingsManager::begin() {
    LOG_INFO(TAG, "⚙️ Initializing Settings Manager...");
    
    // Load all settings from LittleFS
    secondDisplayEnabled = loadBoolean(SECOND_DISPLAY_FILE, true);
    dccEnabled = loadBoolean(DCC_ENABLED_FILE, false);
    imageInterval = loadInteger(IMAGE_INTERVAL_FILE, 10); // Default 10 seconds
    imageEnabled = loadBoolean(IMAGE_ENABLED_FILE, true);
    brightness = loadInteger(BRIGHTNESS_FILE, 200);
    clockEnabled = loadBoolean(CLOCK_ENABLED_FILE, false); // Default disabled
    clockFace = static_cast<ClockFaceType>(loadInteger(CLOCK_FACE_FILE, CLOCK_CLASSIC_ANALOG));
    
    LOG_INFOF(TAG, "📺 Second Display: %s", secondDisplayEnabled ? "enabled" : "disabled");
    LOG_INFOF(TAG, "🚂 DCC Interface: %s", dccEnabled ? "enabled" : "disabled");
    LOG_INFOF(TAG, "⏱️ Image Interval: %d seconds", imageInterval);
    LOG_INFOF(TAG, "🖼️ Image Display: %s", imageEnabled ? "enabled" : "disabled");
    LOG_INFOF(TAG, "🔆 Brightness: %d", brightness);
    LOG_INFOF(TAG, "🕒 Clock Display: %s", clockEnabled ? "enabled" : "disabled");
    
    LOG_INFO(TAG, "✅ Settings Manager initialized");
    return true;
}

// Second Display settings
void SettingsManager::setSecondDisplayEnabled(bool enabled) {
    secondDisplayEnabled = enabled;
    if (saveBoolean(SECOND_DISPLAY_FILE, enabled)) {
        LOG_INFOF(TAG, "💾 Second display setting saved: %s", enabled ? "enabled" : "disabled");
    } else {
        LOG_WARN(TAG, "⚠️ Failed to save second display setting");
    }
}

bool SettingsManager::isSecondDisplayEnabled() {
    return secondDisplayEnabled;
}

// DCC settings
void SettingsManager::setDCCEnabled(bool enabled) {
    dccEnabled = enabled;
    if (saveBoolean(DCC_ENABLED_FILE, enabled)) {
        LOG_INFOF(TAG, "💾 DCC setting saved: %s", enabled ? "enabled" : "disabled");
    } else {
        LOG_WARN(TAG, "⚠️ Failed to save DCC setting");
    }
}

bool SettingsManager::isDCCEnabled() {
    return dccEnabled;
}

// Image settings
void SettingsManager::setImageInterval(int seconds) {
    imageInterval = seconds;
    if (saveInteger(IMAGE_INTERVAL_FILE, seconds)) {
        LOG_INFOF(TAG, "💾 Image interval saved: %d seconds", seconds);
    } else {
        LOG_WARN(TAG, "⚠️ Failed to save image interval");
    }
}

int SettingsManager::getImageInterval() {
    return imageInterval;
}

void SettingsManager::setImageEnabled(bool enabled) {
    imageEnabled = enabled;
    if (saveBoolean(IMAGE_ENABLED_FILE, enabled)) {
        LOG_INFOF(TAG, "💾 Image display setting saved: %s", enabled ? "enabled" : "disabled");
    } else {
        LOG_WARN(TAG, "⚠️ Failed to save image display setting");
    }
}

bool SettingsManager::isImageEnabled() {
    return imageEnabled;
}

// Brightness settings
void SettingsManager::setBrightness(int value) {
    // Clamp brightness value between 0 and 255
    if (value < 0) value = 0;
    if (value > 255) value = 255;
    
    brightness = value;
    if (saveInteger(BRIGHTNESS_FILE, value)) {
        LOG_INFOF(TAG, "💾 Brightness saved: %d", value);
    } else {
        LOG_WARN(TAG, "⚠️ Failed to save brightness setting");
    }
}

int SettingsManager::getBrightness() {
    return brightness;
}

// Utility methods
void SettingsManager::printSettings() {
    LOG_INFO(TAG, "📋 Current Settings:");
    LOG_INFOF(TAG, "  📺 Second Display: %s", secondDisplayEnabled ? "enabled" : "disabled");
    LOG_INFOF(TAG, "  🚂 DCC Interface: %s", dccEnabled ? "enabled" : "disabled");
    LOG_INFOF(TAG, "  ⏱️ Image Interval: %d seconds", imageInterval);
    LOG_INFOF(TAG, "  🖼️ Image Display: %s", imageEnabled ? "enabled" : "disabled");
    LOG_INFOF(TAG, "  🔆 Brightness: %d", brightness);
}

void SettingsManager::resetToDefaults() {
    LOG_INFO(TAG, "🔄 Resetting settings to defaults...");
    setSecondDisplayEnabled(true);
    setDCCEnabled(false);
    setImageInterval(10); // Reset to default 10 seconds
    setImageEnabled(true);
    setBrightness(200);
    LOG_INFO(TAG, "✅ Settings reset to defaults");
}

// Private helper methods
bool SettingsManager::saveBoolean(const char* filename, bool value) {
    File file = LittleFS.open(filename, "w");
    if (!file) {
        return false;
    }
    file.print(value ? "true" : "false");
    file.close();
    return true;
}

bool SettingsManager::loadBoolean(const char* filename, bool defaultValue) {
    if (!LittleFS.exists(filename)) {
        return defaultValue;
    }
    
    File file = LittleFS.open(filename, "r");
    if (!file) {
        return defaultValue;
    }
    
    String value = file.readString();
    file.close();
    value.trim();
    return (value == "true");
}

bool SettingsManager::saveInteger(const char* filename, int value) {
    File file = LittleFS.open(filename, "w");
    if (!file) {
        return false;
    }
    file.print(value);
    file.close();
    return true;
}

int SettingsManager::loadInteger(const char* filename, int defaultValue) {
    if (!LittleFS.exists(filename)) {
        return defaultValue;
    }
    
    File file = LittleFS.open(filename, "r");
    if (!file) {
        return defaultValue;
    }
    
    String value = file.readString();
    file.close();
    value.trim();
    return value.toInt();
}

// Clock settings
void SettingsManager::setClockEnabled(bool enabled) {
    clockEnabled = enabled;
    if (saveBoolean(CLOCK_ENABLED_FILE, enabled)) {
        LOG_INFOF(TAG, "💾 Clock setting saved: %s", enabled ? "enabled" : "disabled");
    } else {
        LOG_WARN(TAG, "⚠️ Failed to save clock setting");
    }
}

bool SettingsManager::isClockEnabled() {
    return clockEnabled;
}

void SettingsManager::setClockFace(ClockFaceType faceType) {
    clockFace = faceType;
    if (saveInteger(CLOCK_FACE_FILE, static_cast<int>(faceType))) {
        LOG_INFOF(TAG, "💾 Clock face setting saved: %d", static_cast<int>(faceType));
    } else {
        LOG_WARN(TAG, "⚠️ Failed to save clock face setting");
    }
}

ClockFaceType SettingsManager::getClockFace() {
    return clockFace;
}
