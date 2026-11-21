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
    vector<vector<unsigned char>> adj;
    vector<vector<int>> connected_components;

    MatrixDfs(int n_):
        n(n_),
        pai(n_ + 1),
        dist(n_ + 1, inf),
        adj(n_ + 1, vector<unsigned char>(n_ + 1))
    {}

    void reset(){
        fill(pai.begin(), pai.end(), 0);
        fill(dist.begin(), dist.end(), inf);
        connected_components.clear();
    }

    void add_edge(int u, int v, T w=1) override{
        (void)w;
        adj[u][v] = 1;
    }

    bool should_run(int n, int) const override{
        return static_cast<__int128>(n) * n <= this->operation_budget;
    }

    void dfs(int v, int parent){
        pai[v] = parent;
        dist[v] = dist[parent] + 1;
        connected_components.back().push_back(v);

        for(int u=1; u<=n; u++){
            if(!adj[v][u] || dist[u] != inf) continue;
            dfs(u, v);
        }
    }

    void traverse_component(int source){
        connected_components.emplace_back();
        dist[source] = -1;
        dfs(source, source);
    }

    void run(int source=1) override{
        reset();
        traverse_component(source);

        for(int v=1; v<=n; v++){
            if(dist[v] != inf) continue;
            traverse_component(v);
        }

        sort(connected_components.begin(), connected_components.end(),
            [](const vector<int>& lhs, const vector<int>& rhs){
                return lhs.size() > rhs.size();
            }
        );
    }

    void run_from(int source){
        reset();
        traverse_component(source);
    }

    T get_aprox_diameter(int source=1){
        int v = source;
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

    T get_dist(int source, int target){
        reset();
        traverse_component(source);
        return dist[target];
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
