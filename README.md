# 📊 Analisador e Visualizador de Grafos

Esta ferramenta é um pipeline completo para carregamento, processamento, benchmarking e visualização de grafos. O sistema é dividido em duas partes: um núcleo de alta performance escrito em **C++17** (responsável por medir tempo, memória e exportar as estruturas do grafo) e um módulo de visualização em **Python** (responsável por gerar gráficos e desenhar as árvores de travessia).

## Pré-requisitos

Para compilar e executar o projeto, você precisará de:

* **Compilador C++**: GCC/G++ com suporte a C++17 (ambiente Linux/Unix recomendado para medição precisa de memória `getrusage`).
* **Make**: Utilitário para automação de compilação.
* **Python 3.8+**: Com as seguintes bibliotecas instaladas:
```bash
pip install pandas matplotlib seaborn networkx numpy

```



## 1. Compilando o Projeto (C++)

O projeto utiliza um `makefile` para gerenciar a compilação. Para construir os executáveis, abra o terminal na raiz do projeto e execute:

```bash
make

```

Isso compilará os arquivos da pasta `Exporters/` combinados com o núcleo `src/` e gerará três executáveis na raiz do diretório:

1. `export_json`
2. `export_grafo`
3. `export_graus`

*(Para limpar os arquivos compilados e os executáveis, utilize `make clean`).*

## 2. Estrutura dos Arquivos de Entrada

Os arquivos de texto dentro da pasta `Entrada/` devem conter a quantidade de vértices na primeira linha, seguida por uma lista de arestas (pares de vértices `u` e `v`), por exemplo:

```text
5
1 2
2 3
3 4
4 5

```

## 3. Como Utilizar (Pipeline)

O pipeline de execução funciona com o padrão de redirecionamento de I/O do terminal. Você deve passar o arquivo de texto como entrada (`<`) e redirecionar a saída (`>`) para as pastas corretas dentro do diretório `Dados/`.

### Passo A: Exportar as Métricas e Estruturas

Execute os comandos abaixo para cada grafo que você deseja analisar. Altere os nomes dos arquivos conforme necessário.

**Gerar a telemetria e o benchmark (JSON):**

```bash
./export_json < Entrada/grafo_1.txt > Dados/JSON/grafo_1.json

```

**Gerar a árvore geradora da BFS (CSV):**

```bash
./export_grafo bfs < Entrada/grafo_1.txt > Dados/Grafos/bfs_1.csv

```

**Gerar a árvore geradora da DFS (CSV):**

```bash
./export_grafo dfs < Entrada/grafo_1.txt > Dados/Grafos/dfs_1.csv

```

**Gerar a distribuição de graus (CSV):**

```bash
./export_graus < Entrada/grafo_1.txt > Dados/Graus/grafo_1.csv

```

> **Aviso de Nomenclatura:** É fundamental que os números nos nomes dos arquivos gerados sejam coincidentes (ex: `grafo_1.json`, `bfs_1.csv`, `dfs_1.csv`, `grafo_1.csv`). O script em Python utiliza esse número para cruzar os dados.

### Passo B: Gerar as Visualizações (Python)

Após preencher as pastas `JSON/`, `Grafos/` e `Graus/` dentro do diretório `Dados/`, basta rodar o módulo analisador. Na raiz do projeto, execute:

```bash
python Analysers/main.py

```

O script lerá todos os dados exportados automaticamente e salvará os artefatos visuais no diretório `Dados/Graficos/`.

## 4. O Que Será Gerado?

Após a execução do script Python, a pasta `Dados/Graficos/` conterá:

* **Curvas de Performance (Linear e Log):** Comparação do tempo médio de execução entre BFS, DFS Iterativa e DFS Recursiva em relação ao tamanho do grafo (V + E).
* **Curvas de Memória:** Gráfico ilustrando o consumo máximo de RAM (em MB) alocado durante a execução dos algoritmos em C++.
* **Time Profiling:** Gráfico de barras empilhadas mostrando o tempo gasto lendo o arquivo (I/O), calculando graus, extraindo componentes e rodando a travessia.
* **Topologias de Árvore (PNG):** Desenhos estéticos mostrando o esqueleto de navegação gerado pela BFS e pela DFS.
* **Histogramas de Grau:** Gráficos de barras apresentando a frequência absoluta de graus dos vértices de cada grafo.