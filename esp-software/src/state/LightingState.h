#pragma once

#include <Arduino.h>
#include <ArduinoJson.h>

class LightingState {
public:
    bool enabled;
    String mode;
    uint8_t brightness;
    uint8_t speed;
    uint8_t rippleSize;
    uint8_t primaryColor[3];
    uint8_t secondaryColor[3];
    uint8_t reedColor[3];
    uint16_t selectedBush;
    bool autoRipple;
    bool autoRippleVariable;
    uint16_t autoRippleInterval;

    LightingState();

    void loadDefaults();
    bool updateFromJson(const JsonObject& json);
    void toJson(JsonObject& json) const;
};

extern LightingState globalState;
