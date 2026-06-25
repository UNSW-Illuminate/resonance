#include "NodeGraph.h"
#include <math.h>

NodeGraph::NodeGraph() : nodeCount(0) {}

bool NodeGraph::addNode(int16_t row, int16_t col) {
  return addNode(row, col, nodeCount, false, 0, 0);
}

bool NodeGraph::addNode(int16_t row, int16_t col, bool isBush) {
  return addNode(row, col, nodeCount, isBush, 0, 0);
}

bool NodeGraph::addNode(
  int16_t row,
  int16_t col,
  uint16_t ledIndex,
  bool isBush,
  uint8_t clusterId,
  uint16_t clusterIndex
) {
  if (nodeCount >= MAX_NODES) {
    return false;
  }

  if (findIndex(row, col) != -1) {
    return false;
  }

  Node& node = nodes[nodeCount];
  node.row = row;
  node.col = col;
  node.ledIndex = ledIndex * NUM_LED_PER_NODE;
  node.brightness = 0;
  node.startLED = 0;
  node.isBush = isBush;
  node.clusterId = clusterId;
  node.clusterIndex = clusterIndex;
  node.neighbourCount = 0;
  node.isTriggeredReed = false;

  uint16_t newIndex = nodeCount;
  nodeCount++;

  if (nodeCount > 1) {
    connectNearest(newIndex);
  }

  return true;
}

bool NodeGraph::removeNode(int16_t row, int16_t col) {
  int index = findIndex(row, col);

  if (index == -1) {
    return false;
  }

  uint16_t removeIndex = (uint16_t)index;

  for (uint16_t i = 0; i < nodeCount; i++) {
    if (i != removeIndex) {
      removeNeighbour(i, removeIndex);
    }
  }

  // Keep the array compact by moving the last node into the removed slot.
  uint16_t lastIndex = nodeCount - 1;
  if (removeIndex != lastIndex) {
    nodes[removeIndex] = nodes[lastIndex];

    // Fix neighbour references that used to point to the last index.
    for (uint16_t i = 0; i < lastIndex; i++) {
      for (uint8_t j = 0; j < nodes[i].neighbourCount; j++) {
        if (nodes[i].neighbours[j] == lastIndex) {
          nodes[i].neighbours[j] = removeIndex;
        }
      }
    }
  }

  nodeCount--;
  return true;
}

int NodeGraph::findIndex(int16_t row, int16_t col) const {
  for (uint16_t i = 0; i < nodeCount; i++) {
    if (nodes[i].row == row && nodes[i].col == col) {
      return (int)i;
    }
  }

  return -1;
}

Node* NodeGraph::getNode(int16_t row, int16_t col) {
  int index = findIndex(row, col);
  return index == -1 ? nullptr : &nodes[index];
}

const Node* NodeGraph::getNode(int16_t row, int16_t col) const {
  int index = findIndex(row, col);
  return index == -1 ? nullptr : &nodes[index];
}

Node& NodeGraph::nodeAt(uint16_t index) {
  return nodes[index];
}

const Node& NodeGraph::nodeAt(uint16_t index) const {
  return nodes[index];
}

uint16_t NodeGraph::count() const {
  return nodeCount;
}

void NodeGraph::clearBrightness() {
  for (uint16_t i = 0; i < nodeCount; i++) {
    nodes[i].brightness = 0;
  }
}

bool NodeGraph::setNodeBrightness(int16_t row, int16_t col, uint8_t brightness) {
  Node* node = getNode(row, col);
  if (node == nullptr) {
    return false;
  }

  node->brightness = brightness;
  return true;
}

void NodeGraph::connectNearest(uint16_t newIndex) {
  bool found = false;
  float radius = 1.0f;

  while (!found && radius < 1000.0f) {
    for (uint16_t i = 0; i < nodeCount; i++) {
      if (i == newIndex) {
        continue;
      }

      float dist = distanceBetween(newIndex, i);
      if (dist <= radius) {
        addNeighbour(newIndex, i);
        addNeighbour(i, newIndex);
        found = true;
      }
    }

    radius += 1.0f;
  }
}

bool NodeGraph::addNeighbour(uint16_t nodeIndex, uint16_t neighbourIndex) {
  if (nodeIndex >= nodeCount || neighbourIndex >= nodeCount) {
    return false;
  }

  Node& node = nodes[nodeIndex];

  for (uint8_t i = 0; i < node.neighbourCount; i++) {
    if (node.neighbours[i] == neighbourIndex) {
      return true;
    }
  }

  if (node.neighbourCount >= MAX_NEIGHBOURS) {
    return false;
  }

  node.neighbours[node.neighbourCount++] = neighbourIndex;
  return true;
}

void NodeGraph::removeNeighbour(uint16_t nodeIndex, uint16_t neighbourIndex) {
  Node& node = nodes[nodeIndex];

  for (uint8_t i = 0; i < node.neighbourCount; i++) {
    if (node.neighbours[i] == neighbourIndex) {
      node.neighbours[i] = node.neighbours[node.neighbourCount - 1];
      node.neighbourCount--;
      return;
    }
  }
}

float NodeGraph::distanceBetween(uint16_t a, uint16_t b) const {
  float dr = (float)nodes[a].row - (float)nodes[b].row;
  float dc = (float)nodes[a].col - (float)nodes[b].col;
  return sqrtf(dr * dr + dc * dc);
}
