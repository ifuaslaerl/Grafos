#include <iostream>
#include <vector>
#include <string>
#include "../src/graph.hpp"
using namespace std;

int main(int argc, char* argv[]) {
    ios_base::sync_with_stdio(false);
    cin.tie(NULL);

    // Define o comportamento através do argumento da linha de comando (padrão: bfs)
    string modo = "bfs";
    if (argc > 1) {
        string arg = argv[1];
        if (arg == "dfs" || arg == "DFS") modo = "dfs";
        else if (arg == "bfs" || arg == "BFS") modo = "bfs";
    }

    int n;
    if (!(cin >> n)) return 0;

    Grafo G(n);
    int u, v;
    while (cin >> u >> v) {
        G.add_edge(u, v);
    }
    
    // Vetores globais para fundir os dados de múltiplos componentes
    vector<int> final_p(n + 1, 0);
    vector<int> final_d(n + 1, -1);
    vector<int> final_ordem(n + 1, -1);
    int ordem_global = 1;

    if (n > 0) {
        // Captura todos os componentes do grafo
        vector<vector<int>> comps = G.get_components();
        
        // Itera sobre cada componente desconexo
        for (const auto& comp : comps) {
            if (comp.empty()) continue;
            
            int start_node = comp[0]; // Pega um vértice arbitrário do componente
            vector<int> p, d;
            vector<int> ordem_visita_list;
            
            // Executa o algoritmo escolhido pelo terminal para o componente atual
            if (modo == "bfs") {
                ordem_visita_list = G.bfs(start_node, p, d);
            } else {
                ordem_visita_list = G.dfs(start_node, p, d);
            }
            
            // Salva os dados do componente nos vetores globais
            for (int vertice : ordem_visita_list) {
                final_p[vertice] = p[vertice];
                final_d[vertice] = d[vertice];
                final_ordem[vertice] = ordem_global++;
            }
        }

        // Exportação do CSV
        cout << "vertice,pai,distancia,ordem_visitacao\n";
        
        for (int i = 1; i <= n; i++) {
            // Se o vértice isolado for a própria raiz, garantimos que o pai seja 0 (ou ele mesmo)
            cout << i << "," 
                 << final_p[i] << "," 
                 << final_d[i] << "," 
                 << final_ordem[i] << "\n";
        }
    }

    return 0;
}