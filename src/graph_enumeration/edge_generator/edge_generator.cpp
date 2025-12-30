#include "edge_generator.h"
#include <vector>

void calculateDegreeFilter(std::vector<triu> &result, unsigned int nNodes) {
  size_t maxSize = nNodes * (nNodes - 1) / 2;
  result.assign(nNodes, 0);
  for (unsigned int k = 0; k < nNodes; ++k) {
    // Edges (u, k) where u < k
    for (unsigned int u = 0; u < k; ++u) {
      size_t count = upperTriuIndex(u, k, nNodes);
      result[k] |= ((triu)1 << (maxSize - 1 - count));
    }
    // Edges (k, v) where v > k
    for (unsigned int v = k + 1; v < nNodes; ++v) {
      size_t count = upperTriuIndex(k, v, nNodes);
      result[k] |= ((triu)1 << (maxSize - 1 - count));
    }
  }
}

bool checkDegrees(const triu triAdjMat, const std::vector<triu> &degreeFilter,
                  const unsigned int nNodes, const unsigned int maxDegree,
                  const unsigned int minDegree) {
  for (size_t i = 0; i < nNodes; ++i) {
    int count = __builtin_popcountll(triAdjMat & degreeFilter[i]);
    if ((unsigned int)count > maxDegree || (unsigned int)count < minDegree) {
      return false;
    }
  }
  return true;
}

void generatePermutations(std::vector<std::vector<int>> &permutations,
                          const std::vector<unsigned int> &nodes,
                          std::vector<int> &currentPermutation,
                          std::unordered_map<int, std::vector<int>> &indexMap,
                          unsigned int currentIndex) {
  const unsigned int nNodes = nodes.size();
  if (currentIndex == nNodes) {
    // Permutation complete
    permutations.push_back(std::vector<int>(currentPermutation));
    return;
  }

  // Get the node type at the current index
  int currentValue = nodes[currentIndex];
  // Get the list of indices for this node type
  auto &indices = indexMap.at(currentValue);

  for (unsigned int i = 0; i < indices.size(); ++i) {
    int index = indices[i];
    indices.erase(indices.begin() + i); // Remove temporarily

    currentPermutation[currentIndex] = index;
    generatePermutations(permutations, nodes, currentPermutation, indexMap,
                         currentIndex + 1);

    indices.insert(indices.begin() + i, index); // Restore (backtrack)
  }
}

std::vector<std::vector<std::pair<unsigned int, unsigned int>>>
generateEdges(std::vector<unsigned int> nodes, unsigned int maxDegree,
              unsigned int minDegree) {
  unsigned int nNodes = nodes.size();
  // Prepare degree filter
  std::vector<triu> degreeFilter(nNodes);
  calculateDegreeFilter(degreeFilter, nNodes);

  // Calculate valid adjacency matrices with min and max number of edges as fast
  // as possible
  std::vector<triu> trius;
  unsigned int totalMaxEdges =
      std::min(nNodes * maxDegree / 2, nNodes * (nNodes - 1) / 2);
  unsigned int triuSize = nNodes * (nNodes - 1) / 2;
  // Parallelize across different numbers of edges
#pragma omp parallel for reduction(merge : trius)
  for (unsigned int i = nNodes - 1; i <= totalMaxEdges; i++) {
    triu v = ((triu)1 << i) - 1;
    triu end = v << (triuSize - i);
    while (v <= end) {
      if (checkDegrees(v, degreeFilter, nNodes, maxDegree, minDegree))
        trius.push_back(v);
      v = nextBitPermutation(v);
    }
  }

  // Generate permutations of nodes used for isomorphism checks in Graph class
  std::unordered_map<int, std::vector<int>> indexMap;
  for (unsigned int i = 0; i < nNodes; ++i) {
    indexMap[nodes[i]].push_back(i);
  }
  std::vector<int> permutation(nNodes, 0);
  std::vector<std::vector<int>> permutations;
  generatePermutations(permutations, nodes, permutation, indexMap, 0);
  // Precompute adjacency triangle bitmasks
  size_t numTriuBits = nNodes * (nNodes - 1) / 2 - 1;
  std::vector<triu> adjTriuMasks;
  adjTriuMasks.reserve(numTriuBits + 1);
  for (size_t count = 0; count <= numTriuBits; ++count) {
    adjTriuMasks.push_back((triu)1 << (numTriuBits - count));
  }

  // Create graphs from adjacency matrices (easy to parallelize!)
  std::vector<Graph> graphs;
#pragma omp parallel for reduction(merge : graphs)
  for (unsigned int i = 0; i < trius.size(); i++) {
    Graph graph =
        Graph(&nodes, trius[i], &degreeFilter, &adjTriuMasks, &permutations);
    if (graph.isConnected())
      graphs.push_back(graph);
  }

  // Filter out identical graphs
  std::map<repr, std::vector<Graph>> uniqueGraphs;
  for (auto &graph : graphs) {
    auto graphMapPtr = uniqueGraphs.find(graph.getRepresentation());
    if (graphMapPtr == uniqueGraphs.end()) {
      uniqueGraphs[graph.getRepresentation()] = std::vector<Graph>();
      uniqueGraphs[graph.getRepresentation()].push_back(graph);
    } else {
      bool found = false;
      for (Graph &uniqueGraph : graphMapPtr->second) {
        bool check = uniqueGraph == graph;
        if (check) {
          found = true;
          break;
        }
      }
      if (!found) {
        graphMapPtr->second.push_back(graph);
      }
    }
  }

  // Combine remaining graphs into a single list
  std::vector<Graph> uniqueGraphsList;
  for (auto &graphMap : uniqueGraphs) {
    uniqueGraphsList.insert(uniqueGraphsList.end(), graphMap.second.begin(),
                            graphMap.second.end());
  }

  // Convert graphs to edge lists and return
  std::vector<std::vector<std::pair<unsigned int, unsigned int>>> edgesResult;
  for (Graph &graph : uniqueGraphsList) {
    edgesResult.push_back(graph.getEdges());
  }
  return edgesResult;
}
