#include <WiFi.h>
#include <WebServer.h>
#include <LittleFS.h>
#include <ArduinoJson.h>

#include "Map.h"
#include "Algorithm.h"
#include "FastLEDInterface.h"
#include "WebUI.h"

// --- Hardware Settings ---
#define LED_PIN     13          // GPIO pin connected to the WS2815 data line
#define COLOR_ORDER GRB         // WS2815 usually uses GRB color ordering
#define CHIPSET     WS2812B     // WS2815 has the same protocol as WS2812B

// --- WiFi Settings ---
const char* AP_SSID = "Resonance-Simulator";
const char* AP_PASSWORD = "resonance-waves"; // Must be at least 8 characters

// --- Globals ---
CRGB leds[MAX_LEDS];
Algorithm algorithm;
WebServer server(80);

Config config;

// Timers
unsigned long lastLedUpdate = 0;
const unsigned long LED_INTERVAL_MS = 16;     // ~60 FPS render rate

// --- Functions ---
void saveConfig();
void loadConfig();
void createDefaultMap();
void applyModeSettings(const String& newMode);
void syncActiveModeSettings();

// --- HTTP Server Handlers ---
void handleRoot() {
    server.send_P(200, "text/html", WEB_UI_HTML);
}

void handleStatus() {
    server.setContentLength(CONTENT_LENGTH_UNKNOWN);
    server.send(200, "application/json", "");
    
    // Config section
    server.sendContent("{\n  \"config\": {\n");
    server.sendContent("    \"mode\": \"" + config.mode + "\",\n");
    server.sendContent("    \"brightness\": " + String(config.brightness) + ",\n");
    server.sendContent("    \"primaryR\": " + String(config.primaryR) + ",\n");
    server.sendContent("    \"primaryG\": " + String(config.primaryG) + ",\n");
    server.sendContent("    \"primaryB\": " + String(config.primaryB) + ",\n");
    server.sendContent("    \"secondaryR\": " + String(config.secondaryR) + ",\n");
    server.sendContent("    \"secondaryG\": " + String(config.secondaryG) + ",\n");
    server.sendContent("    \"secondaryB\": " + String(config.secondaryB) + ",\n");
    server.sendContent("    \"waveSpeed\": " + String(config.waveSpeed, 2) + ",\n");
    server.sendContent("    \"fadeSpeed\": " + String(config.fadeSpeed, 2) + ",\n");
    server.sendContent("    \"sharpness\": " + String(config.sharpness, 2) + ",\n");
    server.sendContent("    \"thickness\": " + String(config.thickness, 2) + ",\n");
    server.sendContent("    \"maxRadius\": " + String(config.maxRadius, 2) + ",\n");
    server.sendContent("    \"numBands\": " + String(config.numBands) + ",\n");
    server.sendContent("    \"takeOver\": " + String(config.takeOver ? "true" : "false") + "\n");
    server.sendContent("  },\n");

    // Map stats section
    int bushes = 0;
    int reeds = 0;
    for (const auto& n : algorithm.nodeMap.nodes) {
        if (n.isBush) bushes++;
        else reeds++;
    }
    server.sendContent("  \"map\": {\n");
    server.sendContent("    \"totalNodes\": " + String(algorithm.nodeMap.nodes.size()) + ",\n");
    server.sendContent("    \"bushes\": " + String(bushes) + ",\n");
    server.sendContent("    \"reeds\": " + String(reeds) + "\n");
    server.sendContent("  },\n");

    // Nodes list section
    server.sendContent("  \"nodes\": [\n");
    for (size_t i = 0; i < algorithm.nodeMap.nodes.size(); ++i) {
        const auto& n = algorithm.nodeMap.nodes[i];
        String nodeStr = "    {\"row\": " + String(n.row) + 
                         ", \"col\": " + String(n.col) + 
                         ", \"isBush\": " + (n.isBush ? "true" : "false") + 
                         ", \"brightness\": " + String(n.brightness, 2) + "}";
        if (i < algorithm.nodeMap.nodes.size() - 1) {
            nodeStr += ",";
        }
        nodeStr += "\n";
        server.sendContent(nodeStr);
    }
    server.sendContent("  ]\n}");
    server.sendContent(""); // End response
}

void handlePostSettings() {
    if (!server.hasArg("plain")) {
        server.send(400, "text/plain", "Body missing");
        return;
    }
    String body = server.arg("plain");
    DynamicJsonDocument doc(1024);
    DeserializationError error = deserializeJson(doc, body);
    if (error) {
        server.send(400, "text/plain", "Invalid JSON settings");
        return;
    }

    if (doc.containsKey("mode")) {
        String newMode = doc["mode"].as<String>();
        if (newMode != config.mode) {
            // Apply parameters from the target mode first
            applyModeSettings(newMode);
        }
    }

    // Now overwrite active parameters if they are in the JSON body
    if (doc.containsKey("brightness")) {
        int val = doc["brightness"].as<int>();
        if (val < 0) val = 0;
        if (val > 255) val = 255;
        config.brightness = val;
        FastLED.setBrightness(config.brightness);
    }
    if (doc.containsKey("primaryR")) config.primaryR = doc["primaryR"].as<uint8_t>();
    if (doc.containsKey("primaryG")) config.primaryG = doc["primaryG"].as<uint8_t>();
    if (doc.containsKey("primaryB")) config.primaryB = doc["primaryB"].as<uint8_t>();
    if (doc.containsKey("secondaryR")) config.secondaryR = doc["secondaryR"].as<uint8_t>();
    if (doc.containsKey("secondaryG")) config.secondaryG = doc["secondaryG"].as<uint8_t>();
    if (doc.containsKey("secondaryB")) config.secondaryB = doc["secondaryB"].as<uint8_t>();
    if (doc.containsKey("waveSpeed")) {
        float val = doc["waveSpeed"].as<float>();
        if (val < 0.1f) val = 0.1f;
        config.waveSpeed = val;
    }
    if (doc.containsKey("fadeSpeed")) {
        float val = doc["fadeSpeed"].as<float>();
        if (val < 0.1f) val = 0.1f;
        config.fadeSpeed = val;
    }
    if (doc.containsKey("sharpness")) {
        float val = doc["sharpness"].as<float>();
        if (val < 1.0f) val = 1.0f;
        config.sharpness = val;
    }
    if (doc.containsKey("thickness")) {
        float val = doc["thickness"].as<float>();
        if (val < 0.5f) val = 0.5f;
        config.thickness = val;
    }
    if (doc.containsKey("maxRadius")) {
        float val = doc["maxRadius"].as<float>();
        if (val < 1.0f) val = 1.0f;
        config.maxRadius = val;
    }
    if (doc.containsKey("numBands")) {
        int val = doc["numBands"].as<int>();
        if (val < 1) val = 1;
        if (val > 10) val = 10;
        config.numBands = val;
    }
    if (doc.containsKey("takeOver")) {
        config.takeOver = doc["takeOver"].as<bool>();
    }

    // Sync active settings back to the active mode configuration
    syncActiveModeSettings();

    // Sync active parameters to the running Algorithm
    algorithm.waveSpeed = config.waveSpeed;
    algorithm.fadeSpeed = config.fadeSpeed;
    algorithm.sharpness = config.sharpness;
    algorithm.thickness = config.thickness;
    algorithm.maxRadius = config.maxRadius;
    algorithm.bands = config.numBands;

    saveConfig();

    server.send(200, "application/json", "{\"status\":\"ok\"}");
}

void handleTrigger() {
    if (!server.hasArg("row") || !server.hasArg("col")) {
        server.send(400, "text/plain", "Missing row/col parameter");
        return;
    }
    float row = server.arg("row").toFloat();
    float col = server.arg("col").toFloat();

    algorithm.triggerLed(row, col);
    server.send(200, "application/json", "{\"status\":\"triggered\",\"row\":" + String(row) + ",\"col\":" + String(col) + "}");
}

void handleGetMap() {
    if (!LittleFS.exists("/map.json")) {
        server.send(200, "application/json", "{\"nodes\":[]}");
        return;
    }
    File file = LittleFS.open("/map.json", "r");
    if (!file) {
        server.send(500, "text/plain", "Failed to open map file");
        return;
    }
    server.streamFile(file, "application/json");
    file.close();
}

void handlePostMap() {
    if (!server.hasArg("plain")) {
        server.send(400, "text/plain", "Body missing");
        return;
    }
    String body = server.arg("plain");

    if (!algorithm.nodeMap.readJSON(body)) {
        server.send(400, "text/plain", "Invalid map JSON format");
        return;
    }

    // Save map file to flash
    File file = LittleFS.open("/map.json", "w");
    if (!file) {
        server.send(500, "text/plain", "Failed to write map to filesystem");
        return;
    }
    file.print(body);
    file.close();

    server.send(200, "application/json", "{\"status\":\"ok\",\"nodesLoaded\":" + String(algorithm.nodeMap.nodes.size()) + "}");
}

void setup() {
    Serial.begin(115200);
    delay(1000);
    Serial.println("\n--- Resonance Simulator ESP32 Starting ---");

    // Initialize LittleFS
    if (!LittleFS.begin(true)) {
        Serial.println("LittleFS Mount Failed, formatting...");
    }

    // Load configurations and map
    loadConfig();

    if (!LittleFS.exists("/map.json")) {
        createDefaultMap();
    }

    // Load map layout
    File mapFile = LittleFS.open("/map.json", "r");
    if (mapFile) {
        String mapJson = mapFile.readString();
        mapFile.close();
        algorithm.nodeMap.readJSON(mapJson);
    } else {
        Serial.println("No map.json found, loading empty map.");
    }

    // Initialize FastLED
    FastLED.addLeds<CHIPSET, LED_PIN, COLOR_ORDER>(leds, MAX_LEDS);
    FastLED.setBrightness(config.brightness);
    fill_solid(leds, MAX_LEDS, CRGB::Black);
    FastLED.show();
    
    // Initialize lastLedUpdate to prevent large delta jumps
    lastLedUpdate = millis();
    
    Serial.print("FastLED initialized on Pin ");
    Serial.print(LED_PIN);
    Serial.print(" with MAX_LEDS: ");
    Serial.println(MAX_LEDS);

    // Setup WiFi Access Point
    WiFi.softAP(AP_SSID, AP_PASSWORD);
    IPAddress IP = WiFi.softAPIP();
    Serial.print("WiFi AP started. SSID: ");
    Serial.println(AP_SSID);
    Serial.print("AP IP address: ");
    Serial.println(IP);

    // Register HTTP handlers
    server.on("/", HTTP_GET, handleRoot);
    server.on("/api/status", HTTP_GET, handleStatus);
    server.on("/api/settings", HTTP_POST, handlePostSettings);
    server.on("/api/trigger", HTTP_POST, handleTrigger);
    server.on("/api/map", HTTP_GET, handleGetMap);
    server.on("/api/map", HTTP_POST, handlePostMap);

    server.begin();
    Serial.println("HTTP server started.");
}

void loop() {
    server.handleClient();

    unsigned long now = millis();

    // Physics and LED Update Loop (16ms interval, ~60 FPS)
    if (now - lastLedUpdate >= LED_INTERVAL_MS) {
        float dt = (now - lastLedUpdate) / 1000.0f; // Time delta in seconds
        lastLedUpdate = now;

        // Safety clamp for dt to prevent giant jumps if the board hangs briefly
        if (dt > 0.2f) dt = 0.2f;

        algorithm.updatePhysics(dt);
        FastLEDInterface::displayMap(algorithm.nodeMap);
    }
}

void saveConfig() {
    DynamicJsonDocument doc(4096); // Space for nested configuration
    doc["mode"] = config.mode;
    
    // Save current active config parameters
    doc["brightness"] = config.brightness;
    doc["primaryR"] = config.primaryR;
    doc["primaryG"] = config.primaryG;
    doc["primaryB"] = config.primaryB;
    doc["secondaryR"] = config.secondaryR;
    doc["secondaryG"] = config.secondaryG;
    doc["secondaryB"] = config.secondaryB;
    doc["waveSpeed"] = config.waveSpeed;
    doc["fadeSpeed"] = config.fadeSpeed;
    doc["sharpness"] = config.sharpness;
    doc["thickness"] = config.thickness;
    doc["maxRadius"] = config.maxRadius;
    doc["numBands"] = config.numBands;
    doc["takeOver"] = config.takeOver;

    // Helper lambda to save mode settings
    auto saveMode = [&](const char* name, const ModeConfig& mc) {
        JsonObject obj = doc.createNestedObject(name);
        obj["brightness"] = mc.brightness;
        obj["primaryR"] = mc.primaryR;
        obj["primaryG"] = mc.primaryG;
        obj["primaryB"] = mc.primaryB;
        obj["secondaryR"] = mc.secondaryR;
        obj["secondaryG"] = mc.secondaryG;
        obj["secondaryB"] = mc.secondaryB;
        obj["waveSpeed"] = mc.waveSpeed;
        obj["fadeSpeed"] = mc.fadeSpeed;
        obj["sharpness"] = mc.sharpness;
        obj["thickness"] = mc.thickness;
        obj["maxRadius"] = mc.maxRadius;
        obj["numBands"] = mc.numBands;
        obj["takeOver"] = mc.takeOver;
    };

    saveMode("ripple", config.ripple);
    saveMode("solid", config.solid);
    saveMode("rainbow", config.rainbow);
    saveMode("glitter", config.glitter);
    saveMode("breathing", config.breathing);

    File file = LittleFS.open("/config.json", "w");
    if (file) {
        serializeJson(doc, file);
        file.close();
        Serial.println("Saved config to LittleFS.");
    }
}

void loadConfig() {
    if (!LittleFS.exists("/config.json")) {
        Serial.println("No config.json found, using defaults.");
        syncActiveModeSettings();
        return;
    }
    File file = LittleFS.open("/config.json", "r");
    if (!file) return;

    DynamicJsonDocument doc(4096);
    DeserializationError error = deserializeJson(doc, file);
    file.close();

    if (error) {
        Serial.println("Failed to parse config.json, using defaults.");
        syncActiveModeSettings();
        return;
    }

    // Helper lambda to load mode settings
    auto loadMode = [&](const char* name, ModeConfig& mc) {
        if (doc.containsKey(name)) {
            JsonObject obj = doc[name];
            if (obj.containsKey("brightness")) mc.brightness = obj["brightness"];
            if (obj.containsKey("primaryR")) mc.primaryR = obj["primaryR"];
            if (obj.containsKey("primaryG")) mc.primaryG = obj["primaryG"];
            if (obj.containsKey("primaryB")) mc.primaryB = obj["primaryB"];
            if (obj.containsKey("secondaryR")) mc.secondaryR = obj["secondaryR"];
            if (obj.containsKey("secondaryG")) mc.secondaryG = obj["secondaryG"];
            if (obj.containsKey("secondaryB")) mc.secondaryB = obj["secondaryB"];
            if (obj.containsKey("waveSpeed")) mc.waveSpeed = obj["waveSpeed"];
            if (obj.containsKey("fadeSpeed")) mc.fadeSpeed = obj["fadeSpeed"];
            if (obj.containsKey("sharpness")) mc.sharpness = obj["sharpness"];
            if (obj.containsKey("thickness")) mc.thickness = obj["thickness"];
            if (obj.containsKey("maxRadius")) mc.maxRadius = obj["maxRadius"];
            if (obj.containsKey("numBands")) mc.numBands = obj["numBands"];
            if (obj.containsKey("takeOver")) mc.takeOver = obj["takeOver"];
        }
    };

    loadMode("ripple", config.ripple);
    loadMode("solid", config.solid);
    loadMode("rainbow", config.rainbow);
    loadMode("glitter", config.glitter);
    loadMode("breathing", config.breathing);

    if (doc.containsKey("mode")) config.mode = doc["mode"].as<String>();
    
    // Apply mode settings for active mode
    applyModeSettings(config.mode);

    Serial.println("Loaded config from LittleFS.");
}

void applyModeSettings(const String& newMode) {
    config.mode = newMode;
    ModeConfig* m = nullptr;
    if (newMode == "ripple") m = &config.ripple;
    else if (newMode == "solid") m = &config.solid;
    else if (newMode == "rainbow") m = &config.rainbow;
    else if (newMode == "glitter") m = &config.glitter;
    else if (newMode == "breathing") m = &config.breathing;
    
    if (m != nullptr) {
        config.brightness = m->brightness;
        config.primaryR = m->primaryR;
        config.primaryG = m->primaryG;
        config.primaryB = m->primaryB;
        config.secondaryR = m->secondaryR;
        config.secondaryG = m->secondaryG;
        config.secondaryB = m->secondaryB;
        config.waveSpeed = m->waveSpeed;
        config.fadeSpeed = m->fadeSpeed;
        config.sharpness = m->sharpness;
        config.thickness = m->thickness;
        config.maxRadius = m->maxRadius;
        config.numBands = m->numBands;
        config.takeOver = m->takeOver;
        
        // Sync with FastLED and Algorithm
        FastLED.setBrightness(config.brightness);
        algorithm.waveSpeed = config.waveSpeed;
        algorithm.fadeSpeed = config.fadeSpeed;
        algorithm.sharpness = config.sharpness;
        algorithm.thickness = config.thickness;
        algorithm.maxRadius = config.maxRadius;
        algorithm.bands = config.numBands;
    }
}

void syncActiveModeSettings() {
    ModeConfig* m = nullptr;
    if (config.mode == "ripple") m = &config.ripple;
    else if (config.mode == "solid") m = &config.solid;
    else if (config.mode == "rainbow") m = &config.rainbow;
    else if (config.mode == "glitter") m = &config.glitter;
    else if (config.mode == "breathing") m = &config.breathing;
    
    if (m != nullptr) {
        m->brightness = config.brightness;
        m->primaryR = config.primaryR;
        m->primaryG = config.primaryG;
        m->primaryB = config.primaryB;
        m->secondaryR = config.secondaryR;
        m->secondaryG = config.secondaryG;
        m->secondaryB = config.secondaryB;
        m->waveSpeed = config.waveSpeed;
        m->fadeSpeed = config.fadeSpeed;
        m->sharpness = config.sharpness;
        m->thickness = config.thickness;
        m->maxRadius = config.maxRadius;
        m->numBands = config.numBands;
        m->takeOver = config.takeOver;
    }
}

void createDefaultMap() {
    String defaultMap = R"rawjson(
{
  "bushes": [
    {"row": 0, "col": 0}, {"row": 4, "col": 12}
  ],
  "reeds": [
    {"row": 0, "col": 4}, {"row": 0, "col": 8}, {"row": 0, "col": 12}, {"row": 0, "col": 16}, {"row": 0, "col": 20},
    {"row": 2, "col": 2}, {"row": 2, "col": 6}, {"row": 2, "col": 10}, {"row": 2, "col": 14}, {"row": 2, "col": 18},
    {"row": 4, "col": 0}, {"row": 4, "col": 4}, {"row": 4, "col": 8}, {"row": 4, "col": 16}, {"row": 4, "col": 20}
  ]
}
)rawjson";
    
    File file = LittleFS.open("/map.json", "w");
    if (file) {
        file.print(defaultMap);
        file.close();
        Serial.println("Created default map.json in LittleFS.");
    }
}
