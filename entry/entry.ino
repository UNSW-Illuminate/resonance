#include "NodeGraph.h"
#include "RippleEngine.h"
#include "FastLED.h"
#include "coordinates_new_json.h"
#include <ArduinoJson.h>
#include <math.h>

#define DATA_PIN 6

#define REED_OUTPUT_1 4
#define REED_OUTPUT_2 5
#define REED_OUTPUT_3 6
#define REED_OUTPUT_4 7
#define REED_OUTPUT_5 8
#define REED_OUTPUT_6 9
#define REED_OUTPUT_7 10


#define NUM_LEDS (MAX_NODES * NUM_LED_PER_NODE)
#define JSON_DOC_CAPACITY 20000

const CRGB RIPPLE_PRIMARY_COLOUR(0, 0, 6);
const CRGB RIPPLE_SECONDARY_COLOUR(20, 220, 255);
const float RIPPLE_SHARPNESS = 1.6f;
const float RIPPLE_VISIBILITY_THRESHOLD = 0.18f;
const uint8_t RIPPLE_PEAK_BRIGHTNESS = 255;
const bool RIPPLE_TAKE_OVER = true;

CRGB leds[NUM_LEDS];

NodeGraph graph;
Coord MAP_COORDS[MAX_NODES];
uint16_t MAP_COUNT = 0;

// parameters in order are thickness, tick_delay (ms), maxRadius, bands, speed
RippleEngine ripple(graph, 100.0f, 120, 1500.0f, 1, 40.0f);

String serialLine = "";

bool appendCoordsFromSection(JsonArray section, bool isBush, uint16_t& mapIndex);
bool loadMapCoordsFromJson();
void readSerialTrigger();
void parseTrigger(String line);
void updateLeds();

void setup() {

  pinMode(REED_OUTPUT_1, INPUT_PULLUP);
  pinMode(REED_OUTPUT_2, INPUT_PULLUP);
  pinMode(REED_OUTPUT_3, INPUT_PULLUP);
  pinMode(REED_OUTPUT_4, INPUT_PULLUP);
  pinMode(REED_OUTPUT_5, INPUT_PULLUP);
  pinMode(REED_OUTPUT_6, INPUT_PULLUP);
  pinMode(REED_OUTPUT_7, INPUT_PULLUP);

  Serial.begin(115200);

  while (!Serial && millis() < 3000) {
    // Allows time for the USB serial monitor to connect on Teensy-style boards.
  }

  if (!loadMapCoordsFromJson()) {
    Serial.println("Failed to load coordinates_new.json");
  }

  FastLED.addLeds<WS2812B, DATA_PIN, GRB>(leds, NUM_LEDS);
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
  // readSerialTrigger();
  checkReedInputs();

  if (ripple.readyToTick()) {
    ripple.updatePhysics();
    ripple.printBrightness();
    updateLeds();
  }
}

void checkReedInputs() {
  // Define reed pin assignments in order
  const uint8_t reedPins[] = {REED_OUTPUT_1, REED_OUTPUT_2, REED_OUTPUT_3, REED_OUTPUT_4, REED_OUTPUT_5, REED_OUTPUT_6, REED_OUTPUT_7};
  uint8_t reedIndex = 0;
  
  // Iterate through all nodes in the graph
  for (uint16_t i = 0; i < graph.count(); i++) {
    Node& node = graph.nodeAt(i);
    
    // Check if this node is a reed (not a bush)
    if (!node.isBush) {
      // Check if we haven't exceeded the number of reed pins
      if (reedIndex < sizeof(reedPins) / sizeof(reedPins[0])) {
        // Check if the corresponding pin is LOW
        if (digitalRead(reedPins[reedIndex]) == LOW) {
          // Trigger the ripple at this reed's row and col
          ripple.trigger(node.row, node.col);
        }
        reedIndex++;
      }
    }
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

    for (uint8_t j = 0; j < NUM_LED_PER_NODE; j++) {
      uint16_t ledIndex = node.ledIndex + j;
      if (ledIndex < NUM_LEDS) {
        leds[ledIndex] = nodeColour;
      }
    }
  }

  FastLED.show();
}
