#include "graph.hpp"

#include <iomanip>
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
        long long weight;
        if(!(cin >> u >> v >> weight)){
            return 1;
        }
        graph.add_arc(u, v, static_cast<long double>(weight));
    }

    vector<int> parent;
    vector<long double> distance;
    graph.dijkstra_heap(1, parent, distance);

    cout << setprecision(15);
    for(int vertex = 1; vertex <= n; ++vertex){
        cout << static_cast<long long>(distance[vertex])
             << (vertex == n ? '\n' : ' ');
    }
}
