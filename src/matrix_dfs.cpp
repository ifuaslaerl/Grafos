#include "algorithm.hpp"

#include <algorithm>
#include <limits>
#include <vector>

using namespace std;

template<typename T>
class MatrixDfs: public ISingleSourceAlgorithm<T>{
public:
    int n;
    const T inf = numeric_limits<T>::max();
    vector<int> pai;
    vector<T> dist;
    vector<vector<int>> adj;
    vector<vector<int>> connected_components;

    MatrixDfs(int n_):
        n(n_),
        pai(n_ + 1),
        dist(n_ + 1, inf),
        adj(n_ + 1, vector<int>(n_ + 1))
    {}

    void reset(){
        fill(pai.begin(), pai.end(), 0);
        fill(dist.begin(), dist.end(), inf);
        connected_components.clear();
        this->zerar_contador();
    }

    void add_edge(int u, int v, T w=1) override{
        (void)w;
        adj[u][v] = 1;
    }

    bool should_run(int n, int) const override{
        const __int128 vertices = (__int128)n + 1;
        return this->fits_memory(
            vertices * vertices * sizeof(int) +
            vertices * (sizeof(vector<int>) + 2 * sizeof(int) + sizeof(T))
        );
    }

    void dfs(int v, int p){
        pai[v] = p;
        dist[v] = dist[p] + 1;
        connected_components.back().push_back(v);

        for(int u=1; u<=n; u++){
            if(!this->contar()) return;
            if(!adj[v][u] || dist[u] != inf) continue;
            dfs(u, v);
        }
    }

    void traverse_component(int s){
        connected_components.emplace_back();
        dist[s] = -1;
        dfs(s, s);
    }

    void run(int s=1) override{
        reset();
        traverse_component(s);

        for(int v=1; v<=n; v++){
            if(this->limite_excedido) return;
            if(dist[v] != inf) continue;
            traverse_component(v);
        }

        sort(connected_components.begin(), connected_components.end(),
            [](const vector<int> &a, const vector<int> &b){
                return a.size() > b.size();
            }
        );
    }

    void run_from(int s){
        reset();
        traverse_component(s);
    }

    T get_aprox_diameter(int s=1){
        int v = s;
        reset();
        traverse_component(v);
        for(int u=1; u<=n; u++){
            if(dist[u] == inf) continue;
            if(dist[u] > dist[v]) v = u;
        }

        reset();
        traverse_component(v);
        for(int u=1; u<=n; u++){
            if(dist[u] == inf) continue;
            if(dist[u] > dist[v]) v = u;
        }

        return dist[v];
    }

    T get_dist(int s, int t){
        reset();
        traverse_component(s);
        return dist[t];
    }

    vector<int> get_path(int s, int t) override{
        get_dist(s, t);
        return reconstruct_path(s, t, dist, pai, inf);
    }

    vector<vector<int>> get_components() const{
        return connected_components;
    }

    vector<T> get_dists() const override{
        return dist;
    }

    vector<int> get_tree() const override{
        return pai;
    }
};
