"""Simple & slow script to test the generation of all possible graphs for a given set of nodes."""

from itertools import product
import networkx as nx
import numpy as np


def _node_match(n1, n2):
    return n1["name"] == n2["name"]


def generate_edges(nodes: tuple[int], max_degree: int = 4, min_degree: int = 1) -> int:
    """
    Test function to generate all possible graphs for a given set of node types.
    Similar to the C++ implementation, the nodes list is expected to start with 0 and
    is strictly increasing.
    This function is not intended for direct use, but rather as a simple test to validate the
    optimized C++ implementation.
    :param nodes: List of node types.
    :param max_degree: Maximum degree of any node.
    :param min_degree: Minimum degree of any node.
    :return: Number of unique edge configurations.
    """
    size = len(nodes)
    graph_map = {}

    attributes = {i: {"name": v} for i, v in enumerate(nodes)}

    for edge in product([0, 1], repeat=size * (size - 1) // 2):
        adjacency = np.zeros((size, size))
        adjacency[np.triu_indices(size, k=1)] = edge
        g = nx.from_numpy_array(adjacency)
        degrees = [val for (_, val) in g.degree()]
        if (
            not nx.is_connected(g)
            or max(degrees) > max_degree
            or min(degrees) < min_degree
        ):
            continue
        nx.set_node_attributes(g, attributes)
        edge_info = sorted(
            [f"{g.nodes[u]['name']}-{g.nodes[v]['name']}" for (u, v) in g.edges()]
        )
        counts = [edge_info.count(edge) for edge in set(edge_info)]
        representation = " ".join(map(str, [sum(edge)] + sorted(degrees) + counts))
        if representation not in graph_map:
            graph_map[representation] = [g]
        else:
            found = False
            for existing_graph in graph_map[representation]:
                if nx.is_isomorphic(existing_graph, g, _node_match):
                    found = True
                    break
            if not found:
                graph_map[representation].append(g)

    result = [list(g.edges) for graphs in graph_map.values() for g in graphs]

    return len(result)
