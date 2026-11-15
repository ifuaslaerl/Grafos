#include <bits/stdc++.h>
using namespace std;

template<typename T>
class Djikstra{
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

    void add_edge(int u, int v, T w){
        adj[u].push_back({v, w});
    }

    void run(int s){
        priority_queue<
            pair<T, int>,
            vector<pair<T, int>>,
            greater<pair<T, int>>
        > fila;

        fila.push({0, s});
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
};