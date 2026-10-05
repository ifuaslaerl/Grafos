#include <iostream>
#include <vector>
#include <chrono>
#include <algorithm>
#include <numeric>
#include <random>
#include <fstream>
#include <string>
#include <utility>
#include <unordered_set>
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
    vector<double> dfs_recursive;
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
    cout << ",\n      \"DFS_Recursiva\": ";
    print_stats(calculate_stats(results.dfs_recursive));
}

int main(){
    ios_base::sync_with_stdio(false);
    cin.tie(nullptr);

    int n;
    if(!(cin >> n) || n < 0) return 0;

    auto start_build = high_resolution_clock::now();
    vector<pair<int, int>> edges;
    vector<int> expected_degrees(n + 1, 0);
    int u, v;
    while(cin >> u >> v){
        if(u < 1 || u > n || v < 1 || v > n){
            cerr << "Aresta com vertice fora do intervalo [1, " << n << "].\n";
            return 1;
        }
        edges.push_back({u, v});
        ++expected_degrees[u];
        ++expected_degrees[v];
    }
    if(cin.bad() || !cin.eof()){
        cerr << "Formato invalido nas arestas de entrada.\n";
        return 1;
    }

    Grafo<int> graph(n);
    graph.reserve_adjacencia(expected_degrees);
    for(const auto& edge : edges){
        graph.add_edge(edge.first, edge.second);
    }
    vector<pair<int, int>>().swap(edges);
    vector<int>().swap(expected_degrees);
    auto end_build = high_resolution_clock::now();
    const double time_build = duration<double, milli>(end_build - start_build).count();

    double rss_before_components_mb;
    if(!get_resident_memory_mb(rss_before_components_mb)) return 1;
    auto start_components = high_resolution_clock::now();
    vector<vector<int>> components = graph.get_components();
    auto end_components = high_resolution_clock::now();
    const double time_components =
        duration<double, milli>(end_components - start_components).count();
    double rss_after_components_mb;
    if(!get_resident_memory_mb(rss_after_components_mb)) return 1;

    vector<int> component_sizes;
    component_sizes.reserve(components.size());
    for(const auto& component : components){
        component_sizes.push_back(static_cast<int>(component.size()));
    }

    vector<vector<int>>().swap(components);

    auto start_degree = high_resolution_clock::now();
    graph.build();
    auto end_degree = high_resolution_clock::now();
    const double time_degree = duration<double, milli>(end_degree - start_degree).count();

    mt19937 generator(42);
    vector<int> start_vertices;
    const int number_of_runs = min(n, NUM_RUNS);
    start_vertices.reserve(number_of_runs);
    if(number_of_runs == n){
        start_vertices.resize(n);
        iota(start_vertices.begin(), start_vertices.end(), 1);
        shuffle(start_vertices.begin(), start_vertices.end(), generator);
    } else{
        unordered_set<int> selected_vertices;
        selected_vertices.reserve(number_of_runs);
        uniform_int_distribution<int> vertex_distribution(1, n);
        while(static_cast<int>(start_vertices.size()) < number_of_runs){
            int start = vertex_distribution(generator);
            if(selected_vertices.insert(start).second){
                start_vertices.push_back(start);
            }
        }
    }

    BenchmarkResults list_results;
    vector<int> parent, distance;
    for(int start : start_vertices){
        auto begin = high_resolution_clock::now();
        graph.bfs(start, parent, distance);
        auto end = high_resolution_clock::now();
        list_results.bfs.push_back(duration<double, milli>(end - begin).count());

        begin = high_resolution_clock::now();
        graph.dfs(start, parent, distance);
        end = high_resolution_clock::now();
        list_results.dfs_iterative.push_back(duration<double, milli>(end - begin).count());

        begin = high_resolution_clock::now();
        vector<int> recursive_distance;
        graph.dfs_recursiva(start, parent, recursive_distance);
        end = high_resolution_clock::now();
        list_results.dfs_recursive.push_back(duration<double, milli>(end - begin).count());
    }

    cout << "{\n";
    cout << "  \"n\": " << graph.get_n() << ",\n";
    cout << "  \"m\": " << graph.get_m() << ",\n";
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
    cout << "  \"benchmark\": {\n";
    cout << "    \"numero_execucoes\": " << start_vertices.size() << ",\n";
    cout << "    \"vertices_iniciais\": "; print_json_array(start_vertices); cout << ",\n";
    cout << "    ";
    print_algorithm_results(list_results);
    cout << ",\n";
    cout << "    \"representacoes\": {\n";
    cout << "      \"lista\": {\n        ";
    cout << "\"disponivel\": true,\n        ";
    print_algorithm_results(list_results);
    cout << "\n      }\n";
    cout << "    }\n";
    cout << "  },\n";
    cout << "  \"tamanhos_componentes\": "; print_json_array(component_sizes); cout << "\n";
    cout << "}\n";

    return 0;
}
