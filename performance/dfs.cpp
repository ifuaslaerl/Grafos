#include <bits/stdc++.h>
#include "algorithm.hpp"
using namespace std;

class Dfs: public IAlgorithm<int>{
public:
    int n;
    const int inf = numeric_limits<int>::max();
    vector<int> pai, dist;
    vector<vector<int>> adj;

    Dfs(int n_): 
        n(n_), 
        pai(n_+1), 
        dist(n_+1, inf), 
        adj(n_+1) 
    {} 

    void add_edge(int u, int v, int w=1) override{
        adj[u].push_back(v);
    }

    void dfs(int v, int p){
        pai[v] = p;
        dist[v] = dist[p]+1;
        for(int u: adj[v]){
            if(dist[u] != inf) continue;
            dfs(u, v);
        }
    }

    void run(int s) override{
        // vai virar dist[s] + 1 = 0 depois
        dist[s] = -1; 
        dfs(s, s);
    }

    vector<int> get_dists() const override {return dist;}
    vector<int> get_tree() const override {return pai;}
};