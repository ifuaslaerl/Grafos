#include <cmath>
#include <iostream>
#include <map>
#include <string>
#include <vector>

using namespace std;

int main(int argc, char* argv[]){
    ios_base::sync_with_stdio(false);
    cin.tie(nullptr);

    const bool weighted = argc == 2 && string(argv[1]) == "--weighted";
    if(argc > 2 || (argc == 2 && !weighted)){
        cerr << "Uso: export_graus [--weighted] < entrada.txt\n";
        return 1;
    }

    int n;
    if(!(cin >> n) || n < 0){
        cerr << "A primeira entrada deve conter um numero de vertices valido.\n";
        return 1;
    }

    vector<int> degrees(n + 1, 0);
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
        if(!isfinite(weight)){
            cerr << "Peso de aresta invalido.\n";
            return 1;
        }
        ++degrees[u];
        ++degrees[v];
    }
    if(cin.bad() || !cin.eof()){
        cerr << "Formato invalido nas arestas de entrada.\n";
        return 1;
    }

    map<int, int> frequencies;
    for(int vertex = 1; vertex <= n; ++vertex){
        ++frequencies[degrees[vertex]];
    }

    cout << "Grau,Frequencia\n";
    for(const auto& [degree, count] : frequencies){
        cout << degree << "," << count << "\n";
    }
    return 0;
}
