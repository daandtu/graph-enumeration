"""Simple graph class."""

import numpy as np
import networkx as nx


class Graph:
    """Class representing a graph with nodes and edges."""

    def __init__(self, nodes: list[str | int], edges: np.ndarray):
        """
        Initialize a Graph instance.
        :param nodes: List of nodes in the graph.
        :param edges: Numpy array of shape (num_edges, 2) representing the edges between nodes.
            Each row is a pair of indices into the `nodes` list.
        """
        self.nodes = nodes
        self.edges = edges

    def to_networkx(self) -> nx.Graph:
        """
        Convert the graph to a NetworkX graph. The node type / color is assigned as an
        attribute "type".
        :return: NetworkX graph representation of the graph.
        """
        graph = nx.Graph()
        for i, node in enumerate(self.nodes):
            graph.add_node(i, type=node)
        for a, b in self.edges:
            graph.add_edge(a, b)
        return graph

    def __repr__(self) -> str:
        nodes = ",".join(map(str, self.nodes))
        edges = ",".join(f"({a}-{b})" for a, b in self.edges)
        return f"Graph(nodes=[{nodes}]; edges=[{edges}])"
