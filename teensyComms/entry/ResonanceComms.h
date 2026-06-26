#pragma once

#include <Arduino.h>
#include <ArduinoJson.h>

struct LightingCommandState {
  bool enabled;
  char mode[16];
  uint8_t brightness;
  uint8_t speed;
  uint8_t rippleSize;
  uint8_t primaryColor[3];
  uint8_t secondaryColor[3];
  uint8_t reedColor[3];
  bool autoRipple;
  bool autoRippleVariable;
  uint16_t autoRippleInterval;
};

struct RippleTriggerCommand {
  uint16_t bush;
  uint16_t nodeId;
  int16_t row;
  int16_t col;
  bool hasNodeId;
  bool hasCoords;
};

class ResonanceComms {
public:
  ResonanceComms();
  void begin(Stream& port);
  void poll();

  bool hasStateUpdate() const;
  bool hasRippleTrigger() const;

  const LightingCommandState& state() const;
  const RippleTriggerCommand& rippleTrigger() const;

  void clearStateUpdate();
  void clearRippleTrigger();

  void sendReady();
  void sendAck(const char* command);
  void sendError(const char* message);

private:
  Stream* port_;
  
  LightingCommandState state_;
  RippleTriggerCommand rippleTrigger_;
  
  bool hasStateUpdate_;
  bool hasRippleTrigger_;

  // Buffer state
  static const size_t kMaxLineLength = 4096;
  char rxBuffer_[512]; 
  size_t rxIndex_;
  bool discardMode_;
  bool isGraphPayload_;
  
  void parseMessage(const char* line);
};
