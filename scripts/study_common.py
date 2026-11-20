from __future__ import annotations

import json
import math
import sys
from dataclasses import dataclass
from pathlib import Path

import grafos


ROOT = Path(__file__).resolve().parents[1]
INPUT = ROOT / "Dados" / "Entrada"
OUTPUT = ROOT / "Dados" / "EstudosColetados"
CHUNK_SIZE = 8192


@dataclass(frozen=True)
class GraphSpec:
    path: Path
    weighted: bool
    directed: bool
    one_field_header: bool = False


def specs_p1() -> list[GraphSpec]:
    return [
        GraphSpec(path, False, False)
        for path in sorted(INPUT.glob("grafo_[0-9]*.txt"))
        if path.stem.removeprefix("grafo_").isdigit()
    ]


def specs_p2() -> list[GraphSpec]:
    specs = [
        GraphSpec(path, True, False)
        for path in sorted(INPUT.glob("grafo_W_*.txt"))
    ]
    network = INPUT / "rede_colaboracao.txt"
    if network.is_file():
        specs.append(GraphSpec(network, True, False))
    return specs


def specs_p3() -> list[GraphSpec]:
    return [
        GraphSpec(path, True, True, True)
        for path in sorted(INPUT.glob("grafo_DP_*.txt"))
    ]


def header(spec: GraphSpec) -> tuple[int, int | None]:
    with spec.path.open(encoding="utf-8") as source:
        fields = source.readline().split()
    expected = 1 if spec.one_field_header else 2
    if len(fields) != expected:
        raise ValueError(f"Cabeçalho inválido em {spec.path}: {fields}")
    n = int(fields[0])
    m = None if spec.one_field_header else int(fields[1])
    if n < 1 or (m is not None and m < 0):
        raise ValueError(f"Dimensões inválidas em {spec.path}: {fields}")
    return n, m


def iter_edges(spec: GraphSpec):
    with spec.path.open(encoding="utf-8") as source:
        next(source)
        for line_number, line in enumerate(source, start=2):
            fields = line.split()
            if not fields:
                continue
            expected = 3 if spec.weighted else 2
            if len(fields) != expected:
                raise ValueError(
                    f"Aresta inválida em {spec.path}:{line_number}: "
                    f"{line.rstrip()}"
                )
            yield (
                int(fields[0]),
                int(fields[1]),
                float(fields[2]) if spec.weighted else 1.0,
            )


def inspect_graph(spec: GraphSpec) -> dict:
    n, declared_m = header(spec)
    m = 0
    min_weight = math.inf
    max_weight = -math.inf
    for u, v, weight in iter_edges(spec):
        if min(u, v) < 1 or max(u, v) > n:
            raise ValueError(f"Vértice fora de [1, N] em {spec.path}: {u}, {v}")
        m += 1
        min_weight = min(min_weight, weight)
        max_weight = max(max_weight, weight)
    if declared_m is not None and m != declared_m:
        raise ValueError(
            f"{spec.path}: cabeçalho declara {declared_m} arestas, "
            f"mas foram lidas {m}"
        )

    result = {
        "vertices": n,
        "edges": m,
        "arcs": m if spec.directed else 2 * m,
        "weighted": spec.weighted,
        "directed": spec.directed,
        "min_weight": min_weight if spec.weighted else None,
        "max_weight": max_weight if spec.weighted else None,
    }
    return result


def load_algorithm(spec: GraphSpec, algorithm_type):
    n, _ = header(spec)
    algorithm = algorithm_type(n)
    unweighted_algorithm = algorithm_type in (grafos.BFS, grafos.DFS)
    batch = []
    m = 0

    def flush():
        if batch:
            algorithm.add_edges(batch)
            batch.clear()

    for u, v, weight in iter_edges(spec):
        value = 1 if unweighted_algorithm or not spec.weighted else weight
        batch.append((u, v, value))
        if not spec.directed:
            batch.append((v, u, value))
        m += 1
        if len(batch) >= CHUNK_SIZE:
            flush()
    flush()

    _, declared_m = header(spec)
    if declared_m is not None and m != declared_m:
        raise ValueError(
            f"{spec.path}: cabeçalho declara {declared_m} arestas, "
            f"mas foram lidas {m}"
        )
    return algorithm


def distance(value):
    if isinstance(value, float):
        if not math.isfinite(value) or value >= sys.float_info.max:
            return None
    elif value >= 2_147_483_647:
        return None
    return value


def select_graphs(specs: list[GraphSpec], names: list[str] | None):
    if not names:
        return specs
    by_name = {spec.path.name: spec for spec in specs}
    missing = set(names) - by_name.keys()
    if missing:
        raise ValueError(f"Arquivos não encontrados: {sorted(missing)}")
    return [by_name[name] for name in names]


def write_jsonl(path: Path, records, overwrite: bool):
    if path.exists() and not overwrite:
        raise FileExistsError(
            f"{path} já existe; use --overwrite para substituí-lo."
        )
    path.parent.mkdir(parents=True, exist_ok=True)
    with path.open("w", encoding="utf-8") as output:
        for record in records:
            output.write(json.dumps(record, ensure_ascii=False) + "\n")
            output.flush()
