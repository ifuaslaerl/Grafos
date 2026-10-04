#include "graph.hpp"

#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>
#include <vector>

using namespace std;

namespace{

bool parse_start(int argc, char* argv[], int& start){
    if(argc == 1){
        start = 1;
        return true;
    }
    if(argc != 2){
        cerr << "Uso: test_graph [vertice_inicial]\n";
        return false;
    }

    try{
        size_t parsed = 0;
        start = stoi(argv[1], &parsed);
        if(parsed != string(argv[1]).size()){
            throw invalid_argument("entrada invalida");
        }
    } catch(const exception&){
        cerr << "Erro: o vertice inicial deve ser um inteiro.\n";
        return false;
    }
    return true;
}

void print_distance(long double distance){
    if(distance == numeric_limits<long double>::infinity()){
        cout << "INF";
    } else{
        cout << distance;
    }
}

} // namespace

int main(int argc, char* argv[]){
    int start, n, u, v;
    if(!parse_start(argc, argv, start)){
        return 1;
    }

    if(!(cin >> n) || n < 0){
        cerr << "Erro: a primeira linha deve conter um numero de vertices valido.\n";
        return 1;
    }

    Grafo<long double> graph(n);
    long double weight;
    while(cin >> u){
        if(!(cin >> v >> weight)){
            cerr << "Erro: cada aresta deve conter u, v e peso.\n";
            return 1;
        }
        if(u < 1 || u > n || v < 1 || v > n){
            cerr << "Erro: endpoint de aresta fora do intervalo de vertices.\n";
            return 1;
        }
        graph.add_edge(u, v, weight);
    }

    if(start < 1 || start > n){
        cerr << "Erro: vertice inicial fora do intervalo 1.." << n << ".\n";
        return 1;
    }

    vector<int> bfs_parent, bfs_level;
    graph.bfs(start, bfs_parent, bfs_level);

    vector<int> dfs_parent;
    vector<long double> dfs_distance;
    graph.dfs_recursiva(start, dfs_parent, dfs_distance);

    vector<int> vector_parent, heap_parent;
    vector<long double> vector_distance, heap_distance;
    graph.dijkstra_vector(start, vector_parent, vector_distance);

    graph.dijkstra_heap(start, heap_parent, heap_distance);

    cout << setprecision(12);
    cout << "vertice,bfs_pai,bfs_nivel,dfs_pai,dfs_nivel,"
                 "dijkstra_vetor_pai,dijkstra_vetor_dist,"
                 "dijkstra_heap_pai,dijkstra_heap_dist\n";
    for(int vertex = 1; vertex <= n; ++vertex){
        cout << vertex << ','
                  << bfs_parent[vertex] << ','
                  << bfs_level[vertex] << ','
                  << dfs_parent[vertex] << ','
                  << dfs_distance[vertex] << ','
                  << vector_parent[vertex] << ',';
        print_distance(vector_distance[vertex]);
        cout << ',' << heap_parent[vertex] << ',';
        print_distance(heap_distance[vertex]);
        cout << '\n';
    }
}
