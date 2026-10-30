#include "graph.hpp"
#include <iostream>
#include <fstream>
#include <chrono>
#include <iomanip>
#include <vector>
#include <queue>

#ifdef _WIN32
#include <windows.h>
#include <psapi.h>
#endif

using namespace std;
using Clock = chrono::steady_clock;

// Benchmark da representacao por MATRIZ de adjacencia: tempo de construcao
// e tempo de BFS sobre a matriz (varredura de linha O(n) por vertice),
// mais pico de memoria. So deve ser usado em grafos pequenos (memoria O(n^2)).

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

int main(int argc, char** argv) {
    string path = argv[1];
    string nome = argv[2];

    ifstream fin(path);
    int n;
    fin >> n;
    Grafo g(n);
    int u, v;
    while (fin >> u >> v) g.add_edge(u, v);
    fin.close();

    auto t0 = Clock::now();
    auto matriz = g.get_matriz_adj();
    auto t1 = Clock::now();

    vector<char> vis(n + 1, 0);
    queue<int> fila;
    vis[1] = 1;
    fila.push(1);
    while (!fila.empty()) {
        int vv = fila.front();
        fila.pop();
        for (int w = 1; w <= n; w++) {
            if (matriz[vv][w] && !vis[w]) {
                vis[w] = 1;
                fila.push(w);
            }
        }
    }
    auto t2 = Clock::now();

    double t_constr = chrono::duration<double, milli>(t1 - t0).count();
    double t_bfs = chrono::duration<double, milli>(t2 - t1).count();
    double mem = peak_memory_mb();

    cout << fixed
         << nome << ";" << n << ";"
         << setprecision(3) << t_constr << ";" << t_bfs << ";"
         << setprecision(2) << mem << "\n";

    return 0;
}
