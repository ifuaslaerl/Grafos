#include <fstream>
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

    if(argc != 2 || (string(argv[1]) != "lista" && string(argv[1]) != "matriz")){
        cerr << "Uso: export_memoria <lista|matriz> < entrada.txt\n";
        return 1;
    }
    const string representacao = argv[1];

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
            if(!(cin >> v)){
                cerr << "Cada aresta deve conter dois vertices.\n";
                return false;
            }
            if(u < 1 || u > n || v < 1 || v > n){
                cerr << "Aresta com vertice fora do intervalo [1, " << n << "].\n";
                return false;
            }
            adicionar_aresta(u, v);
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
        vector<vector<Edge<long double>>> adj(n + 1);
        if(!ler_arestas([&](int u, int v){
            adj[u].push_back({u, v, 1});
            adj[v].push_back({v, u, 1});
        })){
            return 1;
        }

        double memory_mb;
        if(!get_resident_memory_mb(memory_mb)) return 1;
        cout << "representacao,vertices,arestas,memoria_rss_mb\n";
        cout << "lista," << n << "," << m << "," << memory_mb << "\n";
    } else{
        vector<vector<int>> adj(n + 1, vector<int>(n + 1, 0));
        if(!ler_arestas([&](int u, int v){
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
