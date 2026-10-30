#include "graph.hpp"
#include <iostream>
#include <fstream>
#include <vector>
#include <string>
using namespace std;

// Para um grafo, calcula:
//   - pai de 10 (BFS e DFS a partir de 1)
//   - pai de 20 (BFS e DFS a partir de 2)
//   - pai de 30 (BFS e DFS a partir de 3)
//   - distancia(10,20), distancia(10,30), distancia(20,30)

string fmt_pai(int alvo, vector<int>& pai) {
    if (pai[alvo] == 0) return "inalcancavel";
    return to_string(pai[alvo]);
}

string fmt_dist(int d) {
    return d == -1 ? "inalcancavel" : to_string(d);
}

int main(int argc, char** argv) {
    if (argc < 3) {
        cerr << "uso: respostas <arquivo_grafo> <nome_grafo>\n";
        return 1;
    }

    string path = argv[1];
    string nome = argv[2];

    ifstream fin(path);
    int n;
    fin >> n;
    Grafo g(n);
    int u, v;
    while (fin >> u >> v) g.add_edge(u, v);
    fin.close();

    g.build();

    vector<int> pai, dist;

    g.bfs(1, pai, dist);
    string pai10_bfs = fmt_pai(10, pai);
    g.dfs(1, pai, dist);
    string pai10_dfs = fmt_pai(10, pai);

    g.bfs(2, pai, dist);
    string pai20_bfs = fmt_pai(20, pai);
    g.dfs(2, pai, dist);
    string pai20_dfs = fmt_pai(20, pai);

    g.bfs(3, pai, dist);
    string pai30_bfs = fmt_pai(30, pai);
    g.dfs(3, pai, dist);
    string pai30_dfs = fmt_pai(30, pai);

    string d_10_20 = fmt_dist(g.get_dist(10, 20));
    string d_10_30 = fmt_dist(g.get_dist(10, 30));
    string d_20_30 = fmt_dist(g.get_dist(20, 30));

    cout << nome << ";"
         << pai10_bfs << ";" << pai10_dfs << ";"
         << pai20_bfs << ";" << pai20_dfs << ";"
         << pai30_bfs << ";" << pai30_dfs << ";"
         << d_10_20 << ";" << d_10_30 << ";" << d_20_30 << "\n";

    return 0;
}
