#pragma once
#include <WiFi.h>
#include <ESPAsyncWebServer.h>

class WiFiManager {
private:
    AsyncWebServer* server;
    String apSSID;
    String apPassword;
    
public:
    WiFiManager(AsyncWebServer* webServer);
    void initializeAP(const String& ssid = "Billboard-Portal", const String& password = "12345678");
    void setupRoutes();
    String scanNetworks();
    bool connectToWiFi(const String& ssid, const String& password);
    void handleConnect(AsyncWebServerRequest* request);
    void checkHeapHealth();
};