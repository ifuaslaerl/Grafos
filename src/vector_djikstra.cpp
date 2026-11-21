#include "algorithm.hpp"

#include <algorithm>
#include <limits>
#include <utility>
#include <vector>

using namespace std;

template<typename T>
class VectorDjikstra: public ISingleSourceAlgorithm<T>{
public:
    int n;
    const T inf = numeric_limits<T>::max();
    vector<T> dist;
    vector<int> pai;
    vector<vector<pair<int, T>>> adj;
    vector<vector<int>> connected_components;

    VectorDjikstra(int n_):
        n(n_),
        dist(n_ + 1, inf),
        pai(n_ + 1),
        adj(n_ + 1)
    {}

    void reset(){
        fill(dist.begin(), dist.end(), inf);
        fill(pai.begin(), pai.end(), 0);
        connected_components.clear();
        this->zerar_contador();
    }

    void add_edge(int u, int v, T w=1) override{
        adj[u].push_back({v, w});
    }

    bool should_run(int n, int m) const override{
        const __int128 vertices = (__int128)n + 1;
        return this->fits_memory(
            vertices * (sizeof(vector<pair<int, T>>) + sizeof(T) +
                        3 * sizeof(int)) +
            (__int128)m * sizeof(pair<int, T>)
        );
    }

    void go(int s){
        connected_components.emplace_back();
        vector<int> visited(n + 1);
        dist[s] = 0;
        pai[s] = s;

        for(int i=0; i<n; i++){
            int v = 0;
            for(int u=1; u<=n; u++){
                if(!this->contar()) return;
                if(visited[u] || dist[u] == inf) continue;
                if(v == 0 || dist[u] < dist[v]) v = u;
            }
            if(v == 0) break;

            visited[v] = 1;
            connected_components.back().push_back(v);
            for(const auto &[u, w]: adj[v]){
                if(!this->contar()) return;
                if(dist[v] + w >= dist[u]) continue;
                dist[u] = dist[v] + w;
                pai[u] = v;
            }
        }
    }

    void run(int s=1) override{
        reset();
        go(s);

        for(int v=1; v<=n; v++){
            if(this->limite_excedido) return;
            if(dist[v] != inf) continue;
            go(v);
        }

        sort(connected_components.begin(), connected_components.end(),
            [](const vector<int> &a, const vector<int> &b){
                return a.size() > b.size();
            }
        );
    }

    T get_aprox_diameter(int s=1){
        int v = s;
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

    T get_dist(int s, int t){
        reset();
        go(s);
        return dist[t];
    }

    vector<int> get_path(int s, int t) override{
        get_dist(s, t);
        return reconstruct_path(s, t, dist, pai, inf);
    }

    vector<T> get_dists() const override{
        return dist;
    }

    vector<int> get_tree() const override{
        return pai;
    }
};
