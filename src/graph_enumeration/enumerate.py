"""Enumerate undirected graphs based on a given set of node colors / types."""

from typing import Iterator
import logging
from pathlib import Path
from itertools import combinations_with_replacement
import numpy as np
import networkx as nx

from .graph import Graph
from graph_enumeration import EdgeGenerator

logger = logging.getLogger(__name__)


class EdgeConfigurationFileCache:
    """
    Class for handling edge configuration file caching.
    The difficulty of different vector lengths per sparse edge configuration is handled by storing
    all edge configurations in a single flat binary file, along with an offset file tha indicates
    the start and end positions of each configuration in the flat file.
    """

    def __init__(self, cache_path: str | Path, file_basename: str):
        """
        Initialize an EdgeConfigurationCache instance.
        :param cache_path: Path to the directory where cache files are stored.
        :param file_basename: Base name for the cache files.
        """
        Path(cache_path).mkdir(parents=True, exist_ok=True)
        self.cache_path_flat = Path(cache_path) / f"{file_basename}.flat.bin"
        self.cache_path_offset = Path(cache_path) / f"{file_basename}.offset.bin"

    def exists(self) -> bool:
        """Check if both cache files exist."""
        return self.cache_path_flat.exists() and self.cache_path_offset.exists()

    def save(self, edge_configurations: list[list[tuple[int, int]]]):
        """
        Save edge configurations to cache files.
        :param edge_configurations: List of edge configurations to save to the cache files.
        """
        # Flatten edge configurations for storage
        flat = np.array(
            [x for config in edge_configurations for edge in config for x in edge],
            dtype=np.int32,
        )
        # Calculate offsets
        offsets = np.zeros((len(edge_configurations), 2), dtype=np.int64)
        idx = 0
        for i, config in enumerate(edge_configurations):
            offsets[i] = (idx, idx + 2 * len(config))
            idx += 2 * len(config)
        # Save to binary files
        flat.tofile(self.cache_path_flat)
        offsets.tofile(self.cache_path_offset)

    def load_all(
        self,
        index_list: list[int] | None = None,
    ) -> list[np.ndarray]:
        """
        Load all edge configurations from cache files.
        :param index_list: Optional list to map edge indices back to original node indices.
        :return: List of edge configurations.
        """
        # Load from binary files
        flat = np.fromfile(self.cache_path_flat, dtype=np.int32)
        offsets = np.fromfile(self.cache_path_offset, dtype=np.int64).reshape(-1, 2)
        # Split the flat array into tuples
        tuples = flat.reshape(-1, 2)
        # Map back to original indices if provided
        if index_list is not None:
            index_list = np.array(index_list, dtype=np.int32)
            tuples = index_list[tuples]
        # Compute lengths of each sublist
        lengths = (offsets[:, 1] - offsets[:, 0]) // 2
        # Calculate split indices
        split_points = np.cumsum(lengths)[:-1]
        # Split into edge configurations
        edge_configurations = np.split(tuples, split_points)
        return edge_configurations

    def load_random(
        self, n: int | float, index_list: list[int] | None = None
    ) -> list[np.ndarray]:
        """
        Load n random edge configurations from cache files. This function is faster than loading
        all and sampling if n is much smaller than the total number of configurations.
        :param n: Number or percentage of random configurations to load.
        :param index_list: Optional list to map edge indices back to original node indices.
        :return: List of edge configurations.
        """
        # Load from binary files
        flat = np.memmap(self.cache_path_flat, dtype=np.int32)
        offsets = np.memmap(self.cache_path_offset, dtype=np.int64).reshape(-1, 2)
        total_configs = offsets.shape[0]
        # Determine number of configurations to load
        if 0 < n < 1:
            n = max(int(total_configs * n), 1)
        if n < 1 or int(n) != n:
            raise ValueError("Invalid number of configurations to load.")
        n = int(min(n, total_configs))
        # Randomly select indices
        selected_indices = np.random.choice(total_configs, size=n, replace=False)
        if index_list is not None:
            index_list = np.array(index_list, dtype=np.int32)
        edge_configurations = []
        for idx in selected_indices:
            start, end = offsets[idx]
            tuples = flat[start:end].reshape(-1, 2)
            # Map back to original indices if provided
            if index_list is not None:
                tuples = index_list[tuples]
            edge_configurations.append(np.array(tuples))
        return edge_configurations


class GraphGenerator:
    """Class to enumerate undirected graphs based on a given set of node colors / types."""

    def __init__(
        self,
        nodes: list[str | int],
        max_nodes: int,
        max_degree: int | None = None,
        min_degree: int = 1,
        min_nodes: int | None = None,
        max_cycle_size: int | None = None,
        use_memory_cache: bool = True,
        file_cache_path: str | Path | None = None,
    ):
        """
        Initialize the GraphGenerator.

        :param nodes: List of possible node types/colors (e.g. ["C", "H", "O"]).
        :param max_nodes: Maximum number of nodes in the generated graphs.
        :param max_degree: Maximum degree of any node. Defaults to max_nodes - 1.
        :param min_degree: Minimum degree of any node. Defaults to 1.
        :param min_nodes: Minimum number of nodes. Defaults to max_nodes.
        :param max_cycle_size: Maximum size of cycles allowed in the graph. None for no limit.
        :param use_memory_cache: Whether to cache results in memory.
        :param file_cache_path: Path to directory for file-based caching. None to disable file
            cache. Only used for graphs with more than 5 nodes.
        """
        self.nodes = nodes
        self.max_nodes = max_nodes
        self.min_nodes = min_nodes if min_nodes is not None else max_nodes
        self.max_degree = max_degree if max_degree is not None else max_nodes - 1
        self.min_degree = min_degree
        self.use_file_cache = file_cache_path is not None
        if self.use_file_cache:
            self.file_cache_path = Path(file_cache_path)
        self.use_memory_cache = use_memory_cache
        self.max_cycle_size = max_cycle_size
        self.basename_part = f"D{self.min_degree}-{self.max_degree}" + (
            f"C{self.max_cycle_size}" if self.max_cycle_size is not None else ""
        )
        self.memory_cache_size_limit = 5 if self.use_file_cache else np.inf
        self.memory_cache = {}

    @staticmethod
    def _get_canonical_representation(
        node_list: list[str | int],
    ) -> tuple[tuple[int], list[int]]:
        """
        Get the canonical representation of a set of nodes. Two sets of nodes with the same
        canonical representation will have the same edge configuration. For example, the nodes
        [A, B, A] and [C, C, A] will have the same canonical representation [0, 0, 1] and index
        lists of [0, 2, 1] and [0, 1, 2], respectively.
        :param node_list: List of node colors / types / names.
        :return: Tuple of two lists. The first contains the canonical representation
        """
        node_to_index = {}
        for idx, node in enumerate(node_list):
            if node not in node_to_index:
                node_to_index[node] = [idx]
            else:
                node_to_index[node].append(idx)
        node_to_index = sorted(
            node_to_index.items(), key=lambda x: len(x[1]), reverse=True
        )
        canonical_representation = []
        index_list = []
        for canonical_idx, (_, indices) in enumerate(node_to_index):
            canonical_representation += [canonical_idx] * len(indices)
            index_list += indices
        return tuple(canonical_representation), index_list

    def _has_large_cycles(self, edge_config: list[tuple[int, int]]) -> bool:
        """
        Check if the edge configuration has cycles larger than the maximum allowed size.
        :param edge_config: List of edge tuples.
        :return: True if there are large cycles, False otherwise.
        """
        graph = nx.from_edgelist(edge_config)
        cycles = nx.cycle_basis(graph)
        max_cycle = max((len(cycle) for cycle in cycles), default=0)
        return max_cycle > (self.max_cycle_size or max_cycle)

    def get_all_edge_configurations(self, node_types: list[str]) -> list[np.ndarray]:
        """
        Get all unique edge configurations for the given node types.
        :param node_types: List of node types.
        :return: List of unique edge configurations.
        """
        # Get canonical representation
        canonical_rep, index_list = self._get_canonical_representation(node_types)
        # Use memory caching for small systems
        if (
            self.use_memory_cache
            and len(node_types) <= self.memory_cache_size_limit
            and canonical_rep in self.memory_cache
        ):
            edge_configurations = self.memory_cache[canonical_rep]
        else:
            # Use file caching if applicable
            if self.use_file_cache and len(node_types) > 5:
                basename = f"{self.basename_part}B{'-'.join(map(str, canonical_rep))}"
                edge_file_cache = EdgeConfigurationFileCache(
                    self.file_cache_path, basename
                )
                if edge_file_cache.exists():
                    return edge_file_cache.load_all(index_list=index_list)
            # If not cached, generate edge configurations
            edge_configurations = EdgeGenerator.generate(
                canonical_rep, max_degree=self.max_degree, min_degree=self.min_degree
            )
            # Filter out configurations with large cycles
            if self.max_cycle_size is not None:
                edge_configurations = [
                    config
                    for config in edge_configurations
                    if not self._has_large_cycles(config)
                ]
            # Save to memory cache if applicable
            if (
                self.use_memory_cache
                and len(node_types) <= self.memory_cache_size_limit
            ):
                self.memory_cache[canonical_rep] = edge_configurations
            # Save to file cache if applicable
            if self.use_file_cache and len(node_types) > 5:
                edge_file_cache.save(edge_configurations)
        # Map back to original node indices
        edge_configurations = [
            np.array([(index_list[edge[0]], index_list[edge[1]]) for edge in config])
            for config in edge_configurations
        ]
        return edge_configurations

    def iterator(self) -> Iterator[Graph]:
        """
        Graph iterator.
        :return: Iterator over Graph instances.
        """
        for n_nodes in range(self.min_nodes, self.max_nodes + 1):
            node_combinations = list(combinations_with_replacement(self.nodes, n_nodes))
            for node_list in node_combinations:
                for edge_configuration in self.get_all_edge_configurations(node_list):
                    yield Graph(node_list, edge_configuration)
