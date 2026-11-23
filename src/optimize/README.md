# Otimizações: Bellman-Ford e Floyd-Warshall paralelos

Esta pasta é independente do resto de `src/` (nenhum arquivo fora dela foi
alterado). As classes seguem as mesmas interfaces de `../algorithm.hpp`
(`ISingleSourceAlgorithm`, `IAllPairsAlgorithm`, `INegativeCycleDetector`) e
têm o mesmo construtor `(n)`, então podem substituir as originais.

| Arquivo | Conteúdo |
|---|---|
| `parallel_bellman_ford.cpp` | `ParallelBellmanFord<T>` |
| `parallel_floyd_warshall.cpp` | `ParallelFloydWarshall<T>` |
| `numeric.hpp` | infinito por tipo e suporte a `float16` (`optimize::half`) |
| `bench_optimize.cpp` | testes de corretude e benchmark contra as versões originais |
| `plot_optimize.py` | gera os gráficos em `graficos/` a partir de `resultados/` |

## O que foi feito

### Bellman-Ford: `for(k, n vezes){ processar arestas em paralelo }`

Cada rodada relaxa todas as arestas em paralelo com OpenMP. Para que duas
threads nunca escrevam no mesmo `dist[v]`:

- as arestas são agrupadas pelo **destino** (CSR de arestas de entrada,
  montado uma vez em `build()` por counting sort). Cada thread fica com um
  bloco de vértices `v` e relaxa todas as arestas `u -> v` que chegam neles;
- a rodada lê `dist` (rodada anterior) e escreve em `next_dist`, e no fim os
  dois vetores são trocados (relaxação de Jacobi). Nenhuma thread lê um valor
  que outra esteja escrevendo, então não há corrida e o resultado é
  determinístico.

Com Jacobi, depois da rodada `k` todo caminho mínimo com até `k` arestas está
correto. Por isso `n - 1` rodadas continuam bastando, e uma mudança na rodada
`n` indica ciclo negativo, como no original. A parada antecipada (rodada sem
mudança) foi mantida. O CSR também usa menos memória que o `vector<Edge>`
original: são `4 + sizeof(T)` bytes por arco, contra 16 com `double`.

### Floyd-Warshall: `for(k, n vezes){ processar pares i, j em paralelo }`

Para cada `k`, as linhas `i` são divididas entre as threads. Antes do laço
paralelo, a linha `k` é copiada para um buffer. Cada thread lê a própria
linha `i` e a cópia da linha `k`, e só escreve na linha `i`, então não há
corrida. Outras mudanças:

- a matriz fica num **único vetor contíguo** em vez de `vector<vector<T>>`,
  e o laço em `j` é vetorizado (SIMD);
- a escrita só acontece quando a distância melhora, o que reduz o tráfego de
  memória. Isso importa quando a matriz não cabe no cache;
- `schedule(dynamic)`, porque CPUs híbridas (núcleos P e E) fazem a divisão
  estática esperar o núcleo mais lento;
- o diâmetro passa a ser a maior distância **final** entre pares alcançáveis.

### Tipo numérico menor

As classes são templates, então o tipo é escolhido na instância:
`ParallelBellmanFord<float>`, `ParallelFloydWarshall<optimize::half>` etc.

- `float` (4 B) usa metade da memória do `double` (8 B). O erro relativo
  medido ficou na casa de 1e-7, desprezível para os pesos com duas casas
  decimais dos grafos.
- `float16` (`_Float16`, 2 B) é o menor tipo de ponto flutuante disponível.
  Ele tem só cerca de 3 dígitos significativos e valor máximo de 65504, então
  dá erro relativo da ordem de 1e-3 e pode estourar em distâncias grandes.
  Na maioria das CPUs x86 a conta é emulada (converte para `float` e volta),
  então com 1 thread fica mais lento. Só compensa quando o gargalo é a
  memória.

Para tipos de ponto flutuante a distância infinita é `+inf`, e não
`numeric_limits<T>::max()`: como `inf + w == inf`, o laço não precisa testar
infinito e pode ser vetorizado. Para tipos inteiros continua sendo `max()`.
**Atenção:** quem compara o resultado com `numeric_limits<T>::max()` deve
usar o membro `inf` da classe.

### Limite de iterações

O limite de 1,5·10⁹ iterações de `IAlgorithm` foi mantido, só que contado
por rodada (BF) e por `k` (FW), e não por aresta, porque um contador
compartilhado atrapalharia o paralelismo. O campo `enforce_budget = false`
desliga o limite. O benchmark usa isso para medir grafos em que o original
seria interrompido.

## Compilar e rodar

Precisa de `g++` com OpenMP (`-fopenmp`).

```bash
cd src/optimize
make            # compila bench_optimize
make check      # compara com as versões originais de src/
make benchmark  # gera resultados/*.csv (demora alguns minutos)
make graficos   # gera graficos/*.png (precisa de matplotlib)
```

Sem `make`:

```bash
g++ -std=c++17 -O3 -march=native -fopenmp -o bench_optimize bench_optimize.cpp
./bench_optimize check
./bench_optimize fw resultados/fw_tempos.csv resultados/fw_erros.csv 250 500 1000 2000
./bench_optimize bf resultados/bf_tempos.csv resultados/bf_erros.csv \
    ../testes/grafo_1.txt random:100000:500000:1
python3 plot_optimize.py
```

O número de threads segue o OpenMP (`OMP_NUM_THREADS`). O benchmark varia de
1 até o máximo da máquina.
