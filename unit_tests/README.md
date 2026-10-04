# Testes dos problemas CSES

Cada tarefa tem um `main.cpp` adaptador em `mains/<id>/`. Casos de entrada e
saídas de referência ficam em `cases/<id>/input/` e `cases/<id>/expected/`,
respectivamente, com nomes-base coincidentes (`nome.in` e `nome.ans`).

Rode todas as tarefas configuradas:

```bash
./unit_tests/run_tests.sh
```

Rode somente uma tarefa:

```bash
./unit_tests/run_tests.sh 1671
```

O script usa comparação textual para 1671. Os problemas 1667 e 1669 aceitam
qualquer caminho válido, então seus validadores verificam a validade e, no
caso de 1667, a minimalidade, em vez de exigir igualdade com uma saída
específica. Os arquivos `.ans` desses dois problemas são mantidos como
referência legível.

## Adaptadores incluídos

- **1667 — Message Route:** BFS da biblioteca para recuperar o caminho mínimo
  em número de arestas.
- **1669 — Round Trip:** DFS da biblioteca para obter a floresta de busca;
  procura uma aresta não pertencente à árvore e reconstrói um ciclo simples.
- **1671 — Shortest Routes I:** Dijkstra com heap da biblioteca. O problema é
  direcionado, por isso usa `add_arc()`; `add_edge()` continua adicionando uma
  aresta não direcionada.

Adicione os testes oficiais baixados em `cases/<id>/input/` e a saída
correspondente em `cases/<id>/expected/`. Não altere o nome-base entre os dois.

Para 1667 e 1669, os validadores também leem o primeiro token do `.ans` para
saber se o gabarito indica que existe uma solução (`k`) ou que a resposta é
`IMPOSSIBLE`. Assim, não aceitam `IMPOSSIBLE` quando o gabarito indica solução,
nem um caminho/ciclo quando o gabarito indica que não existe solução.
