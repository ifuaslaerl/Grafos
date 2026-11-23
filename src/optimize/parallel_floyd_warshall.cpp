#ifndef OPTIMIZE_PARALLEL_FLOYD_WARSHALL_CPP
#define OPTIMIZE_PARALLEL_FLOYD_WARSHALL_CPP

#include "../algorithm.hpp"
#include "numeric.hpp"

#include <algorithm>
#include <cstddef>
#include <vector>

#ifdef _OPENMP
#include <omp.h>
#endif

using namespace std;

// Floyd-Warshall com os pares (i, j) de cada k processados em paralelo:
//
//     for(k = 1..n){ processar pares i, j em paralelo }
//
// Cada thread fica com um bloco de linhas i. Na iteração k a linha k é
// copiada antes do laço paralelo; assim cada thread só lê a própria linha i e
// a cópia da linha k, e só escreve na própria linha i -- não há corrida.
// (Sem ciclo negativo a linha k não muda durante a iteração k, então a cópia
// dá o mesmo resultado do algoritmo sequencial.)
//
// A matriz é guardada num único vetor contíguo (linha a linha), o que deixa o
// laço em j sequencial na memória e vetorizável.
template<typename T>
class ParallelFloydWarshall:
    public IAllPairsAlgorithm<T>,
    public INegativeCycleDetector{
public:
    int n;
    size_t stride;
    bool negative_cicle = false;
    const T inf = optimize::NumTraits<T>::inf();
    vector<T> dist;
    vector<int> pai;
    T diameter = 0;
    // false desliga o limite de iterações herdado de IAlgorithm (benchmark).
    bool enforce_budget = true;

    ParallelFloydWarshall(int n_):
        n(n_),
        stride((size_t)n_ + 1),
        dist(stride * stride, optimize::NumTraits<T>::inf()),
        pai(stride * stride, 0)
    {
        for(int i=1; i<=n; i++){
            at(i, i) = 0;
            pai[idx(i, i)] = i;
        }
    }

    size_t idx(int i, int j) const{ return (size_t)i * stride + j; }
    T& at(int i, int j){ return dist[idx(i, j)]; }
    const T& at(int i, int j) const{ return dist[idx(i, j)]; }

    void add_edge(int u, int v, T w=1) override{
        if(w < at(u, v)){
            at(u, v) = w;
            pai[idx(u, v)] = u;
        }
    }

    bool should_run(int n_, int) const override{
        const __int128 vertices = (__int128)n_ + 1;
        return this->fits_memory(
            vertices * vertices * (sizeof(T) + sizeof(int)) +
            vertices * (sizeof(T) + sizeof(int))
        );
    }

    void run() override{
        this->zerar_contador();
        const long long per_k = (long long)n * n;
        vector<T> row_k(stride);
        vector<int> pai_k(stride);

        for(int k=1; k<=n; k++){
            // Mesmo limite de iterações do original, contado por k.
            this->cont += per_k;
            if(enforce_budget && this->cont > this->operation_budget){
                this->limite_excedido = true;
                return;
            }

            copy(dist.begin() + idx(k, 0), dist.begin() + idx(k, 0) + stride,
                 row_k.begin());
            copy(pai.begin() + idx(k, 0), pai.begin() + idx(k, 0) + stride,
                 pai_k.begin());
            const T* rk = row_k.data();
            const int* pk = pai_k.data();

            // dynamic: a CPU pode ter núcleos de velocidades diferentes.
            #pragma omp parallel for schedule(dynamic, 8)
            for(int i=1; i<=n; i++){
                T* di = dist.data() + idx(i, 0);
                const T dik = di[k];
                if(!optimize::NumTraits<T>::branchless && dik == inf) continue;
                int* pi = pai.data() + idx(i, 0);
                #pragma omp simd
                for(int j=1; j<=n; j++){
                    const T cand = optimize::add_dist(dik, rk[j], inf);
                    // Escrita só quando melhora: menos tráfego de memória
                    // (vira store mascarado com AVX2/AVX-512).
                    if(cand < di[j]){
                        di[j] = cand;
                        pi[j] = pk[j];
                    }
                }
            }
        }

        // Diâmetro: maior distância finita entre pares alcançáveis.
        double best = 0;
        bool neg = false;
        #pragma omp parallel for schedule(static) reduction(max:best) reduction(||:neg)
        for(int i=1; i<=n; i++){
            const T* di = dist.data() + idx(i, 0);
            if(di[i] < 0) neg = true;
            for(int j=1; j<=n; j++){
                if(di[j] != inf && (double)di[j] > best) best = (double)di[j];
            }
        }
        diameter = (T)best;
        negative_cicle = neg;
    }

    T get_aprox_diameter(int =1){ return diameter; }
    T get_dist(int s, int t) const override{ return at(s, t); }
    vector<T> get_dists(int s) const override{
        return vector<T>(dist.begin() + idx(s, 0),
                         dist.begin() + idx(s, 0) + stride);
    }
    vector<int> get_tree(int s) const override{
        return vector<int>(pai.begin() + idx(s, 0),
                           pai.begin() + idx(s, 0) + stride);
    }
    vector<int> get_path(int s, int t) const override{
        return reconstruct_path(s, t, get_dists(s), get_tree(s), inf);
    }
    bool has_negative_cycle() const override{ return negative_cicle; }

    size_t memory_bytes() const{
        return dist.capacity() * sizeof(T) + pai.capacity() * sizeof(int) +
            stride * (sizeof(T) + sizeof(int));
    }
};

#endif
