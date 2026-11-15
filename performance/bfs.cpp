#include <bits/stdc++.h>
using namespace std;

class Bfs{
    int n;
    const int inf = numeric_limits<int>::max();
    vector<int> pai, dist;
    vector<vector<int>> adj;

    Bfs(int n_): 
        n(n_), 
        pai(n_+1), 
        dist(n+1, inf), 
        adj(n_+1) 
    {} 

    void add_edge(int u, int v){
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
};