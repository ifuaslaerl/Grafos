#ifndef PERFORMANCE_GRAPH_READER_HPP
#define PERFORMANCE_GRAPH_READER_HPP

#include "algorithm.hpp"

#include <istream>
#include <limits>
#include <sstream>
#include <stdexcept>
#include <string>

using namespace std;

namespace graph_io{

struct Header{
    int vertex_count;
    size_t edge_count;
};

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

inline Header read_header(istream& input){
    string line;
    if(!getline(input, line)){
        throw runtime_error("O cabecalho deve conter N e M validos.");
    }

    istringstream fields(line);
    long long vertex_count;
    long long edge_count;
    if(!(fields >> vertex_count >> edge_count) ||
       vertex_count < 0 ||
       vertex_count > numeric_limits<int>::max() ||
       edge_count < 0){
        throw runtime_error("O cabecalho deve conter N e M validos.");
    }

    string extra;
    if(fields >> extra){
        throw runtime_error("O cabecalho deve conter N e M validos.");
    }

    Header header{
        static_cast<int>(vertex_count),
        static_cast<size_t>(edge_count)
    };
    return header;
}

template<typename T>
void read_edges(
    istream& input,
    IAlgorithm<T>& algorithm,
    int vertex_count,
    EdgeWeightFormat weight_format,
    GraphDirection direction,
    size_t expected_edge_count,
    EdgeWeightHandling weight_handling=EdgeWeightHandling::use_input
){
    if(vertex_count < 0){
        throw invalid_argument("O numero de vertices nao pode ser negativo.");
    }

    int u;
    size_t edges_read = 0;
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
            throw out_of_range(
                "Aresta " + to_string(edges_read + 1) + " (" +
                to_string(u) + ", " + to_string(v) +
                ") fora do intervalo inclusivo [1, " +
                to_string(vertex_count) + "]."
            );
        }

        algorithm.add_edge(u, v, weight);
        if(direction == GraphDirection::undirected){
            algorithm.add_edge(v, u, weight);
        }
        ++edges_read;
    }

    if(input.bad() || !input.eof()){
        throw runtime_error("Formato invalido nas arestas de entrada.");
    }
    if(edges_read != expected_edge_count){
        throw runtime_error("M no cabecalho nao coincide com as arestas lidas.");
    }
}

}

#endif
