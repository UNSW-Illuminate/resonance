#ifndef MAP_H
#define MAP_H

#include <vector>
#include <cmath>
#include <Arduino.h>
#include <ArduinoJson.h>

struct Node {
    float brightness; // Brightness: int (using float internally for physics precision)
    float row;        // Row: float
    float col;        // Col: float
    bool isBush;
    std::vector<int> neighbors; // Neighbors: array
};

class Map {
public:
    std::vector<Node> nodes; // Nodes: array

    void clear() {
        nodes.clear();
    }

    bool hasNode(float row, float col) const {
        for (const auto& node : nodes) {
            if (node.row == row && node.col == col) {
                return true;
            }
        }
        return false;
    }

    void addNode(float row, float col, bool isBush) {
        if (hasNode(row, col)) {
            return; // Avoid duplicates
        }

        Node newNode;
        newNode.row = row;
        newNode.col = col;
        newNode.isBush = isBush;
        newNode.brightness = 0.0f;
        nodes.push_back(newNode);

        int newIdx = nodes.size() - 1;
        if (nodes.size() < 2) return;

        bool found = false;
        float radius = 1.0f;

        // Search for closest neighbor by expanding the radius
        while (!found && radius < 1000.0f) {
            for (int i = 0; i < newIdx; ++i) {
                float dist = sqrtf(powf(row - nodes[i].row, 2) + powf(col - nodes[i].col, 2));
                if (dist <= radius) {
                    nodes[newIdx].neighbors.push_back(i);
                    nodes[i].neighbors.push_back(newIdx);
                    found = true;
                }
            }
            if (!found) {
                radius += 1.0f;
            }
        }
    }

    void setNodeBrightness(int index, float brightness) {
        if (index >= 0 && index < (int)nodes.size()) {
            nodes[index].brightness = brightness;
        }
    }

    bool readJSON(const String& jsonContent) {
        DynamicJsonDocument doc(49152); // Allocation for parsing map size
        DeserializationError error = deserializeJson(doc, jsonContent);
        if (error) {
            Serial.print("JSON parsing failed: ");
            Serial.println(error.c_str());
            return false;
        }

        clear();

        if (doc.containsKey("nodes")) {
            JsonArray nodesArr = doc["nodes"];
            for (JsonObject nodeObj : nodesArr) {
                float row = nodeObj["row"];
                float col = nodeObj["col"];
                addNode(row, col, false); // Default to reed
            }
        } else {
            if (doc.containsKey("bushes")) {
                JsonArray bushesArr = doc["bushes"];
                for (JsonObject nodeObj : bushesArr) {
                    float row = nodeObj["row"];
                    float col = nodeObj["col"];
                    addNode(row, col, true);
                }
            }
            if (doc.containsKey("reeds")) {
                JsonArray reedsArr = doc["reeds"];
                for (JsonObject nodeObj : reedsArr) {
                    float row = nodeObj["row"];
                    float col = nodeObj["col"];
                    addNode(row, col, false);
                }
            }
        }

        Serial.print("Successfully loaded map with ");
        Serial.print(nodes.size());
        Serial.println(" nodes.");
        return true;
    }
};

#endif // MAP_H
