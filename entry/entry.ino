#include "NodeGraph.h"
#include "RippleEngine.h"
#include "FastLED.h"
#include "test_json.h"
#include <ArduinoJson.h>
#include <math.h>
#define CLUSTER_CONFIG_LIST(X) \
  X(0, 0) \
  X(1, 1) \
  X(2, 2) \
  X(3, 3) \
  X(4, 4) \
  X(5, 5) \
  X(6, 6) \
  X(7, 7) \
  X(8, 8) \
  X(9, 9) \
  X(10, 10) \
  X(11, 11) \
  X(12, 12) \
  X(13, 13)

#define CLUSTER_PIN_VALUE(clusterId, pin) pin,
#define CLUSTER_COUNT_ENTRY(clusterId, pin) +1

constexpr uint8_t CLUSTER_PIN[] = {CLUSTER_CONFIG_LIST(CLUSTER_PIN_VALUE)};
constexpr uint8_t NUM_CLUSTERS = 0 CLUSTER_CONFIG_LIST(CLUSTER_COUNT_ENTRY);

#define REED_INPUT_1 24

#define NUM_LEDS (MAX_NODES * NUM_LED_PER_NODE)
#define JSON_DOC_CAPACITY 20000

const uint8_t LED_BRIGHTNESS = 180;
const CRGB RIPPLE_PRIMARY_COLOUR(8, 1, 0);
const CRGB RIPPLE_SECONDARY_COLOUR(255, 90, 18);
const float RIPPLE_SHARPNESS = 2.2f;
const float RIPPLE_VISIBILITY_THRESHOLD = 0.10f;
const uint8_t RIPPLE_PEAK_BRIGHTNESS = 220;
const bool RIPPLE_TAKE_OVER = true;

CRGB leds[NUM_CLUSTERS][NUM_LEDS];

NodeGraph graph;
Coord MAP_COORDS[MAX_NODES];
uint16_t MAP_COUNT = 0;

// parameters in order are thickness, tick_delay (ms), maxRadius, bands, speed
RippleEngine ripple(graph, 150.0f, 120, 1500.0f, 1, 80.0f);

String serialLine = "";

bool appendCoordsFromSection(JsonArray section, bool isBush, uint16_t& mapIndex);
bool loadMapCoordsFromJson();
void readSerialTrigger();
void parseTrigger(String line);
void updateLeds();
bool addClusterLeds(uint8_t clusterId);

void setup() {

  pinMode(REED_INPUT_1, INPUT_PULLUP);

  Serial.begin(115200);

  while (!Serial && millis() < 3000) {
    // Allows time for the USB serial monitor to connect on Teensy-style boards.
  }

  if (!loadMapCoordsFromJson()) {
    Serial.println("Failed to load coordinates_sample_test.json");
  }

  for (uint8_t clusterId = 0; clusterId < NUM_CLUSTERS; clusterId++) {
    if (!addClusterLeds(clusterId)) {
      Serial.print("Unsupported cluster pin for cluster ");
      Serial.println(clusterId);
    }
  }

  FastLED.setBrightness(LED_BRIGHTNESS);
  FastLED.clear();
  FastLED.show();

  for (uint16_t i = 0; i < MAP_COUNT; i++) {
    graph.addNode(
      MAP_COORDS[i].row,
      MAP_COORDS[i].col,
      i,
      MAP_COORDS[i].isBush,
      MAP_COORDS[i].clusterId,
      MAP_COORDS[i].clusterIndex
    );
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
  // checkReedInputs();

  if (ripple.readyToTick()) {
    ripple.updatePhysics();
    // ripple.printBrightness();
    updateLeds();
  }
}

void checkReedInputs() {
  if (digitalRead(REED_INPUT_1) == LOW) {
      Serial.println("Reed 1 triggered");
      ripple.trigger(2785, 3184);
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
    MAP_COORDS[mapIndex].clusterId = point["cluster_id"] | 0;
    MAP_COORDS[mapIndex].clusterIndex = point["cluster_index"] | 0;

    Serial.print("Loaded coordinate: row=");
    Serial.print(MAP_COORDS[mapIndex].row);
    Serial.print(", col=");
    Serial.print(MAP_COORDS[mapIndex].col);
    Serial.print(", isBush=");
    Serial.print(MAP_COORDS[mapIndex].isBush);
    Serial.print(", clusterId=");
    Serial.print(MAP_COORDS[mapIndex].clusterId);
    Serial.print(", clusterIndex=");
    Serial.println(MAP_COORDS[mapIndex].clusterIndex);
    mapIndex++;
  }

  return true;
}

bool loadMapCoordsFromJson() {
  Serial.println("Loading coordinates from JSON...");
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

  Serial.print("Triggering ripple at row: ");
  Serial.print(row);
  Serial.print(", col: ");
  Serial.println(col);

  ripple.trigger(row, col);
}

void updateLeds() {
  for (uint8_t clusterId = 0; clusterId < NUM_CLUSTERS; clusterId++) {
    fill_solid(leds[clusterId], NUM_LEDS, RIPPLE_PRIMARY_COLOUR);
  }

  for (uint16_t i = 0; i < graph.count(); i++) {
    Node& node = graph.nodeAt(i);
    // skip reeds that aren't triggered
    

    float t = node.brightness / 255.0f;
    t = constrain(t, 0.0f, 1.0f);

    float visible = 0.0f;
    if (t > RIPPLE_VISIBILITY_THRESHOLD) {
      visible = (t - RIPPLE_VISIBILITY_THRESHOLD) / (1.0f - RIPPLE_VISIBILITY_THRESHOLD);
    }

    float blendFactor = powf(visible, RIPPLE_SHARPNESS);
    uint8_t waveBrightness = (uint8_t)(blendFactor * RIPPLE_PEAK_BRIGHTNESS);

    CRGB nodeColour = RIPPLE_PRIMARY_COLOUR;

    if (RIPPLE_TAKE_OVER) {
      CRGB activeColour = RIPPLE_SECONDARY_COLOUR;
      activeColour.nscale8_video(waveBrightness);
      nodeColour += activeColour;
    } else {
      nodeColour = blend(RIPPLE_PRIMARY_COLOUR, RIPPLE_SECONDARY_COLOUR, waveBrightness);
    }

    if (node.isBush == false) {
      if (node.isTriggeredReed == true) {
        Serial.print("IN ENTRY INO Reed node triggered at row: ");
        Serial.print(node.row);
        Serial.print(", col: ");
        Serial.println(node.col);
        // cycle through all leds in this reed cluster
        for (uint8_t j = 0; j < NUM_LED_PER_NODE; j++) {
          leds[node.clusterId][j] = nodeColour;
        }

        node.isTriggeredReed = false;
      }
      continue;
    }

    uint16_t startLedIndex = node.clusterIndex;
    for (uint8_t j = 0; j < NUM_LED_PER_NODE; j++) {
      uint16_t ledIndex = startLedIndex + j;
      if (ledIndex < NUM_LEDS) {
        leds[node.clusterId][ledIndex] = nodeColour;
      }
    }
  }

  FastLED.show();
}

bool addClusterLeds(uint8_t clusterId) {
  if (clusterId >= NUM_CLUSTERS) {
    return false;
  }

  uint8_t dataPin = CLUSTER_PIN[clusterId];
  bool added = true;

  switch (clusterId) {
#define ADD_CLUSTER_CASE(clusterIdValue, pin) \
    case clusterIdValue:                       \
      FastLED.addLeds<WS2812B, pin, GRB>(      \
        leds[clusterIdValue],                  \
        NUM_LEDS                               \
      );                                       \
      break;
    CLUSTER_CONFIG_LIST(ADD_CLUSTER_CASE)
#undef ADD_CLUSTER_CASE
    default:
      added = false;
      break;
  }

  if (added) {
    Serial.print("Cluster ");
    Serial.print(clusterId);
    Serial.print(" mapped to data pin ");
    Serial.println(dataPin);
  }

  return added;
}
