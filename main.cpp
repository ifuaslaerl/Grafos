#include "src/graph.hpp"
#include <iostream>
#include <vector>
using namespace std;

const int START = 1;      // vertice inicial das buscas
const int U = 1, V = 4;    // par para a distancia
const int MATRIX_VIEW = false;

int main() {
    // Entrada
    int n, u, v;
    cin >> n;
    Grafo g(n);
    while(cin >> u >> v) g.add_edge(u, v);
    g.build();

    // Saida: estatisticas
    cout << "n = " << g.get_n() << ", m = " << g.get_m() << "\n"
         << "grau: min = " << g.get_grau_minimo() << ", max = " << g.get_grau_maximo()
         << ", medio = " << (double) g.get_grau_medio() << ", mediana = " << (double) g.get_grau_mediano() << "\n";

    // Componentes conexas (ordem decrescente de tamanho)
    auto comps = g.get_components();
    cout << "componentes: " << comps.size() << "\n";
    for (auto& c : comps) {
        cout << "  tamanho " << c.size() << ":";
        for (int x : c) cout << ' ' << x;
        cout << "\n";
    }

    // BFS e DFS: pai e nivel de cada vertice
    vector<int> pai, nivel;
    g.bfs(START, pai, nivel);
    cout << "BFS a partir de " << START << " (vertice pai nivel):\n";
    for (int x = 1; x <= n; x++) cout << "  " << x << ' ' << pai[x] << ' ' << nivel[x] << "\n";

    g.dfs(START, pai, nivel);
    cout << "DFS a partir de " << START << " (vertice pai nivel):\n";
    for (int x = 1; x <= n; x++) cout << "  " << x << ' ' << pai[x] << ' ' << nivel[x] << "\n";

    // Distancia e diametro
    cout << "distancia(" << U << ", " << V << ") = " << g.get_dist(U, V) << "\n";
    cout << "diametro = " << g.get_diameter() << "\n";

    // Matriz de adjacencia
    if (MATRIX_VIEW) {
        auto m = g.get_matriz_adj();
        cout << "matriz de adjacencia:\n";
        for (int i = 1; i <= n; i++) {
            for (int j = 1; j <= n; j++) cout << m[i][j] << ' ';
            cout << "\n";
        }
    }
}