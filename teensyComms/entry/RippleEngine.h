#pragma once

#include <Arduino.h>
#include "NodeGraph.h"

#define MAX_RIPPLES 16

struct Ripple {
  int16_t sourceRow;
  int16_t sourceCol;
  float age;
  unsigned long startMs;
  uint8_t bandsRemaining;
};

class RippleEngine {
public:
  RippleEngine(
    NodeGraph& graph,
    float thickness,
    unsigned long tickDelayMs,
    float maxRadius,
    uint8_t bands,
    float speed
  );

  void begin();
  bool readyToTick() const;
  bool trigger(int16_t row, int16_t col);
  void updatePhysics();
  void printBrightness() const;

  void setBandSpace(float newBandSpace);
  void clearBandSpace();

private:
  NodeGraph& graph;

  float thickness;
  unsigned long tickDelayMs;
  float maxRadius;
  float bandSpace;
  bool hasBandSpace;

  uint8_t valueRange;
  uint8_t numBands;
  float wavelength;
  float waveSpeed;

  unsigned long backoffDurationMs;
  unsigned long lastTickMs;

  Ripple activeRipples[MAX_RIPPLES];
  uint8_t activeRippleCount;

  bool appendRipple(
    Ripple* rippleList,
    uint8_t& rippleCount,
    int16_t row,
    int16_t col,
    float age,
    unsigned long startMs,
    uint8_t bandsRemaining
  );

  float distanceToNode(int16_t sourceRow, int16_t sourceCol, const Node& node) const;
};
