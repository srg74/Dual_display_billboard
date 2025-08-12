#include <Arduino.h>
#include <TFT_eSPI.h>

TFT_eSPI tft = TFT_eSPI();

void setup(void) {
  Serial.begin(115200);
  delay(2000);
  
  Serial.println("ESP32-S3 ST7735 Connection Test");
  Serial.println("================================");
  Serial.println("Pin Configuration:");
  Serial.println("CS  (Chip Select):  GPIO 10");
  Serial.println("DC  (Data/Command): GPIO 14");
  Serial.println("RST (Reset):        GPIO 4");
  Serial.println("MOSI (Data Out):    GPIO 11");
  Serial.println("SCLK (Clock):       GPIO 12");
  Serial.println("MISO (Data In):     GPIO 13");
  Serial.println("BL  (Backlight):    GPIO 7");
  Serial.println("VCC: 3.3V");
  Serial.println("GND: Ground");
  Serial.println("================================");
  
  // Test backlight
  Serial.println("Testing backlight...");
  pinMode(7, OUTPUT);
  digitalWrite(7, HIGH);
  Serial.println("Backlight should be ON now");
  delay(2000);
  
  digitalWrite(7, LOW);
  Serial.println("Backlight should be OFF now");
  delay(2000);
  
  digitalWrite(7, HIGH);
  Serial.println("Backlight ON again");
  
  // Test display initialization
  Serial.println("Initializing display...");
  tft.init();
  Serial.println("Display init complete");
  
  // Test basic display functions
  Serial.println("Testing display output...");
  
  tft.fillScreen(TFT_BLACK);
  delay(1000);
  
  tft.fillScreen(TFT_RED);
  Serial.println("Screen should be RED");
  delay(2000);
  
  tft.fillScreen(TFT_GREEN);
  Serial.println("Screen should be GREEN");
  delay(2000);
  
  tft.fillScreen(TFT_BLUE);
  Serial.println("Screen should be BLUE");
  delay(2000);
  
  tft.fillScreen(TFT_WHITE);
  Serial.println("Screen should be WHITE");
  delay(2000);
  
  // Display connection verification
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_WHITE);
  tft.setTextSize(1);
  tft.setCursor(0, 0);
  tft.println("ST7735 Test");
  tft.println("80x160 Display");
  tft.println("");
  tft.println("If you can read");
  tft.println("this text,");
  tft.println("connections");
  tft.println("are OK!");
  
  Serial.println("Connection test complete!");
  Serial.println("If display shows text, all connections are working.");
}

void loop() {
  delay(10000);
  Serial.println("Test running... Check display for text output.");
}
