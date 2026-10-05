# Biblioteca e ferramentas para grafos

Repositório de uma biblioteca C++17 para grafos e ferramentas para coleta de
métricas, exportação de estruturas, análise e testes de problemas do CSES.

## Requisitos

- GCC/G++ com suporte a C++17
- GNU Make
- Python 3.8 ou superior para executar a análise e os validadores dos testes
- Dependências Python dos gráficos:

```bash
python3 -m pip install pandas matplotlib seaborn networkx numpy
```

## Compilar

Na raiz do repositório:

```bash
make
```

Isso compila os exportadores `export_json`, `export_grafo`, `export_graus` e
`export_memoria`.
Para compilar também o programa interativo de teste do grafo ponderado:

```bash
make test_graph
```

Para remover binários, objetos e artefatos gerados (JSON, CSV e PNG):

```bash
make clean
```

O `clean` preserva grafos de entrada e casos de teste.

## Biblioteca C++

A biblioteca está em `src/graph.hpp`, com implementações template em
`src/graph_general.tpp` e `src/graph_algorithms.tpp`. O tipo dos pesos é
escolhido ao instanciar o grafo, por exemplo:

```cpp
Grafo<long double> graph(n);
graph.add_edge(u, v, peso); // aresta não direcionada
graph.add_arc(u, v, peso);  // arco direcionado
```

Os algoritmos disponíveis incluem BFS, DFS iterativa e recursiva, Dijkstra com
vetor ou heap, componentes conexas, distância e diâmetro. BFS e DFS iterativa
percorrem arestas sem peso; Dijkstra calcula distâncias ponderadas e pressupõe
pesos não negativos.

### Programa para testes manuais

`make test_graph` gera `./test_graph`. Ele lê `n` na primeira linha e, em cada
linha seguinte, `u v peso`. O vértice inicial é opcional e vale `1` por padrão:

```bash
./test_graph 1 < grafo_ponderado.txt
```

O programa imprime em CSV pais e distâncias de BFS, DFS recursiva e das duas
implementações de Dijkstra.

## Exportadores da Parte 1

Os exportadores atuais leem grafos **não ponderados**: a primeira linha contém
`n`, seguida por linhas `u v`. São arestas não direcionadas, com vértices de
`1` a `n`.

Exemplo:

```text
5
1 2
2 5
5 3
4 5
1 5
```

Os grafos podem ser colocados localmente em `Dados/Entrada/` (a pasta é
ignorada pelo Git por conter arquivos grandes). Exemplo para o grafo 1:

```bash
mkdir -p Dados/JSON Dados/Grafos Dados/Graus
./export_json < Dados/Entrada/grafo_1.txt > Dados/JSON/grafo_1.json
./export_grafo bfs < Dados/Entrada/grafo_1.txt > Dados/Grafos/bfs_1.csv
./export_grafo dfs < Dados/Entrada/grafo_1.txt > Dados/Grafos/dfs_1.csv
./export_grafo bfs 1 < Dados/Entrada/grafo_1.txt > Dados/Grafos/bfs_1_inicio_1.csv
./export_graus < Dados/Entrada/grafo_1.txt > Dados/Graus/grafo_1.csv
```

O número identificador nos nomes dos arquivos deve coincidir. O JSON contém
métricas, telemetria e benchmarks; os CSVs armazenam dados por vértice ou
distribuições extensas, como árvores de busca e frequências de grau.
O CSV de `export_grafo` inclui `componente_conexa`, com identificadores
iniciando em 1 e atribuídos pela ordem decrescente do tamanho das componentes.
Sem um vértice inicial, `export_grafo` percorre cada componente a partir de um
vértice arbitrário, como antes. Com o argumento opcional `vertice_inicial`
(após `bfs` ou `dfs`), executa uma única busca a partir desse vértice. Os
vértices fora da árvore resultante permanecem no CSV com `pai=0`,
`distancia=-1` e `ordem_visitacao=-1`; a coluna de componente continua
preenchida para todos os vértices.

Por padrão, `export_json` executa o benchmark completo: mede a construção do
grafo, chama `get_components()` e exporta quantidade, menor e maior tamanho,
além dos tamanhos de todas as componentes em ordem decrescente. A telemetria
inclui os tempos de construção e de `get_components()` e a variação do RSS
durante essa chamada. Também mede BFS, DFS iterativa e DFS recursiva usando somente a
lista de adjacência.

O benchmark usa até 100 vértices iniciais distintos,
selecionados aleatoriamente com semente fixa, e mede BFS, DFS iterativa e DFS
recursiva para cada origem. Se o grafo tiver menos de 100 vértices, usa todos
eles. Os dados ficam em `benchmark.representacoes.lista`; as chaves históricas `BFS`,
`DFS_Iterativa` e `DFS_Recursiva` continuam representando os tempos da lista.
O total de origens executadas é registrado em
`benchmark.numero_execucoes`. O exporter não constrói nem testa a matriz de
adjacência. Para `get_components()`, a telemetria registra o RSS imediatamente
antes e depois da chamada e a diferença entre esses valores em
`memoria_get_components_delta_mb`; essa diferença é específica à etapa, não o
pico transitório de alocações internas. No `export_json`, o peso não usado é
armazenado como `int`, e a capacidade das listas de adjacência é reservada após
contar os graus, reduzindo o uso de memória em grafos grandes.

### Comparar memória das representações

O `export_memoria` mede o RSS atual do processo em MiB, logo após carregar o
grafo na representação selecionada e antes de executar algoritmos. Cada
invocação constrói somente uma representação; execute ambas com o mesmo
arquivo para comparar:

```bash
./export_memoria lista < Dados/Entrada/grafo_1.txt
./export_memoria matriz < Dados/Entrada/grafo_1.txt
```

Ambas as execuções imprimem um CSV com representação, número de vértices,
número de arestas e `memoria_rss_mb`. A medição usa `VmRSS` de
`/proc/self/status` (Linux), inclui a memória-base do processo e seus
metadados, e não é o pico acumulado registrado no JSON. A lista armazena
arestas como `Edge<int>` nas duas direções; a matriz armazena inteiros
por célula e marca ambas as direções. A comparação é da memória residente total
do processo naquele ponto.

## Gerar gráficos

Depois de gerar os arquivos JSON e CSV para os grafos que serão analisados:

```bash
python3 Analysers/main.py
```

O analisador procura os dados em `Dados/JSON/`, `Dados/Grafos/` e
`Dados/Graus/`, e grava os gráficos em `Dados/Graficos/`. São produzidos
gráficos de desempenho e memória, perfil de tempo, histogramas de grau,
árvores BFS/DFS e sobreposição das árvores.

## Testes CSES

Os adaptadores para as tarefas CSES estão em `unit_tests/mains/<id>/main.cpp`:

- **1667 — Message Route:** BFS para obter uma rota com número mínimo de
  computadores.
- **1669 — Round Trip:** DFS e reconstrução de um ciclo.
- **1671 — Shortest Routes I:** Dijkstra com heap em grafo direcionado,
  representado usando `add_arc()`.

Coloque cada entrada e seu gabarito nos diretórios ignorados pelo Git:

```text
unit_tests/cases/<id>/input/
unit_tests/cases/<id>/expected/
```

O runner associa arquivos com nomes-base correspondentes. Para arquivos
oficiais nomeados `test_input...txt` e `test_output...txt`, o script associa
automaticamente os pares. Para outros nomes, use `.in` para a entrada e a
mesma base com `.ans` para o gabarito.

Execute todos os problemas configurados:

```bash
./unit_tests/run_tests.sh
```

Execute apenas um:

```bash
./unit_tests/run_tests.sh 1671
```

O problema 1671 compara os valores de saída com o gabarito, ignorando
diferenças de espaços em branco. Como 1667 e 1669 aceitam qualquer caminho
válido, validadores verificam a validade da rota/ciclo, a minimalidade da rota
no 1667 e se existe solução conforme indicado pelo primeiro token do gabarito.
Os binários temporários de teste são gravados em `unit_tests/.build/`.

## Arquivos gerados e Git

O `.gitignore` exclui os grafos grandes de `Dados/Entrada/`, as saídas em
`Dados/JSON/`, `Dados/Grafos/`, `Dados/Graus/` e `Dados/Graficos/`, assim como
os arquivos de entrada e gabarito de `unit_tests/cases/`. Os adaptadores,
validadores e scripts de teste continuam versionados.
