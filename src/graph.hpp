#ifndef GRAFO_HPP
#define GRAFO_HPP

#include <vector>
#include <utility>

using namespace std;

typedef long double ld;
typedef long long ll;

// Fronteira carrega o par: (Vértice Atual, Pai que o descobriu)
class Fronteira {
public:
    virtual void push(int v, int p) = 0;
    virtual pair<int, int> pop() = 0;
    virtual bool empty() = 0;
    virtual ~Fronteira() = default;
};

class Grafo {
private:
    int n, m;
    int menor, maior, total;
    ld mediana_val;

    vector<int> grau;
    vector<vector<int>> adj;

    vector<int> traverse_impl(
        int start,
        vector<int>& pai,
        vector<int>& dist,
        Fronteira& fronteira
    );

    void dfs_recursiva_util(int u, int pai, int dist_atual, std::vector<int>& p, std::vector<int>& d, std::vector<bool>& vis, std::vector<int>& ordem);

public:
    Grafo(int n_);

    #pragma region Métodos Getters

    int get_n() const;
    int get_m() const;
    int get_grau_minimo() const;
    int get_grau_maximo() const;
    ld get_grau_medio() const;
    ld get_grau_mediano() const;

    const vector<vector<int>>& get_lista_adj() const;
    vector<vector<int>> get_matriz_adj() const;

    #pragma endregion

    #pragma region Construção do Grafo

    void add_edge(int u, int v);
    void build();

    #pragma endregion

    #pragma region Algoritmos em Grafos

    vector<vector<int>> get_components();

    vector<int> dfs(
        int start,
        vector<int>& pai,
        vector<int>& dist
    );

    vector<int> dfs_recursiva(int start, std::vector<int>& p, std::vector<int>& d);
    
    vector<int> bfs(
        int start,
        vector<int>& pai,
        vector<int>& dist
    );

    int get_dist(int u, int v);
    int get_diameter();

    #pragma endregion
};

#endif // GRAFO_HPP