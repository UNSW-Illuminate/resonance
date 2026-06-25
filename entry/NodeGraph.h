#pragma once

#include <Arduino.h>

#define MAX_NODES 100
#define MAX_NEIGHBOURS 8
#define NUM_LED_PER_NODE 9

struct Coord {
  int16_t row;
  int16_t col;
  bool isBush;
  uint8_t clusterId;
  uint16_t clusterIndex;
};

struct Node {
  int16_t row;
  int16_t col;
  uint16_t ledIndex;
  uint8_t brightness;
  uint8_t startLED;
  bool isBush;
  bool isTriggeredReed;
  uint8_t clusterId;
  uint16_t clusterIndex;

  uint16_t neighbours[MAX_NEIGHBOURS];
  uint8_t neighbourCount;
};

class NodeGraph {
public:
  NodeGraph();

  bool addNode(int16_t row, int16_t col);
  bool addNode(int16_t row, int16_t col, bool isBush);
  bool addNode(
    int16_t row,
    int16_t col,
    uint16_t ledIndex,
    bool isBush = false,
    uint8_t clusterId = 0,
    uint16_t clusterIndex = 0
  );
  bool removeNode(int16_t row, int16_t col);

  int findIndex(int16_t row, int16_t col) const;

  Node* getNode(int16_t row, int16_t col);
  const Node* getNode(int16_t row, int16_t col) const;

  Node& nodeAt(uint16_t index);
  const Node& nodeAt(uint16_t index) const;

  uint16_t count() const;
  void clearBrightness();
  bool setNodeBrightness(int16_t row, int16_t col, uint8_t brightness);

private:
  Node nodes[MAX_NODES];
  uint16_t nodeCount;

  void connectNearest(uint16_t newIndex);
  bool addNeighbour(uint16_t nodeIndex, uint16_t neighbourIndex);
  void removeNeighbour(uint16_t nodeIndex, uint16_t neighbourIndex);
  float distanceBetween(uint16_t a, uint16_t b) const;
};
