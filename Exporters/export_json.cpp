#include <iostream>
#include <vector>
#include <chrono>
#include <algorithm>
#include <cmath>
#include <fstream>
#include <numeric>
#include <string>
#include <utility>
#include "../src/graph.hpp"

using namespace std;
using namespace std::chrono;

const int NUM_RUNS = 100;

bool get_resident_memory_mb(double& memory_mb){
    ifstream status("/proc/self/status");
    if(!status){
        cerr << "Nao foi possivel ler /proc/self/status.\n";
        return false;
    }

    string key;
    while(status >> key){
        if(key == "VmRSS:"){
            double memory_kb;
            string unit;
            if(!(status >> memory_kb >> unit) || unit != "kB"){
                cerr << "Formato inesperado para VmRSS em /proc/self/status.\n";
                return false;
            }
            memory_mb = memory_kb / 1024.0;
            return true;
        }
        string rest_of_line;
        getline(status, rest_of_line);
    }

    cerr << "VmRSS nao encontrado em /proc/self/status.\n";
    return false;
}

struct TimeStats{
    vector<double> raw;
    double mean;
    double median;
};

struct BenchmarkResults{
    vector<double> bfs;
    vector<double> dfs_iterative;
    vector<double> dijkstra_heap;
};

TimeStats calculate_stats(const vector<double>& times){
    if(times.empty()) return {{}, 0.0, 0.0};
    TimeStats stats;
    stats.raw = times;
    double sum = accumulate(times.begin(), times.end(), 0.0);
    stats.mean = sum / times.size();

    vector<double> sorted_times = times;
    sort(sorted_times.begin(), sorted_times.end());
    if(sorted_times.size() % 2 == 0){
        stats.median = (sorted_times[sorted_times.size() / 2 - 1] +
                        sorted_times[sorted_times.size() / 2]) / 2.0;
    } else{
        stats.median = sorted_times[sorted_times.size() / 2];
    }
    return stats;
}

template<typename T>
void print_json_array(const vector<T>& values){
    cout << "[";
    for(size_t i = 0; i < values.size(); ++i){
        cout << values[i] << (i + 1 == values.size() ? "" : ", ");
    }
    cout << "]";
}

void print_stats(const TimeStats& stats){
    cout << "{\n";
    cout << "        \"raw_ms\": "; print_json_array(stats.raw); cout << ",\n";
    cout << "        \"mean_ms\": " << stats.mean << ",\n";
    cout << "        \"median_ms\": " << stats.median << "\n";
    cout << "      }";
}

void print_algorithm_results(const BenchmarkResults& results){
    cout << "\"BFS\": ";
    print_stats(calculate_stats(results.bfs));
    cout << ",\n      \"DFS_Iterativa\": ";
    print_stats(calculate_stats(results.dfs_iterative));
    cout << ",\n      \"Dijkstra_Heap\": ";
    print_stats(calculate_stats(results.dijkstra_heap));
}

int main(int argc, char* argv[]){
    ios_base::sync_with_stdio(false);
    cin.tie(nullptr);

    const bool weighted = argc == 2 && string(argv[1]) == "--weighted";
    if(argc > 2 || (argc == 2 && !weighted)){
        cerr << "Uso: export_json [--weighted] < entrada.txt\n";
        return 1;
    }

    int n;
    if(!(cin >> n) || n < 0) return 0;

    auto start_build = high_resolution_clock::now();
    struct InputEdge{
        int u;
        int v;
        double weight;
    };
    vector<InputEdge> edges;
    vector<int> expected_degrees(n + 1, 0);
    int u, v;
    while(cin >> u){
        if(!(cin >> v)){
            cerr << "Cada aresta deve conter pelo menos os vertices u e v.\n";
            return 1;
        }
        double weight = 1;
        if(weighted && !(cin >> weight)){
            cerr << "Cada aresta ponderada deve conter u, v e peso.\n";
            return 1;
        }
        if(u < 1 || u > n || v < 1 || v > n){
            cerr << "Aresta com vertice fora do intervalo [1, " << n << "].\n";
            return 1;
        }
        if(!isfinite(weight) || weight < 0){
            cerr << "Os pesos devem ser finitos e nao negativos para Dijkstra.\n";
            return 1;
        }
        edges.push_back({u, v, weight});
        ++expected_degrees[u];
        ++expected_degrees[v];
    }
    if(cin.bad() || !cin.eof()){
        cerr << "Formato invalido nas arestas de entrada.\n";
        return 1;
    }

    Grafo<double> graph(n);
    graph.reserve_adjacencia(expected_degrees);
    for(const auto& edge : edges){
        graph.add_edge(edge.u, edge.v, edge.weight);
    }
    vector<InputEdge>().swap(edges);
    vector<int>().swap(expected_degrees);
    auto end_build = high_resolution_clock::now();
    const double time_build = duration<double, milli>(end_build - start_build).count();

    double rss_before_components_mb;
    if(!get_resident_memory_mb(rss_before_components_mb)) return 1;
    auto start_components = high_resolution_clock::now();
    vector<vector<int>> components;
    vector<int> component_edges;
    graph_lib::BfsFrontier<double> initial_frontier;
    graph.get_components(components, component_edges, initial_frontier);
    auto end_components = high_resolution_clock::now();
    const double time_components =
        duration<double, milli>(end_components - start_components).count();
    double rss_after_components_mb;
    if(!get_resident_memory_mb(rss_after_components_mb)) return 1;

    vector<int> component_sizes;
    for(size_t index = 0; index < components.size(); ++index){
        const int component_size = static_cast<int>(components[index].size());
        component_sizes.push_back(component_size);
    }
    int topology_component_id = 0;
    int topology_component_ve = 0;
    for(size_t index = 0; index < component_edges.size(); ++index){
        const int component_ve = component_sizes[index] + component_edges[index];
        if(component_edges[index] > 0 && component_ve > topology_component_ve){
            topology_component_id = static_cast<int>(index) + 1;
            topology_component_ve = component_ve;
        }
    }

    vector<vector<int>>().swap(components);

    auto start_degree = high_resolution_clock::now();
    graph.build();
    auto end_degree = high_resolution_clock::now();
    const double time_degree = duration<double, milli>(end_degree - start_degree).count();

    const int number_of_runs = min(n, NUM_RUNS);
    BenchmarkResults component_results;
    vector<vector<int>> benchmark_components;
    vector<int> benchmark_component_edges;
    graph_lib::BfsFrontier<double> bfs_frontier;
    graph_lib::DfsFrontier<double> dfs_frontier;
    graph_lib::DijkstraHeapFrontier<double> dijkstra_frontier;
    for(int run = 0; run < number_of_runs; ++run){
        auto begin = high_resolution_clock::now();
        graph.get_components(
            benchmark_components,
            benchmark_component_edges,
            bfs_frontier
        );
        auto end = high_resolution_clock::now();
        component_results.bfs.push_back(
            duration<double, milli>(end - begin).count()
        );

        begin = high_resolution_clock::now();
        graph.get_components(
            benchmark_components,
            benchmark_component_edges,
            dfs_frontier
        );
        end = high_resolution_clock::now();
        component_results.dfs_iterative.push_back(
            duration<double, milli>(end - begin).count()
        );

        begin = high_resolution_clock::now();
        graph.get_components(
            benchmark_components,
            benchmark_component_edges,
            dijkstra_frontier
        );
        end = high_resolution_clock::now();
        component_results.dijkstra_heap.push_back(
            duration<double, milli>(end - begin).count()
        );
    }

    cout << "{\n";
    cout << "  \"ponderado\": " << (weighted ? "true" : "false") << ",\n";
    cout << "  \"n\": " << graph.get_n() << ",\n";
    cout << "  \"m\": " << graph.get_m() << ",\n";
    cout << "  \"v_plus_e\": " << graph.get_n() + graph.get_m() << ",\n";
    cout << "  \"graus\": {\n";
    cout << "    \"minimo\": " << graph.get_grau_minimo() << ",\n";
    cout << "    \"maximo\": " << graph.get_grau_maximo() << ",\n";
    cout << "    \"medio\": " << graph.get_grau_medio() << ",\n";
    cout << "    \"mediano\": " << graph.get_grau_mediano() << "\n";
    cout << "  },\n";
    cout << "  \"telemetria\": {\n";
    cout << "    \"tempo_build_ms\": " << time_build << ",\n";
    cout << "    \"tempo_grau_ms\": " << time_degree << ",\n";
    cout << "    \"tempo_comp_ms\": " << time_components << ",\n";
    cout << "    \"rss_antes_get_components_mb\": "
         << rss_before_components_mb << ",\n";
    cout << "    \"rss_depois_get_components_mb\": "
         << rss_after_components_mb << ",\n";
    cout << "    \"memoria_get_components_delta_mb\": "
         << rss_after_components_mb - rss_before_components_mb << "\n";
    cout << "  },\n";
    cout << "  \"benchmark_get_components\": {\n";
    cout << "    \"numero_execucoes\": " << number_of_runs << ",\n";
    cout << "    ";
    print_algorithm_results(component_results);
    cout << "\n  },\n";
    cout << "  \"tamanhos_componentes\": ";
    print_json_array(component_sizes);
    cout << ",\n";
    cout << "  \"componente_topologia\": {\"id\": "
         << topology_component_id << ", \"n\": "
         << (topology_component_id == 0
                 ? 0
                 : component_sizes[topology_component_id - 1])
         << ", \"m\": "
         << (topology_component_id == 0
                 ? 0
                 : component_edges[topology_component_id - 1])
         << ", \"v_plus_e\": " << topology_component_ve << "}\n";
    cout << "}\n";

    return 0;
}
