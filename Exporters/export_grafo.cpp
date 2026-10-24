#include <cmath>
#include <charconv>
#include <iomanip>
#include <iostream>
#include <string>
#include <vector>
#include "../src/graph.hpp"

using namespace std;

int main(int argc, char* argv[]){
    ios_base::sync_with_stdio(false);
    cin.tie(nullptr);

    string mode = "bfs";
    bool weighted = false;
    bool start_provided = false;
    int start = 0;

    if(argc > 1){
        mode = argv[1];
    }
    if(mode != "bfs" && mode != "BFS" && mode != "dfs" && mode != "DFS" &&
       mode != "dijkstra" && mode != "DIJKSTRA"){
        cerr << "Uso: export_grafo [bfs|dfs|dijkstra] [--weighted] [vertice_inicial]\n";
        return 1;
    }
    if(mode == "BFS") mode = "bfs";
    if(mode == "DFS") mode = "dfs";
    if(mode == "DIJKSTRA") mode = "dijkstra";

    for(int index = 2; index < argc; ++index){
        const string argument = argv[index];
        if(argument == "--weighted"){
            if(weighted){
                cerr << "Opcao --weighted informada mais de uma vez.\n";
                return 1;
            }
            weighted = true;
            continue;
        }
        if(start_provided){
            cerr << "Uso: export_grafo [bfs|dfs|dijkstra] [--weighted] [vertice_inicial]\n";
            return 1;
        }
        const auto result = from_chars(
            argument.data(), argument.data() + argument.size(), start
        );
        if(result.ec != errc{} || result.ptr != argument.data() + argument.size()){
            cerr << "Vertice inicial invalido: " << argument << "\n";
            return 1;
        }
        start_provided = true;
    }

    int n;
    if(!(cin >> n) || n < 0){
        cerr << "A primeira entrada deve conter um numero de vertices valido.\n";
        return 1;
    }
    if(start_provided && (start < 1 || start > n)){
        cerr << "Vertice inicial deve estar entre 1 e " << n << ".\n";
        return 1;
    }

    Grafo<double> graph(n);
    int u, v;
    while(cin >> u){
        double weight = 1;
        if(!(cin >> v) || (weighted && !(cin >> weight))){
            cerr << "Formato invalido para aresta.\n";
            return 1;
        }
        if(u < 1 || u > n || v < 1 || v > n){
            cerr << "Aresta com vertice fora do intervalo [1, " << n << "].\n";
            return 1;
        }
        if(!isfinite(weight) || weight < 0){
            cerr << "Os pesos devem ser finitos e nao negativos para Dijkstra.\n";
            return 1;
        }
        graph.add_edge(u, v, weight);
    }
    if(cin.bad() || !cin.eof()){
        cerr << "Formato invalido nas arestas de entrada.\n";
        return 1;
    }

    vector<int> final_parent(n + 1, 0);
    vector<double> final_distance(n + 1, -1);
    vector<int> final_order(n + 1, -1);
    vector<int> final_component(n + 1, 0);
    int global_order = 1;

    if(n > 0){
        vector<vector<int>> components;
        vector<int> component_edge_counts;
        graph_lib::BfsFrontier<double> frontier;
        graph.get_components(components, component_edge_counts, frontier);
        for(size_t index = 0; index < components.size(); ++index){
            for(int vertex : components[index]){
                final_component[vertex] = static_cast<int>(index) + 1;
            }
        }

        auto register_search = [&](int source){
            vector<int> parent;
            vector<int> order;
            vector<int> integer_distance;
            vector<double> weighted_distance;

            if(mode == "bfs"){
                order = graph.bfs(source, parent, integer_distance);
            } else if(mode == "dfs"){
                order = graph.dfs(source, parent, integer_distance);
            } else{
                order = graph.dijkstra_heap(source, parent, weighted_distance);
            }

            for(int vertex : order){
                final_parent[vertex] = parent[vertex];
                final_distance[vertex] =
                    mode == "dijkstra"
                        ? weighted_distance[vertex]
                        : static_cast<double>(integer_distance[vertex]);
                final_order[vertex] = global_order++;
            }
        };

        if(start_provided){
            register_search(start);
        } else{
            for(const auto& component : components){
                if(!component.empty()){
                    register_search(component.front());
                }
            }
        }

        cout << setprecision(12);
        cout << "vertice,pai,distancia,ordem_visitacao,componente_conexa\n";
        for(int vertex = 1; vertex <= n; ++vertex){
            cout << vertex << ',' << final_parent[vertex] << ','
                 << final_distance[vertex] << ',' << final_order[vertex] << ','
                 << final_component[vertex] << '\n';
        }
    }

    return 0;
}
