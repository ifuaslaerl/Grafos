#include <bits/stdc++.h>
using namespace std;

template<typename T>
class BellmanFord{

    struct Edge{
        T w;
        int u, v;
        Edge(int u_, int v_, T w_): u(u_), v(v_), w(w_) {}
    };

    int n;
    bool negative_cicle;
    const T inf = numeric_limits<T>::max();
    vector<Edge> edges;
    vector<int> pai;
    vector<T> dist;

    BellmanFord(int n_): 
        n(n_), 
        negative_cicle(false),
        pai(n_+1),
        dist(n_+1, inf)
    {} 

    void add_edge(int u, int v, T w){
        edges.push_back(Edge(u, v, w));
    }

    void run(int t){
        dist[t] = 0;
        pai[t] = t;

        for(int k=1; k<=n; k++){
            for(Edge e: edges){
                auto[u, v, w] = e;
                if(dist[u] == inf) continue;
                if(dist[v] <= dist[u] + w) continue;
                if(k==n) negative_cicle = true;
                dist[v] = dist[u] + w;
                pai[v] = u;
            }
        }
    }
};