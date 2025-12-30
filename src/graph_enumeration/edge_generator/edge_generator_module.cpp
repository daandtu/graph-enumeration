/*
    EdgeGenerator module for fast edge generation in C++
    This file contains the Python bindings for the edge generation function.
*/

#define PY_SSIZE_T_CLEAN
#include "edge_generator.h"
#include <Python.h>

/**
 * @brief Python wrapper for the edge generation function.
 *
 * This function parses the Python arguments, converts them to C++ types,
 * calls the C++ `generateEdges` function, and converts the result back to a
 * Python list.
 *
 * @param self The module object (unused).
 * @param args The positional arguments.
 * @param kwds The keyword arguments.
 * @return A Python list of edge lists, or NULL on error.
 */
static PyObject *generatePyEdges(PyObject *self, PyObject *args,
                                 PyObject *kwds) {
  // Intialize argument variables
  PyObject *nodeTuple = nullptr;
  int pyMaxDegree;
  int pyMinDegree;
  // Parse arguments and check types
  static const char *kwlist[] = {"nodes", "max_degree", "min_degree", nullptr};
  if (!PyArg_ParseTupleAndKeywords(args, kwds, "OII", (char **)kwlist,
                                   &nodeTuple, &pyMaxDegree, &pyMinDegree)) {
    PyErr_SetString(PyExc_TypeError, "Expected a tuple of integers, an integer "
                                     "max_degree, and an integer min_degree");
    return nullptr;
  }
  if (!PyTuple_Check(nodeTuple)) {
    PyErr_SetString(PyExc_TypeError,
                    "First argument must be a tuple of integer node values");
    return nullptr;
  }
  if (pyMaxDegree < pyMinDegree) {
    PyErr_SetString(PyExc_ValueError,
                    "max_degree must be greater than or equal to min_degree");
    return nullptr;
  } else if (pyMinDegree < 0) {
    PyErr_SetString(PyExc_ValueError,
                    "min_degree must be a non-negative integer");
    return nullptr;
  }
  unsigned int maxDegree = (unsigned int)pyMaxDegree;
  unsigned int minDegree = (unsigned int)pyMinDegree;

  // Convert the tuple to a vector of integers and perform some checks
  unsigned int size = (unsigned int)PyTuple_GET_SIZE(nodeTuple);
  if (size == 0) {
    PyErr_SetString(PyExc_ValueError, "Node value tuple must not be empty");
    return nullptr;
  } else if (size > 11) {
    PyErr_SetString(PyExc_ValueError, "Only up to 11 nodes are supported");
    return nullptr;
  }
  std::vector<unsigned int> nodes;
  nodes.reserve(size);
  for (unsigned int i = 0; i < size; i++) {
    PyObject *pyNode = PyTuple_GET_ITEM(nodeTuple, i);
    if (!PyLong_Check(pyNode)) {
      PyErr_SetString(PyExc_TypeError, "All node values must be integers");
      return nullptr;
    }
    long node = PyLong_AsLong(pyNode);
    if (node < 0) {
      PyErr_SetString(PyExc_ValueError,
                      "All node values must be non-negative integers");
      return nullptr;
    }
    if (i == 0 && node != 0) {
      PyErr_SetString(PyExc_ValueError,
                      "The first node value in the tuple must be 0");
      return nullptr;
    } else if (i > 0 && (node != nodes[i - 1] && node != nodes[i - 1] + 1)) {
      PyErr_SetString(PyExc_ValueError,
                      "The node values must be strictly increasing");
      return nullptr;
    }
    nodes.push_back((unsigned int)node);
  }

  // Generate all possible edge configurations and convert to Python list
  if (size > 1) {
    // Actual edge generation
    std::vector<std::vector<std::pair<unsigned int, unsigned int>>> edges =
        generateEdges(nodes, maxDegree, minDegree);

    // Convert to Python list
    PyObject *result = PyList_New(edges.size());
    for (unsigned int i = 0; i < edges.size(); i++) {
      PyObject *edgeList = PyList_New(edges[i].size());
      for (unsigned int j = 0; j < edges[i].size(); j++) {
        PyList_SET_ITEM(
            edgeList, j,
            Py_BuildValue("(ii)", edges[i][j].first, edges[i][j].second));
      }
      PyList_SET_ITEM(result, i, edgeList);
    }

    return result;
  } else {
    // If there is only one node, there are no edges
    PyObject *result = PyList_New(1);
    PyList_SET_ITEM(result, 0, PyList_New(0));
    return result;
  }
}

static PyMethodDef EdgeGeneratorMethods[] = {
    {"generate", (PyCFunction)generatePyEdges, METH_VARARGS | METH_KEYWORDS,
     "Generate all valid and unique edge configurations for a given sequence "
     "of ordered nodes "
     "and minimum and maximum numbers of edges per node."},
    {nullptr, nullptr, 0, nullptr}};

static struct PyModuleDef edgeGeneratorModule = {
    PyModuleDef_HEAD_INIT, "EdgeGenerator",
    "Helper module for fast edge generation.", -1, EdgeGeneratorMethods};

PyMODINIT_FUNC PyInit_EdgeGenerator(void) {
  return PyModule_Create(&edgeGeneratorModule);
}
