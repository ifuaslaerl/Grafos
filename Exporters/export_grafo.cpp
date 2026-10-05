#include <iostream>
#include <vector>
#include <string>
#include <charconv>
#include "../src/graph.hpp"
using namespace std;

int main(int argc, char* argv[]){
    ios_base::sync_with_stdio(false);
    cin.tie(NULL);

    // Define o comportamento através do argumento da linha de comando (padrão: bfs)
    string modo = "bfs";
    if(argc > 1){
        string arg = argv[1];
        if(arg == "dfs" || arg == "DFS") modo = "dfs";
        else if(arg == "bfs" || arg == "BFS") modo = "bfs";
    }

    int origem = 0;
    const bool origem_informada = argc > 2;
    if(argc > 3){
        cerr << "Uso: export_grafo [bfs|dfs] [vertice_inicial]\n";
        return 1;
    }
    if(origem_informada){
        const string arg = argv[2];
        const auto resultado = from_chars(arg.data(), arg.data() + arg.size(), origem);
        if(resultado.ec != errc{} || resultado.ptr != arg.data() + arg.size()){
            cerr << "Vertice inicial invalido: " << arg << "\n";
            return 1;
        }
    }

    int n;
    if(!(cin >> n)) return 0;
    if(origem_informada && (origem < 1 || origem > n)){
        cerr << "Vertice inicial deve estar entre 1 e " << n << ".\n";
        return 1;
    }

    Grafo<long double> G(n);
    int u, v;
    while(cin >> u >> v){
        G.add_edge(u, v);
    }
    
    // Vetores globais para exportar os dados da busca.
    vector<int> final_p(n + 1, 0);
    vector<int> final_d(n + 1, -1);
    vector<int> final_ordem(n + 1, -1);
    vector<int> final_componente(n + 1, 0);
    int ordem_global = 1;

    if(n > 0){
        vector<vector<int>> comps = G.get_components();
        for(size_t i = 0; i < comps.size(); ++i){
            const auto& comp = comps[i];
            for(int vertice : comp){
                final_componente[vertice] = static_cast<int>(i) + 1;
            }
        }

        auto registrar_busca = [&](int start_node){
            vector<int> p, d;
            vector<int> ordem_visita_list;
            if(modo == "bfs"){
                ordem_visita_list = G.bfs(start_node, p, d);
            } else{
                ordem_visita_list = G.dfs(start_node, p, d);
            }

            for(int vertice : ordem_visita_list){
                final_p[vertice] = p[vertice];
                final_d[vertice] = d[vertice];
                final_ordem[vertice] = ordem_global++;
            }
        };

        if(origem_informada){
            registrar_busca(origem);
        } else{
            for(const auto& comp : comps){
                if(!comp.empty()){
                    registrar_busca(comp[0]);
                }
            }
        }

        // Exportação do CSV
        cout << "vertice,pai,distancia,ordem_visitacao,componente_conexa\n";
        
        for(int i = 1; i <= n; i++){
            // Se o vértice isolado for a própria raiz, garantimos que o pai seja 0 (ou ele mesmo)
            cout << i << "," 
                 << final_p[i] << "," 
                 << final_d[i] << "," 
                 << final_ordem[i] << ","
                 << final_componente[i] << "\n";
        }
    }

    return 0;
}