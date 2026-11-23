// Benchmark das versões otimizadas (paralelas e com tipos menores) contra
// as implementações originais de src/.
//
// Uso:
//   ./bench_optimize fw <tempos.csv> <erros.csv> [n1 n2 ...]
//   ./bench_optimize bf <tempos.csv> <erros.csv> <grafo> [grafo ...]
//   ./bench_optimize check
//
// <grafo> é um arquivo (primeira linha "n" ou "n m", depois "u v [peso]",
// tratado como não direcionado) ou "random:n:m:semente" para um grafo
// aleatório conexo com pesos em [0.01, 1.00] com duas casas decimais.

#include "../bellman_ford.cpp"
#include "../floyd_warshall.cpp"
#include "numeric.hpp"
#include "parallel_bellman_ford.cpp"
#include "parallel_floyd_warshall.cpp"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <functional>
#include <iostream>
#include <random>
#include <stdexcept>
#include <string>
#include <vector>

#ifdef _OPENMP
#include <omp.h>
#endif

using namespace std;
using optimize::NumTraits;

namespace{

constexpr int repetitions = 3;

struct Graph{
    string name;
    int n = 0;
    vector<int> u, v;
    vector<double> w;
};

int max_threads(){
#ifdef _OPENMP
    return omp_get_max_threads();
#else
    return 1;
#endif
}

void set_threads(int t){
#ifdef _OPENMP
    omp_set_num_threads(t);
#else
    (void)t;
#endif
}

vector<int> thread_list(){
    const int hw = max_threads();
    vector<int> list;
    for(int t=1; t<hw; t*=2) list.push_back(t);
    list.push_back(hw);
    return list;
}

double now_ms(){
    using namespace chrono;
    return duration<double, milli>(
        steady_clock::now().time_since_epoch()
    ).count();
}

// Leitura rápida: arquivo inteiro na memória e parse manual.
Graph load_file(const string& path){
    FILE* f = fopen(path.c_str(), "rb");
    if(!f) throw runtime_error("Nao foi possivel abrir " + path);
    fseek(f, 0, SEEK_END);
    const long size = ftell(f);
    fseek(f, 0, SEEK_SET);
    string buf((size_t)size, '\0');
    if(fread(buf.data(), 1, (size_t)size, f) != (size_t)size){
        fclose(f);
        throw runtime_error("Falha ao ler " + path);
    }
    fclose(f);

    Graph g;
    g.name = path.substr(path.find_last_of("/\\") + 1);
    size_t p = 0;
    const size_t len = buf.size();

    // Lê os números de uma linha.
    auto read_line = [&](vector<double>& out){
        out.clear();
        while(p < len && buf[p] != '\n'){
            while(p < len && (buf[p] == ' ' || buf[p] == '\t' ||
                              buf[p] == '\r')) p++;
            if(p >= len || buf[p] == '\n') break;
            char* end = nullptr;
            const double x = strtod(buf.c_str() + p, &end);
            const size_t q = (size_t)(end - buf.c_str());
            if(q == p) throw runtime_error("Token invalido em " + path);
            out.push_back(x);
            p = q;
        }
        if(p < len) p++;
    };

    vector<double> fields;
    read_line(fields);
    if(fields.empty()) throw runtime_error("Cabecalho vazio em " + path);
    g.n = (int)fields[0];
    while(p < len){
        read_line(fields);
        if(fields.empty()) continue;
        if(fields.size() < 2) throw runtime_error("Aresta invalida em " + path);
        g.u.push_back((int)fields[0]);
        g.v.push_back((int)fields[1]);
        g.w.push_back(fields.size() >= 3 ? fields[2] : 1.0);
    }
    return g;
}

// Grafo aleatório conexo: árvore aleatória + arestas extras.
Graph random_graph(int n, long long m, unsigned seed){
    mt19937 rng(seed);
    uniform_int_distribution<int> weight(1, 100);
    Graph g;
    g.name = "random_n" + to_string(n) + "_m" + to_string(m);
    g.n = n;
    auto push = [&](int a, int b){
        g.u.push_back(a);
        g.v.push_back(b);
        g.w.push_back(weight(rng) / 100.0);
    };
    for(int i=2; i<=n; i++){
        uniform_int_distribution<int> parent(1, i - 1);
        push(parent(rng), i);
    }
    uniform_int_distribution<int> any(1, n);
    for(long long e=n-1; e<m; e++){
        int a = any(rng), b = any(rng);
        while(b == a) b = any(rng);
        push(a, b);
    }
    return g;
}

Graph load_graph(const string& spec){
    if(spec.rfind("random:", 0) == 0){
        int n; long long m; unsigned seed;
        if(sscanf(spec.c_str(), "random:%d:%lld:%u", &n, &m, &seed) != 3){
            throw runtime_error("Use random:n:m:semente");
        }
        return random_graph(n, m, seed);
    }
    return load_file(spec);
}

template<typename Algo, typename T>
void add_undirected(Algo& algo, const Graph& g){
    for(size_t i=0; i<g.u.size(); i++){
        algo.add_edge(g.u[i], g.v[i], (T)g.w[i]);
        algo.add_edge(g.v[i], g.u[i], (T)g.w[i]);
    }
}

double median(vector<double> xs){
    sort(xs.begin(), xs.end());
    return xs[xs.size() / 2];
}

// Compara um vetor de distâncias com a referência em double.
struct ErrorStats{
    double max_abs = 0;
    double max_rel = 0;
    long long rel_over_1e3 = 0;
    long long compared = 0;
    long long reach_mismatch = 0;
};

template<typename A, typename B>
void accumulate_error(ErrorStats& s, A ref, A ref_inf, B got, B got_inf){
    const bool ref_ok = ref != ref_inf;
    const bool got_ok = got != got_inf && isfinite((double)got);
    if(ref_ok != got_ok){
        s.reach_mismatch++;
        return;
    }
    if(!ref_ok) return;
    const double r = (double)ref, x = (double)got;
    const double abs_err = fabs(r - x);
    const double rel_err = r != 0 ? abs_err / fabs(r) : abs_err;
    s.max_abs = max(s.max_abs, abs_err);
    s.max_rel = max(s.max_rel, rel_err);
    if(rel_err > 1e-3) s.rel_over_1e3++;
    s.compared++;
}

void write_error(ostream& out, const string& algo, const Graph& g,
                 const string& type, const ErrorStats& s){
    out << algo << ',' << g.name << ',' << g.n << ',' << g.u.size() * 2
        << ',' << type << ',' << s.max_abs << ',' << s.max_rel << ','
        << s.rel_over_1e3 << ',' << s.compared << ',' << s.reach_mismatch
        << '\n';
}

// ----------------------------------------------------------------------------
// Floyd-Warshall

template<typename T>
size_t original_fw_bytes(const FloydWarshall<T>& fw){
    size_t bytes = 0;
    for(const auto& row : fw.dist) bytes += row.capacity() * sizeof(T);
    for(const auto& row : fw.pai) bytes += row.capacity() * sizeof(int);
    return bytes;
}

void fw_row(ostream& out, const string& algo, const string& type, int threads,
            const Graph& g, int rep, double ms, size_t bytes, bool limited){
    out << algo << ',' << type << ',' << threads << ',' << g.n << ','
        << g.u.size() * 2 << ',' << rep << ',' << ms << ',' << bytes << ','
        << (limited ? "iteration_limit" : "ok") << '\n';
    out.flush();
}

template<typename T>
void bench_parallel_fw(ostream& out, const Graph& g, int threads,
                       const vector<double>& ref, ostream& err,
                       bool record_error){
    set_threads(threads);
    const string type = NumTraits<T>::name();
    vector<double> times;
    for(int rep=1; rep<=repetitions; rep++){
        ParallelFloydWarshall<T> fw(g.n);
        fw.enforce_budget = false;
        add_undirected<ParallelFloydWarshall<T>, T>(fw, g);
        const double t0 = now_ms();
        fw.run();
        const double ms = now_ms() - t0;
        times.push_back(ms);
        fw_row(out, "FloydWarshall_paralelo", type, threads, g, rep, ms,
               fw.memory_bytes(), fw.limite_excedido);

        if(rep == 1 && record_error){
            ErrorStats s;
            const double ref_inf = numeric_limits<double>::infinity();
            for(int i=1; i<=g.n; i++){
                for(int j=1; j<=g.n; j++){
                    accumulate_error(s, ref[(size_t)i * (g.n + 1) + j],
                                     ref_inf, fw.get_dist(i, j), fw.inf);
                }
            }
            write_error(err, "FloydWarshall", g, type, s);
        }
    }
    cerr << "  FW paralelo " << type << " threads=" << threads
         << ": " << median(times) << " ms\n";
}

void run_fw(const string& times_path, const string& err_path,
            const vector<int>& sizes){
    ofstream out(times_path), err(err_path);
    out << "algoritmo,tipo,threads,n,arcos,repeticao,tempo_ms,"
           "memoria_bytes,status\n";
    err << "algoritmo,grafo,n,arcos,tipo,erro_abs_max,erro_rel_max,"
           "pares_erro_rel_maior_1e-3,pares_comparados,"
           "alcance_divergente\n";
    const int hw = max_threads();

    for(int n : sizes){
        const Graph g = random_graph(n, 4LL * n, 1000 + n);
        cerr << "[fw] n=" << n << " arcos=" << g.u.size() * 2 << '\n';

        // Referência: paralelo double (verificado contra o original abaixo).
        set_threads(hw);
        ParallelFloydWarshall<double> refw(g.n);
        refw.enforce_budget = false;
        add_undirected<ParallelFloydWarshall<double>, double>(refw, g);
        refw.run();
        const vector<double>& ref = refw.dist;

        // Original (só onde cabe no limite de iterações).
        if((long long)n * n * n <= IAlgorithm<double>::operation_budget){
            vector<double> times;
            for(int rep=1; rep<=repetitions; rep++){
                FloydWarshall<double> fw(g.n);
                add_undirected<FloydWarshall<double>, double>(fw, g);
                const double t0 = now_ms();
                fw.run();
                const double ms = now_ms() - t0;
                times.push_back(ms);
                fw_row(out, "FloydWarshall_original", "double", 1, g, rep,
                       ms, original_fw_bytes(fw), fw.limite_excedido);
                if(rep == 1){
                    ErrorStats s;
                    for(int i=1; i<=g.n; i++){
                        for(int j=1; j<=g.n; j++){
                            const double r = ref[(size_t)i * (g.n + 1) + j];
                            accumulate_error(
                                s, fw.get_dist(i, j), fw.inf,
                                r, numeric_limits<double>::infinity()
                            );
                        }
                    }
                    write_error(err, "FloydWarshall_original_vs_paralelo",
                                g, "double", s);
                }
            }
            cerr << "  FW original: " << median(times) << " ms\n";
        }

        for(int t : thread_list()){
            bench_parallel_fw<double>(out, g, t, ref, err, false);
        }
        for(int t : {1, hw}){
            bench_parallel_fw<float>(out, g, t, ref, err, t == hw);
#if OPTIMIZE_HAS_HALF
            bench_parallel_fw<optimize::half>(out, g, t, ref, err, t == hw);
#endif
        }
        err.flush();
    }
}

// ----------------------------------------------------------------------------
// Bellman-Ford

void bf_row(ostream& out, const Graph& g, const string& algo,
            const string& type, int threads, int rep, double ms, int rounds,
            size_t bytes, bool limited){
    out << g.name << ',' << algo << ',' << type << ',' << threads << ','
        << g.n << ',' << g.u.size() * 2 << ',' << rep << ',' << ms << ','
        << rounds << ',' << bytes << ','
        << (limited ? "iteration_limit" : "ok") << '\n';
    out.flush();
}

template<typename T>
void bench_parallel_bf(ostream& out, const Graph& g, int threads,
                       const vector<double>& ref, ostream& err,
                       bool record_error){
    set_threads(threads);
    const string type = NumTraits<T>::name();
    ParallelBellmanFord<T> bf(g.n);
    bf.enforce_budget = false;
    add_undirected<ParallelBellmanFord<T>, T>(bf, g);
    bf.build();
    vector<double> times;
    for(int rep=1; rep<=repetitions; rep++){
        const double t0 = now_ms();
        bf.run(1);
        const double ms = now_ms() - t0;
        times.push_back(ms);
        bf_row(out, g, "BellmanFord_paralelo", type, threads, rep, ms,
               bf.rounds, bf.memory_bytes(), bf.limite_excedido);
    }
    if(record_error){
        ErrorStats s;
        for(int v=1; v<=g.n; v++){
            accumulate_error(s, ref[v], numeric_limits<double>::max(),
                             bf.dist[v], bf.inf);
        }
        write_error(err, type == "double"
                             ? "BellmanFord_original_vs_paralelo"
                             : "BellmanFord", g, type, s);
    }
    cerr << "  BF paralelo " << type << " threads=" << threads << ": "
         << median(times) << " ms (" << bf.rounds << " rodadas)\n";
}

void run_bf(const string& times_path, const string& err_path,
            const vector<string>& specs){
    ofstream out(times_path), err(err_path);
    out << "grafo,algoritmo,tipo,threads,n,arcos,repeticao,tempo_ms,"
           "rodadas,memoria_bytes,status\n";
    err << "algoritmo,grafo,n,arcos,tipo,erro_abs_max,erro_rel_max,"
           "vertices_erro_rel_maior_1e-3,vertices_comparados,"
           "alcance_divergente\n";
    const int hw = max_threads();

    for(const string& spec : specs){
        const Graph g = load_graph(spec);
        cerr << "[bf] " << g.name << " n=" << g.n << " arcos="
             << g.u.size() * 2 << '\n';

        // Original sequencial: referência de corretude.
        vector<double> ref;
        {
            BellmanFord<double> bf(g.n);
            bf.edges.reserve(g.u.size() * 2);
            add_undirected<BellmanFord<double>, double>(bf, g);
            vector<double> times;
            for(int rep=1; rep<=repetitions; rep++){
                const double t0 = now_ms();
                bf.run(1);
                const double ms = now_ms() - t0;
                times.push_back(ms);
                const size_t bytes = bf.edges.capacity() *
                        sizeof(BellmanFord<double>::Edge) +
                    bf.dist.capacity() * sizeof(double) +
                    bf.pai.capacity() * sizeof(int);
                bf_row(out, g, "BellmanFord_original", "double", 1, rep, ms,
                       -1, bytes, bf.limite_excedido);
            }
            ref = bf.dist;
            cerr << "  BF original: " << median(times) << " ms"
                 << (bf.limite_excedido ? " (limite de iteracoes)" : "")
                 << '\n';
        }

        for(int t : thread_list()){
            bench_parallel_bf<double>(out, g, t, ref, err, t == hw);
        }
        for(int t : {1, hw}){
            bench_parallel_bf<float>(out, g, t, ref, err, t == hw);
#if OPTIMIZE_HAS_HALF
            bench_parallel_bf<optimize::half>(out, g, t, ref, err, t == hw);
#endif
        }
        err.flush();
    }
}

// ----------------------------------------------------------------------------
// Testes rápidos de corretude.

int run_check(){
    int failures = 0;
    auto expect = [&](bool ok, const string& what){
        cout << (ok ? "[ok]   " : "[FALHA] ") << what << '\n';
        if(!ok) failures++;
    };

    for(int seed=1; seed<=5; seed++){
        const Graph g = random_graph(300, 900, seed);
        FloydWarshall<double> a(g.n);
        ParallelFloydWarshall<double> b(g.n);
        add_undirected<FloydWarshall<double>, double>(a, g);
        add_undirected<ParallelFloydWarshall<double>, double>(b, g);
        a.run();
        b.run();
        bool same = true;
        for(int i=1; i<=g.n && same; i++){
            for(int j=1; j<=g.n; j++){
                if(fabs(a.get_dist(i, j) - b.get_dist(i, j)) > 1e-9){
                    same = false;
                    break;
                }
            }
        }
        expect(same, "Floyd-Warshall paralelo == original (seed " +
                         to_string(seed) + ")");

        BellmanFord<double> c(g.n);
        ParallelBellmanFord<double> d(g.n);
        add_undirected<BellmanFord<double>, double>(c, g);
        add_undirected<ParallelBellmanFord<double>, double>(d, g);
        c.run(7);
        d.run(7);
        bool same_bf = true;
        for(int v=1; v<=g.n; v++){
            if(fabs(c.dist[v] - d.dist[v]) > 1e-9) same_bf = false;
            if(fabs(d.dist[v] - b.get_dist(7, v)) > 1e-9) same_bf = false;
        }
        const vector<int> path = d.get_path(7, g.n);
        bool path_ok = !path.empty() && path.front() == 7 &&
            path.back() == g.n;
        expect(same_bf && path_ok,
               "Bellman-Ford paralelo == original e caminho valido (seed " +
                   to_string(seed) + ")");
    }

    // Ciclo negativo: 1 -> 2 -> 3 -> 1 com soma -1.
    {
        ParallelBellmanFord<double> bf(4);
        bf.add_edge(1, 2, 1);
        bf.add_edge(2, 3, -3);
        bf.add_edge(3, 1, 1);
        bf.add_edge(3, 4, 2);
        bf.run(1);
        expect(bf.has_negative_cycle(), "Bellman-Ford detecta ciclo negativo");

        ParallelFloydWarshall<double> fw(4);
        fw.add_edge(1, 2, 1);
        fw.add_edge(2, 3, -3);
        fw.add_edge(3, 1, 1);
        fw.add_edge(3, 4, 2);
        fw.run();
        expect(fw.has_negative_cycle(),
               "Floyd-Warshall detecta ciclo negativo");
    }
    // Pesos negativos sem ciclo negativo (direcionado).
    {
        BellmanFord<double> a(5);
        ParallelBellmanFord<float> b(5);
        const int E[][3] = {{1,2,4},{1,3,2},{3,2,-1},{2,4,2},{3,4,5},{4,5,-3}};
        for(auto& e : E){
            a.add_edge(e[0], e[1], e[2]);
            b.add_edge(e[0], e[1], (float)e[2]);
        }
        a.run(1);
        b.run(1);
        bool same = !b.has_negative_cycle();
        for(int v=1; v<=5; v++) same = same && a.dist[v] == (double)b.dist[v];
        expect(same, "Bellman-Ford float com pesos negativos == original");
    }

    cout << (failures == 0 ? "Todos os testes passaram.\n"
                           : to_string(failures) + " falha(s).\n");
    return failures == 0 ? 0 : 1;
}

}

int main(int argc, char** argv){
    try{
        if(argc >= 2 && string(argv[1]) == "check") return run_check();
        if(argc >= 4 && string(argv[1]) == "fw"){
            vector<int> sizes;
            for(int i=4; i<argc; i++) sizes.push_back(atoi(argv[i]));
            if(sizes.empty()) sizes = {250, 500, 750, 1000, 1500, 2000};
            run_fw(argv[2], argv[3], sizes);
            return 0;
        }
        if(argc >= 5 && string(argv[1]) == "bf"){
            run_bf(argv[2], argv[3], vector<string>(argv + 4, argv + argc));
            return 0;
        }
    } catch(const exception& e){
        cerr << "Erro: " << e.what() << '\n';
        return 1;
    }
    cerr << "Uso:\n"
            "  bench_optimize check\n"
            "  bench_optimize fw <tempos.csv> <erros.csv> [n ...]\n"
            "  bench_optimize bf <tempos.csv> <erros.csv> <grafo> [grafo ...]\n";
    return 2;
}
