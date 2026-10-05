#ifndef GRAFO_GENERAL_TPP
#define GRAFO_GENERAL_TPP

#include <algorithm>
#include <cassert>

using namespace std;

template <typename T>
Grafo<T>::Grafo(int n_)
    : n(n_),
      m(0),
      menor(0),
      maior(0),
      total(0),
      mediana_val(0),
      grau(n_ + 1, 0),
      adj(n_ + 1){
    assert(n_ >= 0);
}

template <typename T>
int Grafo<T>::get_n() const{
    return n;
}

template <typename T>
int Grafo<T>::get_m() const{
    return m;
}

template <typename T>
int Grafo<T>::get_grau_minimo() const{
    return menor;
}

template <typename T>
int Grafo<T>::get_grau_maximo() const{
    return maior;
}

template <typename T>
long double Grafo<T>::get_grau_medio() const{
    return n > 0 ? static_cast<long double>(total) / n : 0;
}

template <typename T>
long double Grafo<T>::get_grau_mediano() const{
    return mediana_val;
}

template <typename T>
const vector<vector<Edge<T>>>& Grafo<T>::get_lista_adj() const{
    return adj;
}

template <typename T>
void Grafo<T>::reserve_adjacencia(const vector<int>& capacidades){
    if(capacidades.size() != adj.size()){
        throw invalid_argument("Capacidades devem ter tamanho n + 1.");
    }
    for(size_t vertex = 0; vertex < adj.size(); ++vertex){
        if(capacidades[vertex] < 0){
            throw invalid_argument("Capacidade de adjacencia nao pode ser negativa.");
        }
        adj[vertex].reserve(static_cast<size_t>(capacidades[vertex]));
    }
}

template <typename T>
vector<vector<int>> Grafo<T>::get_matriz_adj() const{
    vector<vector<int>> matriz(n + 1, vector<int>(n + 1, 0));
    for(int u = 1; u <= n; ++u){
        for(const Edge<T>& edge : adj[u]){
            matriz[u][edge.v] = 1;
        }
    }
    return matriz;
}

template <typename T>
void Grafo<T>::add_edge(int u, int v, T w){
    assert(1 <= u && u <= n);
    assert(1 <= v && v <= n);

    adj[u].push_back({u, v, w});
    adj[v].push_back({v, u, w});

    ++grau[u];
    ++grau[v];
    ++m;
}

template <typename T>
void Grafo<T>::add_arc(int u, int v, T w){
    assert(1 <= u && u <= n);
    assert(1 <= v && v <= n);

    adj[u].push_back({u, v, w});
    ++grau[u];
    ++m;
}

template <typename T>
void Grafo<T>::build(){
    if(n == 0){
        menor = 0;
        maior = 0;
        total = 0;
        mediana_val = 0;
        return;
    }

    menor = grau[1];
    maior = 0;
    total = 0;

    vector<int> ord_graus;
    ord_graus.reserve(n);

    for(int v = 1; v <= n; ++v){
        const int d = grau[v];
        total += d;
        menor = min(menor, d);
        maior = max(maior, d);
        ord_graus.push_back(d);
    }

    sort(ord_graus.begin(), ord_graus.end());
    if(n % 2 == 0){
        mediana_val =
            (static_cast<long double>(ord_graus[n / 2 - 1]) +
             ord_graus[n / 2]) /
            2.0L;
    } else{
        mediana_val = ord_graus[n / 2];
    }
}

#endif
