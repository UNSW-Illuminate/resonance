#include "RippleEngine.h"
#include <math.h>

static const float TWO_PI_F = 6.28318530718f;

RippleEngine::RippleEngine(
  NodeGraph& graph,
  float thickness,
  unsigned long tickDelayMs,
  float maxRadius,
  uint8_t bands,
  float speed
)
  : graph(graph),
    thickness(thickness),
    tickDelayMs(tickDelayMs),
    maxRadius(maxRadius),
    bandSpace(0.0f),
    hasBandSpace(false),
    valueRange(255),
    numBands(bands),
    wavelength(thickness * 2.0f),
    waveSpeed(speed),
    backoffDurationMs(2000),
    lastTickMs(0),
    activeRippleCount(0) {}

void RippleEngine::begin() {
  lastTickMs = millis();
}

bool RippleEngine::readyToTick() const {
  return millis() - lastTickMs >= tickDelayMs;
}

bool RippleEngine::trigger(int16_t row, int16_t col) {
  Node* source = graph.getNode(row, col);
  if (source == nullptr) {
    Serial.print("Node not found at row: ");
    Serial.print(row);
    Serial.print(", col: ");
    Serial.println(col);
    return false;
  }
  if (source->isBush == false) {
    // is reed
    source->isTriggeredReed = true;
    Serial.print("IN RIPPLE ENGINE Reed node triggered at row: ");
    Serial.print(row);
    Serial.print(", col: ");
    Serial.println(col);
  }

  unsigned long now = millis();

  for (uint8_t i = 0; i < activeRippleCount; i++) {
    const Ripple& ripple = activeRipples[i];
    bool sameSource = ripple.sourceRow == row && ripple.sourceCol == col;
    bool withinBackoff = now - ripple.startMs < backoffDurationMs;

    if (sameSource && withinBackoff) {
      return false;
    }
  }

  uint8_t secondaryBands = numBands > 0 ? numBands - 1 : 0;

  return appendRipple(
    activeRipples,
    activeRippleCount,
    row,
    col,
    0.0f,
    now,
    secondaryBands
  );
}

void RippleEngine::updatePhysics() {
  lastTickMs = millis();
  graph.clearBrightness();

  Ripple nextRipples[MAX_RIPPLES];
  uint8_t nextRippleCount = 0;

  for (uint8_t i = 0; i < activeRippleCount; i++) {
    Ripple ripple = activeRipples[i];

    float waveFront = ripple.age * waveSpeed;
    uint8_t bandsStarted = numBands - ripple.bandsRemaining;
    float buffer = 1.5f;
    float spacing = hasBandSpace ? bandSpace : thickness;

    if (
      ripple.bandsRemaining > 0 &&
      waveFront > spacing * bandsStarted * buffer
    ) {
      appendRipple(
        nextRipples,
        nextRippleCount,
        ripple.sourceRow,
        ripple.sourceCol,
        0.0f,
        millis(),
        0
      );

      ripple.bandsRemaining--;
    }

    for (uint16_t j = 0; j < graph.count(); j++) {
      Node& node = graph.nodeAt(j);

      float dist = distanceToNode(ripple.sourceRow, ripple.sourceCol, node);
      if (dist > maxRadius) {
        continue;
      }

      float offset = dist - waveFront;
      float envelope = 1.0f - fabsf(offset) / thickness;

      if (envelope <= 0.0f) {
        continue;
      }

      float phase = TWO_PI_F * offset / wavelength;
      float rawBrightness = envelope * ((sinf(phase) + 1.0f) / 2.0f) * valueRange;

      int brightness = (int)roundf(rawBrightness);
      brightness = constrain(brightness, 0, valueRange);

      if (brightness > node.brightness) {
        node.brightness = (uint8_t)brightness;
      }
    }

    ripple.age += 1.0f;

    if (waveFront <= maxRadius) {
      appendRipple(
        nextRipples,
        nextRippleCount,
        ripple.sourceRow,
        ripple.sourceCol,
        ripple.age,
        ripple.startMs,
        ripple.bandsRemaining
      );
    }
  }

  activeRippleCount = nextRippleCount;

  for (uint8_t i = 0; i < nextRippleCount; i++) {
    activeRipples[i] = nextRipples[i];
  }
}

void RippleEngine::printBrightness() const {
  Serial.println("---- brightness ----");

  for (uint16_t i = 0; i < graph.count(); i++) {
    const Node& node = graph.nodeAt(i);

    Serial.print('(');
    Serial.print(node.row);
    Serial.print(", ");
    Serial.print(node.col);
    Serial.print(") LED ");
    Serial.print(node.ledIndex);
    Serial.print(": ");
    Serial.println(node.brightness);
  }
}

void RippleEngine::setBandSpace(float newBandSpace) {
  bandSpace = newBandSpace;
  hasBandSpace = true;
}

void RippleEngine::clearBandSpace() {
  bandSpace = 0.0f;
  hasBandSpace = false;
}

bool RippleEngine::appendRipple(
  Ripple* rippleList,
  uint8_t& rippleCount,
  int16_t row,
  int16_t col,
  float age,
  unsigned long startMs,
  uint8_t bandsRemaining
) {
  if (rippleCount >= MAX_RIPPLES) {
    return false;
  }

  Ripple& ripple = rippleList[rippleCount++];
  ripple.sourceRow = row;
  ripple.sourceCol = col;
  ripple.age = age;
  ripple.startMs = startMs;
  ripple.bandsRemaining = bandsRemaining;

  return true;
}

float RippleEngine::distanceToNode(int16_t sourceRow, int16_t sourceCol, const Node& node) const {
  float dr = (float)sourceRow - (float)node.row;
  float dc = (float)sourceCol - (float)node.col;
  return sqrtf(dr * dr + dc * dc);
}
