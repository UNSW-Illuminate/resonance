#pragma once

#include <Arduino.h>
#include "../state/LightingState.h"

class WledClient {
public:
    void begin();
    void loop();
    void syncState(const LightingState& state);
    void triggerRipple();

    bool isConnected() const { return _connected; }
    String getLastError() const { return _lastError; }
    uint32_t getLastSyncMs() const { return _lastSyncMs; }

private:
    void sendPost(const String& payload);
    String buildJsonState(const LightingState& state);

    bool _connected = false;
    String _lastError = "";
    uint32_t _lastSyncMs = 0;
};

extern WledClient wledClient;
