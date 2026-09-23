#include <iostream>
#include <vector>
#include <chrono>
#include <algorithm>
#include <numeric>
#include <queue>
#include <sys/resource.h>
#include "src/graph.hpp"

using namespace std;
using namespace std::chrono;

const int MAX_DRAW_VERTICES = 500;
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

void print_edges(const vector<pair<int,int>>& edges) {
    cout << "[";
    for (size_t i = 0; i < edges.size(); i++) {
        cout << "[" << edges[i].first << ", " << edges[i].second << "]";
        if (i + 1 < edges.size()) cout << ", ";
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

    vector<double> bfs_times, dfs_times;
    vector<int> p, d;
    
    for (int i = 0; i < NUM_RUNS; i++) {
        auto start_bfs = high_resolution_clock::now();
        if(n > 0) G.bfs(1, p, d);
        auto end_bfs = high_resolution_clock::now();
        bfs_times.push_back(duration<double, milli>(end_bfs - start_bfs).count());

        auto start_dfs = high_resolution_clock::now();
        if(n > 0) G.dfs(1, p, d);
        auto end_dfs = high_resolution_clock::now();
        dfs_times.push_back(duration<double, milli>(end_dfs - start_dfs).count());
    }

    TimeStats bfs_stats = calculate_stats(bfs_times);
    TimeStats dfs_stats = calculate_stats(dfs_times);

    vector<int> graus_dist;
    graus_dist.reserve(n + 1);
    for (int i = 0; i <= n; i++) {
        graus_dist.push_back(G.get_lista_adj()[i].size());
    }

    vector<int> comp_sizes;
    comp_sizes.reserve(comps.size());
    for (const auto& c : comps) {
        comp_sizes.push_back(c.size());
    }

    vector<int> nodes_to_draw;
    int current_total_vertices = 0;
    
    for (const auto& c : comps) {
        if (current_total_vertices + c.size() <= MAX_DRAW_VERTICES) {
            current_total_vertices += c.size();
            nodes_to_draw.insert(nodes_to_draw.end(), c.begin(), c.end());
        }
    }

    if (nodes_to_draw.empty() && !comps.empty()) {
        queue<int> q;
        vector<int> vis(n + 1, 0);
        int start_node = comps[0][0];
        q.push(start_node);
        vis[start_node] = 1;
        while (!q.empty() && nodes_to_draw.size() < MAX_DRAW_VERTICES) {
            int current = q.front();
            q.pop();
            nodes_to_draw.push_back(current);
            for (int vizinho : G.get_lista_adj()[current]) {
                if (!vis[vizinho]) {
                    vis[vizinho] = 1;
                    q.push(vizinho);
                }
            }
        }
    }

    vector<bool> in_subgraph(n + 1, false);
    for (int v_node : nodes_to_draw) in_subgraph[v_node] = true;

    // --- CLASSIFICAÇÃO DE ARESTAS: BFS ---
    vector<pair<int,int>> bfs_tree, bfs_cross;
    vector<int> bfs_dist(n + 1, -1);
    vector<int> bfs_parent(n + 1, 0);
    vector<pair<int, int>> bfs_profundidades;
    
    for (int v_start : nodes_to_draw) {
        if (bfs_dist[v_start] == -1) {
            queue<int> q;
            q.push(v_start);
            bfs_dist[v_start] = 0;
            while (!q.empty()) {
                int curr = q.front(); q.pop();
                bfs_profundidades.push_back({curr, bfs_dist[curr]});
                for (int viz : G.get_lista_adj()[curr]) {
                    if (!in_subgraph[viz]) continue;
                    
                    if (bfs_dist[viz] == -1) {
                        bfs_dist[viz] = bfs_dist[curr] + 1;
                        bfs_parent[viz] = curr;
                        bfs_tree.push_back({curr, viz});
                        q.push(viz);
                    } else if (viz != bfs_parent[curr] && curr < viz) {
                        bfs_cross.push_back({curr, viz});
                    }
                }
            }
        }
    }

    // --- CLASSIFICAÇÃO E PONTES (ALGORITMO DE TARJAN ACOPLADO): DFS ---
    vector<pair<int,int>> dfs_tree, dfs_back, bridges;
    vector<int> articulations;
    vector<pair<int, int>> dfs_profundidades;
    
    vector<int> tin(n + 1, -1);
    vector<int> low(n + 1, -1);
    vector<bool> is_art(n + 1, false);
    int timer = 0;
    
    auto dfs_func = [&](auto& self, int curr, int parent, int profundidade) -> void {
        tin[curr] = low[curr] = ++timer;
        dfs_profundidades.push_back({curr, profundidade});
        int children = 0;
        
        for (int viz : G.get_lista_adj()[curr]) {
            if (!in_subgraph[viz]) continue;
            
            if (tin[viz] == -1) { // Nao visitado: Tree Edge
                children++;
                dfs_tree.push_back({curr, viz});
                self(self, viz, curr, profundidade + 1);
                low[curr] = min(low[curr], low[viz]);
                
                // Deteta Ponte
                if (low[viz] > tin[curr]) {
                    bridges.push_back({curr, viz});
                }
                // Deteta Articulacao
                if (parent != 0 && low[viz] >= tin[curr]) {
                    is_art[curr] = true;
                }
            } else if (viz != parent && tin[viz] < tin[curr]) { // Back Edge
                dfs_back.push_back({curr, viz});
                low[curr] = min(low[curr], tin[viz]);
            }
        }
        if (parent == 0 && children > 1) {
            is_art[curr] = true;
        }
    };
    
    for (int v_start : nodes_to_draw) {
        if (tin[v_start] == -1) {
            dfs_func(dfs_func, v_start, 0, 0);
        }
    }
    
    for(int i = 1; i <= n; i++) {
        if(is_art[i]) articulations.push_back(i);
    }

    cout << "{\n";
    cout << "  \"n\": " << n << ",\n";
    cout << "  \"m\": " << G.get_m() << ",\n";
    cout << "  \"v_plus_e\": " << n + G.get_m() << ",\n";
    
    cout << "  \"telemetria\": {\n";
    cout << "    \"tempo_build_ms\": " << time_build << ",\n";
    cout << "    \"tempo_grau_ms\": " << time_grau << ",\n";
    cout << "    \"tempo_comp_ms\": " << time_comp << ",\n";
    cout << "    \"peak_memory_mb\": " << get_peak_memory_mb() << "\n";
    cout << "  },\n";
    
    cout << "  \"benchmark\": {\n";
    cout << "    \"BFS\": {\n";
    cout << "      \"raw\": "; print_json_array(bfs_stats.raw); cout << ",\n";
    cout << "      \"mean\": " << bfs_stats.mean << ",\n";
    cout << "      \"median\": " << bfs_stats.median << "\n";
    cout << "    },\n";
    cout << "    \"DFS\": {\n";
    cout << "      \"raw\": "; print_json_array(dfs_stats.raw); cout << ",\n";
    cout << "      \"mean\": " << dfs_stats.mean << ",\n";
    cout << "      \"median\": " << dfs_stats.median << "\n";
    cout << "    }\n";
    cout << "  },\n";

    cout << "  \"distribuicao_graus\": "; print_json_array(graus_dist); cout << ",\n";
    cout << "  \"tamanhos_componentes\": "; print_json_array(comp_sizes); cout << ",\n";

    cout << "  \"desenho_grafo\": {\n";
    cout << "    \"nos\": "; print_json_array(nodes_to_draw); cout << ",\n";
    cout << "    \"pontes\": "; print_edges(bridges); cout << ",\n";
    cout << "    \"articulacoes\": "; print_json_array(articulations); cout << ",\n";
    cout << "    \"bfs\": {\n";
    cout << "      \"tree_edges\": "; print_edges(bfs_tree); cout << ",\n";
    cout << "      \"cross_edges\": "; print_edges(bfs_cross); cout << ",\n";
    cout << "      \"profundidades\": "; print_edges(bfs_profundidades); cout << "\n";
    cout << "    },\n";
    cout << "    \"dfs\": {\n";
    cout << "      \"tree_edges\": "; print_edges(dfs_tree); cout << ",\n";
    cout << "      \"back_edges\": "; print_edges(dfs_back); cout << ",\n";
    cout << "      \"profundidades\": "; print_edges(dfs_profundidades); cout << "\n";
    cout << "    }\n";
    cout << "  }\n";
    cout << "}\n";

    return 0;
}