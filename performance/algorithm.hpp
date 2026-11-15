#ifndef PERFORMANCE_ALGORITHM_HPP
#define PERFORMANCE_ALGORITHM_HPP

#include <limits>
#include <stdexcept>
#include <type_traits>
#include <vector>

template<typename T>
struct AlgorithmArc{
    int from;
    int to;
    T weight;
};

template<typename T>
class IAlgorithm{
public:
    explicit IAlgorithm(int vertex_count): vertex_count_(vertex_count){
        if(vertex_count < 0){
            throw std::invalid_argument("O numero de vertices nao pode ser negativo.");
        }
    }

    virtual ~IAlgorithm() = default;

    int vertex_count() const{
        return vertex_count_;
    }

    void add_arc(int from, int to, T weight = T{1}){
        validate_vertex(from);
        validate_vertex(to);
        arcs_.push_back({from, to, weight});
    }

    void add_edge(int u, int v, T weight = T{1}){
        add_arc(u, v, weight);
        add_arc(v, u, weight);
    }

    virtual void run(int source) = 0;
    virtual T distance_to(int vertex) const = 0;
    virtual int parent_of(int vertex) const = 0;
    virtual bool has_negative_cycle() const{
        return false;
    }

protected:
    void validate_vertex(int vertex) const{
        if(vertex < 1 || vertex > vertex_count_){
            throw std::out_of_range("Vertice fora do intervalo 1..n.");
        }
    }

    const std::vector<AlgorithmArc<T>>& arcs() const{
        return arcs_;
    }

    static T infinity(){
        if constexpr(std::numeric_limits<T>::has_infinity){
            return std::numeric_limits<T>::infinity();
        }
        return std::numeric_limits<T>::max();
    }

    static bool add_distances(T lhs, T rhs, T& result){
        if constexpr(std::numeric_limits<T>::is_integer &&
                     std::numeric_limits<T>::is_signed){
            if(rhs > T{} &&
               lhs > std::numeric_limits<T>::max() - rhs){
                return false;
            }
            if(rhs < T{} &&
               lhs < std::numeric_limits<T>::lowest() - rhs){
                return false;
            }
        } else if constexpr(std::numeric_limits<T>::is_integer){
            if(lhs > std::numeric_limits<T>::max() - rhs){
                return false;
            }
        }
        result = lhs + rhs;
        return true;
    }

    int vertex_count_;

private:
    std::vector<AlgorithmArc<T>> arcs_;
};

#endif
