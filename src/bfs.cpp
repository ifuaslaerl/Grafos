#include "algorithm.hpp"

#include <cstddef>
#include <limits>
#include <queue>
#include <vector>

using namespace std;

template<typename T>
class Bfs: public IAlgorithm<T>{
public:
    int n;
    const T inf = numeric_limits<T>::max();
    vector<int> pai;
    vector<T> dist;
    vector<vector<int>> adj;

    Bfs(int n_): 
        n(n_), 
        pai(n_+1), 
        dist(n+1, inf), 
        adj(n_+1) 
    {} 

    void add_edge(int u, int v, T w=1) override{
        adj[u].push_back(v);
    }

    bool should_run(int n, int m) const override{
        return static_cast<long long>(n) + m <= this->operation_budget;
    }

    void go(int s=1){
        queue<int> fila;
        fila.push(s);
        dist[s] = 0;
        pai[s] = s;

        while(!fila.empty()){
            int v = fila.front();
            fila.pop();

            for(int u: adj[v]){
                if(dist[u] != inf) continue;
                dist[u] = dist[v]+1;
                pai[u] = v;
                fila.push(u);
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
        return dist;
    }

    vector<int> get_tree(int s=1) const override {
        return pai;
    }

};
