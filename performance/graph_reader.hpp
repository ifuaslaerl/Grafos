#ifndef PERFORMANCE_GRAPH_READER_HPP
#define PERFORMANCE_GRAPH_READER_HPP

#include "algorithm.hpp"

#include <istream>
#include <stdexcept>

using namespace std;

namespace graph_io{

enum class EdgeWeightFormat{
    unweighted,
    weighted
};

enum class GraphDirection{
    directed,
    undirected
};

enum class EdgeWeightHandling{
    use_input,
    ignore_input
};

inline int read_vertex_count(istream& input){
    int vertex_count;
    if(!(input >> vertex_count) || vertex_count < 0){
        throw runtime_error("Numero de vertices invalido na entrada.");
    }
    return vertex_count;
}

template<typename T>
void read_edges(
    istream& input,
    IAlgorithm<T>& algorithm,
    int vertex_count,
    EdgeWeightFormat weight_format,
    GraphDirection direction,
    EdgeWeightHandling weight_handling=EdgeWeightHandling::use_input
){
    if(vertex_count < 0){
        throw invalid_argument("O numero de vertices nao pode ser negativo.");
    }

    int u;
    while(input >> u){
        int v;
        T weight{1};
        if(!(input >> v)){
            throw runtime_error("Aresta incompleta na entrada.");
        }
        if(weight_format == EdgeWeightFormat::weighted){
            if(weight_handling == EdgeWeightHandling::use_input){
                if(!(input >> weight)){
                    throw runtime_error("Peso invalido ou ausente na entrada.");
                }
            } else{
                long double ignored_weight;
                if(!(input >> ignored_weight)){
                    throw runtime_error("Peso invalido ou ausente na entrada.");
                }
            }
        }
        if(u < 1 || u > vertex_count || v < 1 || v > vertex_count){
            throw out_of_range("Endpoint fora do intervalo 1..n.");
        }

        algorithm.add_edge(u, v, weight);
        if(direction == GraphDirection::undirected){
            algorithm.add_edge(v, u, weight);
        }
    }

    if(input.bad() || !input.eof()){
        throw runtime_error("Formato invalido nas arestas de entrada.");
    }
}

}

#endif
