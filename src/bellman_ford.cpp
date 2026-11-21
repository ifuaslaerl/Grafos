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
    // Origem cujo resultado está em dist/pai (0 = nenhuma).
    int cached_source = 0;
    vector<vector<T>> all_dist;
    vector<vector<int>> all_pai;

    BellmanFord(int n_): 
        n(n_), 
        negative_cicle(false),
        pai(n_+1),
        dist(n_+1, inf)
    {} 

    void clear_state(){
        fill(pai.begin(), pai.end(), 0);
        fill(dist.begin(), dist.end(), inf);
        negative_cicle = false;
        cached_source = 0;
    }

    void reset(){
        clear_state();
        this->zerar_contador();
    }

    void add_edge(int u, int v, T w=1) override{
        edges.push_back(Edge(u, v, w));
        cached_source = 0;
        all_dist.clear();
        all_pai.clear();
    }

    bool should_run(int n, int m) const override{
        const __int128 vertices = (__int128)n + 1;
        return this->fits_memory(
            vertices * (sizeof(T) + sizeof(int)) +
            (__int128)m * sizeof(Edge)
        );
    }

    bool should_run_all_pairs(int n, int m) const{
        const __int128 vertices = (__int128)n + 1;
        return this->fits_memory(
            vertices * (sizeof(T) + sizeof(int)) +
            (__int128)m * sizeof(Edge) +
            vertices * vertices * (sizeof(T) + sizeof(int))
        );
    }

    void run(int s=1) override{
        reset();
        solve(s);
    }

    // Executa a partir de todas as origens acumulando o contador de iterações.
    void run_all_pairs(){
        reset();
        all_dist.assign(n+1, vector<T>());
        all_pai.assign(n+1, vector<int>());
        bool any_negative_cycle = false;
        for(int s=1; s<=n; s++){
            clear_state();
            solve(s);
            all_dist[s] = dist;
            all_pai[s] = pai;
            any_negative_cycle = any_negative_cycle || negative_cicle;
            if(this->limite_excedido) break;
        }
        negative_cicle = any_negative_cycle;
        cached_source = 0;
    }

    void solve(int s){
        dist[s] = 0;
        pai[s] = s;
        cached_source = s;

        for(int k=1; k<=n; k++){
            bool mudou=false;
            for(Edge &e: edges){
                if(!this->contar()) return;
                auto[u, v, w] = e;
                if(dist[u] == inf) continue;
                if(dist[v] <= dist[u] + w) continue;
                if(k==n) negative_cicle = true;
                dist[v] = dist[u] + w;
                pai[v] = u;
                mudou = true;
            }
            if(!mudou) break;
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
        if(cached_source != s) run(s);
        return dist[t];
    }

    // Requer run_all_pairs(); origens não processadas retornam inf.
    T get_all_dist(int s, int t) const{
        if(s < 1 || s > n || all_dist.empty() || all_dist[s].empty()){
            return inf;
        }
        return all_dist[s][t];
    }

    vector<int> get_all_path(int s, int t) const{
        if(s < 1 || s > n || all_dist.empty() || all_dist[s].empty()){
            return {};
        }
        return reconstruct_path(s, t, all_dist[s], all_pai[s], inf);
    }

    vector<int> get_path(int s, int t) override{
        get_dist(s, t);
        return reconstruct_path(s, t, dist, pai, inf);
    }

    vector<T> get_dists() const override {
        return dist;
    }

    vector<int> get_tree() const override {
        return pai;
    }

    bool has_negative_cycle() const override {return negative_cicle;}

};