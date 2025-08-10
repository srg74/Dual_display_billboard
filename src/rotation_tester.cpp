#include "rotation_tester.h"
#include <Arduino.h>
#include <LittleFS.h>
#include <TJpg_Decoder.h>
#include <TFT_eSPI.h>

static DisplayManager* g_displayManager = nullptr;
static uint8_t g_currentTestDisplay = 1;

RotationTester::RotationTester(DisplayManager* dm) : displayManager(dm) {
    g_displayManager = dm;
}

RotationTester::~RotationTester() {
    g_displayManager = nullptr;
}

bool RotationTester::begin() {
    if (!displayManager) {
        Serial.println("ROT_TEST: DisplayManager not available");
        return false;
    }
    
    Serial.println("ROT_TEST: ✅ Rotation tester initialized");
    return true;
}

bool RotationTester::testRotation(uint8_t rotation, uint8_t displayNum) {
    if (!displayManager) {
        Serial.println("ROT_TEST: DisplayManager not available");
        return false;
    }
    
    // Find first available image
    String testImage = getFirstAvailableImage();
    if (testImage.isEmpty()) {
        Serial.println("ROT_TEST: No images found");
        return false;
    }
    
    String filepath = "/images/" + testImage;
    Serial.printf("ROT_TEST: Testing rotation %d with image %s\n", rotation, testImage.c_str());
    
    if (!LittleFS.exists(filepath)) {
        Serial.println("ROT_TEST: Image file not found: " + filepath);
        return false;
    }
    
    File file = LittleFS.open(filepath, "r");
    if (!file) {
        Serial.println("ROT_TEST: Failed to open image file");
        return false;
    }
    
    size_t fileSize = file.size();
    uint8_t* buffer = (uint8_t*)malloc(fileSize);
    if (!buffer) {
        Serial.println("ROT_TEST: Failed to allocate memory");
        file.close();
        return false;
    }
    
    file.read(buffer, fileSize);
    file.close();
    
    // Set up TJpg decoder for this specific test
    TJpgDec.setJpgScale(1);
    TJpgDec.setSwapBytes(true);
    g_currentTestDisplay = displayNum; // Set global for callback
    TJpgDec.setCallback([](int16_t x, int16_t y, uint16_t w, uint16_t h, uint16_t* bitmap) -> bool {
        if (!g_displayManager) return false;
        
        // Use the global test display number
        TFT_eSPI* tft = g_displayManager->getTFT(g_currentTestDisplay);
        if (tft) {
            tft->pushImage(x, y, w, h, bitmap);
            return true;
        }
        return false;
    });
    
    // Set rotation DIRECTLY on TFT and display image
    displayManager->selectDisplay(displayNum);
    TFT_eSPI* tft = displayManager->getTFT(displayNum);
    if (tft) {
        tft->setRotation(rotation);  // THIS IS THE KEY TEST
        tft->fillScreen(TFT_BLACK);
        
        // Display image
        bool success = (TJpgDec.drawJpg(0, 0, buffer, fileSize) == JDR_OK);
        
        // Add rotation label
        tft->setTextColor(TFT_YELLOW, TFT_BLACK);
        tft->setTextSize(2);
        String label = "ROT" + String(rotation);
        tft->drawString(label, 10, 10, 2);
        
        Serial.printf("ROT_TEST: Rotation %d %s on display %d\n", rotation, 
                     success ? "SUCCESS" : "FAILED", displayNum);
                     
        displayManager->deselectAll();
        free(buffer);
        return success;
    }
    
    displayManager->deselectAll();
    free(buffer);
    return false;
}

String RotationTester::getFirstAvailableImage() {
    File dir = LittleFS.open("/images");
    if (!dir || !dir.isDirectory()) {
        Serial.println("ROT_TEST: /images directory not found");
        return "";
    }
    
    File file = dir.openNextFile();
    while (file) {
        if (!file.isDirectory()) {
            String filename = file.name();
            int lastSlash = filename.lastIndexOf('/');
            if (lastSlash >= 0) {
                filename = filename.substring(lastSlash + 1);
            }
            
            if (filename.endsWith(".jpg") || filename.endsWith(".jpeg")) {
                Serial.println("ROT_TEST: Found test image: " + filename);
                return filename;
            }
        }
        file = dir.openNextFile();
    }
    
    Serial.println("ROT_TEST: No JPEG images found");
    return "";
}

String RotationTester::getWebInterface() {
    String html = "<!DOCTYPE html><html><head><title>Rotation Tester</title>";
    html += "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">";
    html += "<style>";
    html += "body{font-family:Arial,sans-serif;margin:20px;background-color:#f0f0f0;}";
    html += ".container{max-width:600px;margin:0 auto;background:white;padding:20px;border-radius:10px;box-shadow:0 2px 10px rgba(0,0,0,0.1);}";
    html += "h1{color:#333;text-align:center;}";
    html += ".button-grid{display:grid;grid-template-columns:repeat(2,1fr);gap:15px;margin:20px 0;}";
    html += ".test-button{background-color:#007bff;color:white;border:none;padding:20px;font-size:18px;border-radius:8px;cursor:pointer;}";
    html += ".test-button:hover{background-color:#0056b3;}";
    html += ".info{background-color:#e9ecef;padding:15px;border-radius:5px;margin:10px 0;}";
    html += ".status{margin-top:20px;padding:10px;border-radius:5px;text-align:center;display:none;}";
    html += ".status.success{background-color:#d4edda;color:#155724;border:1px solid #c3e6cb;}";
    html += ".status.error{background-color:#f8d7da;color:#721c24;border:1px solid #f5c6cb;}";
    html += "</style></head><body>";
    html += "<div class=\"container\">";
    html += "<h1>🔄 Rotation Tester</h1>";
    html += "<div class=\"info\">";
    html += "<p><strong>Instructions:</strong></p>";
    html += "<p>Click each button to test the first stored image with different rotations.</p>";
    html += "<p>Note which rotation number displays your image correctly oriented.</p>";
    html += "</div>";
    html += "<div class=\"button-grid\">";
    html += "<button class=\"test-button\" onclick=\"testRotation(0)\">ROT 0</button>";
    html += "<button class=\"test-button\" onclick=\"testRotation(1)\">ROT 1</button>";
    html += "<button class=\"test-button\" onclick=\"testRotation(2)\">ROT 2</button>";
    html += "<button class=\"test-button\" onclick=\"testRotation(3)\">ROT 3</button>";
    html += "</div>";
    html += "<div id=\"status\" class=\"status\"></div>";
    html += "</div>";
    html += "<script>";
    html += "function testRotation(r){";
    html += "var s=document.getElementById('status');";
    html += "s.style.display='block';s.className='status';";
    html += "s.innerHTML='Testing rotation '+r+'...';";
    html += "fetch('/debug/rotation-test?rotation='+r,{method:'GET'})";
    html += ".then(function(response){return response.text();})";
    html += ".then(function(data){s.className='status success';s.innerHTML='Rotation '+r+' test completed! Check your display.';})";
    html += ".catch(function(error){s.className='status error';s.innerHTML='Error testing rotation '+r+': '+error;});";
    html += "}";
    html += "</script>";
    html += "</body></html>";
    return html;
}
