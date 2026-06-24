#ifndef FAST_LED_INTERFACE_H
#define FAST_LED_INTERFACE_H

#define FASTLED_INTERNAL
#include <FastLED.h>
#include "Map.h"

struct ModeConfig {
    int brightness = 128;
    uint8_t primaryR = 255;
    uint8_t primaryG = 0;
    uint8_t primaryB = 0;
    uint8_t secondaryR = 0;
    uint8_t secondaryG = 0;
    uint8_t secondaryB = 0;
    float waveSpeed = 1.0f;
    float fadeSpeed = 1.0f;
    float sharpness = 2.0f;
    float thickness = 5.0f;
    float maxRadius = 20.0f;
    int numBands = 1;
    bool takeOver = false;
};

// Config configuration is declared in the .ino file, so we access it using extern
struct Config {
    String mode = "ripple";
    
    // Mode-specific configurations
    ModeConfig ripple;
    ModeConfig solid;
    ModeConfig rainbow;
    ModeConfig glitter;
    ModeConfig breathing;
    
    // Active parameters
    int brightness = 128;
    uint8_t primaryR = 255;
    uint8_t primaryG = 0;
    uint8_t primaryB = 0;
    uint8_t secondaryR = 0;
    uint8_t secondaryG = 0;
    uint8_t secondaryB = 0;
    float waveSpeed = 1.0f;
    float fadeSpeed = 1.0f;
    float sharpness = 2.0f;
    float thickness = 5.0f;
    float maxRadius = 20.0f;
    int numBands = 1;
    bool takeOver = false;
};
extern Config config;

// Max LEDs supported
#define MAX_LEDS 4500
extern CRGB leds[MAX_LEDS];

namespace FastLEDInterface {
    void displayMap(const Map& nodeMap) {
        int activeLedsCount = nodeMap.nodes.size() * 9;
        if (activeLedsCount > MAX_LEDS) activeLedsCount = MAX_LEDS;

        if (config.mode == "off") {
            fill_solid(leds, MAX_LEDS, CRGB::Black);
        } 
        else if (config.mode == "solid") {
            CRGB primaryColor(config.primaryR, config.primaryG, config.primaryB);
            fill_solid(leds, activeLedsCount, primaryColor);
            if (MAX_LEDS > activeLedsCount) {
                fill_solid(leds + activeLedsCount, MAX_LEDS - activeLedsCount, CRGB::Black);
            }
        } 
        else if (config.mode == "rainbow") {
            static float hueAccumulator = 0.0f;
            float speedFactor = (config.waveSpeed > 0.0f) ? config.waveSpeed : 0.0f;
            hueAccumulator += speedFactor;
            uint8_t startHue = (uint8_t)hueAccumulator;
            uint8_t deltaHue = (activeLedsCount > 0) ? (255 / activeLedsCount) : 1;
            if (deltaHue == 0) deltaHue = 1;
            fill_rainbow(leds, activeLedsCount, startHue, deltaHue);
            if (MAX_LEDS > activeLedsCount) {
                fill_solid(leds + activeLedsCount, MAX_LEDS - activeLedsCount, CRGB::Black);
            }
        } 
        else if (config.mode == "ripple") {
            for (size_t i = 0; i < nodeMap.nodes.size(); ++i) {
                const auto& node = nodeMap.nodes[i];
                
                // Value range is 9.0f
                float t = node.brightness / 9.0f;
                if (t > 1.0f) t = 1.0f;
                if (t < 0.0f) t = 0.0f;
                
                // Color Sharpness transition
                float blendFactor = powf(t, config.sharpness);
                
                uint8_t r, g, b;
                if (config.takeOver && t > 0.01f) {
                    // Secondary color takes over the primary color completely
                    r = (uint8_t)(blendFactor * config.secondaryR);
                    g = (uint8_t)(blendFactor * config.secondaryG);
                    b = (uint8_t)(blendFactor * config.secondaryB);
                } else {
                    // Normal blend
                    r = (uint8_t)((1.0f - blendFactor) * config.primaryR + blendFactor * config.secondaryR);
                    g = (uint8_t)((1.0f - blendFactor) * config.primaryG + blendFactor * config.secondaryG);
                    b = (uint8_t)((1.0f - blendFactor) * config.primaryB + blendFactor * config.secondaryB);
                }
                
                CRGB nodeColor(r, g, b);
                
                int startLed = i * 9;
                for (int ledOffset = 0; ledOffset < 9; ++ledOffset) {
                    if (startLed + ledOffset < MAX_LEDS) {
                        leds[startLed + ledOffset] = nodeColor;
                    }
                }
            }
            if (MAX_LEDS > activeLedsCount) {
                fill_solid(leds + activeLedsCount, MAX_LEDS - activeLedsCount, CRGB::Black);
            }
        }
        else if (config.mode == "glitter") {
            // Slowly blend all active LEDs back to the primary background color
            CRGB primaryColor(config.primaryR, config.primaryG, config.primaryB);
            CRGB secondaryColor(config.secondaryR, config.secondaryG, config.secondaryB);
            
            // Fade speed maps to config.fadeSpeed (ranges 0.1 to 5.0).
            // We map 1.0 fadeSpeed to 12.0 decay.
            uint8_t fadeAmount = (uint8_t)constrain(config.fadeSpeed * 12.0f, 1.0f, 255.0f);
            for (int i = 0; i < activeLedsCount; ++i) {
                leds[i] = nblend(leds[i], primaryColor, fadeAmount);
            }
            
            // Randomly trigger glitter sparkles using the secondary color.
            // config.waveSpeed (intensity) ranges 0.1 to 5.0.
            // 1.0 intensity -> 5% spawn chance per frame at 60FPS.
            if (activeLedsCount > 0) {
                float spawnChance = config.waveSpeed * 50.0f; // 1.0 -> 50 / 1000 = 5%
                if (random16(1000) < (uint16_t)spawnChance) {
                    int pos = random16(activeLedsCount);
                    // size/thickness ranges 1.0 to 15.0
                    int size = (int)config.thickness;
                    if (size < 1) size = 1;
                    
                    for (int j = 0; j < size; ++j) {
                        int idx = (pos + j) % activeLedsCount;
                        leds[idx] = secondaryColor;
                    }
                }
            }
            
            if (MAX_LEDS > activeLedsCount) {
                fill_solid(leds + activeLedsCount, MAX_LEDS - activeLedsCount, CRGB::Black);
            }
        }
        else if (config.mode == "breathing") {
            // Breathing cycle speed is controlled by config.fadeSpeed (1.0 = 2 seconds cycle)
            float speedFactor = (config.fadeSpeed > 0.05f) ? config.fadeSpeed : 0.05f;
            float periodMs = 2000.0f / speedFactor;
            
            // Use modulo on millis() to keep phase input to sinf within bounds for perfect 60FPS precision
            float phase = (millis() % (unsigned long)periodMs) / periodMs * 2.0f * M_PI;
            float breath = (sinf(phase) + 1.0f) / 2.0f; // Smooth 0.0 to 1.0 oscillation
            
            uint8_t r = (uint8_t)((1.0f - breath) * config.primaryR + breath * config.secondaryR);
            uint8_t g = (uint8_t)((1.0f - breath) * config.primaryG + breath * config.secondaryG);
            uint8_t b = (uint8_t)((1.0f - breath) * config.primaryB + breath * config.secondaryB);
            
            CRGB color(r, g, b);
            fill_solid(leds, activeLedsCount, color);
            
            if (MAX_LEDS > activeLedsCount) {
                fill_solid(leds + activeLedsCount, MAX_LEDS - activeLedsCount, CRGB::Black);
            }
        }
        
        FastLED.show();
    }
}

#endif // FAST_LED_INTERFACE_H
