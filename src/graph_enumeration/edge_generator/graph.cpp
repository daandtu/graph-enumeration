#include "graph.h"

Graph::Graph(const std::vector<unsigned int> *nodes, triu adjacencyTriu,
             const std::vector<triu> *degreeFilter,
             const std::vector<triu> *adjacencyTriuMasks,
             const std::vector<std::vector<int>> *permutations)
    : nNodes(nodes->size()), nodes(nodes), adjacencyTriu(adjacencyTriu),
      degreeFilter(degreeFilter), adjacencyTriuMasks(adjacencyTriuMasks),
      permutations(permutations) {
  // Precompute connectivity and invariant representation
  checkConnected();
  calculateRepresentation();
}

bool Graph::operator==(const Graph &other) const {
  if (representation != other.representation)
    return false;
  // Check all possible permutations
  for (auto &permutation : *permutations) {
    bool equal = true;
    for (size_t i = 0; i < nNodes; ++i) {
      const auto &permutationI = permutation[i];
      for (size_t j = i + 1; j < nNodes; ++j) {
        if (isEdge(i, j) != other.isEdge(permutationI, permutation[j])) {
          equal = false;
          break;
        }
      }
      if (!equal)
        break;
    }
    if (equal)
      return true;
  }
  return false;
}

bool Graph::isEdge(unsigned int u, unsigned int v) const {
  if (u == v)
    return false;
  size_t index = upperTriuIndex(u, v, nNodes);
  return (adjacencyTriu & (*adjacencyTriuMasks)[index]) != 0;
}

bool Graph::isConnected() const { return connected; }

repr Graph::getRepresentation() const { return representation; }

void Graph::calculateRepresentation() {
  // 1. Total number of edges (one integer)
  representation.push_back(__builtin_popcountll(adjacencyTriu));
  // 2. Degree of each node sorted within each node type group (nNodes integers)
  size_t maxNode = (*nodes)[nNodes - 1] + 1;
  auto degreesList = std::vector<std::vector<int>>(maxNode, std::vector<int>());
  for (size_t i = 0; i < nNodes; ++i) {
    degreesList[(*nodes)[i]].push_back(
        __builtin_popcountll(adjacencyTriu & (*degreeFilter)[i]));
  }
  for (auto &degrees : degreesList) {
    std::sort(degrees.begin(), degrees.end());
    representation.insert(representation.end(), degrees.begin(), degrees.end());
  }
  // 3. Number of edges for each edge type (size depends on number of different
  // node types)
  std::vector<std::vector<unsigned int>> edgeList(
      maxNode, std::vector<unsigned int>(maxNode, 0));
  size_t count = 0;
  for (size_t i = 0; i < nNodes; ++i) {
    for (size_t j = i + 1; j < nNodes; ++j) {
      if ((adjacencyTriu & (*adjacencyTriuMasks)[count]) != 0) {
        unsigned int a = (*nodes)[i];
        unsigned int b = (*nodes)[j];
        if (a > b)
          std::swap(a, b); // ensure a <= b
        edgeList[a][b]++;
      }
      ++count;
    }
  }
  for (size_t i = 0; i < maxNode; ++i) {
    for (size_t j = i; j < maxNode; ++j) { // only upper triangle
      representation.push_back(edgeList[i][j]);
    }
  }
}

void Graph::checkConnected() {
  // Use BFS to check connectivity
  uint64_t visited = 0;
  std::queue<unsigned int> queue;
  queue.push(0);
  visited |= 1;
  unsigned int visitedCount = 0;
  while (!queue.empty()) {
    unsigned int current = queue.front();
    queue.pop();
    visitedCount++;
    for (unsigned int i = 0; i < nNodes; ++i) {
      if (!(visited & (1ULL << i)) && isEdge(current, i)) {
        visited |= (1ULL << i);
        queue.push(i);
      }
    }
  }
  connected = (visitedCount == nNodes);
}

std::vector<std::vector<uint8_t>> Graph::getAdjacencyMatrix() const {
  std::vector<std::vector<uint8_t>> matrix(nNodes,
                                           std::vector<uint8_t>(nNodes, 0));
  for (size_t i = 0; i < nNodes; ++i) {
    for (size_t j = i + 1; j < nNodes; ++j) {
      if (isEdge(i, j)) {
        matrix[i][j] = 1;
        matrix[j][i] = 1;
      }
    }
  }
  return matrix;
}

std::vector<std::pair<unsigned int, unsigned int>> Graph::getEdges() const {
  std::vector<std::pair<unsigned int, unsigned int>> edges;
  // Iterate over upper triangle of adjacency matrix
  for (unsigned int i = 0; i < nNodes; ++i) {
    for (unsigned int j = i + 1; j < nNodes; ++j) {
      if (isEdge(i, j)) {
        edges.push_back({i, j});
      }
    }
  }
  return edges;
}
