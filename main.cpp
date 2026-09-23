#include <iostream>
#include <vector>
#include <chrono>
#include <iomanip>
#include <sys/resource.h>
#include "src/graph.hpp"

using namespace std;
using namespace std::chrono;

double get_peak_memory_mb() {
    struct rusage r_usage;
    getrusage(RUSAGE_SELF, &r_usage);
    return (double)r_usage.ru_maxrss / 1024.0;
}

int main() {
    ios_base::sync_with_stdio(false);
    cin.tie(NULL);

    int n;
    if (!(cin >> n)) return 0;

    cout << "==================================================\n";
    cout << "       RELATÓRIO DE ANÁLISE DO GRAFO\n";
    cout << "==================================================\n\n";

    // 1. CONSTRUÇÃO DO GRAFO
    auto start_total = high_resolution_clock::now();
    auto start_build = high_resolution_clock::now();
    
    Grafo G(n);
    int u, v;
    while (cin >> u >> v) {
        G.add_edge(u, v);
    }
    
    auto end_build = high_resolution_clock::now();
    double time_build = duration<double, milli>(end_build - start_build).count();

    // 2. ESTATÍSTICAS DE GRAU
    auto start_grau = high_resolution_clock::now();
    G.build(); 
    auto end_grau = high_resolution_clock::now();
    double time_grau = duration<double, milli>(end_grau - start_grau).count();

    long long max_edges = (long long)n * (n - 1) / 2;
    double density = max_edges > 0 ? (double)G.get_m() / max_edges : 0;
    
    int folhas = 0, isolados = 0;
    for (int i = 1; i <= n; i++) {
        int g = G.get_lista_adj()[i].size();
        if (g == 1) folhas++;
        if (g == 0) isolados++;
    }

    cout << "[ ESTATÍSTICAS GLOBAIS E DE GRAU ]\n";
    cout << "Vertices (n): " << G.get_n() << "\n";
    cout << "Arestas  (m): " << G.get_m() << "\n";
    cout << "Densidade   : " << fixed << setprecision(6) << density << "\n";
    cout << "Grau Mínimo : " << G.get_grau_minimo() << " (Isolados: " << isolados << ")\n";
    cout << "Grau Máximo : " << G.get_grau_maximo() << "\n";
    cout << "Grau Médio  : " << fixed << setprecision(2) << G.get_grau_medio() << "\n";
    cout << "Grau Mediano: " << fixed << setprecision(2) << G.get_grau_mediano() << "\n";
    cout << "Folhas (d=1): " << folhas << "\n\n";

    // 3. COMPONENTES E DIÂMETRO
    auto start_comp = high_resolution_clock::now();
    vector<vector<int>> comps = G.get_components();
    auto end_comp = high_resolution_clock::now();
    double time_comp = duration<double, milli>(end_comp - start_comp).count();

    int max_comp_size = comps.empty() ? 0 : comps[0].size();
    double percent_max_comp = n > 0 ? ((double)max_comp_size / n) * 100.0 : 0;

    auto start_diam = high_resolution_clock::now();
    int diametro = G.get_diameter();
    auto end_diam = high_resolution_clock::now();
    double time_diam = duration<double, milli>(end_diam - start_diam).count();

    cout << "[ TOPOLOGIA ]\n";
    cout << "Componentes Conexas: " << comps.size() << "\n";
    cout << "Tamanho Maior Comp.: " << max_comp_size << " vértices (" << fixed << setprecision(2) << percent_max_comp << "% do grafo)\n";
    cout << "Diâmetro Aproximado: " << diametro << "\n\n";

    // 4. ESTUDO DE CASO: BFS E DFS (Origens 1, 2 e 3 -> Destinos 10, 20 e 30)
    cout << "[ PAIS E DISTÂNCIAS DOS VÉRTICES (BFS / DFS) ]\n";
    vector<int> origens = {1, 2, 3};
    vector<int> destinos = {10, 20, 30};
    
    vector<int> pai_bfs, dist_bfs;
    vector<int> pai_dfs, dist_dfs;
    
    double time_bfs_total = 0;
    double time_dfs_total = 0;
    int runs = 0;

    for (int orig : origens) {
        if (orig > n) continue;

        auto start_bfs = high_resolution_clock::now();
        G.bfs(orig, pai_bfs, dist_bfs);
        auto end_bfs = high_resolution_clock::now();
        time_bfs_total += duration<double, milli>(end_bfs - start_bfs).count();

        auto start_dfs = high_resolution_clock::now();
        G.dfs(orig, pai_dfs, dist_dfs);
        auto end_dfs = high_resolution_clock::now();
        time_dfs_total += duration<double, milli>(end_dfs - start_dfs).count();

        runs++;

        cout << "Origem " << orig << ":\n";
        for (int dest : destinos) {
            if (dest > n) continue;
            
            cout << "  -> Destino " << dest << ":\n";
            if (dist_bfs[dest] > n) { 
                cout << "     Status: Inalcançável\n";
            } else {
                cout << "     Pai       : " << pai_bfs[dest] << " (BFS) / " << pai_dfs[dest] << " (DFS)\n";
                cout << "     Distância : " << dist_bfs[dest] << " (BFS) / " << dist_dfs[dest] << " (DFS)\n";
            }
        }
    }
    cout << "\n";

    // 5. DISTÂNCIAS DIRETAS (10, 20 e 30)
    cout << "[ DISTÂNCIAS ENTRE PARES ]\n";
    if (n >= 30) {
        int d10_20 = G.get_dist(10, 20);
        int d10_30 = G.get_dist(10, 30);
        int d20_30 = G.get_dist(20, 30);
        
        cout << "dist(10, 20): " << (d10_20 == -1 ? "Inalcançável" : to_string(d10_20)) << "\n";
        cout << "dist(10, 30): " << (d10_30 == -1 ? "Inalcançável" : to_string(d10_30)) << "\n";
        cout << "dist(20, 30): " << (d20_30 == -1 ? "Inalcançável" : to_string(d20_30)) << "\n";
    } else {
         cout << "Grafo pequeno demais para testar vértices 10, 20 e 30.\n";
    }
    cout << "\n";

    // 6. MATRIZ VS LISTA (Corte de Segurança N <= 10000)
    cout << "[ LISTA VS MATRIZ DE ADJACÊNCIA ]\n";
    if (n <= 10000) {
        auto start_matriz = high_resolution_clock::now();
        vector<vector<int>> matriz = G.get_matriz_adj();
        auto end_matriz = high_resolution_clock::now();
        
        cout << "Construção Matriz : " << duration<double, milli>(end_matriz - start_matriz).count() << " ms\n";
        cout << "Status: Matriz gerada com sucesso (Grafo pequeno).\n";
    } else {
        cout << "Status: Corte de segurança ativado! Matriz ignorada para n = " << n << ".\n";
        cout << "Motivo: O(n^2) consumiria memória excessiva e causaria crash.\n";
    }
    cout << "\n";

    auto end_total = high_resolution_clock::now();
    double time_total = duration<double, milli>(end_total - start_total).count();

    // 7. TELEMETRIA FINAL
    cout << "[ TELEMETRIA DE PERFORMANCE ]\n";
    cout << "Tempo Construção  : " << fixed << setprecision(3) << time_build << " ms\n";
    cout << "Tempo Graus       : " << time_grau << " ms\n";
    cout << "Tempo Componentes : " << time_comp << " ms\n";
    
    double bfs_media = runs > 0 ? (time_bfs_total / runs) : 0;
    double dfs_media = runs > 0 ? (time_dfs_total / runs) : 0;
    
    cout << "Tempo BFS (média) : " << bfs_media << " ms\n";
    cout << "Tempo DFS (média) : " << dfs_media << " ms\n";
    cout << "Tempo Diâmetro    : " << time_diam << " ms\n";
    cout << "--------------------------------------------------\n";
    cout << "TEMPO TOTAL GASTO : " << time_total << " ms\n";
    cout << "PICO DE MEMÓRIA   : " << get_peak_memory_mb() << " MB\n";
    cout << "==================================================\n";

    return 0;
}