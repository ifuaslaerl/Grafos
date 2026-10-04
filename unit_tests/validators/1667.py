import collections
import sys

input_path, expected_path, output_path = sys.argv[1:4]

with open(input_path, encoding="utf-8") as file:
    n, m = map(int, file.readline().split())
    adjacency = [[] for _ in range(n + 1)]
    for _ in range(m):
        u, v = map(int, file.readline().split())
        adjacency[u].append(v)
        adjacency[v].append(u)

with open(output_path, encoding="utf-8") as file:
    tokens = file.read().split()

with open(expected_path, encoding="utf-8") as file:
    expected_tokens = file.read().split()

if not expected_tokens:
    raise SystemExit("gabarito vazio")

expected_impossible = expected_tokens[0] == "IMPOSSIBLE"
output_impossible = tokens == ["IMPOSSIBLE"]
if expected_impossible != output_impossible:
    raise SystemExit("a existência de uma rota difere do gabarito")

if tokens == ["IMPOSSIBLE"]:
    sys.exit(0)

if not tokens:
    raise SystemExit("saída vazia")

k = int(tokens[0])
path = list(map(int, tokens[1:]))
if len(path) != k or k < 1:
    raise SystemExit("quantidade de computadores incompatível com o caminho")
if path[0] != 1 or path[-1] != n:
    raise SystemExit("caminho não começa em 1 e termina em n")
if len(set(path)) != len(path):
    raise SystemExit("o caminho repete computadores")
if any(v < 1 or v > n for v in path):
    raise SystemExit("o caminho contém computador fora do intervalo")
if any(v not in adjacency[u] for u, v in zip(path, path[1:])):
    raise SystemExit("o caminho contém conexão inexistente")

distances = [-1] * (n + 1)
distances[1] = 0
queue = collections.deque([1])
while queue:
    u = queue.popleft()
    for v in adjacency[u]:
        if distances[v] == -1:
            distances[v] = distances[u] + 1
            queue.append(v)
if distances[n] != k - 1:
    raise SystemExit("o caminho não tem comprimento mínimo")
