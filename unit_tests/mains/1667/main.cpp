#include "graph.hpp"

#include <algorithm>
#include <iostream>
#include <vector>

using namespace std;

int main(){
    int n, m;
    if(!(cin >> n >> m)){
        return 1;
    }

    Grafo<long double> graph(n);
    for(int i = 0; i < m; ++i){
        int u, v;
        if(!(cin >> u >> v)){
            return 1;
        }
        graph.add_edge(u, v);
    }

    vector<int> parent, distance;
    graph.bfs(1, parent, distance);
    if(distance[n] == -1){
        cout << "IMPOSSIBLE\n";
        return 0;
    }

    vector<int> path;
    for(int vertex = n; vertex != 1; vertex = parent[vertex]){
        if(vertex < 1 || vertex > n || parent[vertex] == 0){
            cerr << "Falha ao reconstruir caminho BFS.\n";
            return 1;
        }
        path.push_back(vertex);
    }
    path.push_back(1);
    reverse(path.begin(), path.end());

    cout << path.size() << '\n';
    for(size_t i = 0; i < path.size(); ++i){
        cout << path[i] << (i + 1 == path.size() ? '\n' : ' ');
    }
}
