# Análise de Grafos em Larga Escala

Este projeto é uma biblioteca em C++ e um pipeline de visualização em Python desenvolvidos para a disciplina de Teoria dos Grafos. O sistema permite ler grafos não-direcionados a partir de arquivos de texto, extrair estatísticas complexas (componentes conexas, diâmetro, distribuições de grau, caminhos via BFS/DFS) e gerar gráficos analíticos e topológicos em alta resolução.

## Pré-requisitos

Para compilar e executar o projeto, você precisará ter instalado:

* Compilador C++ (suporte a C++17, ex: `g++`)
* Ferramenta `make`
* Python 3.x

Instale as bibliotecas Python necessárias executando o seguinte comando:

```bash
pip install matplotlib seaborn networkx numpy

```

## Compilação

O projeto acompanha um `Makefile` para automatizar a compilação dos códigos em C++. No terminal, na raiz do projeto, execute:

```bash
make

```

Isso irá gerar dois executáveis principais (verifique os nomes exatos gerados pelo seu `Makefile`, geralmente `main.out` e `export.out`).

Para limpar os arquivos compilados antigos, você pode usar:

```bash
make clean

```

## Como Usar (Pipeline de Execução)

O fluxo de uso do projeto é dividido em três etapas: relatório em texto, serialização de dados e geração de gráficos.

### 1. Gerar Relatório de Console (`main.out`)

Executa a análise completa do grafo e imprime as estatísticas, tempos de execução, caminhos de vértices específicos e consumo de RAM diretamente no terminal.

```bash
./main.out < caminho/para/o/grafo.txt

```

### 2. Exportar Dados Topológicos (`export_json.out` ou `export.out`)

Gera a telemetria, classifica as arestas (árvores, pontes, articulações) e exporta toda a estrutura do grafo para um arquivo JSON que será lido pelo Python.

```bash
./export.out < caminho/para/o/grafo.txt > grafo_1.json

```

*Dica: Repita este passo para todos os grafos que deseja analisar, gerando arquivos como `grafo_1.json`, `grafo_2.json`, etc., sempre salvos na mesma pasta do script Python.*

### 3. Gerar Visualizações (`visualize.py`)

Lê automaticamente todos os arquivos `.json` presentes no diretório raiz e gera os gráficos de desempenho, consumo de memória, histogramas de graus e as árvores de topologia.

```bash
python3 visualize.py

```

*As imagens geradas (em escalas lineares e logarítmicas) serão salvas automaticamente dentro da pasta `graficos_saida/`.*