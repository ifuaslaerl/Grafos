#include <bits/stdc++.h>
#include "algorithm.hpp"
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

    void add_edge(int u, int v, T w=1){
        adj[u].push_back(v);
    }

    void run(int s){
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

    vector<T> get_dists() const {return dist;}
    vector<int> get_tree() const {return pai;}
};