#include "algorithm.hpp"

#include <algorithm>
#include <limits>
#include <vector>

using namespace std;

template<typename T>
class FloydWarshall: public IAlgorithm<T>{
public:
    int n;
    bool negative_cicle;
    const T inf = numeric_limits<T>::max();
    vector<vector<T>> dist;
    vector<vector<int>> pai;

    FloydWarshall(int n_): 
        n(n_),
        negative_cicle(false),
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

    bool should_run(int n, int) const override{
        return static_cast<__int128>(n) * n * n
            <= this->operation_budget;
    }

    void run(int s=1) override{
        (void)s;
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

    vector<T> get_dists(int s=1) const override {return dist[s];}
    vector<int> get_tree(int s=1) const override {return pai[s];}
    bool has_negative_cycle() const override {return negative_cicle;}

};