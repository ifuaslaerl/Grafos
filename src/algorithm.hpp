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
};

template<typename T>
class ISingleSourceAlgorithm : public IAlgorithm<T>{
public:
    virtual void run(int source) = 0;
    virtual vector<T> get_dists() const = 0;
    virtual vector<int> get_tree() const = 0;
};

template<typename T>
class IAllPairsAlgorithm : public IAlgorithm<T>{
public:
    virtual void run() = 0;
    virtual T get_dist(int source, int target) const = 0;
    virtual vector<T> get_dists(int source) const = 0;
    virtual vector<int> get_tree(int source) const = 0;
};

class INegativeCycleDetector{
public:
    virtual ~INegativeCycleDetector() = default;
    virtual bool has_negative_cycle() const = 0;
};

#endif
