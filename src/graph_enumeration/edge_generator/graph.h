#pragma once

#include <algorithm>
#include <cstdint>
#include <queue>
#include <vector>

/**
 * @brief Numerical representation of a graph.
 *
 * This vector contains graph invariants (like edge counts, degree sequences)
 * and is used for a fast initial comparison to filter out obviously
 * non-isomorphic graphs. It is not a unique identifier (canonical label).
 */
using repr = std::vector<int>;

/**
 * @brief Binary representation of the upper triangular part of the adjacency
 * matrix.
 *
 * Since the graphs are undirected, the adjacency matrix is symmetric. We only
 * store the upper triangle (excluding the diagonal, as there are no self-loops)
 * packed into a 64-bit integer. This limits the maximum number of nodes to
 * roughly 11 (since 11*10/2 = 55 bits).
 */
using triu = std::uint64_t;

/**
 * @brief Get the index in the flattened upper triangular matrix for a given
 * edge (i, j).
 *
 * @param i Node index i.
 * @param j Node index j.
 * @param nNodes Number of nodes.
 * @return The bit index corresponding to edge (i, j) in the `triu`
 * representation.
 */
inline size_t upperTriuIndex(unsigned int i, unsigned int j,
                             unsigned int nNodes) {
  if (i > j)
    std::swap(i, j);
  return i * nNodes - (i * i + 3 * i) / 2 + j - 1;
}

/**
 * @class Graph
 * @brief Represents an undirected graph with colored (typed) nodes.
 *
 * This class provides methods to check for connectivity, isomorphism, and to
 * retrieve graph properties like the adjacency matrix and edge list.
 * It uses a compact binary representation for the adjacency matrix to optimize
 * memory usage and performance during enumeration.
 */
class Graph {
public:
  /**
   * @brief Construct a new Graph object.
   *
   * @param nodes Pointer to vector of node colors/types.
   * @param adjacencyTriu Binary representation of the upper triangular
   * adjacency matrix.
   * @param degreeFilter Pointer to precomputed degree filter bitmasks.
   * @param adjacencyTriuMasks Pointer to precomputed bitmasks for accessing
   * individual edges in the binary representation.
   * @param permutations Pointer to precomputed node permutations for
   * isomorphism checks.
   */
  Graph(const std::vector<unsigned int> *nodes, triu adjacencyTriu,
        const std::vector<triu> *degreeFilter,
        const std::vector<triu> *adjacencyTriuMasks,
        const std::vector<std::vector<int>> *permutations);

  /**
   * @brief Check if this graph is isomorphic to another graph.
   *
   * This operator first compares the numerical representations (invariants).
   * If they match, it performs a brute-force check over all valid permutations
   * of nodes (respecting node colors) to see if the adjacency matrices can be
   * made identical.
   *
   * @param other The other graph to compare with.
   * @return true if the graphs are isomorphic, false otherwise.
   */
  bool operator==(const Graph &other) const;

  // Getter methods

  /**
   * @brief Get the numerical representation (invariants) of the graph.
   * @return A vector of integers representing the graph invariants.
   */
  repr getRepresentation() const;

  /**
   * @brief Get the full adjacency matrix.
   * @return A 2D vector representing the adjacency matrix (0 or 1).
   */
  std::vector<std::vector<uint8_t>> getAdjacencyMatrix() const;

  /**
   * @brief Check if the graph is connected.
   * @return true if the graph is connected, false otherwise.
   */
  bool isConnected() const;

  /**
   * @brief Get the list of edges in the graph.
   * @return A vector of pairs, where each pair represents an edge (u, v).
   */
  std::vector<std::pair<unsigned int, unsigned int>> getEdges() const;

private:
  // Instance specific attributes
  size_t nNodes;                          ///< Number of nodes
  const std::vector<unsigned int> *nodes; ///< Array of node types/names/colors
  repr representation; ///< Numerical representation of the graph
  bool connected;      ///< Whether the graph is connected
  triu adjacencyTriu; ///< Upper triangle adjacency matrix binary representation

  // Pointers to precomputed attributes shared across graphs
  const std::vector<triu> *degreeFilter;       ///< Degree filter bitmask array
  const std::vector<triu> *adjacencyTriuMasks; ///< Adjacency triangle bitmasks
  const std::vector<std::vector<int>> *permutations; ///< Possible permutations

  // Graph property calculation methods called during initialization
  void checkConnected();          // Check if the graph is connected
  void calculateRepresentation(); // Compute numerical representation
  bool isEdge(unsigned int u, unsigned int v) const; // Check if edge exists
};