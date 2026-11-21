#include "algorithm.hpp"

#include <algorithm>
#include <cmath>
#include <functional>
#include <limits>
#include <queue>
#include <utility>
#include <vector>

using namespace std;

template<typename T>
class Djikstra: public ISingleSourceAlgorithm<T>{
public:
    int n;
    const T inf = numeric_limits<T>::max();
    vector<T> dist;
    vector<int> pai;
    vector<vector<int>> conected_components;
    vector<vector<pair<int, T>>> adj;

    Djikstra(int n_): 
        n(n_), 
        dist(n_+1, inf), 
        pai(n_+1),
        adj(n_+1) 
    {} 

    void reset(){
        fill(dist.begin(), dist.end(), inf);
        fill(pai.begin(), pai.end(), 0);
        conected_components.clear();
    }

    void add_edge(int u, int v, T w=1) override{
        adj[u].push_back({v, w});
    }

    bool should_run(int n, int m) const override{
        const int log_n = (int)floor(log2(n));
        return (ll)n * log_n + m
            <= this->operation_budget;
    }

    void go(int s=1){
        conected_components.emplace_back();
        priority_queue<
            pair<T, int>,
            vector<pair<T, int>>,
            greater<pair<T, int>>
        > fila;

        fila.push({0, s});
        pai[s] = s;
        dist[s] = 0;

        while(!fila.empty()){
            auto[current_distance, v] = fila.top();
            fila.pop();
            if(current_distance != dist[v]) continue;

            conected_components.back().push_back(v);

            for(auto[u, w]: adj[v]){
                if(dist[u] <= dist[v] + w) continue;
                pai[u] = v;
                dist[u] = dist[v] + w;
                fila.push({dist[u], u});
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

        sort(conected_components.begin(), conected_components.end(),
            [](const vector<int> &a, const vector<int> &b){
                return a.size() > b.size();
            }
        );
    }

    T get_aprox_diameter(int s=1){
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

    T get_dist(int s, int t){
        reset();
        go(s);
        return dist[t];
    }

    vector<int> get_path(int s, int t) override{
        get_dist(s, t);
        return reconstruct_path(s, t, dist, pai, inf);
    }

    vector<vector<int>> get_components() const {
        return conected_components;
    }

    vector<T> get_dists() const override {
        return dist;
    }

    vector<int> get_tree() const override {
        return pai;
    }

};
