#ifndef OPTIMIZE_PARALLEL_BELLMAN_FORD_CPP
#define OPTIMIZE_PARALLEL_BELLMAN_FORD_CPP

#include "../algorithm.hpp"
#include "numeric.hpp"

#include <algorithm>
#include <cstddef>
#include <utility>
#include <vector>

#ifdef _OPENMP
#include <omp.h>
#endif

using namespace std;

// Bellman-Ford com as arestas de cada rodada processadas em paralelo:
//
//     for(k = 1..n){ processar todas as arestas em paralelo }
//
// Para não haver corrida entre threads, as arestas ficam agrupadas por
// destino (CSR de arestas de entrada) e cada thread fica com um bloco de
// vértices v, relaxando todas as arestas u->v que chegam nele. Assim só uma
// thread escreve em dist[v]. A rodada lê dist (rodada anterior) e escreve em
// next_dist (Jacobi), então nenhuma thread lê um valor que outra esteja
// escrevendo. O resultado é determinístico e igual ao da versão sequencial.
//
// Com relaxação de Jacobi, depois da rodada k todo caminho mínimo com até k
// arestas está correto, então n-1 rodadas bastam e uma mudança na rodada n
// indica ciclo negativo, exatamente como no Bellman-Ford original.
template<typename T>
class ParallelBellmanFord:
    public ISingleSourceAlgorithm<T>,
    public INegativeCycleDetector{
public:
    int n;
    bool negative_cicle = false;
    const T inf = optimize::NumTraits<T>::inf();

    // Arestas recebidas por add_edge (lista crua, até build()).
    vector<int> raw_u, raw_v;
    vector<T> raw_w;

    // CSR por destino: arestas que chegam em v estão em [start[v], start[v+1]).
    vector<size_t> start;
    vector<int> from;
    vector<T> weight;
    bool built = false;

    vector<T> dist, next_dist;
    vector<int> pai, next_pai;
    int cached_source = 0;
    int rounds = 0; // rodadas executadas na última chamada de run()
    // false desliga o limite de iterações herdado de IAlgorithm (benchmark).
    bool enforce_budget = true;

    ParallelBellmanFord(int n_): n(n_){}

    void add_edge(int u, int v, T w=1) override{
        raw_u.push_back(u);
        raw_v.push_back(v);
        raw_w.push_back(w);
        built = false;
        cached_source = 0;
    }

    size_t arc_count() const{
        return built ? from.size() : raw_u.size();
    }

    bool should_run(int n_, int m) const override{
        const __int128 vertices = (__int128)n_ + 2;
        return this->fits_memory(
            vertices * (2 * sizeof(T) + 2 * sizeof(int) + sizeof(size_t)) +
            (__int128)m * (sizeof(int) + sizeof(T))
        );
    }

    // Monta o CSR por destino (counting sort, O(n + m)).
    void build(){
        if(built) return;
        const size_t m = raw_u.size();
        start.assign((size_t)n + 2, 0);
        for(size_t i=0; i<m; i++) start[raw_v[i] + 1]++;
        for(int v=1; v<=n+1; v++) start[v] += start[v-1];

        from.resize(m);
        weight.resize(m);
        vector<size_t> pos(start.begin(), start.end() - 1);
        for(size_t i=0; i<m; i++){
            const size_t p = pos[raw_v[i]]++;
            from[p] = raw_u[i];
            weight[p] = raw_w[i];
        }

        vector<int>().swap(raw_u);
        vector<int>().swap(raw_v);
        vector<T>().swap(raw_w);
        built = true;
    }

    void reset(){
        dist.assign((size_t)n + 1, inf);
        next_dist.assign((size_t)n + 1, inf);
        pai.assign((size_t)n + 1, 0);
        next_pai.assign((size_t)n + 1, 0);
        negative_cicle = false;
        cached_source = 0;
        rounds = 0;
        this->zerar_contador();
    }

    void run(int s=1) override{
        build();
        reset();
        solve(s);
    }

    void solve(int s){
        dist[s] = 0;
        pai[s] = s;
        next_dist[s] = 0;
        next_pai[s] = s;
        cached_source = s;
        const long long m = (long long)from.size();

        for(int k=1; k<=n; k++){
            // Mesmo limite de iterações do original, contado por rodada.
            this->cont += m;
            if(enforce_budget && this->cont > this->operation_budget){
                this->limite_excedido = true;
                return;
            }
            rounds = k;

            bool mudou = false;
            const T* d = dist.data();
            T* nd = next_dist.data();
            int* np = next_pai.data();
            const int* fr = from.data();
            const T* wt = weight.data();
            const size_t* st = start.data();

            #pragma omp parallel for schedule(dynamic, 2048) reduction(||:mudou)
            for(int v=1; v<=n; v++){
                T best = d[v];
                int best_pai = pai[v];
                for(size_t e=st[v]; e<st[v+1]; e++){
                    const T cand = optimize::add_dist(d[fr[e]], wt[e], inf);
                    if(cand < best){
                        best = cand;
                        best_pai = fr[e];
                    }
                }
                nd[v] = best;
                np[v] = best_pai;
                if(best < d[v]) mudou = true;
            }

            dist.swap(next_dist);
            pai.swap(next_pai);
            if(!mudou) break;
            if(k == n) negative_cicle = true;
        }
    }

    T get_dist(int s, int t){
        if(cached_source != s) run(s);
        return dist[t];
    }

    vector<int> get_path(int s, int t) override{
        get_dist(s, t);
        return reconstruct_path(s, t, dist, pai, inf);
    }

    vector<T> get_dists() const override{ return dist; }
    vector<int> get_tree() const override{ return pai; }
    bool has_negative_cycle() const override{ return negative_cicle; }

    // Bytes ocupados pelas estruturas (após build()).
    size_t memory_bytes() const{
        return start.capacity() * sizeof(size_t) +
            from.capacity() * sizeof(int) +
            weight.capacity() * sizeof(T) +
            (dist.capacity() + next_dist.capacity()) * sizeof(T) +
            (pai.capacity() + next_pai.capacity()) * sizeof(int) +
            raw_u.capacity() * sizeof(int) +
            raw_v.capacity() * sizeof(int) +
            raw_w.capacity() * sizeof(T);
    }
};

#endif
