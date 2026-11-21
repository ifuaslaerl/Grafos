#include "algorithm.hpp"

#include <algorithm>
#include <limits>
#include <vector>

using namespace std;

template<typename T>
class BellmanFord:
    public ISingleSourceAlgorithm<T>,
    public INegativeCycleDetector{
public:

    struct Edge{
        int u, v;
        T w;
        Edge(int u_, int v_, T w_): u(u_), v(v_), w(w_){}
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

    void reset(){
        fill(pai.begin(), pai.end(), 0);
        fill(dist.begin(), dist.end(), inf);
        negative_cicle = false;
    }

    void add_edge(int u, int v, T w=1) override{
        edges.push_back(Edge(u, v, w));
    }

    bool should_run(int n, int m) const override{
        return static_cast<long long>(n) * m
            <= this->operation_budget;
    }

    void run(int s=1) override{
        reset();
        dist[s] = 0;
        pai[s] = s;

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

    T get_aprox_diameter(int s=1){
        int v=s;
        reset();
        run(v);
        for(int u=1; u<=n; u++){
            if(dist[u] == inf) continue;
            if(dist[u] > dist[v]) v = u;
        }

        reset();
        run(v);
        for(int u=1; u<=n; u++){
            if(dist[u] == inf) continue;
            if(dist[u] > dist[v]) v = u;
        }

        return dist[v];
    }

    T get_dist(int s, int t){
        reset();
        run(s);
        return dist[t];
    }

    vector<T> get_dists() const override {
        return dist;
    }

    vector<int> get_tree() const override {
        return pai;
    }

    bool has_negative_cycle() const override {return negative_cicle;}

};