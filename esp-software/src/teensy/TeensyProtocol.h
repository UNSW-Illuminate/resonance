#pragma once

#include <Arduino.h>
#include "../state/LightingState.h"

class TeensyProtocol {
public:
    void begin();
    void loop();
    void sendState(const LightingState& state);
    void sendTrigger(uint16_t bush, int16_t row, int16_t col);
    void sendNodeTrigger(uint16_t nodeId);

private:
    void parseIncomingLine(const String& line);
    String _buffer;
};

extern TeensyProtocol teensyProtocol;
