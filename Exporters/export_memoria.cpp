#include <fstream>
#include <cmath>
#include <iostream>
#include <string>
#include <vector>
#include "../src/graph.hpp"

using namespace std;

bool get_resident_memory_mb(double& memory_mb){
    ifstream status("/proc/self/status");
    if(!status){
        cerr << "Nao foi possivel ler /proc/self/status.\n";
        return false;
    }

    string key;
    while(status >> key){
        if(key == "VmRSS:"){
            double memory_kb;
            string unit;
            if(!(status >> memory_kb >> unit) || unit != "kB"){
                cerr << "Formato inesperado para VmRSS em /proc/self/status.\n";
                return false;
            }
            memory_mb = memory_kb / 1024.0;
            return true;
        }
        string rest_of_line;
        getline(status, rest_of_line);
    }

    cerr << "VmRSS nao encontrado em /proc/self/status.\n";
    return false;
}

int main(int argc, char* argv[]){
    ios_base::sync_with_stdio(false);
    cin.tie(nullptr);

    if(argc < 2 || argc > 3 ||
       (string(argv[1]) != "lista" && string(argv[1]) != "matriz") ||
       (argc == 3 && string(argv[2]) != "--weighted")){
        cerr << "Uso: export_memoria <lista|matriz> [--weighted] < entrada.txt\n";
        return 1;
    }
    const string representacao = argv[1];
    const bool weighted = argc == 3;
    if(weighted && representacao == "matriz"){
        cerr << "A representacao matricial ponderada nao esta disponivel; use lista.\n";
        return 1;
    }

    int n;
    if(!(cin >> n) || n < 0){
        cerr << "A primeira entrada deve ser um numero de vertices nao negativo.\n";
        return 1;
    }

    vector<int> grau(n + 1, 0);
    int m = 0;
    auto ler_arestas = [&](auto&& adicionar_aresta){
        int u, v;
        while(cin >> u){
            double weight = 1;
            if(!(cin >> v) || (weighted && !(cin >> weight))){
                cerr << (weighted
                    ? "Cada aresta ponderada deve conter u, v e peso.\n"
                    : "Cada aresta deve conter dois vertices.\n");
                return false;
            }
            if(u < 1 || u > n || v < 1 || v > n){
                cerr << "Aresta com vertice fora do intervalo [1, " << n << "].\n";
                return false;
            }
            if(!isfinite(weight) || weight < 0){
                cerr << "Os pesos devem ser finitos e nao negativos.\n";
                return false;
            }
            adicionar_aresta(u, v, weight);
            ++grau[u];
            ++grau[v];
            ++m;
        }
        if(cin.bad() || !cin.eof()){
            cerr << "Formato invalido nas arestas de entrada.\n";
            return false;
        }
        return true;
    };

    if(representacao == "lista"){
        double memory_mb;
        if(weighted){
            vector<vector<Edge<double>>> adj(n + 1);
            if(!ler_arestas([&](int u, int v, double weight){
                adj[u].push_back({u, v, weight});
                adj[v].push_back({v, u, weight});
            })){
                return 1;
            }
            if(!get_resident_memory_mb(memory_mb)) return 1;
        } else{
            vector<vector<Edge<int>>> adj(n + 1);
            if(!ler_arestas([&](int u, int v, double){
                adj[u].push_back({u, v, 1});
                adj[v].push_back({v, u, 1});
            })){
                return 1;
            }
            if(!get_resident_memory_mb(memory_mb)) return 1;
        }
        cout << "representacao,vertices,arestas,memoria_rss_mb\n";
        cout << (weighted ? "lista_ponderada," : "lista,")
             << n << "," << m << "," << memory_mb << "\n";
    } else{
        vector<vector<int>> adj(n + 1, vector<int>(n + 1, 0));
        if(!ler_arestas([&](int u, int v, double){
            adj[u][v] = 1;
            adj[v][u] = 1;
        })){
            return 1;
        }

        double memory_mb;
        if(!get_resident_memory_mb(memory_mb)) return 1;
        cout << "representacao,vertices,arestas,memoria_rss_mb\n";
        cout << "matriz," << n << "," << m << "," << memory_mb << "\n";
    }

    return 0;
}
