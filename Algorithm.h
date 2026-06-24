#ifndef ALGORITHM_H
#define ALGORITHM_H

#include <vector>
#include <cmath>
#include <Arduino.h>
#include "Map.h"

struct Ripple {
    float sourceRow;
    float sourceCol;
    float age;
    unsigned long startTimeMs;
    int numBands;
};

class Algorithm {
public:
    double thickness = 5.0;      // Thickness: double
    double tickDelay = 0.05;     // Tick delay: double (50ms interval)
    double maxRadius = 20.0;     // Max radius: double
    int bands = 2;               // Bands: int
    double waveSpeed = 1.0;      // Wave speed: double
    float fadeSpeed = 1.0f;      // Fade speed (from custom tuning)
    float sharpness = 2.0f;      // Color sharpness exponent
    float valueRange = 9.0f;
    unsigned long backoffDurationMs = 4000;

    Map nodeMap; // Map (instance of Map class)

    std::vector<Ripple> activeRipples;

    void triggerLed(float row, float col) {
        // Verify if node exists at coordinates
        bool exists = false;
        for (const auto& node : nodeMap.nodes) {
            if (node.row == row && node.col == col) {
                exists = true;
                break;
            }
        }
        if (!exists) return;

        unsigned long now = millis();

        // Backoff check: don't trigger if a ripple started here recently
        for (const auto& r : activeRipples) {
            if (r.sourceRow == row && r.sourceCol == col && (now - r.startTimeMs) < backoffDurationMs) {
                return;
            }
        }

        Ripple newRipple;
        newRipple.sourceRow = row;
        newRipple.sourceCol = col;
        newRipple.age = 0.0f;
        newRipple.startTimeMs = now;
        newRipple.numBands = (bands - 1 >= 0) ? (bands - 1) : 0;
        activeRipples.push_back(newRipple);
    }

    void updatePhysics(float dt) {
        // Reset all node brightness to 0.0f
        for (size_t i = 0; i < nodeMap.nodes.size(); ++i) {
            nodeMap.setNodeBrightness(i, 0.0f);
        }

        std::vector<Ripple> stillActive;
        std::vector<Ripple> secondaryRipples;
        float buffer = 1.5f;
        float wavelength = thickness * 2.0f;
        double activeTickDelay = (tickDelay > 0.0) ? tickDelay : 0.05;

        for (auto& ripple : activeRipples) {
            // Safety clamp: if bands was decreased dynamically, clamp ripple.numBands
            // to prevent crashes caused by negative band started calculations
            if (ripple.numBands > bands - 1) {
                ripple.numBands = (bands - 1 >= 0) ? (bands - 1) : 0;
            }

            float wavefront = ripple.age * waveSpeed;

            // Handle secondary bands
            int numBandsStarted = bands - ripple.numBands;
            if (ripple.numBands > 0 && wavefront > (thickness * numBandsStarted * buffer)) {
                Ripple sec;
                sec.sourceRow = ripple.sourceRow;
                sec.sourceCol = ripple.sourceCol;
                sec.age = 0.0f;
                sec.startTimeMs = millis();
                sec.numBands = 0;
                secondaryRipples.push_back(sec);

                ripple.numBands -= 1;
            }

            // Calculate brightness for every node based on this ripple
            for (size_t i = 0; i < nodeMap.nodes.size(); ++i) {
                const auto& node = nodeMap.nodes[i];
                float dist = sqrtf(powf(ripple.sourceRow - node.row, 2) + powf(ripple.sourceCol - node.col, 2));

                if (dist <= maxRadius) {
                    float offset = dist - wavefront;
                    float divisor = (offset < 0.0f) ? (thickness / fadeSpeed) : thickness;
                    float envelope = fmaxf(0.0f, 1.0f - fabsf(offset) / divisor);

                    if (envelope > 0.0f) {
                        float phase = (2.0f * M_PI * offset) / wavelength;
                        float attenuation = 1.0f - (dist / maxRadius);
                        float brightness = envelope * ((sinf(phase) + 1.0f) / 2.0f) * valueRange * attenuation;
                        nodeMap.setNodeBrightness(i, fmaxf(node.brightness, brightness));
                    }
                }
            }

            // Continuous time-delta independent aging
            ripple.age += (float)(dt / activeTickDelay);
            if (ripple.age * waveSpeed <= maxRadius) {
                stillActive.push_back(ripple);
            }
        }

        // Add secondary ripples
        for (const auto& sec : secondaryRipples) {
            stillActive.push_back(sec);
        }

        activeRipples = stillActive;
    }
};

#endif // ALGORITHM_H
