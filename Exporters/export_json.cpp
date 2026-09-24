#include <iostream>
#include <vector>
#include <chrono>
#include <algorithm>
#include <numeric>
#include <random>
#include <sys/resource.h>
#include "../src/graph.hpp"
using namespace std;
using namespace std::chrono;

const int NUM_RUNS = 10;

double get_peak_memory_mb() {
    struct rusage r_usage;
    getrusage(RUSAGE_SELF, &r_usage);
    return (double)r_usage.ru_maxrss / 1024.0;
}

struct TimeStats {
    vector<double> raw;
    double mean;
    double median;
};

TimeStats calculate_stats(vector<double>& times) {
    if (times.empty()) return {{}, 0.0, 0.0};
    TimeStats stats;
    stats.raw = times;
    double sum = accumulate(times.begin(), times.end(), 0.0);
    stats.mean = sum / times.size();
    
    vector<double> sorted_times = times;
    sort(sorted_times.begin(), sorted_times.end());
    if (sorted_times.size() % 2 == 0) {
        stats.median = (sorted_times[sorted_times.size() / 2 - 1] + sorted_times[sorted_times.size() / 2]) / 2.0;
    } else {
        stats.median = sorted_times[sorted_times.size() / 2];
    }
    return stats;
}

template<typename T>
void print_json_array(const vector<T>& v) {
    cout << "[";
    for (size_t i = 0; i < v.size(); i++) {
        cout << v[i] << (i + 1 == v.size() ? "" : ", ");
    }
    cout << "]";
}

int main() {
    ios_base::sync_with_stdio(false);
    cin.tie(NULL);

    int n;
    if (!(cin >> n)) return 0;

    auto start_build = high_resolution_clock::now();
    Grafo G(n);
    int u, v;
    while (cin >> u >> v) {
        G.add_edge(u, v);
    }
    auto end_build = high_resolution_clock::now();
    double time_build = duration<double, milli>(end_build - start_build).count();
    
    auto start_grau = high_resolution_clock::now();
    G.build();
    auto end_grau = high_resolution_clock::now();
    double time_grau = duration<double, milli>(end_grau - start_grau).count();

    auto start_comp = high_resolution_clock::now();
    vector<vector<int>> comps = G.get_components();
    auto end_comp = high_resolution_clock::now();
    double time_comp = duration<double, milli>(end_comp - start_comp).count();

    vector<double> bfs_times, dfs_iter_times, dfs_rec_times;
    vector<int> start_vertices, p, d; 
    
    random_device rd;
    mt19937 gen(rd());
    uniform_int_distribution<> dist(1, n > 0 ? n : 1); 

    for (int i = 0; i < NUM_RUNS; i++) {
        int start_v = dist(gen);
        start_vertices.push_back(start_v);

        if(n > 0) {
            auto start_bfs = high_resolution_clock::now();
            G.bfs(start_v, p, d);
            auto end_bfs = high_resolution_clock::now();
            bfs_times.push_back(duration<double, milli>(end_bfs - start_bfs).count());

            auto start_dfs_i = high_resolution_clock::now();
            G.dfs(start_v, p, d);
            auto end_dfs_i = high_resolution_clock::now();
            dfs_iter_times.push_back(duration<double, milli>(end_dfs_i - start_dfs_i).count());

            auto start_dfs_r = high_resolution_clock::now();
            G.dfs_recursiva(start_v, p, d);
            auto end_dfs_r = high_resolution_clock::now();
            dfs_rec_times.push_back(duration<double, milli>(end_dfs_r - start_dfs_r).count());
        }
    }

    TimeStats bfs_stats = calculate_stats(bfs_times);
    TimeStats dfs_iter_stats = calculate_stats(dfs_iter_times);
    TimeStats dfs_rec_stats = calculate_stats(dfs_rec_times);

    vector<int> comp_sizes;
    comp_sizes.reserve(comps.size());
    for (const auto& c : comps) comp_sizes.push_back(c.size());

    cout << "{\n";
    cout << "  \"n\": " << G.get_n() << ",\n";
    cout << "  \"m\": " << G.get_m() << ",\n";
    cout << "  \"graus\": {\n";
    cout << "    \"minimo\": " << G.get_grau_minimo() << ",\n";
    cout << "    \"maximo\": " << G.get_grau_maximo() << ",\n";
    cout << "    \"medio\": " << G.get_grau_medio() << ",\n";
    cout << "    \"mediano\": " << G.get_grau_mediano() << "\n";
    cout << "  },\n";
    cout << "  \"telemetria\": {\n";
    cout << "    \"tempo_build_ms\": " << time_build << ",\n";
    cout << "    \"tempo_grau_ms\": " << time_grau << ",\n";
    cout << "    \"tempo_comp_ms\": " << time_comp << ",\n";
    cout << "    \"peak_memory_mb\": " << get_peak_memory_mb() << "\n";
    cout << "  },\n";
    cout << "  \"benchmark\": {\n";
    cout << "    \"vertices_iniciais\": "; print_json_array(start_vertices); cout << ",\n";
    cout << "    \"BFS\": {\n";
    cout << "      \"raw_ms\": "; print_json_array(bfs_stats.raw); cout << ",\n";
    cout << "      \"mean_ms\": " << bfs_stats.mean << ",\n";
    cout << "      \"median_ms\": " << bfs_stats.median << "\n";
    cout << "    },\n";
    cout << "    \"DFS_Iterativa\": {\n";
    cout << "      \"raw_ms\": "; print_json_array(dfs_iter_stats.raw); cout << ",\n";
    cout << "      \"mean_ms\": " << dfs_iter_stats.mean << ",\n";
    cout << "      \"median_ms\": " << dfs_iter_stats.median << "\n";
    cout << "    },\n";
    cout << "    \"DFS_Recursiva\": {\n";
    cout << "      \"raw_ms\": "; print_json_array(dfs_rec_stats.raw); cout << ",\n";
    cout << "      \"mean_ms\": " << dfs_rec_stats.mean << ",\n";
    cout << "      \"median_ms\": " << dfs_rec_stats.median << "\n";
    cout << "    }\n";
    cout << "  },\n";
    cout << "  \"tamanhos_componentes\": "; print_json_array(comp_sizes); cout << "\n";
    cout << "}\n";

    return 0;
}