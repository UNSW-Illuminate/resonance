#include "NodeGraph.h"
#include "RippleEngine.h"
#include "FastLED.h"
#include "coordinates_final_json.h"
#include <ArduinoJson.h>
#include <math.h>

#define DATA_PIN 6
#define NUM_LEDS (MAX_NODES * NUM_LED_PER_NODE)
#define JSON_DOC_CAPACITY 16384

const CRGB RIPPLE_PRIMARY_COLOUR(0, 40, 90);
const CRGB RIPPLE_SECONDARY_COLOUR(0, 180, 255);
const float RIPPLE_SHARPNESS = 2.0f;
const bool RIPPLE_TAKE_OVER = false;

CRGB leds[NUM_LEDS];

NodeGraph graph;
Coord MAP_COORDS[MAX_NODES];
uint16_t MAP_COUNT = 0;

// Mirrors: RippleSimulator(5, 0.1, 20, 2, 1)
// Python tick_delay is in seconds. Arduino uses milliseconds, so 0.1s -> 100ms.
// parameters in order are thickness, tick_delay (ms), maxRadius, bands, speed
RippleEngine ripple(graph, 300f, 200, 2.0f, 1, 0.5f);

String serialLine = "";

bool appendCoordsFromSection(JsonArray section, bool isBush, uint16_t& mapIndex);
bool loadMapCoordsFromJson();
void readSerialTrigger();
void parseTrigger(String line);
void updateLeds();

void setup() {
  Serial.begin(115200);

  while (!Serial && millis() < 3000) {
    // Allows time for the USB serial monitor to connect on Teensy-style boards.
  }

  if (!loadMapCoordsFromJson()) {
    Serial.println("Failed to load coordinates_final.json");
  }

  FastLED.addLeds<WS2812B, DATA_PIN, GRB>(leds, NUM_LEDS);
  FastLED.clear();
  FastLED.show();

  for (uint16_t i = 0; i < MAP_COUNT; i++) {
    graph.addNode(MAP_COORDS[i].row, MAP_COORDS[i].col, i, MAP_COORDS[i].isBush);
  }

  ripple.begin();

  Serial.println("Ripple simulator ready.");
  Serial.print("Loaded nodes: ");
  Serial.println(MAP_COUNT);
  Serial.println("Send trigger as: row,col");
  Serial.println("Example: 3002,2988");
}

void loop() {
  readSerialTrigger();

  if (ripple.readyToTick()) {
    ripple.updatePhysics();
    ripple.printBrightness();
    updateLeds();
  }
}

bool appendCoordsFromSection(JsonArray section, bool isBush, uint16_t& mapIndex) {
  for (JsonObject point : section) {
    if (mapIndex >= MAX_NODES) {
      Serial.println("Coordinate list exceeded MAX_NODES.");
      return false;
    }

    MAP_COORDS[mapIndex].row = (int16_t)lroundf(point["y"].as<float>());
    MAP_COORDS[mapIndex].col = (int16_t)lroundf(point["x"].as<float>());
    MAP_COORDS[mapIndex].isBush = isBush;
    mapIndex++;
  }

  return true;
}

bool loadMapCoordsFromJson() {
  DynamicJsonDocument doc(JSON_DOC_CAPACITY);
  DeserializationError error = deserializeJson(doc, COORDINATES_JSON);

  if (error) {
    Serial.print("JSON parse failed: ");
    Serial.println(error.c_str());
    MAP_COUNT = 0;
    return false;
  }

  uint16_t mapIndex = 0;
  bool bushesLoaded = appendCoordsFromSection(doc["bushes"].as<JsonArray>(), true, mapIndex);
  bool reedsLoaded = appendCoordsFromSection(doc["reeds"].as<JsonArray>(), false, mapIndex);

  MAP_COUNT = mapIndex;
  return bushesLoaded && reedsLoaded;
}

void readSerialTrigger() {
  while (Serial.available() > 0) {
    char ch = Serial.read();

    if (ch == '\n') {
      parseTrigger(serialLine);
      serialLine = "";
    } else if (ch != '\r') {
      serialLine += ch;
    }
  }
}

void parseTrigger(String line) {
  int commaIndex = line.indexOf(',');

  if (commaIndex == -1) {
    return;
  }

  int row = line.substring(0, commaIndex).toInt();
  int col = line.substring(commaIndex + 1).toInt();

  ripple.trigger(row, col);
}

void updateLeds() {
  FastLED.clear();

  for (uint16_t i = 0; i < graph.count(); i++) {
    Node& node = graph.nodeAt(i);
    float t = node.brightness / 255.0f;
    t = constrain(t, 0.0f, 1.0f);

    float blendFactor = powf(t, RIPPLE_SHARPNESS);

    uint8_t r = 0;
    uint8_t g = 0;
    uint8_t b = 0;

    if (RIPPLE_TAKE_OVER && t > 0.01f) {
      r = (uint8_t)(blendFactor * RIPPLE_SECONDARY_COLOUR.r);
      g = (uint8_t)(blendFactor * RIPPLE_SECONDARY_COLOUR.g);
      b = (uint8_t)(blendFactor * RIPPLE_SECONDARY_COLOUR.b);
    } else {
      r = (uint8_t)((1.0f - blendFactor) * RIPPLE_PRIMARY_COLOUR.r + blendFactor * RIPPLE_SECONDARY_COLOUR.r);
      g = (uint8_t)((1.0f - blendFactor) * RIPPLE_PRIMARY_COLOUR.g + blendFactor * RIPPLE_SECONDARY_COLOUR.g);
      b = (uint8_t)((1.0f - blendFactor) * RIPPLE_PRIMARY_COLOUR.b + blendFactor * RIPPLE_SECONDARY_COLOUR.b);
    }

    CRGB nodeColour(r, g, b);

    for (uint8_t j = 0; j < NUM_LED_PER_NODE; j++) {
      uint16_t ledIndex = node.ledIndex + j;
      if (ledIndex < NUM_LEDS) {
        leds[ledIndex] = nodeColour;
      }
    }
  }

  FastLED.show();
}
