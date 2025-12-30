from graph_enumeration import GraphGenerator


def test_canonical_representation():
    """Test the canonical representation of node lists."""
    # Test case 1: Simple list
    nodes = ["A", "B", "A"]
    canonical, indices = GraphGenerator._get_canonical_representation(nodes)
    assert canonical == (0, 0, 1)
    assert indices == [0, 2, 1]

    # Test case 2: All same
    nodes = [1, 1, 1]
    canonical, indices = GraphGenerator._get_canonical_representation(nodes)
    assert canonical == (0, 0, 0)
    assert indices == [0, 1, 2]

    # Test case 3: All different
    nodes = ["X", "Y", "Z"]
    canonical, indices = GraphGenerator._get_canonical_representation(nodes)
    assert canonical == (0, 1, 2)
    assert indices == [0, 1, 2]


def test_simple_graph_generation_count():
    """Test that the generator produces the correct number of graphs for a known case."""
    gen = GraphGenerator(["A"], max_nodes=3, min_nodes=3, max_degree=2, min_degree=1)
    graphs = list(gen.iterator())
    # Expected: Path (1-2-3) and Triangle (1-2-3-1)
    assert len(graphs) == 2


def test_specific_large_case():
    """
    Test that the GraphGenerator setup with 7 node types and max_nodes=5, min_nodes=1
    (other params default) generates exactly 124,327 graphs.
    """
    node_types = [0, 1, 2, 3, 4, 5, 6]
    gen = GraphGenerator(
        node_types,
        max_nodes=5,
        min_nodes=1,
        # Defaults: max_degree = max_nodes - 1 = 4, min_degree = 1
    )
    count = sum(1 for _ in gen.iterator())
    # Number used in this publication: https://doi.org/10.1039/D2SC00116K
    assert count == 124327
