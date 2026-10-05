#ifndef GRAFO_HPP
#define GRAFO_HPP

#include <limits>
#include <stdexcept>
#include <vector>

using namespace std;

namespace graph_lib{
template <typename T>
class IFronteira;
}

template <typename T>
struct Edge{
    int u, v;
    T w;
};

template <typename T>
class Grafo{
private:
    int n, m, menor, maior, total;
    long double mediana_val;

    vector<int> grau;
    vector<vector<Edge<T>>> adj;

    vector<int> traverse_impl(
        int start,
        vector<int>& pai,
        vector<T>& dist,
        graph_lib::IFronteira<T>& fronteira
    );

    void dfs_recursiva_util(
        int u,
        int pai,
        vector<int>& p,
        vector<T>& dist,
        vector<int>& ordem
    );

public:
    explicit Grafo(int n_);

    int get_n() const;
    int get_m() const;
    int get_grau_minimo() const;
    int get_grau_maximo() const;
    long double get_grau_medio() const;
    long double get_grau_mediano() const;

    const vector<vector<Edge<T>>>& get_lista_adj() const;
    vector<vector<int>> get_matriz_adj() const;

    void reserve_adjacencia(const vector<int>& capacidades);
    void add_edge(int u, int v, T w = T{1});
    void add_arc(int u, int v, T w);
    void build();

    vector<vector<int>> get_components();

    vector<int> dfs(
        int start,
        vector<int>& pai,
        vector<int>& dist
    );

    vector<int> dfs_recursiva(
        int start,
        vector<int>& pai,
        vector<T>& dist
    );

    vector<int> bfs(
        int start,
        vector<int>& pai,
        vector<int>& dist
    );

    vector<int> dijkstra_vector(
        int start,
        vector<int>& pai,
        vector<T>& dist
    );

    vector<int> dijkstra_heap(
        int start,
        vector<int>& pai,
        vector<T>& dist
    );

    T get_dist(int u, int v, graph_lib::IFronteira<T>& fronteira);
    int get_diameter();
};

#include "graph_general.tpp"
#include "graph_algorithms.tpp"

#endif
