#include "graph.hpp"
#include <iostream>
#include <fstream>
#include <vector>
using namespace std;

int main(int argc, char** argv) {
    string path = argv[1];
    int alvo = argc > 2 ? stoi(argv[2]) : 4;

    ifstream fin(path);
    int n;
    fin >> n;
    Grafo g(n);
    int u, v;
    while (fin >> u >> v) g.add_edge(u, v);
    fin.close();

    vector<int> pai_bfs, dist_bfs, pai_dfs, dist_dfs;
    g.bfs(1, pai_bfs, dist_bfs);
    g.dfs(1, pai_dfs, dist_dfs);

    cout << "BFS a partir de 1 -> vertice " << alvo
         << ": pai = " << pai_bfs[alvo] << ", nivel = " << dist_bfs[alvo] << "\n";
    cout << "DFS a partir de 1 -> vertice " << alvo
         << ": pai = " << pai_dfs[alvo] << ", nivel = " << dist_dfs[alvo] << "\n";
    cout << "distancia(1, " << alvo << ") = " << g.get_dist(1, alvo) << "\n";

    return 0;
}
