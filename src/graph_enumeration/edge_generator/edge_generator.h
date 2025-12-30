#pragma once

#include "graph.h"
#include <algorithm>
#include <chrono>
#include <ctime>
#include <iomanip>
#include <iostream>
#include <map>
#include <math.h>
#include <omp.h>
#include <queue>
#include <unordered_map>
#include <utility>
#include <vector>

#pragma omp declare reduction(merge : std::vector<Graph> : omp_out.insert(     \
        omp_out.end(), omp_in.begin(), omp_in.end()))
#pragma omp declare reduction(merge : std::vector<triu> : omp_out.insert(      \
        omp_out.end(), omp_in.begin(), omp_in.end()))

// HELPER FUNCTIONS

/**
 * @brief Precompute degree filter bitmasks.
 *
 * For each node, this function generates a bitmask that selects all edges
 * connected to that node in the upper triangular adjacency matrix
 * representation.
 *
 * @param result Vector to store the resulting bitmasks.
 * @param nNodes Number of nodes in the graph.
 */
void calculateDegreeFilter(std::vector<triu> &result, unsigned int nNodes);

/**
 * @brief Check if the adjacency matrix represented by a binary number is valid.
 *
 * A valid adjacency matrix must satisfy the degree constraints for all nodes.
 *
 * @param triAdjMat Binary representation of the upper adjacency matrix.
 * @param degreeFilter Precomputed degree filter bitmasks.
 * @param nNodes Number of nodes.
 * @param maxDegree Maximum allowed degree per node.
 * @param minDegree Minimum allowed degree per node.
 * @return true if the degree constraints are satisfied, false otherwise.
 */
bool checkDegrees(const triu triAdjMat, const std::vector<triu> &degreeFilter,
                  const unsigned int nNodes, const unsigned int maxDegree,
                  const unsigned int minDegree);

/**
 * @brief Get the next lexicographical permutation of a binary number with the
 * same number of set bits.
 *
 * This is used to iterate over all adjacency matrices with a fixed number of
 * edges. See:
 * https://graphics.stanford.edu/~seander/bithacks.html#NextBitPermutation
 *
 * @param v Current binary number.
 * @return The next binary number with the same number of set bits.
 */
inline triu nextBitPermutation(triu v) {
  triu t = v | (v - 1);
  return (t + 1) | (((~t & -~t) - 1) >> (__builtin_ctzll(v) + 1));
}

/**
 * @brief Recursively generate all valid permutations of node indices.
 *
 * This function generates permutations that respect node colors (types).
 * Only nodes of the same color can be swapped.
 *
 * @param permutations Vector to store the generated permutations.
 * @param nodes Vector of node colors.
 * @param nNodes Number of nodes.
 * @param currentPermutation Buffer for the current permutation being built.
 * @param indexMap Map from node color to list of original indices.
 * @param currentIndex Current recursion depth (node index).
 */
void generatePermutations(std::vector<std::vector<int>> &permutations,
                          const std::vector<unsigned int> &nodes,
                          std::vector<int> &currentPermutation,
                          std::unordered_map<int, std::vector<int>> &indexMap,
                          unsigned int currentIndex);

// MAIN FUNCTION

/**
 * @brief Generate all valid edge configurations for a given sequence of nodes.
 *
 * This is the main entry point. It generates all non-isomorphic connected
 * graphs with the given node colors and degree constraints.
 *
 * @param nodes Vector of node colors (must be start with 0 and strictly
 * increasing).
 * @param maxDegree Maximum degree per node.
 * @param minDegree Minimum degree per node.
 * @return A vector of edge lists, where each edge list represents a unique
 * graph.
 */
std::vector<std::vector<std::pair<unsigned int, unsigned int>>>
generateEdges(std::vector<unsigned int> nodes, unsigned int maxDegree,
              unsigned int minDegree);
