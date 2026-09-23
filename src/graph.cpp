#include "graph.hpp"
#include <algorithm>
#include <queue>
#include <stack>
#include <cassert>

const int inf = 2e9;

#pragma region Fronteiras

class FilaFronteira : public Fronteira {
    queue<pair<int, int>> q;
public:
    void push(int v, int p) override { q.push({v, p}); }
    pair<int, int> pop() override { 
        auto item = q.front(); 
        q.pop(); 
        return item; 
    }
    bool empty() override { return q.empty(); }
};

class PilhaFronteira : public Fronteira {
    stack<pair<int, int>> s;
public:
    void push(int v, int p) override { s.push({v, p}); }
    pair<int, int> pop() override { 
        auto item = s.top(); 
        s.pop(); 
        return item; 
    }
    bool empty() override { return s.empty(); }
};

#pragma endregion

#pragma region Métodos privados

vector<int> Grafo::traverse_impl(
    int start,
    vector<int>& pai,
    vector<int>& dist,
    Fronteira& f
) {
    vector<int> comp;

    f.push(start, start);

    while(!f.empty()){
        auto [v, p] = f.pop();

        // MARCAÇÃO NO POP
        if(pai[v]) continue;
        
        pai[v] = p;
        dist[v] = (v == start) ? 0 : dist[p] + 1;
        
        comp.push_back(v);

        for(int prox : adj[v]){
            if(!pai[prox]){
                f.push(prox, v);
            }
        }
    }
    
    return comp;
}

#pragma endregion

Grafo::Grafo(int n_)
    : n(n_),
      m(0),
      menor(0),
      maior(0),
      total(0),
      mediana_val(0),
      grau(n_ + 1, 0),
      adj(n_ + 1) {}

#pragma region Métodos Getters

int Grafo::get_n() const { return n; }
int Grafo::get_m() const { return m; }
int Grafo::get_grau_minimo() const { return menor; }
int Grafo::get_grau_maximo() const { return maior; }
ld Grafo::get_grau_medio() const { return (n > 0) ? (ld) total / n : 0; }
ld Grafo::get_grau_mediano() const { return mediana_val; }
const vector<vector<int>>& Grafo::get_lista_adj() const { return adj; }

vector<vector<int>> Grafo::get_matriz_adj() const {
    vector<vector<int>> matriz(n + 1, vector<int>(n + 1, 0));
    for (int u = 1; u <= n; u++) {
        for (int v : adj[u]) {
            matriz[u][v] = 1;
        }
    }
    return matriz;
}

#pragma endregion

#pragma region Construção do Grafo

void Grafo::add_edge(int u, int v) {
    assert(u && v);
    assert(1 <= u && u <= n);
    assert(1 <= v && v <= n);

    adj[u].push_back(v);
    adj[v].push_back(u);

    grau[u]++;
    grau[v]++;
    m++;
}

void Grafo::build() {
    if (n == 0) return;

    menor = inf;
    maior = 0;
    total = 0;

    vector<int> ord_graus;
    ord_graus.reserve(n);

    for (int v = 1; v <= n; v++) {
        int d = grau[v];
        total += d;
        menor = min(menor, d);
        maior = max(maior, d);
        ord_graus.push_back(d);
    }

    sort(ord_graus.begin(), ord_graus.end());

    if (n % 2 == 0) {
        mediana_val = (ord_graus[n / 2 - 1] + ord_graus[n / 2])/ 2.0;
    } else {
        mediana_val = ord_graus[n / 2];
    }
}

#pragma endregion

#pragma region Algoritmos em Grafos

vector<vector<int>> Grafo::get_components(){
    vector<vector<int>> asw;
    
    vector<int> pai(n + 1, 0);
    vector<int> dist(n + 1, inf);

    for(int v = 1; v <= n; v++){
        if(pai[v]) continue;
        
        FilaFronteira fila;
        vector<int> comp = traverse_impl(v, pai, dist, fila);
        asw.push_back(comp);
    }

    sort(asw.begin(), asw.end(), [](const vector<int>& a, const vector<int>& b){
        return a.size() > b.size();
    });

    return asw;
}

vector<int> Grafo::dfs(int start, vector<int>& pai, vector<int>& dist){
    pai.assign(n + 1, 0);
    dist.assign(n + 1, inf);
    
    PilhaFronteira pilha;
    return traverse_impl(start, pai, dist, pilha);
}

vector<int> Grafo::bfs(int start, vector<int>& pai, vector<int>& dist){
    pai.assign(n + 1, 0);
    dist.assign(n + 1, inf);
    
    FilaFronteira fila;
    return traverse_impl(start, pai, dist, fila);
}

int Grafo::get_dist(int u, int v) {
    vector<int> p, d;
    bfs(u, p, d);
    return d[v] != inf ? d[v] : -1;
}

int Grafo::get_diameter() {
    vector<int> p, d;
    int asw = 0;
    for(vector<int> &component: get_components()){
        bfs(component[0], p, d);
        int best = component[0];
        
        for(int v = 1; v <= n; v++){
            if(d[v] == inf) continue;
            if(d[best] < d[v]){
                best = v;
            }
        }
        
        bfs(best, p, d);
        
        for(int v = 1; v <= n; v++){
            if(d[v] == inf) continue;
            if(d[best] < d[v]){
                best = v;
            }
        }

        asw = max(asw, d[best]);
    }
    return asw;
}

#pragma endregion