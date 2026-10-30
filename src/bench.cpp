#include "graph.hpp"
#include <iostream>
#include <fstream>
#include <chrono>
#include <iomanip>
#include <string>
#include <vector>
#include <algorithm>

#ifdef _WIN32
#include <windows.h>
#include <psapi.h>
#endif

using namespace std;
using Clock = chrono::steady_clock;

static double peak_memory_mb() {
#ifdef _WIN32
    PROCESS_MEMORY_COUNTERS_EX pmc;
    pmc.cb = sizeof(pmc);
    GetProcessMemoryInfo(GetCurrentProcess(), reinterpret_cast<PROCESS_MEMORY_COUNTERS*>(&pmc), sizeof(pmc));
    return static_cast<double>(pmc.PeakWorkingSetSize) / (1024.0 * 1024.0);
#else
    return -1.0;
#endif
}

static double mediana(vector<double> v) {
    sort(v.begin(), v.end());
    size_t k = v.size();
    return (k % 2) ? v[k / 2] : (v[k / 2 - 1] + v[k / 2]) / 2.0;
}

int main(int argc, char** argv) {
    if (argc < 3) {
        cerr << "Uso: " << argv[0] << " <arquivo_grafo> <nome_amigavel> [repeticoes=3]\n";
        return 1;
    }

    string path = argv[1];
    string nome = argv[2];
    int reps = (argc >= 4) ? stoi(argv[3]) : 3;

    ifstream fin(path);
    if (!fin) {
        cerr << "Erro ao abrir " << path << "\n";
        return 1;
    }

    // ---- Fase de leitura + construcao a partir do arquivo (NAO cronometrada, feita uma unica vez) ----
    int n;
    fin >> n;
    Grafo g(n);

    int u, v;
    while (fin >> u >> v) g.add_edge(u, v);
    fin.close();

    // ---- Fases cronometradas, repetidas `reps` vezes; reporta a mediana de cada uma ----
    vector<double> t_grau, t_componentes, t_bfs;
    size_t num_componentes = 0;

    for (int r = 0; r < reps; r++) {
        auto t0 = Clock::now();
        g.build();
        auto t1 = Clock::now();

        auto comps = g.get_components();
        auto t2 = Clock::now();

        vector<int> pai, dist;
        g.bfs(1, pai, dist);
        auto t3 = Clock::now();

        t_grau.push_back(chrono::duration<double, milli>(t1 - t0).count());
        t_componentes.push_back(chrono::duration<double, milli>(t2 - t1).count());
        t_bfs.push_back(chrono::duration<double, milli>(t3 - t2).count());
        num_componentes = comps.size();
    }

    double tempo_grau_ms = mediana(t_grau);
    double tempo_componentes_ms = mediana(t_componentes);
    double tempo_bfs_ms = mediana(t_bfs);
    double tempo_total_ms = tempo_grau_ms + tempo_componentes_ms + tempo_bfs_ms;

    // Pico de memoria e o mesmo independente do numero de repeticoes (marca d'agua do processo),
    // entao e medido uma unica vez, apos todas as execucoes.
    double mem_mb = peak_memory_mb();

    cout << fixed
         << nome << ";"
         << g.get_n() << ";"
         << g.get_m() << ";"
         << num_componentes << ";"
         << reps << ";"
         << setprecision(3)
         << tempo_grau_ms << ";"
         << tempo_componentes_ms << ";"
         << tempo_bfs_ms << ";"
         << tempo_total_ms << ";"
         << setprecision(2) << mem_mb
         << "\n";

    return 0;
}
