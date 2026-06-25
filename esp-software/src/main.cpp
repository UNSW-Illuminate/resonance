#include <Arduino.h>
#include <WiFi.h>
#include <LittleFS.h>
#include "config/Config.h"
#include "api/WebApi.h"
#include "teensy/TeensyProtocol.h"
#include "wled/WledClient.h"
#include "state/LightingState.h"

void setup() {
    Serial.begin(115200);
    delay(1000);
    Serial.println("Starting Resonance ESP32 Controller...");

    if (!LittleFS.begin(true)) {
        Serial.println("LittleFS Mount Failed");
        return;
    }

    // Configure AP explicitly for better compatibility
    WiFi.mode(WIFI_AP);
    
    // Configure AP IP settings before starting AP
    IPAddress IP(192, 168, 4, 1);
    IPAddress gateway(192, 168, 4, 1);
    IPAddress subnet(255, 255, 255, 0);
    WiFi.softAPConfig(IP, gateway, subnet);
    
    // Start AP with max 10 connections on Channel 1
    WiFi.softAP(Config::AP_SSID, Config::AP_PASSWORD, 1, 0, 10);
    
    Serial.print("AP Started at ");
    Serial.println(WiFi.softAPIP());

    // Initialize Subsystems
    teensyProtocol.begin();
    wledClient.begin();
    webApi.begin();

    // Initial sync
    teensyProtocol.sendState(globalState);
    wledClient.syncState(globalState);
}

void loop() {
    teensyProtocol.loop();
    wledClient.loop();
    delay(5);
}
