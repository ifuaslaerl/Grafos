#include "algorithm.hpp"

#include <algorithm>
#include <cstddef>
#include <limits>
#include <queue>
#include <vector>

using namespace std;

template<typename T>
class Bfs: public ISingleSourceAlgorithm<T>{
public:
    int n;
    const T inf = numeric_limits<T>::max();
    vector<int> pai;
    vector<T> dist;
    vector<vector<int>> adj, connected_components;

    Bfs(int n_):
        n(n_),
        pai(n_ + 1),
        dist(n_ + 1, inf),
        adj(n_ + 1)
    {}

    void reset(){
        fill(pai.begin(), pai.end(), 0);
        fill(dist.begin(), dist.end(), inf);
        connected_components.clear();
    }

    void add_edge(int u, int v, T w=1) override{
        (void)w;
        adj[u].push_back(v);
    }

    bool should_run(int n, int m) const override{
        return (ll)n + m <= this->operation_budget;
    }

    void go(int s=1){
        connected_components.emplace_back();
        queue<int> fila;
        fila.push(s);
        dist[s] = 0;
        pai[s] = s;

        while(!fila.empty()){
            int v = fila.front();
            fila.pop();

            connected_components.back().push_back(v);

            for(int u: adj[v]){
                if(dist[u] != inf) continue;
                dist[u] = dist[v]+1;
                pai[u] = v;
                fila.push(u);
            }
        }
    }

    void run(int s=1) override{
        reset();
        go(s);

        for(int v=1; v<=n; v++){
            if(dist[v] != inf) continue;
            go(v);
        }

        sort(connected_components.begin(), connected_components.end(),
            [](const vector<int> &a, const vector<int> &b){
                return a.size() > b.size();
            }
        );
    }

    void run_from(int s){
        reset();
        go(s);
    }

    int get_aprox_diameter(int s=1){
        int v=s;
        reset();
        go(v);
        for(int u=1; u<=n; u++){
            if(dist[u] == inf) continue;
            if(dist[u] > dist[v]) v = u;
        }

        reset();
        go(v);
        for(int u=1; u<=n; u++){
            if(dist[u] == inf) continue;
            if(dist[u] > dist[v]) v = u;
        }

        return dist[v];
    }

    int get_dist(int s, int t){
        reset();
        go(s);
        return dist[t];
    }

    vector<int> get_path(int s, int t) override{
        get_dist(s, t);
        return reconstruct_path(s, t, dist, pai, inf);
    }

    vector<vector<int>> get_components() const {
        return connected_components;
    }

    vector<T> get_dists() const override {
        return dist;
    }

    vector<int> get_tree() const override {
        return pai;
    }

};
