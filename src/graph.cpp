#include <bits/stdc++.h>
using namespace std;

typedef long double ld;
typedef long long ll;
const int inf = 2e9;

class Grafo{
private:
    int n, m;
    int menor, maior, total;
    ld mediana_val;
    
    vector<int> grau;
    vector<vector<int>> adj;

    void dfs_componentes(int v, vector<int>& c, vector<int>& vis){
        c.push_back(v);
        vis[v] = 1;
        for(int prox: adj[v]){
            if(vis[prox]) continue;
            dfs_componentes(prox, c, vis);
        }
    }

    void dfs_impl(int v, vector<int>& pai, vector<int>& dist){
        for(int prox : adj[v]){
            if(pai[prox]) continue;
            pai[prox] = v;
            dist[prox] = dist[v] + 1;
            dfs_impl(prox, pai, dist);
        }
    }

    bool is_a_tree(int v, vector<int>& pai, vector<int>& dist){
        if(m != n-1) return false;
        bfs(1, pai, dist);
        return *min_element(pai.begin()+1, pai.end());
    }

public:
    Grafo(int n_) : n(n_), m(0), menor(0), maior(0), total(0), 
                    mediana_val(0), grau(n_+1, 0), adj(n_+1) {}

    #pragma region Métodos Getters 
    int get_n() const { return n; }
    int get_m() const { return m; }
    int get_grau_minimo() const { return menor; }
    int get_grau_maximo() const { return maior; }
    ld  get_grau_medio()  const { return (n > 0) ? (ld)total / n : 0; }
    ld  get_grau_mediano()const { return mediana_val; }

    const vector<vector<int>>& get_lista_adj() const { return adj; }

    vector<vector<int>> get_matriz_adj() const {
        vector<vector<int>> matriz(n + 1, vector<int>(n + 1, 0));
        for(int u = 1; u <= n; u++) {
            for(int v : adj[u]) matriz[u][v] = 1;
        }
        return matriz;
    }
    #pragma endregion

    #pragma region Construção do Grafo
    void add_edge(int u, int v){
        adj[u].push_back(v);
        adj[v].push_back(u);
        grau[u]++;
        grau[v]++;
        m++;
    }

    void build(){
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
            mediana_val = (ord_graus[n / 2 - 1] + ord_graus[n / 2]) / 2.0;
        } else {
            mediana_val = ord_graus[n / 2];
        }
    }
    #pragma endregion

    #pragma region Algoritmos em Grafos
    vector<vector<int>> get_components(){
        vector<vector<int>> asw;
        vector<int> vis(n+1, 0);
        
        for(int v=1; v<=n; v++){
            if(vis[v]) continue;
            vector<int> comp;
            dfs_componentes(v, comp, vis);
            asw.push_back(comp);
        }
        
        sort(asw.begin(), asw.end(), [](const vector<int> &a, const vector<int> &b){
            return a.size() > b.size();
        });

        return asw;
    }

    void dfs(int start, vector<int> &pai, vector<int> &dist){
        pai.assign(n+1, 0);
        dist.assign(n+1, 0);

        pai[start] = start;
        dfs_impl(start, pai, dist);
    }

    void bfs(int start, vector<int> &pai, vector<int> &dist){
        pai.assign(n+1, 0);
        dist.assign(n+1, 0);

        queue<int> fila;
        
        fila.push(start);
        pai[start] = start; 
        dist[start] = 0; 
        
        while(!fila.empty()){
            int v = fila.front();
            fila.pop();
            for(int prox: adj[v]){
                if(pai[prox]) continue;
                dist[prox] = dist[v] + 1;
                pai[prox] = v;
                fila.push(prox);
            }
        }
    }

    int get_dist(int u, int v){ 
        vector<int> p, d;
        bfs(u, p, d);
        return d[v];
    }

    int get_diameter(){ 
        vector<int> p, d;

        if(is_a_tree(1, p, d)){
            int v1 = max_element(d.begin()+1, d.end()) - d.begin();
            bfs(v1, p, d);
            return *max_element(d.begin()+1, d.end());
        }
        
        int asw = 0;
        for(int v=1; v<=n; v++){
            bfs(v, p, d);
            asw = max(asw, *max_element(d.begin()+1, d.end()));
        }
        return asw;
    }
    #pragma endregion
};