#ifndef GRAFO_HPP
#define GRAFO_HPP

#include <vector>

typedef long double ld;
typedef long long ll;

class Grafo {
private:
    int n, m;
    int menor, maior, total;
    ld mediana_val;

    std::vector<int> grau;
    std::vector<std::vector<int>> adj;

    void dfs_componentes(
        int v,
        std::vector<int>& c,
        std::vector<int>& vis
    );

    void dfs_impl(
        int v,
        std::vector<int>& pai,
        std::vector<int>& dist
    );

    bool is_a_tree(
        int v,
        std::vector<int>& pai,
        std::vector<int>& dist
    );

public:
    Grafo(int n_);

    #pragma region Métodos Getters

    int get_n() const;
    int get_m() const;
    int get_grau_minimo() const;
    int get_grau_maximo() const;
    ld get_grau_medio() const;
    ld get_grau_mediano() const;

    const std::vector<std::vector<int>>& get_lista_adj() const;

    std::vector<std::vector<int>> get_matriz_adj() const;

    #pragma endregion

    #pragma region Construção do Grafo

    void add_edge(int u, int v);
    void build();

    #pragma endregion

    #pragma region Algoritmos em Grafos

    std::vector<std::vector<int>> get_components();

    void dfs(
        int start,
        std::vector<int>& pai,
        std::vector<int>& dist
    );

    void bfs(
        int start,
        std::vector<int>& pai,
        std::vector<int>& dist
    );

    int get_dist(int u, int v);
    int get_diameter();

    #pragma endregion
};

#endif // GRAFO_HPP