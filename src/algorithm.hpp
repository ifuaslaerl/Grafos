#ifndef PERFORMANCE_ALGORITHM_HPP
#define PERFORMANCE_ALGORITHM_HPP

#include <cstddef>
#include <vector>
using namespace std;

template<typename T>
class IAlgorithm{
public:
    static constexpr int operation_budget = 2e9;

    IAlgorithm() = default;
    virtual ~IAlgorithm() = default;

    virtual void add_edge(int u, int v, T w=1) = 0;
    virtual bool should_run(int n, int m) const = 0;
    virtual void run(int s=1) = 0;
    virtual vector<T> get_dists(int s=1) const = 0;
    virtual vector<int> get_tree(int s=1) const = 0;
    virtual bool has_negative_cycle() const {return false;}
};

#endif
