#include "graph.hpp"
#include <iostream>
#include <fstream>
#include <vector>
using namespace std;

int main(int argc, char** argv) {
    string path = argv[1];
    string nome = argv[2];

    ifstream fin(path);
    int n;
    fin >> n;
    Grafo g(n);
    int u, v;
    while (fin >> u >> v) g.add_edge(u, v);
    fin.close();

    g.build();
    auto comps = g.get_components();
    int diam = g.get_diameter();

    cout << nome << ";" << g.get_n() << ";" << g.get_m() << ";"
         << comps.size() << ";" << diam << "\n";

    return 0;
}
