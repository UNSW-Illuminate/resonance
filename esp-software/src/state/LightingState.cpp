#include "LightingState.h"

LightingState globalState;

LightingState::LightingState() {
    loadDefaults();
}

void LightingState::loadDefaults() {
    enabled = true;
    mode = "static";
    brightness = 200;
    speed = 50;
    rippleSize = 32;
    primaryColor[0] = 255; primaryColor[1] = 0;   primaryColor[2] = 0;
    secondaryColor[0] = 0; secondaryColor[1] = 180; secondaryColor[2] = 255;
    reedColor[0] = 0; reedColor[1] = 0; reedColor[2] = 255;
    selectedBush = 0;
    autoRipple = false;
    autoRippleVariable = false;
    autoRippleInterval = 5000;
}

bool LightingState::updateFromJson(const JsonObject& json) {
    bool changed = false;

    if (json.containsKey("enabled") && enabled != json["enabled"].as<bool>()) {
        enabled = json["enabled"].as<bool>();
        changed = true;
    }
    if (json.containsKey("mode") && mode != json["mode"].as<String>()) {
        mode = json["mode"].as<String>();
        changed = true;
    }
    if (json.containsKey("brightness") && brightness != json["brightness"].as<uint8_t>()) {
        brightness = json["brightness"].as<uint8_t>();
        changed = true;
    }
    if (json.containsKey("speed") && speed != json["speed"].as<uint8_t>()) {
        speed = json["speed"].as<uint8_t>();
        changed = true;
    }
    if (json.containsKey("rippleSize") && rippleSize != json["rippleSize"].as<uint8_t>()) {
        rippleSize = json["rippleSize"].as<uint8_t>();
        changed = true;
    }
    if (json.containsKey("primaryColor")) {
        JsonArray pc = json["primaryColor"].as<JsonArray>();
        if (pc.size() == 3) {
            primaryColor[0] = pc[0];
            primaryColor[1] = pc[1];
            primaryColor[2] = pc[2];
            changed = true;
        }
    }
    if (json.containsKey("secondaryColor")) {
        JsonArray sc = json["secondaryColor"].as<JsonArray>();
        if (sc.size() == 3) {
            secondaryColor[0] = sc[0];
            secondaryColor[1] = sc[1];
            secondaryColor[2] = sc[2];
            changed = true;
        }
    }
    if (json.containsKey("reedColor")) {
        JsonArray rc = json["reedColor"].as<JsonArray>();
        if (rc.size() == 3) {
            reedColor[0] = rc[0];
            reedColor[1] = rc[1];
            reedColor[2] = rc[2];
            changed = true;
        }
    }
    if (json.containsKey("selectedBush") && selectedBush != json["selectedBush"].as<uint16_t>()) {
        selectedBush = json["selectedBush"].as<uint16_t>();
        changed = true;
    }
    if (json.containsKey("autoRipple") && autoRipple != json["autoRipple"].as<bool>()) {
        autoRipple = json["autoRipple"].as<bool>();
        changed = true;
    }
    if (json.containsKey("autoRippleVariable") && autoRippleVariable != json["autoRippleVariable"].as<bool>()) {
        autoRippleVariable = json["autoRippleVariable"].as<bool>();
        changed = true;
    }
    if (json.containsKey("autoRippleInterval") && autoRippleInterval != json["autoRippleInterval"].as<uint16_t>()) {
        autoRippleInterval = json["autoRippleInterval"].as<uint16_t>();
        changed = true;
    }

    return changed;
}

void LightingState::toJson(JsonObject& json) const {
    json["enabled"] = enabled;
    json["mode"] = mode;
    json["brightness"] = brightness;
    json["speed"] = speed;
    json["rippleSize"] = rippleSize;
    
    JsonArray pc = json.createNestedArray("primaryColor");
    pc.add(primaryColor[0]);
    pc.add(primaryColor[1]);
    pc.add(primaryColor[2]);

    JsonArray sc = json.createNestedArray("secondaryColor");
    sc.add(secondaryColor[0]);
    sc.add(secondaryColor[1]);
    sc.add(secondaryColor[2]);

    JsonArray rc = json.createNestedArray("reedColor");
    rc.add(reedColor[0]);
    rc.add(reedColor[1]);
    rc.add(reedColor[2]);

    json["selectedBush"] = selectedBush;
    json["autoRipple"] = autoRipple;
    json["autoRippleVariable"] = autoRippleVariable;
    json["autoRippleInterval"] = autoRippleInterval;
}
