#include "bellman_ford.cpp"
#include "bfs.cpp"
#include "dfs.cpp"
#include "djikstra.cpp"
#include "floyd_warshall.cpp"
#include "matrix_bfs.cpp"
#include "matrix_dfs.cpp"
#include "matrix_djikstra.cpp"
#include "statistics.cpp"

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include <tuple>
#include <type_traits>
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
            "get_dist",
            [](Algorithm& instance, int source, int target){
                return instance.get_dist(source, target);
            },
            py::arg("source"),
            py::arg("target")
        )
        .def(
            "get_aprox_diameter",
            [](Algorithm& instance, int source){
                return instance.get_aprox_diameter(source);
            },
            py::arg("source") = 1
        );

    if constexpr(std::is_base_of_v<IAllPairsAlgorithm<Weight>, Algorithm>){
        algorithm
            .def(
                "run",
                [](Algorithm& instance){ instance.run(); }
            )
            .def(
                "get_dists",
                [](const Algorithm& instance, int source){
                    return instance.get_dists(source);
                },
                py::arg("source")
            )
            .def(
                "get_tree",
                [](const Algorithm& instance, int source){
                    return instance.get_tree(source);
                },
                py::arg("source")
            );
    } else{
        algorithm
            .def(
                "run",
                [](Algorithm& instance, int source){
                    instance.run(source);
                },
                py::arg("source") = 1
            )
            .def(
                "get_dists",
                [](const Algorithm& instance){
                    return instance.get_dists();
                }
            )
            .def(
                "get_tree",
                [](const Algorithm& instance){
                    return instance.get_tree();
                }
            );
    }

    if constexpr(std::is_base_of_v<INegativeCycleDetector, Algorithm>){
        algorithm.def(
            "has_negative_cycle",
            [](const Algorithm& instance){
                return instance.has_negative_cycle();
            }
        );
    }

    if constexpr(std::is_same_v<Algorithm, Bfs<int>> ||
                 std::is_same_v<Algorithm, Dfs<int>> ||
                 std::is_same_v<Algorithm, MatrixBfs<int>> ||
                 std::is_same_v<Algorithm, MatrixDfs<int>>){
        algorithm
            .def(
                "run_from",
                [](Algorithm& instance, int source){
                    instance.run_from(source);
                },
                py::arg("source")
            )
            .def(
                "get_components",
                [](const Algorithm& instance){
                    return instance.get_components();
                }
            );
    }
}

PYBIND11_MODULE(grafos, module){
    module.doc() = "Python bindings for the graph algorithms";

    bind_algorithm<Bfs<int>, int>(module, "BFS");
    bind_algorithm<Dfs<int>, int>(module, "DFS");
    bind_algorithm<Djikstra<double>, double>(module, "Dijkstra");
    bind_algorithm<BellmanFord<double>, double>(module, "BellmanFord");
    bind_algorithm<FloydWarshall<double>, double>(module, "FloydWarshall");
    bind_algorithm<MatrixBfs<int>, int>(module, "MatrixBFS");
    bind_algorithm<MatrixDfs<int>, int>(module, "MatrixDFS");
    bind_algorithm<MatrixDjikstra<double>, double>(
        module, "MatrixDijkstra"
    );

    py::class_<Statistics<double>>(module, "Statistics")
        .def(py::init<int>(), py::arg("vertex_count"))
        .def(
            "add_edge",
            &Statistics<double>::add_edge,
            py::arg("u"),
            py::arg("v"),
            py::arg("weight") = 1.0
        )
        .def(
            "add_edges",
            [](Statistics<double>& instance,
               const std::vector<std::tuple<int, int, double>>& edges){
                for(const auto& [u, v, weight] : edges){
                    instance.add_edge(u, v, weight);
                }
            },
            py::arg("edges")
        )
        .def("run", &Statistics<double>::run)
        .def_readonly("degree_min", &Statistics<double>::gmin)
        .def_readonly("degree_max", &Statistics<double>::gmax)
        .def_readonly("degree_mean", &Statistics<double>::gmean)
        .def_readonly("degree_median", &Statistics<double>::gmedian);
}
