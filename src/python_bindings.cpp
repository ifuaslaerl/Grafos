#include "bellman_ford.cpp"
#include "bfs.cpp"
#include "dfs.cpp"
#include "djikstra.cpp"
#include "floyd_warshall.cpp"

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include <tuple>
#include <vector>

namespace py = pybind11;

template<typename Algorithm, typename Weight>
void bind_algorithm(py::module_& module, const char* name){
    py::class_<Algorithm> algorithm(module, name);
    algorithm
        .def(py::init<int>(), py::arg("vertex_count"))
        .def(
            "add_edge",
            [](Algorithm& instance, int u, int v, Weight weight){
                instance.add_edge(u, v, weight);
            },
            py::arg("u"),
            py::arg("v"),
            py::arg("weight") = Weight{1}
        )
        .def(
            "add_edges",
            [](Algorithm& instance,
               const std::vector<std::tuple<int, int, Weight>>& edges){
                for(const auto& [u, v, weight] : edges){
                    instance.add_edge(u, v, weight);
                }
            },
            py::arg("edges")
        )
        .def(
            "should_run",
            [](const Algorithm& instance, int vertex_count, int edge_count){
                return instance.should_run(vertex_count, edge_count);
            },
            py::arg("vertex_count"),
            py::arg("edge_count")
        )
        .def(
            "run",
            [](Algorithm& instance, int source){ instance.run(source); },
            py::arg("source") = 1
        )
        .def(
            "get_dists",
            [](const Algorithm& instance, int source){
                return instance.get_dists(source);
            },
            py::arg("source") = 1
        )
        .def(
            "get_tree",
            [](const Algorithm& instance, int source){
                return instance.get_tree(source);
            },
            py::arg("source") = 1
        )
        .def(
            "has_negative_cycle",
            [](const Algorithm& instance){
                return instance.has_negative_cycle();
            }
        );
}

PYBIND11_MODULE(grafos, module){
    module.doc() = "Python bindings for the graph algorithms";

    bind_algorithm<Bfs<int>, int>(module, "BFS");
    bind_algorithm<Dfs, int>(module, "DFS");
    bind_algorithm<Djikstra<double>, double>(module, "Dijkstra");
    bind_algorithm<BellmanFord<double>, double>(module, "BellmanFord");
    bind_algorithm<FloydWarshall<double>, double>(module, "FloydWarshall");
}
