#ifndef PERFORMANCE_ALGORITHM_HPP
#define PERFORMANCE_ALGORITHM_HPP

#include <algorithm>
#include <vector>
using namespace std;
typedef long long ll;

template<typename T>
class IAlgorithm{
public:
    static constexpr int operation_budget = 15e8;

    ll cont = 0;
    bool limite_excedido = false;

    void zerar_contador(){
        cont = 0;
        limite_excedido = false;
    }

    // Conta uma iteração; retorna false quando o limite fixo é excedido.
    bool contar(){
        if(limite_excedido) return false;
        if(++cont > operation_budget) limite_excedido = true;
        return !limite_excedido;
    }

    IAlgorithm() = default;
    virtual ~IAlgorithm() = default;

    virtual void add_edge(int u, int v, T w=1) = 0;
    virtual bool should_run(int n, int m) const = 0;
};

template<typename T>
vector<int> reconstruct_path(
    int s,
    int t,
    const vector<T> &dist,
    const vector<int> &pai,
    T inf
){
    const int n = (int)dist.size();
    if((int)pai.size() != n || s <= 0 || t <= 0 ||
       s >= n || t >= n || dist[t] == inf){
        return {};
    }

    vector<int> path;
    int v = t;
    for(int i=0; i<n; i++){
        if(v <= 0 || v >= n){
            return {};
        }
        path.push_back(v);
        if(v == s){
            reverse(path.begin(), path.end());
            return path;
        }

        const int p = pai[v];
        if(p <= 0 || p == v){
            return {};
        }
        v = p;
    }
    return {};
}

template<typename T>
class ISingleSourceAlgorithm : public IAlgorithm<T>{
public:
    virtual void run(int s) = 0;
    virtual vector<T> get_dists() const = 0;
    virtual vector<int> get_tree() const = 0;
    virtual vector<int> get_path(int s, int t) = 0;
};

template<typename T>
class IAllPairsAlgorithm : public IAlgorithm<T>{
public:
    virtual void run() = 0;
    virtual T get_dist(int s, int t) const = 0;
    virtual vector<T> get_dists(int s) const = 0;
    virtual vector<int> get_tree(int s) const = 0;
    virtual vector<int> get_path(int s, int t) const = 0;
};

class INegativeCycleDetector{
public:
    virtual ~INegativeCycleDetector() = default;
    virtual bool has_negative_cycle() const = 0;
};

#endif
