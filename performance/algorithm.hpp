#ifndef PERFORMANCE_ALGORITHM_HPP
#define PERFORMANCE_ALGORITHM_HPP

#include <vector>
using namespace std;

template<typename T>
class IAlgorithm{
public:
    IAlgorithm() = default;
    virtual ~IAlgorithm() = default;

    virtual void add_edge(int u, int v, T w=1) = 0;
    virtual void run(int source) = 0;
    virtual vector<T> get_dists() const = 0;
    virtual vector<int> get_tree() const = 0;
    virtual bool has_negative_cycle() const {return false;}
};

#endif
