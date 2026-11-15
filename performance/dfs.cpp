#include "algorithm.hpp"

#include <limits>
#include <vector>

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

    void run(int s=1) override{
        // vai virar dist[s] + 1 = 0 depois
        dist[s] = -1; 
        dfs(s, s);
    }

    vector<int> get_dists(int s=1) const override {
        (void)s;
        return dist;
    }

    vector<int> get_tree(int s=1) const override {
        (void)s;
        return pai;
    }
};