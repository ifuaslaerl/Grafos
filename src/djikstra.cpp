#include "algorithm.hpp"

#include <cmath>
#include <functional>
#include <limits>
#include <queue>
#include <utility>
#include <vector>

using namespace std;

template<typename T>
class Djikstra: public IAlgorithm<T>{
public:
    int n;
    const T inf = numeric_limits<T>::max();
    vector<T> dist;
    vector<int> pai;
    vector<vector<pair<int, T>>> adj;

    Djikstra(int n_): 
        n(n_), 
        dist(n_+1, inf), 
        pai(n_+1),
        adj(n_+1) 
    {} 

    void add_edge(int u, int v, T w=1) override{
        adj[u].push_back({v, w});
    }

    bool should_run(int n, int m) const override{
        const int log_n = static_cast<int>(floor(log2(n)));
        return static_cast<long long>(n) * log_n + m
            <= this->operation_budget;
    }

    void go(int s=1){
        priority_queue<
            pair<T, int>,
            vector<pair<T, int>>,
            greater<pair<T, int>>
        > fila;

        fila.push({0, s});
        pai[s] = s;
        dist[s] = 0;

        while(!fila.empty()){
            auto[_, v] = fila.top();
            fila.pop();

            for(auto[u, w]: adj[v]){
                if(dist[u] <= dist[v] + w) continue;
                pai[u] = v;
                dist[u] = dist[v] + w;
                fila.push({dist[u], u});
            }
        }
    }

    void run(int s=1) override{
        go(s);
        for(int v=1; v<=n; v++){
            if(dist[v] != inf) continue;
            go(v);
        }
    }

    vector<T> get_dists(int s=1) const override {
        (void)s;
        return dist;
    }

    vector<int> get_tree(int s=1) const override {
        (void)s;
        return pai;
    }

};
