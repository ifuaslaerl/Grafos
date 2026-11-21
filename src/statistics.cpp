#include "algorithm.hpp"

#include <algorithm>
#include <numeric>
#include <unordered_map>
#include <vector>

using namespace std;
using ld = long double;

template<typename T>
class Statistics {
private:
    template<typename Val>
    void calc(vector<Val>& vals, Val& vmin, Val& vmax, ld& vmean, ld& vmedian) {
        if (n <= 0) return;

        int active = vals.size();
        int zeros = max(0, n - active);

        // Soma e extremos dos vértices ativos
        Val active_min = vals.empty() ? Val{} : *min_element(vals.begin(), vals.end());
        Val active_max = vals.empty() ? Val{} : *max_element(vals.begin(), vals.end());
        ld sum = accumulate(vals.begin(), vals.end(), 0.0L);

        vmin = (zeros > 0) ? min(Val{}, active_min) : active_min;
        vmax = (zeros > 0) ? max(Val{}, active_max) : active_max;
        vmean = sum / n;

        // Mediana em O(active) usando nth_element (considerando os 'zeros' implícitos)
        auto get_kth = [&](int k) -> Val {
            if (k < zeros) return Val{};
            int idx = k - zeros;
            nth_element(vals.begin(), vals.begin() + idx, vals.end());
            return vals[idx];
        };

        if (n % 2 == 1) {
            vmedian = get_kth(n / 2);
        } else {
            Val m1 = get_kth(n / 2 - 1);
            Val m2 = get_kth(n / 2);
            vmedian = (static_cast<ld>(m1) + static_cast<ld>(m2)) / 2.0L;
        }
    }

public:
    int n, gmin = 0, gmax = 0;
    T wmin = T{}, wmax = T{};
    ld gmean = 0, gmedian = 0, wmean = 0, wmedian = 0;
    unordered_map<int, int> deg;
    unordered_map<int, T> degW;

    Statistics(int n_) : n(n_) {}

    void add_edge(int u, int v, T w = 1) {
        deg[u]++; deg[v]++;
        degW[u] += w; degW[v] += w;
    }

    void run() {
        vector<int> g;
        g.reserve(deg.size());
        for (auto& [u, d] : deg) g.push_back(d);

        vector<T> w;
        w.reserve(degW.size());
        for (auto& [u, d] : degW) w.push_back(d);

        calc(g, gmin, gmax, gmean, gmedian);
        calc(w, wmin, wmax, wmean, wmedian);
    }
};