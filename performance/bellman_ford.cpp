#include "algorithm.hpp"

#include <limits>
#include <vector>

using namespace std;

template<typename T>
class BellmanFord: public IAlgorithm<T>{
public:

    struct Edge{
        int u, v;
        T w;
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

    void add_edge(int u, int v, T w=1) override{
        edges.push_back(Edge(u, v, w));
    }

    void run(int s=1) override{
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

    vector<T> get_dists(int s=1) const override {
        (void)s;
        return dist;
    }

    vector<int> get_tree(int s=1) const override {
        (void)s;
        return pai;
    }

    bool has_negative_cycle() const override {return negative_cicle;}
};