import pytest
from graph_enumeration import EdgeGenerator
from test_helper import generate_edges


@pytest.mark.parametrize(
    "nodes, max_degree, min_degree",
    [
        ((0, 0, 0), 2, 1),
        ((0, 1, 2), 2, 1),
        ((0, 0, 1), 2, 1),
        ((0, 0, 0, 0), 3, 1),
        ((0, 0, 1, 1), 3, 1),
        ((0, 1, 2, 3), 3, 1),
        ((0, 0, 1, 2), 3, 1),
        ((0, 0, 0, 1, 1), 4, 1),
        ((0, 1, 2, 3, 4), 4, 1),
        ((0, 0, 1, 2, 3), 4, 1),
        ((0, 0, 0), 2, 2),
        ((0, 1, 2), 2, 2),
        ((0, 0, 1), 2, 2),
    ],
)
def test_small_graphs(nodes, max_degree, min_degree):
    """Test edge generation for small graphs against the slow Python implementation."""
    # C++ implementation
    cpp_edges = EdgeGenerator.generate(
        nodes, max_degree=max_degree, min_degree=min_degree
    )
    cpp_count = len(cpp_edges)

    # Python implementation
    py_count = generate_edges(nodes, max_degree=max_degree, min_degree=min_degree)

    assert cpp_count == py_count, (
        f"Mismatch for nodes={nodes}, max_degree={max_degree}, min_degree={min_degree}"
    )


def test_invalid_inputs():
    """Test that invalid inputs raise appropriate errors."""
    with pytest.raises(TypeError):
        EdgeGenerator.generate("not a tuple", 2, 1)
    with pytest.raises(ValueError):
        EdgeGenerator.generate((), 2, 1)  # Empty tuple
    with pytest.raises(ValueError):
        EdgeGenerator.generate((0, 0, 2), 2, 1)  # Non-increasing/gap in types
    with pytest.raises(ValueError):
        EdgeGenerator.generate((0, 1, 1), -1, 1)  # Negative max_degree
    with pytest.raises(ValueError):
        EdgeGenerator.generate((1, 2, 3), 2, 1)  # Does not start with 0
    with pytest.raises(ValueError):
        EdgeGenerator.generate((0, 1, 2), 1, 2)  # min_degree > max_degree


def test_single_node():
    """Test edge generation for a single node."""
    edges = EdgeGenerator.generate((0,), max_degree=1, min_degree=0)
    assert len(edges) == 1
    assert len(edges[0]) == 0  # No edges
