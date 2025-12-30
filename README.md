# Graph enumeration for colored graphs

`graph-enumeration` is a library for the exhaustive generation of unique, undirected graphs with colored nodes. It generates all non-isomorphic connected graphs definable over a specific multiset of node types (colors), subject to topological constraints such as minimum and maximum degree and maximum cycle size. Due to the internal 64-bit representation of the adjacency matrix, the library supports graphs with a maximum of 11 nodes.

While Python offers robust libraries for graph manipulation (e.g., [NetworkX](https://networkx.org/), [igraph](https://igraph.org/), [graph-tool](https://graph-tool.skewed.de/)), they typically lack efficient algorithms for exhaustive combinatorial enumeration. `geng` (part of the [nauty package](https://pallini.di.uniroma1.it/)) is the standard for enumerating uncolored graphs but is unsuitable for domains where node identity is significant (e.g., chemical graph theory or heterogeneous networks). This library addresses the enumeration of graphs with distinguishable node types, ensuring the generation of topologically unique structures.

The library combines Python and C++ to maximize performance. Python manages the combinatorial generation of node multisets, while the C++ extension executes the edge enumeration logic.
1.  The upper-triangular adjacency matrix is encoded as a 64-bit integer, enabling efficient bitwise operations.
2.  The algorithm iterates through the space of possible edge configurations, pruning candidates that violate degree constraints or connectivity requirements.
3.  Uniqueness is enforced by filtering isomorphic duplicates through the verification of all valid node permutations that preserve color invariants.
4.  The procedure is parallelized via OpenMP to exploit multi-core architectures.

### Installation instructions

To install the library, you need a C++ compiler supporting C++17 and OpenMP.

```bash
pip install .
```

### Usage

Simple example of how to use the `GraphGenerator` class:

```python
from graph_enumeration import GraphGenerator

node_types = ["C", "O", "N", "F"]

# Initialize the enumerator with constraints
enumerator = GraphGenerator(
    nodes=node_types,
    max_nodes=5,
    min_nodes=1,
    max_degree=3,
    min_degree=1,
    max_cycle_size=4,
)

# Generate all unique graphs
graphs = list(enumerator.iterator())

print(f"Generated {len(graphs)} unique graphs.")
```

### Instructions to run the tests with pytest

To run the tests, you first need to install `pytest` e.g. via the installation of the development dependencies:

```bash
pip install -e .[dev]
```

Then, you can execute the tests using `pytest`:

```bash
pytest
```
