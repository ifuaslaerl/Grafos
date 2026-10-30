# Apresentação interativa

São 17 slides navegáveis com as setas ← →. Eles mostram BFS, DFS e Dijkstra rodando ao vivo, cenas em 3D e os resultados reais dos benchmarks e dos estudos de caso.

## Abrir

Dê dois cliques em `site/index.html`. Não precisa de servidor nem de build, porque o D3 e o 3d-force-graph estão em `js/vendor/`.
Sem internet, só as fontes trocam para as do sistema. As cenas 3D precisam de WebGL; sem ele, o site mostra um aviso no lugar.

Atalhos: `←` `→` navegam, `O` abre a visão geral, `F` liga a tela cheia e `T` liga o cronômetro de 8 min (duplo clique zera).

## Dados

- `data/dados.js` e `data/dados3d.js`: gerados a partir de `Dados/` (os JSON, CSV e PNGs dos exportadores):

  ```
  python site/tools/build_data.py
  ```

- `data/estudos.js`: resultados dos estudos de caso 1, 2 e 3, gerados a partir de `Dados/Estudos/` (saída de `Estudos/run_estudos.sh`):

  ```
  python site/tools/build_estudos.py
  ```

  Enquanto um estudo não tiver resultado, o slide dele mostra um aviso no lugar.

## Estrutura

| arquivo | o quê |
|---|---|
| `js/engine.js` | BFS/DFS/Dijkstra (vetor e heap) no mesmo laço do `traverse_impl`, gerando um snapshot por passo |
| `js/graphview.js` | desenho e edição do grafo (arrastar, criar/apagar vértice e aresta, editar peso) e geradores (grade, árvore, ciclo, aleatório) |
| `js/widgets.js` | widget "ao vivo": fronteira (fila/pilha/vetor/heap), tabela pai/dist, código, controles |
| `js/scenes.js` | cenas animadas: capa 3D, lista sendo montada, matriz esparsa, quatro fronteiras, vetor × heap 3D, árvores, explorador 3D, rede de pesquisadores |
| `js/charts.js` | gráficos dos dados reais (desempenho, graus, memória, complexidade) |
| `js/app.js` | navegação dos slides e inicialização de cada um |
| `css/style.css`, `css/scenes.css` | estilos base e das cenas |
