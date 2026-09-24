#include <iostream>
#include <vector>
#include <map>
#include "../src/graph.hpp"
using namespace std;

int main() {
    ios_base::sync_with_stdio(false);
    cin.tie(NULL);

    int n;
    if (!(cin >> n)) return 0;

    Grafo G(n);
    int u, v;
    while (cin >> u >> v) {
        G.add_edge(u, v);
    }
    
    map<int, int> freq;
    const auto& adj = G.get_lista_adj();
    
    for (int i = 1; i <= n; i++) {
        freq[adj[i].size()]++;
    }
    
    cout << "Grau,Frequencia\n";
    for (auto const& [grau, count] : freq) {
        cout << grau << "," << count << "\n";
    }

    return 0;
}