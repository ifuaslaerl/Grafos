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

    vector<int> parent, distance, visited(n + 1, 0);
    vector<int> cycle;
    const vector<vector<Edge<long double>>>& adjacency = graph.get_lista_adj();

    for(int start = 1; start <= n && cycle.empty(); ++start){
        if(visited[start]){
            continue;
        }

        const vector<int> component = graph.dfs(start, parent, distance);
        for(int vertex : component){
            visited[vertex] = 1;
        }

        for(int u : component){
            for(const Edge<long double>& edge : adjacency[u]){
                const int v = edge.v;
                if(u >= v || parent[u] == v || parent[v] == u){
                    continue;
                }

                vector<int> ancestors(n + 1, -1);
                vector<int> path_from_u;
                for(int vertex = u; ; vertex = parent[vertex]){
                    ancestors[vertex] = static_cast<int>(path_from_u.size());
                    path_from_u.push_back(vertex);
                    if(parent[vertex] == vertex){
                        break;
                    }
                }

                vector<int> path_from_v;
                int lca = v;
                while(ancestors[lca] == -1){
                    path_from_v.push_back(lca);
                    lca = parent[lca];
                }

                cycle.insert(
                    cycle.end(),
                    path_from_u.begin(),
                    path_from_u.begin() + ancestors[lca] + 1
                );
                reverse(path_from_v.begin(), path_from_v.end());
                cycle.insert(cycle.end(), path_from_v.begin(), path_from_v.end());
                cycle.push_back(u);
                break;
            }
            if(!cycle.empty()){
                break;
            }
        }
    }

    if(cycle.empty()){
        cout << "IMPOSSIBLE\n";
        return 0;
    }

    cout << cycle.size() << '\n';
    for(size_t i = 0; i < cycle.size(); ++i){
        cout << cycle[i] << (i + 1 == cycle.size() ? '\n' : ' ');
    }
}
