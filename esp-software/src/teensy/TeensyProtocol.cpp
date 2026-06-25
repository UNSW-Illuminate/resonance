#include "TeensyProtocol.h"
#include "../config/Config.h"
#include <ArduinoJson.h>
#include <ESPAsyncWebServer.h>

extern AsyncEventSource events;

TeensyProtocol teensyProtocol;

void TeensyProtocol::begin() {
    Serial2.begin(Config::TEENSY_BAUD, SERIAL_8N1, Config::TEENSY_RX_PIN, Config::TEENSY_TX_PIN);
    _buffer.reserve(256);
}

void TeensyProtocol::loop() {
    while (Serial2.available()) {
        char c = Serial2.read();
        if (c == '\n') {
            parseIncomingLine(_buffer);
            _buffer = "";
        } else if (c != '\r') {
            _buffer += c;
            if (_buffer.length() > 512) {
                // Buffer overflow protection
                _buffer = "";
            }
        }
    }
}

void TeensyProtocol::parseIncomingLine(const String& line) {
    if (line.isEmpty()) return;
    
    StaticJsonDocument<512> doc;
    DeserializationError error = deserializeJson(doc, line);
    
    if (error) {
        Serial.print("Teensy JSON Error: ");
        Serial.println(error.c_str());
        return;
    }
    
    // Log messages from Teensy
    String type = doc["type"] | "unknown";
    Serial.print("Teensy msg: ");
    Serial.println(type);
    
    String uiLog = "Teensy replied: " + type;
    if (doc.containsKey("command")) {
        uiLog += " (" + doc["command"].as<String>() + ")";
    }
    events.send(uiLog.c_str(), "log");
}

void TeensyProtocol::sendState(const LightingState& state) {
    StaticJsonDocument<512> doc;
    JsonObject root = doc.to<JsonObject>();
    root["type"] = "state";
    state.toJson(root);
    
    serializeJson(doc, Serial2);
    Serial2.println();
}

void TeensyProtocol::sendTrigger(uint16_t bush, int16_t row, int16_t col) {
    StaticJsonDocument<128> doc;
    doc["type"] = "triggerRipple";
    doc["bush"] = bush;
    doc["row"] = row;
    doc["col"] = col;
    
    
    serializeJson(doc, Serial2);
    Serial2.println();
}

void TeensyProtocol::sendNodeTrigger(uint16_t nodeId) {
    StaticJsonDocument<64> doc;
    doc["type"] = "triggerRipple";
    doc["nodeId"] = nodeId;
    
    serializeJson(doc, Serial2);
    Serial2.println();
}
