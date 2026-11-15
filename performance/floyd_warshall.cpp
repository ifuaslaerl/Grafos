#include <bits/stdc++.h>
#include "algorithm.hpp"
using namespace std;

template<typename T>
class FloydWarshall: public IAlgorithm<T>{
public:
    int n;
    bool negative_cicle;
    int source;
    const T inf = numeric_limits<T>::max();
    vector<vector<T>> dist;
    vector<vector<int>> pai;

    FloydWarshall(int n_): 
        n(n_),
        negative_cicle(false),
        source(0),
        dist(n_+1, vector<T>(n_+1, inf)), 
        pai(n_+1, vector<int>(n_+1)) 
    {
        for(int i=1; i<=n; i++){
            dist[i][i] = 0;
            pai[i][i] = i;
        }
    } 

    void add_edge(int u, int v, T w=1) override{
        dist[u][v] = min(dist[u][v], w);
        pai[u][v] = u;
    }

    void run(int source_) override{
        source = source_;
        for(int k=1; k<=n; k++){
            for(int i=1; i<=n; i++){
                for(int j=1; j<=n; j++){
                    if(dist[i][k] == inf) continue;
                    if(dist[k][j] == inf) continue;
                    if(dist[i][j] > dist[i][k] + dist[k][j]){
                        dist[i][j] = dist[i][k] + dist[k][j];
                        pai[i][j] = pai[k][j];
                    }
                }
            }

        }

        for(int v=1; v<=n; v++){
            if(dist[v][v] >= 0) continue;
            negative_cicle=true;
        }
    }

    vector<T> get_dists() const override {return dist[source];}
    vector<int> get_tree() const override {return pai[source];}
    bool has_negative_cycle() const override {return negative_cicle;}
};