#include "ResonanceComms.h"

#define DEBUG_RESONANCE_COMMS 1

#if DEBUG_RESONANCE_COMMS
#define DEBUG_PRINT(x) Serial.print(x)
#define DEBUG_PRINTLN(x) Serial.println(x)
#else
#define DEBUG_PRINT(x)
#define DEBUG_PRINTLN(x)
#endif

ResonanceComms::ResonanceComms()
  : port_(nullptr),
    hasStateUpdate_(false),
    hasRippleTrigger_(false),
    rxIndex_(0),
    discardMode_(false),
    isGraphPayload_(false) {
  memset(&state_, 0, sizeof(state_));
  memset(&rippleTrigger_, 0, sizeof(rippleTrigger_));
}

void ResonanceComms::begin(Stream& port) {
  port_ = &port;
}

void ResonanceComms::poll() {
  if (!port_) return;

  while (port_->available() > 0) {
    char ch = port_->read();

    if (ch == '\n') {
      if (isGraphPayload_) {
        isGraphPayload_ = false;
        rxIndex_ = 0;
        DEBUG_PRINTLN("[ResonanceComms] Graph payload ended. Sending ACK.");
        sendAck("graph");
        continue;
      }
      if (discardMode_) {
        // Stop discarding now that we reached a newline
        discardMode_ = false;
      } else if (rxIndex_ > 0) {
        rxBuffer_[rxIndex_] = '\0';
        parseMessage(rxBuffer_);
      }
      rxIndex_ = 0;
      continue;
    }

    if (ch == '\r') {
      continue;
    }

    if (isGraphPayload_ || discardMode_) {
      continue;
    }

    // If buffer is about to overflow, drop the data and enter discard mode
    if (rxIndex_ >= sizeof(rxBuffer_) - 1) {
      discardMode_ = true;
      rxIndex_ = 0;
      DEBUG_PRINTLN("[ResonanceComms] ERROR: UART buffer overflow! Discarding...");
      sendError("UART line exceeded buffer; discarding until newline");
      continue;
    }

    rxBuffer_[rxIndex_++] = ch;

    // Check if this is the start of a graph payload
    if (rxIndex_ == 15) {
      if (strncmp(rxBuffer_, "{\"type\":\"graph\"", 15) == 0) {
        isGraphPayload_ = true;
        DEBUG_PRINTLN("[ResonanceComms] Detected graph payload prefix. Discarding rest of line...");
      }
    }
  }
}

bool ResonanceComms::hasStateUpdate() const { return hasStateUpdate_; }
bool ResonanceComms::hasRippleTrigger() const { return hasRippleTrigger_; }

const LightingCommandState& ResonanceComms::state() const { return state_; }
const RippleTriggerCommand& ResonanceComms::rippleTrigger() const { return rippleTrigger_; }

void ResonanceComms::clearStateUpdate() { hasStateUpdate_ = false; }
void ResonanceComms::clearRippleTrigger() { hasRippleTrigger_ = false; }

void ResonanceComms::sendReady() {
  if (port_) port_->println("{\"type\":\"ready\"}");
}

void ResonanceComms::sendAck(const char* command) {
  if (port_) {
    port_->print("{\"type\":\"ack\",\"command\":\"");
    port_->print(command);
    port_->println("\"}");
  }
}

void ResonanceComms::sendError(const char* message) {
  if (port_) {
    port_->print("{\"type\":\"error\",\"message\":\"");
    port_->print(message);
    port_->println("\"}");
  }
}

void ResonanceComms::parseMessage(const char* line) {
  DEBUG_PRINT("[ResonanceComms] RX: ");
  DEBUG_PRINTLN(line);

  // Use a StaticJsonDocument to avoid heap fragmentation (ArduinoJson v6 syntax)
  StaticJsonDocument<768> doc;
  DeserializationError error = deserializeJson(doc, line);

  if (error) {
    DEBUG_PRINT("[ResonanceComms] ERROR: JSON parse failed: ");
    DEBUG_PRINTLN(error.c_str());
    sendError("JSON parse failed");
    return;
  }

  const char* type = doc["type"];
  if (!type) {
    DEBUG_PRINTLN("[ResonanceComms] ERROR: Missing command type");
    sendError("Missing command type");
    return;
  }

  if (strcmp(type, "state") == 0) {
    state_.enabled = doc["enabled"] | false;
    
    const char* modeStr = doc["mode"] | "off";
    strncpy(state_.mode, modeStr, sizeof(state_.mode) - 1);
    state_.mode[sizeof(state_.mode) - 1] = '\0';
    
    state_.brightness = doc["brightness"] | 0;
    state_.speed = doc["speed"] | 0;
    state_.rippleSize = doc["rippleSize"] | 0;

    JsonArray prim = doc["primaryColor"];
    if (prim && prim.size() == 3) {
      state_.primaryColor[0] = prim[0];
      state_.primaryColor[1] = prim[1];
      state_.primaryColor[2] = prim[2];
    }

    JsonArray sec = doc["secondaryColor"];
    if (sec && sec.size() == 3) {
      state_.secondaryColor[0] = sec[0];
      state_.secondaryColor[1] = sec[1];
      state_.secondaryColor[2] = sec[2];
    }

    JsonArray reed = doc["reedColor"];
    if (reed && reed.size() == 3) {
      state_.reedColor[0] = reed[0];
      state_.reedColor[1] = reed[1];
      state_.reedColor[2] = reed[2];
    }

    state_.autoRipple = doc["autoRipple"] | false;
    state_.autoRippleVariable = doc["autoRippleVariable"] | false;
    state_.autoRippleInterval = doc["autoRippleInterval"] | 5000;

    hasStateUpdate_ = true;
    DEBUG_PRINTLN("[ResonanceComms] State updated successfully.");
    sendAck("state");
  } 
  else if (strcmp(type, "triggerRipple") == 0) {
    rippleTrigger_.bush = doc["bush"] | 0;
    
    rippleTrigger_.hasNodeId = doc.containsKey("nodeId");
    if (rippleTrigger_.hasNodeId) {
      rippleTrigger_.nodeId = doc["nodeId"];
    }

    rippleTrigger_.hasCoords = doc.containsKey("row") && doc.containsKey("col");
    if (rippleTrigger_.hasCoords) {
      rippleTrigger_.row = doc["row"];
      rippleTrigger_.col = doc["col"];
    }

    hasRippleTrigger_ = true;
    DEBUG_PRINTLN("[ResonanceComms] Ripple trigger received.");
    sendAck("triggerRipple");
  } 
  else if (strcmp(type, "graph") == 0) {
    DEBUG_PRINTLN("[ResonanceComms] Graph command received and ignored.");
    // Acknowledge graph receipt but do not process/store it to save RAM
    sendAck("graph");
  } 
  else {
    DEBUG_PRINT("[ResonanceComms] ERROR: Unknown command type: ");
    DEBUG_PRINTLN(type);
    sendError("Unknown command type");
  }
}
