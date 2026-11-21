#include "algorithm.hpp"

#include <algorithm>
#include <limits>
#include <vector>

using namespace std;

template<typename T>
class MatrixDjikstra: public ISingleSourceAlgorithm<T>{
public:
    int n;
    const T inf = numeric_limits<T>::max();
    vector<T> dist;
    vector<int> pai;
    vector<vector<T>> adj;
    vector<vector<int>> connected_components;

    MatrixDjikstra(int n_):
        n(n_),
        dist(n_ + 1, inf),
        pai(n_ + 1),
        adj(n_ + 1, vector<T>(n_ + 1, inf))
    {}

    void reset(){
        fill(dist.begin(), dist.end(), inf);
        fill(pai.begin(), pai.end(), 0);
        connected_components.clear();
    }

    void add_edge(int u, int v, T w=1) override{
        adj[u][v] = min(adj[u][v], w);
    }

    bool should_run(int n, int) const override{
        return static_cast<__int128>(n) * n <= this->operation_budget;
    }

    void go(int source){
        connected_components.emplace_back();
        vector<unsigned char> visited(n + 1);
        dist[source] = 0;
        pai[source] = source;

        for(int step=0; step<n; step++){
            int v = 0;
            for(int candidate=1; candidate<=n; candidate++){
                if(visited[candidate] || dist[candidate] == inf) continue;
                if(v == 0 || dist[candidate] < dist[v]) v = candidate;
            }
            if(v == 0) break;

            visited[v] = 1;
            connected_components.back().push_back(v);
            for(int u=1; u<=n; u++){
                if(adj[v][u] == inf || dist[v] + adj[v][u] >= dist[u]){
                    continue;
                }
                dist[u] = dist[v] + adj[v][u];
                pai[u] = v;
            }
        }
    }

    void run(int source=1) override{
        reset();
        go(source);

        for(int v=1; v<=n; v++){
            if(dist[v] != inf) continue;
            go(v);
        }

        sort(connected_components.begin(), connected_components.end(),
            [](const vector<int>& lhs, const vector<int>& rhs){
                return lhs.size() > rhs.size();
            }
        );
    }

    T get_aprox_diameter(int source=1){
        int v = source;
        reset();
        go(v);
        for(int u=1; u<=n; u++){
            if(dist[u] == inf) continue;
            if(dist[u] > dist[v]) v = u;
        }

        reset();
        go(v);
        for(int u=1; u<=n; u++){
            if(dist[u] == inf) continue;
            if(dist[u] > dist[v]) v = u;
        }

        return dist[v];
    }

    T get_dist(int source, int target){
        reset();
        go(source);
        return dist[target];
    }

    vector<T> get_dists() const override{
        return dist;
    }

    vector<int> get_tree() const override{
        return pai;
    }
};
