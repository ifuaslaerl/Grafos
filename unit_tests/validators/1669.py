import sys

input_path, expected_path, output_path = sys.argv[1:4]

with open(input_path, encoding="utf-8") as file:
    n, m = map(int, file.readline().split())
    roads = set()
    for _ in range(m):
        u, v = map(int, file.readline().split())
        roads.add((min(u, v), max(u, v)))

with open(output_path, encoding="utf-8") as file:
    tokens = file.read().split()

with open(expected_path, encoding="utf-8") as file:
    expected_tokens = file.read().split()

if not expected_tokens:
    raise SystemExit("gabarito vazio")

expected_impossible = expected_tokens[0] == "IMPOSSIBLE"
output_impossible = tokens == ["IMPOSSIBLE"]
if expected_impossible != output_impossible:
    raise SystemExit("a existência de um ciclo difere do gabarito")

if tokens == ["IMPOSSIBLE"]:
    sys.exit(0)

if not tokens:
    raise SystemExit("saída vazia")

k = int(tokens[0])
cycle = list(map(int, tokens[1:]))
if len(cycle) != k or k < 4:
    raise SystemExit("tamanho de ciclo inválido")
if cycle[0] != cycle[-1]:
    raise SystemExit("o ciclo não termina na cidade inicial")
if len(set(cycle[:-1])) != k - 1:
    raise SystemExit("o ciclo repete uma cidade intermediária")
if any(vertex < 1 or vertex > n for vertex in cycle):
    raise SystemExit("o ciclo contém cidade fora do intervalo")
if any(
    (min(u, v), max(u, v)) not in roads
    for u, v in zip(cycle, cycle[1:])
):
    raise SystemExit("o ciclo contém estrada inexistente")
