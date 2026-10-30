// Estudos de caso da Parte 2.
//
// Le um grafo ponderado da entrada padrao ("n" e depois "u v peso") e
// imprime um JSON com o resultado do modo pedido:
//
//   caminhos   distancia e caminho minimo de --origem ate cada --destinos
//              (Estudos 1 e 3). Com --nomes, traduz indices para nomes.
//   tempos     tempo medio do Dijkstra com heap e com vetor a partir de
//              vertices aleatorios (Estudo 2). Cada implementacao roda ate
//              --k execucoes ou ate estourar --orcamento segundos (minimo 1).
//              --max-n-vetor pula o vetor em grafos grandes demais.
//
// Exemplos:
//   gzip -dc grafo_W_3.txt.gz | ./estudos caminhos --nome W_3 --origem 10 --destinos 20,30,40,50,60
//   gzip -dc grafo_W_3.txt.gz | ./estudos tempos --nome W_3 --k 100 --orcamento 600
//   ./estudos caminhos --nome rede --nomes rede_colaboracao_vertices.txt \
//       --origem-nome "Edsger W. Dijkstra" --destinos-nomes "Alan M. Turing;J. B. Kruskal" < rede_colaboracao.txt

#include "../src/graph.hpp"

#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>
#include <map>
#include <random>
#include <sstream>
#include <string>
#include <vector>

using namespace std;
using Clock = chrono::steady_clock;

namespace {

// ------------------------------------------------------------ leitura
// Leitor rapido: os maiores grafos tem centenas de milhoes de numeros.
class Leitor {
    static constexpr size_t BUF = 1 << 22;
    vector<char> buf_;
    size_t pos_ = 0, len_ = 0;
    FILE* f_;

    int get(){
        if(pos_ == len_){
            len_ = fread(buf_.data(), 1, BUF, f_);
            pos_ = 0;
            if(len_ == 0) return -1;
        }
        return buf_[pos_++];
    }

public:
    explicit Leitor(FILE* f) : buf_(BUF), f_(f){}

    bool token(string& out){
        out.clear();
        int c = get();
        while(c != -1 && isspace(c)) c = get();
        if(c == -1) return false;
        while(c != -1 && !isspace(c)){ out.push_back(static_cast<char>(c)); c = get(); }
        return true;
    }

    bool inteiro(long long& x){
        int c = get();
        while(c != -1 && isspace(c)) c = get();
        if(c == -1) return false;
        bool neg = false;
        if(c == '-'){ neg = true; c = get(); }
        x = 0;
        while(c >= '0' && c <= '9'){ x = x * 10 + (c - '0'); c = get(); }
        if(neg) x = -x;
        return true;
    }

    bool real(double& x){
        string t;
        if(!token(t)) return false;
        x = strtod(t.c_str(), nullptr);
        return true;
    }
};

struct Aresta { int u, v; double w; };

struct Entrada {
    int n = 0;
    vector<Aresta> arestas;
    int negativas = 0;
};

Entrada ler_grafo(){
    Leitor in(stdin);
    Entrada e;
    long long n;
    if(!in.inteiro(n)) throw runtime_error("entrada vazia");
    e.n = static_cast<int>(n);
    long long u, v;
    double w;
    while(in.inteiro(u)){
        if(!in.inteiro(v) || !in.real(w)) throw runtime_error("aresta incompleta");
        e.arestas.push_back({static_cast<int>(u), static_cast<int>(v), w});
        if(w < 0) ++e.negativas;
    }
    return e;
}

// Monta o Grafo<double> com a capacidade exata de cada lista de adjacencia.
Grafo<double> montar(const Entrada& e){
    Grafo<double> g(e.n);
    vector<int> grau(e.n + 1, 0);
    for(const auto& a : e.arestas){ ++grau[a.u]; ++grau[a.v]; }
    g.reserve_adjacencia(grau);
    for(const auto& a : e.arestas) g.add_edge(a.u, a.v, a.w);
    return g;
}

// ------------------------------------------------------------ utilidades
string json_str(const string& s){
    string o = "\"";
    for(char c : s){
        if(c == '"' || c == '\\'){ o += '\\'; o += c; }
        else if(static_cast<unsigned char>(c) < 0x20){ o += ' '; }
        else o += c;
    }
    return o + "\"";
}

string json_num(double x){
    if(!isfinite(x)) return "null";
    ostringstream s;
    s.precision(10);
    s << x;
    return s.str();
}

vector<int> caminho(const vector<int>& pai, int v){
    vector<int> p;
    if(pai[v] == 0) return p;
    p.push_back(v);
    while(pai[v] != v){ v = pai[v]; p.push_back(v); }
    return vector<int>(p.rbegin(), p.rend());
}

vector<string> dividir(const string& s, char sep){
    vector<string> out;
    string cur;
    for(char c : s){
        if(c == sep){ if(!cur.empty()) out.push_back(cur); cur.clear(); }
        else cur += c;
    }
    if(!cur.empty()) out.push_back(cur);
    return out;
}

// indice,nome por linha
map<int, string> ler_nomes(const string& arq, map<string, int>& por_nome){
    map<int, string> nomes;
    ifstream f(arq);
    if(!f) throw runtime_error("nao abriu " + arq);
    string linha;
    while(getline(f, linha)){
        if(!linha.empty() && linha.back() == '\r') linha.pop_back();
        const size_t c = linha.find(',');
        if(c == string::npos) continue;
        const int id = atoi(linha.substr(0, c).c_str());
        const string nome = linha.substr(c + 1);
        nomes[id] = nome;
        por_nome.emplace(nome, id);
    }
    return nomes;
}

double segundos(Clock::time_point a, Clock::time_point b){
    return chrono::duration<double>(b - a).count();
}

}  // namespace

int main(int argc, char* argv[]){
    if(argc < 2){
        cerr << "uso: estudos caminhos|tempos [opcoes] < grafo.txt\n";
        return 1;
    }
    const string modo = argv[1];
    map<string, string> op;
    for(int i = 2; i + 1 < argc; i += 2) op[argv[i]] = argv[i + 1];
    auto opt = [&](const string& k, const string& def){ return op.count(k) ? op[k] : def; };

    const auto t0 = Clock::now();
    Entrada e = ler_grafo();
    const auto t1 = Clock::now();
    Grafo<double> g = montar(e);
    const size_t m = e.arestas.size();
    const int negativas = e.negativas;
    e.arestas.clear();
    e.arestas.shrink_to_fit();
    const auto t2 = Clock::now();
    cerr << "[" << opt("--nome", "?") << "] n=" << g.get_n() << " m=" << m
         << " leitura " << segundos(t0, t1) << "s, montagem " << segundos(t1, t2) << "s\n";

    ostringstream js;
    js << "{\"nome\":" << json_str(opt("--nome", "?")) << ",\"n\":" << g.get_n() << ",\"m\":" << m;

    if(negativas > 0){
        // Mesmo comportamento pedido no enunciado: nao roda Dijkstra.
        js << ",\"erro\":\"A biblioteca ainda nao implementa caminhos minimos com pesos negativos ("
           << negativas << " arestas negativas)\"}";
        cout << js.str() << "\n";
        return 0;
    }

    if(modo == "caminhos"){
        map<int, string> nomes;
        map<string, int> por_nome;
        if(op.count("--nomes")) nomes = ler_nomes(op["--nomes"], por_nome);

        int origem;
        vector<int> destinos;
        if(op.count("--origem-nome")){
            origem = por_nome.at(op["--origem-nome"]);
            for(const string& s : dividir(op["--destinos-nomes"], ';')) destinos.push_back(por_nome.at(s));
        } else{
            origem = atoi(opt("--origem", "10").c_str());
            for(const string& s : dividir(opt("--destinos", "20,30,40,50,60"), ',')) destinos.push_back(atoi(s.c_str()));
        }

        vector<int> pai;
        vector<double> dist;
        const auto a = Clock::now();
        g.dijkstra_heap(origem, pai, dist);
        const auto b = Clock::now();

        auto rotulo = [&](int v){ return nomes.count(v) ? json_str(nomes[v]) : to_string(v); };
        js << ",\"origem\":" << rotulo(origem) << ",\"origem_indice\":" << origem
           << ",\"tempo_dijkstra_s\":" << json_num(segundos(a, b)) << ",\"destinos\":[";
        for(size_t i = 0; i < destinos.size(); ++i){
            const int d = destinos[i];
            const vector<int> p = caminho(pai, d);
            js << (i ? "," : "") << "{\"destino\":" << rotulo(d) << ",\"indice\":" << d
               << ",\"distancia\":" << json_num(dist[d]) << ",\"caminho\":";
            if(p.empty()) js << "null";
            else{
                js << "[";
                for(size_t j = 0; j < p.size(); ++j) js << (j ? "," : "") << rotulo(p[j]);
                js << "]";
            }
            js << "}";
        }
        js << "]}";
    } else if(modo == "tempos"){
        const int k = atoi(opt("--k", "100").c_str());
        const double orcamento = atof(opt("--orcamento", "600").c_str());
        const long long max_n_vetor = atoll(opt("--max-n-vetor", "2000000").c_str());
        mt19937 rng(static_cast<unsigned>(atoi(opt("--semente", "2026").c_str())));
        uniform_int_distribution<int> sorteio(1, g.get_n());
        vector<int> origens(k);
        for(int& s : origens) s = sorteio(rng);

        vector<int> pai;
        vector<double> dist, dist_heap_primeira;

        const double orcamento_vetor = atof(opt("--orcamento-vetor", opt("--orcamento", "600")).c_str());
        auto medir = [&](bool heap, vector<double>& tempos){
            const double limite = heap ? orcamento : orcamento_vetor;
            const auto inicio = Clock::now();
            for(int i = 0; i < k; ++i){
                const auto a = Clock::now();
                if(heap) g.dijkstra_heap(origens[i], pai, dist);
                else g.dijkstra_vector(origens[i], pai, dist);
                const auto b = Clock::now();
                tempos.push_back(segundos(a, b));
                if(heap && i == 0) dist_heap_primeira = dist;
                cerr << "  " << (heap ? "heap " : "vetor") << " #" << i + 1 << " origem " << origens[i]
                     << ": " << tempos.back() << "s\n";
                if(segundos(inicio, Clock::now()) > limite) break;
            }
        };

        vector<double> th, tv;
        medir(true, th);
        const bool roda_vetor = g.get_n() <= max_n_vetor;
        double dif_max = -1;
        if(roda_vetor){
            medir(false, tv);
            // confere: a primeira origem e a mesma nas duas implementacoes
            g.dijkstra_vector(origens[0], pai, dist);
            dif_max = 0;
            for(size_t v = 1; v < dist.size(); ++v){
                if(isinf(dist[v]) && isinf(dist_heap_primeira[v])) continue;
                dif_max = max(dif_max, fabs(dist[v] - dist_heap_primeira[v]));
            }
        }

        auto lista = [&](const vector<double>& t){
            ostringstream s;
            s << "[";
            for(size_t i = 0; i < t.size(); ++i) s << (i ? "," : "") << json_num(t[i] * 1000);
            return s.str() + "]";
        };
        auto media = [](const vector<double>& t){
            double s = 0;
            for(double x : t) s += x;
            return t.empty() ? NAN : s / t.size();
        };
        js << ",\"k_pedido\":" << k << ",\"orcamento_heap_s\":" << orcamento << ",\"orcamento_vetor_s\":" << orcamento_vetor
           << ",\"heap_ms\":" << json_num(media(th) * 1000) << ",\"k_heap\":" << th.size()
           << ",\"heap_execucoes_ms\":" << lista(th)
           << ",\"vetor_ms\":" << json_num(roda_vetor ? media(tv) * 1000 : NAN)
           << ",\"k_vetor\":" << tv.size()
           << ",\"vetor_execucoes_ms\":" << lista(tv)
           << ",\"diferenca_max_vetor_heap\":" << json_num(dif_max) << "}";
    } else{
        cerr << "modo desconhecido: " << modo << "\n";
        return 1;
    }

    cout << js.str() << "\n";
    return 0;
}
