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
  X(6, 6)

#define CLUSTER_PIN_VALUE(clusterId, pin) pin,
#define CLUSTER_COUNT_ENTRY(clusterId, pin) +1

constexpr uint8_t CLUSTER_PIN[] = {CLUSTER_CONFIG_LIST(CLUSTER_PIN_VALUE)};
constexpr uint8_t NUM_CLUSTERS = 0 CLUSTER_CONFIG_LIST(CLUSTER_COUNT_ENTRY);

#define REED_OUTPUT_1 8

#define NUM_LEDS (MAX_NODES * NUM_LED_PER_NODE)
#define JSON_DOC_CAPACITY 20000

struct FastLedModeConfig {
  const char* name;
  uint8_t brightness;
  CRGB primaryColour;
  CRGB secondaryColour;
  float sharpness;
  float visibilityThreshold;
  uint8_t peakBrightness;
  bool takeOver;
};

const FastLedModeConfig FASTLED_MODES[] = {
  {
    "ripple",
    255,
    CRGB(0, 0, 6),
    CRGB(20, 220, 255),
    1.6f,
    0.18f,
    255,
    true
  },
  {
    "ember",
    180,
    CRGB(8, 1, 0),
    CRGB(255, 90, 18),
    2.2f,
    0.10f,
    220,
    true
  },
  {
    "bloom",
    200,
    CRGB(0, 6, 4),
    CRGB(120, 255, 170),
    1.2f,
    0.08f,
    255,
    false
  }
};

constexpr uint8_t FASTLED_MODE_COUNT = sizeof(FASTLED_MODES) / sizeof(FASTLED_MODES[0]);

CRGB leds[NUM_CLUSTERS][NUM_LEDS];

NodeGraph graph;
Coord MAP_COORDS[MAX_NODES];
uint16_t MAP_COUNT = 0;
const FastLedModeConfig* activeMode = &FASTLED_MODES[0];

// parameters in order are thickness, tick_delay (ms), maxRadius, bands, speed
RippleEngine ripple(graph, 100.0f, 120, 1500.0f, 1, 80.0f);

String serialLine = "";

bool appendCoordsFromSection(JsonArray section, bool isBush, uint16_t& mapIndex);
bool loadMapCoordsFromJson();
void readSerialTrigger();
void parseTrigger(String line);
void updateLeds();
bool addClusterLeds(uint8_t clusterId);
const FastLedModeConfig* findModeByName(const String& modeName);
void applyMode(const FastLedModeConfig& mode);
void printAvailableModes();

void setup() {

  pinMode(REED_OUTPUT_1, INPUT_PULLUP);

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

  applyMode(*activeMode);
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
  Serial.println("Switch modes with: mode:<name>");
  printAvailableModes();
}

void loop() {
  // readSerialTrigger();
  checkReedInputs();

  if (ripple.readyToTick()) {
    ripple.updatePhysics();
    // ripple.printBrightness();
    updateLeds();
  }
}

void checkReedInputs() {
  if (digitalRead(REED_OUTPUT_1) == LOW) {
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
  line.trim();

  if (line.startsWith("mode:")) {
    String modeName = line.substring(5);
    modeName.trim();

    const FastLedModeConfig* mode = findModeByName(modeName);
    if (mode == nullptr) {
      Serial.print("Unknown mode: ");
      Serial.println(modeName);
      printAvailableModes();
      return;
    }

    applyMode(*mode);
    return;
  }

  if (line.equalsIgnoreCase("modes")) {
    printAvailableModes();
    return;
  }

  int commaIndex = line.indexOf(',');

  if (commaIndex == -1) {
    return;
  }

  int row = line.substring(0, commaIndex).toInt();
  int col = line.substring(commaIndex + 1).toInt();

  ripple.trigger(row, col);
}

void updateLeds() {
  for (uint8_t clusterId = 0; clusterId < NUM_CLUSTERS; clusterId++) {
    fill_solid(leds[clusterId], NUM_LEDS, activeMode->primaryColour);
  }

  for (uint16_t i = 0; i < graph.count(); i++) {
    Node& node = graph.nodeAt(i);
    // skips reeds, so they don't get updated with their 'brightness' - check that they are set to constant brightness
    if (node.isBush == false) {
      continue;
    }

    float t = node.brightness / 255.0f;
    t = constrain(t, 0.0f, 1.0f);

    float visible = 0.0f;
    if (t > activeMode->visibilityThreshold) {
      visible = (t - activeMode->visibilityThreshold) / (1.0f - activeMode->visibilityThreshold);
    }

    float blendFactor = powf(visible, activeMode->sharpness);
    uint8_t waveBrightness = (uint8_t)(blendFactor * activeMode->peakBrightness);

    CRGB nodeColour = activeMode->primaryColour;

    if (activeMode->takeOver) {
      CRGB activeColour = activeMode->secondaryColour;
      activeColour.nscale8_video(waveBrightness);
      nodeColour += activeColour;
    } else {
      nodeColour = blend(activeMode->primaryColour, activeMode->secondaryColour, waveBrightness);
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

const FastLedModeConfig* findModeByName(const String& modeName) {
  for (uint8_t i = 0; i < FASTLED_MODE_COUNT; i++) {
    if (modeName.equalsIgnoreCase(FASTLED_MODES[i].name)) {
      return &FASTLED_MODES[i];
    }
  }

  return nullptr;
}

void applyMode(const FastLedModeConfig& mode) {
  activeMode = &mode;
  FastLED.setBrightness(mode.brightness);

  Serial.print("Active mode: ");
  Serial.println(mode.name);
}

void printAvailableModes() {
  Serial.println("Available modes:");
  for (uint8_t i = 0; i < FASTLED_MODE_COUNT; i++) {
    Serial.print(" - ");
    Serial.println(FASTLED_MODES[i].name);
  }
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
