#include <ArduinoJson.h> // Include BEFORE FastLED to prevent PROGMEM conflicts
#include "NodeGraph.h"
#include "RippleEngine.h"
#include "FastLED.h"
#include "test_json.h" // Assuming this contains your coordinates
#include <math.h>
#include "ResonanceComms.h"

// Define Hardware Setup
#define NUM_CLUSTERS 7
#define NUM_LEDS 680
#define JSON_DOC_CAPACITY 20000
#define REED_INPUT_1 24

// Define 7 GPIO Pins for the LED branches
const uint8_t CLUSTER_PINS[NUM_CLUSTERS] = {2, 3, 4, 5, 6, 7, 9};

// Ripple physics tuning
const float RIPPLE_SHARPNESS = 2.2f;
const float RIPPLE_VISIBILITY_THRESHOLD = 0.10f;
const bool RIPPLE_TAKE_OVER = true;

// LED Memory Array (7 strips x 680 LEDs)
CRGB leds[NUM_CLUSTERS][NUM_LEDS];

// Logic Core
NodeGraph graph;
Coord MAP_COORDS[MAX_NODES];
uint16_t MAP_COUNT = 0;

// parameters: thickness, tick_delay (ms), maxRadius, bands, speed
RippleEngine ripple(graph, 150.0f, 120, 1500.0f, 1, 80.0f);

// Comms Core
ResonanceComms comms;
LightingCommandState currentState;
static uint8_t serialRxBuffer[2048];

// Function Prototypes
bool appendCoordsFromSection(JsonArray section, bool isBush, uint16_t& mapIndex);
bool loadMapCoordsFromJson();
void updateLeds();
void checkReedInputs();

void setup() {
  pinMode(REED_INPUT_1, INPUT_PULLUP);
  Serial.begin(115200);
  delay(1000);
  Serial.println("Starting Setup...");

  // Setup Hardware Serial3 (RX Pin 15, TX Pin 14) for ESP32 communication
  Serial3.addMemoryForRead(serialRxBuffer, sizeof(serialRxBuffer));
  Serial3.begin(115200);
  Serial.println("Serial3 started. Waiting for ESP32...");

  // Load JSON mapping
  Serial.println("Loading Node Map...");
  if (!loadMapCoordsFromJson()) {
    Serial.println("Warning: Failed to load map or map is empty.");
  }
  
  // Register Nodes in Graph
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

  // Initialize LED Clusters (using FastLED's native parallel capability)
  Serial.println("Adding LED clusters...");
  FastLED.addLeds<WS2812B, 2, GRB>(leds[0], NUM_LEDS);
  FastLED.addLeds<WS2812B, 3, GRB>(leds[1], NUM_LEDS);
  FastLED.addLeds<WS2812B, 4, GRB>(leds[2], NUM_LEDS);
  FastLED.addLeds<WS2812B, 5, GRB>(leds[3], NUM_LEDS);
  FastLED.addLeds<WS2812B, 6, GRB>(leds[4], NUM_LEDS);
  FastLED.addLeds<WS2812B, 7, GRB>(leds[5], NUM_LEDS);
  FastLED.addLeds<WS2812B, 9, GRB>(leds[6], NUM_LEDS);

  // Set Default State
  currentState.enabled = true;
  strcpy(currentState.mode, "palette_ocean");
  currentState.brightness = 150;
  currentState.speed = 50;
  currentState.rippleSize = 32;
  currentState.primaryColor[0] = 0; currentState.primaryColor[1] = 180; currentState.primaryColor[2] = 255;
  currentState.secondaryColor[0] = 255; currentState.secondaryColor[1] = 90; currentState.secondaryColor[2] = 18;

  FastLED.setBrightness(currentState.brightness);
  FastLED.clear();
  FastLED.show();

  // Initialize Engines
  ripple.begin();
  comms.begin(Serial3);
  comms.sendReady();

  Serial.println("Resonance Core initialized.");
  Serial.print("Nodes tracked: ");
  Serial.println(MAP_COUNT);
}

void loop() {
  // 1. Process incoming ESP32 Commands
  comms.poll();

  if (comms.hasStateUpdate()) {
      currentState = comms.state();
      comms.clearStateUpdate();
      Serial.println("Received UI State Update");
  }

  checkReedInputs();

  // 2. Process Ripples triggers from ESP32
  if (comms.hasRippleTrigger()) {
      const auto& trigger = comms.rippleTrigger();
      if (trigger.hasCoords) {
          ripple.trigger(trigger.row, trigger.col);
      } else if (trigger.hasNodeId && trigger.nodeId < graph.count()) {
          Node& node = graph.nodeAt(trigger.nodeId);
          ripple.trigger(node.row, node.col);
      }
      comms.clearRippleTrigger();
  }

  // 3. Auto Ripple logic
  if (currentState.autoRipple && strcmp(currentState.mode, "ripple") == 0) {
      static unsigned long lastAutoRippleMs = 0;
      static uint16_t currentDelay = currentState.autoRippleInterval;

      if (millis() - lastAutoRippleMs > currentDelay) {
          lastAutoRippleMs = millis();
          if (graph.count() > 0) {
              uint16_t randNode = random(graph.count());
              Node& node = graph.nodeAt(randNode);
              ripple.trigger(node.row, node.col);
              Serial.print("Auto Ripple Triggered at node: ");
              Serial.println(randNode);
          }
          if (currentState.autoRippleVariable) {
              currentDelay = random(500, currentState.autoRippleInterval + 1);
          } else {
              currentDelay = currentState.autoRippleInterval;
          }
      }
  }

  // 4. Render LED Animation frames
  if (ripple.readyToTick()) {
    ripple.updatePhysics();
    updateLeds();
  }
}

// --------------------------------------------------------------------------
// rendering logic
// --------------------------------------------------------------------------
void checkReedInputs() {
  if (digitalRead(REED_INPUT_1) == LOW) {
      Serial.println("Reed 1 triggered");
      ripple.trigger(2785, 3184);
  }
}

void updateLeds() {
    if (!currentState.enabled || strcmp(currentState.mode, "off") == 0) {
        FastLED.clear();
        FastLED.show();
        return;
    }

    FastLED.setBrightness(currentState.brightness);
    CRGB primary(currentState.primaryColor[0], currentState.primaryColor[1], currentState.primaryColor[2]);
    CRGB secondary(currentState.secondaryColor[0], currentState.secondaryColor[1], currentState.secondaryColor[2]);

    if (strcmp(currentState.mode, "ripple") == 0) {
        // Render base color
        for (uint8_t clusterId = 0; clusterId < NUM_CLUSTERS; clusterId++) {
            fill_solid(leds[clusterId], NUM_LEDS, primary);
        }

        // Overlay Ripples
        for (uint16_t i = 0; i < graph.count(); i++) {
            Node& node = graph.nodeAt(i);
            
            float t = node.brightness / 255.0f;
            t = constrain(t, 0.0f, 1.0f);

            float visible = 0.0f;
            if (t > RIPPLE_VISIBILITY_THRESHOLD) {
                visible = (t - RIPPLE_VISIBILITY_THRESHOLD) / (1.0f - RIPPLE_VISIBILITY_THRESHOLD);
            }

            float blendFactor = powf(visible, RIPPLE_SHARPNESS);
            uint8_t waveBrightness = (uint8_t)(blendFactor * 255.0f);

            CRGB nodeColour = primary;

            if (RIPPLE_TAKE_OVER) {
                CRGB activeColour = secondary;
                activeColour.nscale8_video(waveBrightness);
                nodeColour += activeColour;
            } else {
                nodeColour = blend(primary, secondary, waveBrightness);
            }

            if (!node.isBush) {
                if (node.isTriggeredReed) {
                    Serial.print("IN ENTRY INO Reed node triggered at row: ");
                    Serial.print(node.row);
                    Serial.print(", col: ");
                    Serial.println(node.col);
                    
                    uint16_t startLedIndex = node.clusterIndex;
                    // cycle through all leds in this reed cluster
                    for (uint8_t j = 0; j < NUM_LED_PER_NODE; j++) {
                        uint16_t ledIndex = startLedIndex + j;
                        if (ledIndex < NUM_LEDS && node.clusterId < NUM_CLUSTERS) {
                            leds[node.clusterId][ledIndex] = nodeColour;
                        }
                    }
                    node.isTriggeredReed = false;
                }
                continue;
            }

            uint16_t startLedIndex = node.clusterIndex;
            for (uint8_t j = 0; j < NUM_LED_PER_NODE; j++) {
                uint16_t ledIndex = startLedIndex + j;
                if (ledIndex < NUM_LEDS && node.clusterId < NUM_CLUSTERS) {
                    leds[node.clusterId][ledIndex] = nodeColour;
                }
            }
        }
    } else if (strncmp(currentState.mode, "palette_", 8) == 0 || strcmp(currentState.mode, "rainbow") == 0) {
        static uint8_t startIndex = 0;
        startIndex = startIndex + (currentState.speed / 10);
        
        CRGBPalette16 currentPalette;
        if (strcmp(currentState.mode, "palette_ocean") == 0) currentPalette = OceanColors_p;
        else if (strcmp(currentState.mode, "palette_lava") == 0) currentPalette = LavaColors_p;
        else if (strcmp(currentState.mode, "palette_forest") == 0) currentPalette = ForestColors_p;
        else if (strcmp(currentState.mode, "palette_party") == 0) currentPalette = PartyColors_p;
        else if (strcmp(currentState.mode, "palette_cloud") == 0) {
            // Custom light blue palette for clouds
            currentPalette = CRGBPalette16(CRGB::LightSkyBlue, CRGB::SkyBlue, CRGB::DeepSkyBlue, CRGB::AliceBlue);
        }
        else currentPalette = RainbowColors_p;

        for (int i = 0; i < NUM_CLUSTERS; i++) {
            fill_palette(leds[i], NUM_LEDS, startIndex, 7, currentPalette, 255, LINEARBLEND);
        }
    } else if (strcmp(currentState.mode, "glitter") == 0) {
        for (int i = 0; i < NUM_CLUSTERS; i++) {
            fill_solid(leds[i], NUM_LEDS, primary);
            if (random8() < currentState.speed) {
                uint8_t sparkCount = (currentState.rippleSize / 32) + 1;
                for(uint8_t s = 0; s < sparkCount; s++) {
                    leds[i][random16(NUM_LEDS)] = secondary;
                }
            }
        }
    } else {
        // Static
        for (int i = 0; i < NUM_CLUSTERS; i++) {
            fill_solid(leds[i], NUM_LEDS, primary);
        }
    }

    FastLED.show();
}

// --------------------------------------------------------------------------
// JSON Map Loading Logic
// --------------------------------------------------------------------------
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
