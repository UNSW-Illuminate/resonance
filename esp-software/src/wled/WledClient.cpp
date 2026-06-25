#include "WledClient.h"
#include "../config/Config.h"
#include <HTTPClient.h>
#include <WiFi.h>

WledClient wledClient;

void WledClient::begin() {
    _connected = false;
    _lastError = "";
}

void WledClient::loop() {
    // Background tasks if any
}

String WledClient::buildJsonState(const LightingState& state) {
    StaticJsonDocument<512> doc;
    
    if (!state.enabled || state.mode == "off") {
        doc["on"] = false;
    } else {
        doc["on"] = true;
        
        // Map palettes to neutral static colors
        uint8_t r = state.primaryColor[0];
        uint8_t g = state.primaryColor[1];
        uint8_t b = state.primaryColor[2];
        int fx = 0; // Default to solid

        if (state.mode == "palette_ocean") { r = 0; g = 105; b = 148; } // Ocean Blue
        else if (state.mode == "palette_lava") { r = 255; g = 69; b = 0; } // Orange Red
        else if (state.mode == "palette_forest") { r = 34; g = 139; b = 34; } // Forest Green
        else if (state.mode == "palette_party") { r = 255; g = 20; b = 147; } // Deep Pink
        else if (state.mode == "palette_cloud") { r = 135; g = 206; b = 250; } // Light Sky Blue
        else if (state.mode == "rainbow") { 
            r = 255; g = 255; b = 255; 
            fx = 9; // WLED Rainbow Effect 
        }

        doc["seg"][0]["fx"] = fx;
        doc["seg"][0]["sx"] = state.speed;
        doc["seg"][0]["ix"] = state.rippleSize;

        JsonArray col1 = doc["seg"][0]["col"].createNestedArray();
        col1.add(r);
        col1.add(g);
        col1.add(b);

        JsonArray col2 = doc["seg"][0]["col"].createNestedArray();
        col2.add(state.secondaryColor[0]);
        col2.add(state.secondaryColor[1]);
        col2.add(state.secondaryColor[2]);
    }

    String output;
    serializeJson(doc, output);
    return output;
}

void WledClient::syncState(const LightingState& state) {
    if (!Config::WLED_ENABLED) return;
    String payload = buildJsonState(state);
    sendPost(payload);
}

void WledClient::triggerRipple() {
    if (!Config::WLED_ENABLED) return;
    // Just force an effect refresh or send a minor momentary bump
    // E.g. setting fx again
    StaticJsonDocument<128> doc;
    doc["seg"][0]["fx"] = 0;
    String output;
    serializeJson(doc, output);
    sendPost(output);
}

void WledClient::sendPost(const String& payload) {
    if (WiFi.status() != WL_CONNECTED && WiFi.getMode() != WIFI_AP && WiFi.getMode() != WIFI_AP_STA) {
        _lastError = "WiFi not ready";
        _connected = false;
        return;
    }

    HTTPClient http;
    String url = String("http://") + Config::WLED_HOST + "/json/state";
    
    http.begin(url);
    http.setTimeout(Config::WLED_TIMEOUT_MS);
    http.addHeader("Content-Type", "application/json");

    int httpCode = http.POST(payload);
    
    if (httpCode > 0) {
        if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_NO_CONTENT) {
            _connected = true;
            _lastError = "";
            _lastSyncMs = millis();
        } else {
            _connected = false;
            _lastError = "HTTP Error: " + String(httpCode);
        }
    } else {
        _connected = false;
        _lastError = http.errorToString(httpCode);
    }
    
    http.end();
}
