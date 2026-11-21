#include "bellman_ford.cpp"
#include "bfs.cpp"
#include "dfs.cpp"
#include "djikstra.cpp"
#include "floyd_warshall.cpp"
#include "graph_reader.hpp"
#include "matrix_bfs.cpp"
#include "matrix_dfs.cpp"
#include "run_measurement.hpp"
#include "vector_djikstra.cpp"

#include <algorithm>
#include <array>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>
#include <vector>

using namespace std;

namespace{

constexpr int repetitions = 10;

struct GraphInput{
    filesystem::path path;
    int vertex_count;
    size_t input_edges;
    graph_io::EdgeWeightFormat weight_format;
};

string csv_quote(const string& value){
    string quoted = "\"";
    for(char character : value){
        if(character == '"'){
            quoted += "\"\"";
        } else{
            quoted += character;
        }
    }
    quoted += '"';
    return quoted;
}

bool is_graph_input(const filesystem::path& path){
    const string name = path.filename().string();
    return name == "rede_colaboracao.txt" ||
        (name.rfind("grafo_", 0) == 0 && path.extension() == ".txt");
}

GraphInput inspect_graph(const filesystem::path& path){
    ifstream input(path);
    if(!input){
        throw runtime_error("Nao foi possivel abrir " + path.string() + ".");
    }

    const graph_io::Header header = graph_io::read_header(input);

    const string name = path.filename().string();
    const bool weighted = name.rfind("grafo_W_", 0) == 0 ||
        name == "rede_colaboracao.txt";
    return {
        path,
        header.vertex_count,
        header.edge_count,
        weighted
            ? graph_io::EdgeWeightFormat::weighted
            : graph_io::EdgeWeightFormat::unweighted
    };
}

vector<GraphInput> list_graphs(const filesystem::path& directory){
    if(!filesystem::is_directory(directory)){
        throw runtime_error("Diretorio de entrada inexistente: " +
                            directory.string());
    }

    vector<GraphInput> graphs;
    for(const auto& entry : filesystem::directory_iterator(directory)){
        if(entry.is_regular_file() && is_graph_input(entry.path())){
            cerr << "[benchmark] Inspecionando " << entry.path() << "...\n";
            graphs.push_back(inspect_graph(entry.path()));
        }
    }
    sort(graphs.begin(), graphs.end(), [](const auto& lhs, const auto& rhs){
        return lhs.path.filename() < rhs.path.filename();
    });
    if(graphs.empty()){
        throw runtime_error("Nenhum arquivo de arestas encontrado em " +
                            directory.string());
    }
    return graphs;
}

string direction_name(graph_io::GraphDirection direction){
    return direction == graph_io::GraphDirection::directed
        ? "directed"
        : "undirected";
}

void print_skipped(
    ostream& output,
    const string& algorithm,
    const GraphInput& graph,
    graph_io::GraphDirection direction,
    size_t arc_count,
    const string& reason
){
    output << algorithm << ','
           << csv_quote(graph.path.filename().string()) << ','
           << direction_name(direction) << ",0,"
           << graph.vertex_count << ',' << graph.input_edges << ','
           << arc_count << ",,,skipped_" << reason << '\n';
}

template<typename Algorithm, typename T>
void benchmark_algorithm(
    ostream& output,
    const string& name,
    const GraphInput& graph,
    size_t graph_index,
    size_t graph_count,
    bool ignore_input_weights
){
    const Algorithm eligibility_probe(0);
    const array<graph_io::GraphDirection, 2> directions{{
        graph_io::GraphDirection::directed,
        graph_io::GraphDirection::undirected
    }};

    cerr << "[benchmark] Iniciando " << name << "...\n";
    for(graph_io::GraphDirection direction : directions){
        const size_t multiplier =
            direction == graph_io::GraphDirection::undirected ? 2 : 1;
        if(graph.input_edges > numeric_limits<size_t>::max() / multiplier){
            throw overflow_error("Contagem de arcos excede size_t.");
        }
        const size_t arc_count = graph.input_edges * multiplier;
        const int estimated_arc_count = arc_count >
                static_cast<size_t>(IAlgorithm<T>::operation_budget)
            ? IAlgorithm<T>::operation_budget + 1
            : static_cast<int>(arc_count);
        if(!eligibility_probe.should_run(
               graph.vertex_count, estimated_arc_count
           )){
            const string reason = "cost_estimate";
            cerr << "[benchmark] Ignorando " << name << " | "
                 << graph.path.filename() << " | "
                 << direction_name(direction) << " (n="
                 << graph.vertex_count << ", arcos=" << arc_count
                 << "): estimativa de custo excede "
                 << IAlgorithm<T>::operation_budget << ".\n";
            print_skipped(
                output, name, graph, direction, arc_count, reason
            );
            continue;
        }

        for(int repetition = 1; repetition <= repetitions; ++repetition){
            cerr << "[benchmark] " << name << " | grafo "
                 << graph_index + 1 << '/' << graph_count << " "
                 << graph.path.filename() << " | "
                 << direction_name(direction) << " | repeticao "
                 << repetition << '/' << repetitions
                 << ": preparando grafo.\n";
            Algorithm algorithm(graph.vertex_count);
            ifstream input(graph.path);
            if(!input){
                throw runtime_error("Nao foi possivel abrir " +
                                    graph.path.string() + ".");
            }
            const graph_io::Header header =
                graph_io::read_header(input);
            if(header.vertex_count != graph.vertex_count ||
               header.edge_count != graph.input_edges){
                throw runtime_error("O numero de vertices mudou em " +
                                    graph.path.string() + ".");
            }

            const auto weight_handling = ignore_input_weights
                ? graph_io::EdgeWeightHandling::ignore_input
                : graph_io::EdgeWeightHandling::use_input;
            graph_io::read_edges<T>(
                input,
                algorithm,
                graph.vertex_count,
                graph.weight_format,
                direction,
                graph.input_edges,
                weight_handling
            );

            cerr << "[benchmark] " << name << " | "
                 << graph.path.filename() << " | "
                 << direction_name(direction) << " | repeticao "
                 << repetition << '/' << repetitions
                 << ": executando algoritmo.\n";
            const performance::RunMeasurement measurement =
                performance::measure_run([&algorithm]{
                    algorithm.run();
                });
            output << name << ','
                   << csv_quote(graph.path.filename().string()) << ','
                   << direction_name(direction) << ','
                   << repetition << ',' << graph.vertex_count << ','
                   << graph.input_edges << ',' << arc_count << ','
                   << measurement.elapsed_nanoseconds << ','
                   << measurement.peak_rss_delta_bytes << ",ok\n";
            cerr << "[benchmark] " << name << " | "
                 << graph.path.filename() << " | "
                 << direction_name(direction) << " | repeticao "
                 << repetition << '/' << repetitions
                 << " concluida: " << measurement.elapsed_nanoseconds
                 << " ns, pico RSS adicional "
                 << measurement.peak_rss_delta_bytes << " bytes.\n";
        }
    }
    cerr << "[benchmark] " << name << " concluido.\n";
}

}

int main(int argc, char* argv[]){
    if(argc > 3){
        cerr << "Uso: " << argv[0]
             << " [diretorio_de_entrada] [arquivo_csv]\n";
        return 2;
    }

    try{
        const filesystem::path input_directory =
            argc >= 2 ? argv[1] : "Dados/Entrada";
        const filesystem::path output_path =
            argc == 3 ? argv[2] : "Dados/Performance/benchmark.csv";
        cerr << "[benchmark] Listando grafos em " << input_directory << "...\n";
        const vector<GraphInput> graphs = list_graphs(input_directory);
        cerr << "[benchmark] Encontrados " << graphs.size()
             << " grafos. CSV de saida: " << output_path << '\n';
        if(!output_path.parent_path().empty()){
            filesystem::create_directories(output_path.parent_path());
        }
        ofstream output(output_path);
        if(!output){
            throw runtime_error("Nao foi possivel criar " +
                                output_path.string() + ".");
        }
        cerr << "[benchmark] Cabecalho CSV gravado; iniciando medicoes.\n";
        output << "algorithm,graph,direction,repetition,n,m_input,m_arcs,"
                  "run_ns,peak_rss_delta_bytes,status\n";

        for(size_t graph_index = 0; graph_index < graphs.size(); ++graph_index){
            const GraphInput& graph = graphs[graph_index];
            cerr << "[benchmark] Iniciando grafo " << graph_index + 1 << '/'
                 << graphs.size() << ' ' << graph.path.filename() << ".\n";
            benchmark_algorithm<Bfs<int>, int>(
                output, "Bfs<int>", graph, graph_index, graphs.size(), true
            );
            benchmark_algorithm<Dfs<int>, int>(
                output, "Dfs", graph, graph_index, graphs.size(), true
            );
            benchmark_algorithm<Djikstra<double>, double>(
                output, "Djikstra<double>", graph, graph_index,
                graphs.size(), false
            );
            benchmark_algorithm<MatrixBfs<int>, int>(
                output, "MatrixBfs<int>", graph, graph_index,
                graphs.size(), true
            );
            benchmark_algorithm<MatrixDfs<int>, int>(
                output, "MatrixDfs<int>", graph, graph_index,
                graphs.size(), true
            );
            benchmark_algorithm<VectorDjikstra<double>, double>(
                output, "VectorDjikstra<double>", graph, graph_index,
                graphs.size(), false
            );
            benchmark_algorithm<BellmanFord<double>, double>(
                output, "BellmanFord<double>", graph, graph_index,
                graphs.size(), false
            );
            benchmark_algorithm<FloydWarshall<double>, double>(
                output, "FloydWarshall<double>", graph, graph_index,
                graphs.size(), false
            );
            cerr << "[benchmark] Grafo " << graph.path.filename()
                 << " concluido.\n";
        }
        output.flush();
        if(!output){
            throw runtime_error("Falha ao gravar " + output_path.string() + ".");
        }
        cerr << "Resultados salvos em " << output_path << '\n';
    } catch(const exception& error){
        cerr << "Erro no benchmark: " << error.what() << '\n';
        return 1;
    }
}
