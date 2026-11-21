#!/usr/bin/env python3
"""Collect P2 shortest-path and Dijkstra performance results."""

import argparse
import csv
import math
import random
import sys
import time
from pathlib import Path

import grafos

from study_common import (
    INPUT,
    OUTPUT,
    distance,
    inspect_graph,
    load_algorithm,
    select_graphs,
    specs_p2,
    write_jsonl,
)


OPERATION_BUDGET = 1_500_000_000
SOURCE = "Edsger W. Dijkstra"
TARGETS = (
    "Alan M. Turing",
    "J. B. Kruskal",
    "Jon M. Kleinberg",
    "Éva Tardos",
    "Daniel R. Figueiredo",
)


def load_benchmark(graph_names):
    path = INPUT.parent / "benchmark.csv"
    results = {}
    if not path.is_file():
        return results

    names = {
        "Djikstra<double>": "Dijkstra_heap",
        "VectorDjikstra<double>": "Dijkstra_vector",
    }
    with path.open(encoding="utf-8", newline="") as source:
        for row in csv.DictReader(source):
            graph = row["graph"]
            algorithm = names.get(row["algorithm"])
            if graph not in graph_names or algorithm is None:
                continue
            if row["direction"] != "undirected":
                continue
            if row.get("measurement", "full_graph") != "full_graph":
                continue

            key = graph, algorithm
            result = results.setdefault(key, {"runs": 0, "times": []})
            if row["status"] == "ok" and row["run_ns"]:
                result["runs"] += 1
                result["times"].append(float(row["run_ns"]) / 1e9)
            else:
                result["status"] = row["status"]

    for result in results.values():
        if result["times"]:
            result["status"] = "ok"
            result["mean_seconds"] = math.fsum(result["times"]) / len(
                result["times"]
            )
        result.pop("times")
    return results


def load_sample_benchmark(graph_names, sample_size, seed):
    path = INPUT.parent / "benchmark.csv"
    results = {}
    if not path.is_file():
        return results

    names = {
        "Djikstra<double>": "Dijkstra_heap",
        "VectorDjikstra<double>": "Dijkstra_vector",
    }
    with path.open(encoding="utf-8", newline="") as source:
        for row in csv.DictReader(source):
            algorithm = names.get(row["algorithm"])
            if (
                row["graph"] not in graph_names
                or algorithm is None
                or row["direction"] != "undirected"
                or row.get("measurement") != "single_source"
                or row.get("sample_size") != str(sample_size)
                or row.get("sample_seed") != str(seed)
            ):
                continue

            key = row["graph"], algorithm
            result = results.setdefault(
                key,
                {
                    "runs": 0,
                    "requested_samples": sample_size,
                    "sources": [],
                    "times": [],
                },
            )
            if row["status"] == "ok" and row["run_ns"] and row["source"]:
                result["runs"] += 1
                result["sources"].append(int(row["source"]))
                result["times"].append(float(row["run_ns"]) / 1e9)
            else:
                result["status"] = row["status"]

    for result in results.values():
        if result["times"]:
            result["status"] = (
                "ok"
                if result["runs"] == sample_size
                else "sample_limited"
            )
            result["mean_seconds"] = math.fsum(result["times"]) / len(
                result["times"]
            )
            result["total_seconds"] = math.fsum(result["times"])
            result["data_source"] = "Dados/benchmark.csv"
        result.pop("times")
    return results


def benchmark_summary(benchmark, graph_name):
    result = {}
    for name in ("Dijkstra_heap", "Dijkstra_vector"):
        sample = benchmark.get((graph_name, name))
        if sample is None:
            result[name] = {"status": "missing"}
            continue
        result[name] = {
            "status": sample["status"],
            "runs": sample["runs"],
        }
        if "mean_seconds" in sample:
            result[name]["mean_seconds"] = sample["mean_seconds"]
    return result


def reconstruct_path(s, t, dist, pai):
    if t >= len(dist) or distance(dist[t]) is None:
        return []

    path = []
    v = t
    for i in range(len(pai)):
        if v <= 0 or v >= len(pai):
            return []
        path.append(v)
        if v == s:
            path.reverse()
            return path
        p = pai[v]
        if p <= 0 or p == v:
            return []
        v = p
    return []


def collect_paths(algorithm, s, targets):
    targets = [t for t in targets if t < len(algorithm.get_tree())]
    if not targets:
        return {"status": "targets_out_of_range"}

    algorithm.get_path(s, targets[0])
    dist = algorithm.get_dists()
    pai = algorithm.get_tree()
    result = {
        str(t): {
            "distance": distance(dist[t]),
            "path": reconstruct_path(s, t, dist, pai),
        }
        for t in targets
    }
    return {"status": "ok", "source": s, "targets": result}


def researcher_vertices(path, wanted):
    found = {}
    with path.open(encoding="utf-8-sig", newline="") as source:
        for row in csv.reader(source):
            if len(row) < 2:
                raise ValueError(f"Vértice inválido em {path}: {row}")
            name = ",".join(row[1:])
            if name in wanted:
                found[name] = int(row[0])
                if len(found) == len(wanted):
                    break

    missing = wanted - found.keys()
    if missing:
        raise ValueError(
            f"Nomes não encontrados em {path}: {sorted(missing)}"
        )
    return found


def researcher_path_names(path, ids):
    if not ids:
        return {}

    found = {}
    with path.open(encoding="utf-8-sig", newline="") as source:
        for row in csv.reader(source):
            if len(row) < 2:
                raise ValueError(f"Vértice inválido em {path}: {row}")
            v = int(row[0])
            if v in ids:
                found[v] = ",".join(row[1:])
                if len(found) == len(ids):
                    break
    return found


def path_result(algorithm, s, targets):
    result = collect_paths(algorithm, s, [v for v, _ in targets])
    if result["status"] != "ok":
        return result

    for v, name in targets:
        item = result["targets"][str(v)]
        if name is not None:
            item["name"] = name
    return result


def estimated_operations(algorithm_type, n, m):
    if algorithm_type is grafos.Dijkstra:
        return n * int(math.log2(n)) + m
    return n * n + m


def timing_result(
    algorithm,
    algorithm_type,
    algorithm_name,
    graph_name,
    n,
    m,
    sources,
    sample_size,
):
    if not algorithm.should_run(n, m):
        print(
            f"[P2] {graph_name}: {algorithm_name} ignorado por "
            "should_run() (orçamento de memória).",
            file=sys.stderr,
        )
        return {
            "status": "skipped_memory_budget",
            "requested_samples": sample_size,
            "runs": 0,
        }

    estimate = max(1, estimated_operations(algorithm_type, n, m))
    run_count = min(len(sources), max(1, OPERATION_BUDGET // estimate))
    selected = sources[:run_count]
    if run_count < sample_size:
        print(
            f"[P2] {graph_name}: amostra limitada pelo orçamento; serão "
            f"executadas {run_count} de {sample_size} origens.",
            file=sys.stderr,
        )
    times = []
    for index, s in enumerate(selected, start=1):
        print(
            f"[P2] {graph_name}: {algorithm_name}, amostra "
            f"{index}/{run_count}, origem {s}: iniciando cálculo das "
            "distâncias para todos os vértices.",
            file=sys.stderr,
        )
        start = time.perf_counter()
        algorithm.get_dist(s, s)
        elapsed = time.perf_counter() - start
        times.append(elapsed)
        print(
            f"[P2] {graph_name}: {algorithm_name}, amostra "
            f"{index}/{run_count}, origem {s}: concluída em "
            f"{elapsed:.6f} s.",
            file=sys.stderr,
        )

    return {
        "status": "ok" if run_count == sample_size else "sample_limited",
        "requested_samples": sample_size,
        "runs": run_count,
        "sources": selected,
        "mean_seconds": math.fsum(times) / run_count,
        "total_seconds": math.fsum(times),
        "estimated_operations_per_run": estimate,
        "data_source": "scripts/collect_p2.py",
    }


def collect(spec, sample_size, rng, benchmark, sampled_benchmark):
    print(
        f"[P2] {spec.path.name}: inspecionando arquivo de entrada.",
        file=sys.stderr,
    )
    info = inspect_graph(spec)
    n, m = info["vertices"], info["arcs"]
    print(
        f"[P2] {spec.path.name}: grafo com {n} vértices, "
        f"{info['edges']} arestas e {m} arcos; peso mínimo "
        f"{info['min_weight']}, máximo {info['max_weight']}.",
        file=sys.stderr,
    )
    result = {
        "part": "P2",
        "graph": spec.path.name,
        "performance_data_source": "Dados/benchmark.csv",
        "summary": info,
        "benchmark_csv_full_run": benchmark_summary(
            benchmark, spec.path.name
        ),
    }

    if info["min_weight"] < 0:
        print(
            f"[P2] {spec.path.name}: pesos negativos; Dijkstra não será "
            "executado.",
            file=sys.stderr,
        )
        result["source_10_paths"] = {
            "status": "unsupported_negative_weights"
        }
        result["Dijkstra_heap"] = {
            "status": "unsupported_negative_weights"
        }
        result["Dijkstra_vector"] = {
            "status": "unsupported_negative_weights"
        }
        return result

    if spec.path.name == "rede_colaboracao.txt":
        path = INPUT.parent / "rede_colaboracao_vertices.txt"
        wanted = {SOURCE, *TARGETS}
        ids = researcher_vertices(path, wanted)
        s = ids[SOURCE]
        targets = [(ids[name], name) for name in TARGETS]
        probe = grafos.Dijkstra(0)
        if probe.should_run(n, m):
            print(
                f"[P2] {spec.path.name}: carregando Dijkstra heap para "
                f"calcular caminhos de {SOURCE} (vértice {s}) até "
                f"{len(targets)} pesquisadores.",
                file=sys.stderr,
            )
            algorithm = load_algorithm(spec, grafos.Dijkstra)
            paths = path_result(algorithm, s, targets)
            if paths["status"] == "ok":
                path_ids = {
                    v
                    for target in paths["targets"].values()
                    for v in target["path"]
                }
                names = researcher_path_names(path, path_ids)
                paths["source_name"] = SOURCE
                for target in paths["targets"].values():
                    target["path_names"] = [
                        names[v] for v in target["path"] if v in names
                    ]
            print(
                f"[P2] {spec.path.name}: caminhos dos pesquisadores "
                f"finalizados (status={paths['status']}).",
                file=sys.stderr,
            )
            result["researcher_paths"] = paths
        else:
            print(
                f"[P2] {spec.path.name}: caminhos dos pesquisadores "
                "ignorados por should_run().",
                file=sys.stderr,
            )
            result["researcher_paths"] = {
                "status": "skipped_memory_budget"
            }
        return result

    result["researcher_paths"] = {"status": "not_applicable"}
    probe = grafos.Dijkstra(0)
    if probe.should_run(n, m):
        print(
            f"[P2] {spec.path.name}: should_run() autorizou Dijkstra heap; "
            "carregando o grafo.",
            file=sys.stderr,
        )
        heap = load_algorithm(spec, grafos.Dijkstra)
    else:
        heap = None
    if n < 10:
        result["source_10_paths"] = {"status": "source_out_of_range"}
    elif heap is None:
        result["source_10_paths"] = {
            "status": "skipped_memory_budget"
        }
    else:
        if n < 10:
            result["source_10_paths"] = {"status": "source_out_of_range"}
            print(
                f"[P2] {spec.path.name}: origem 10 fora do intervalo "
                f"de 1 a {n}; consulta não executada.",
                file=sys.stderr,
            )
        elif heap is None:
            result["source_10_paths"] = {
                "status": "skipped_memory_budget"
            }
            print(
                f"[P2] {spec.path.name}: caminhos da origem 10 ignorados "
                "por should_run().",
                file=sys.stderr,
            )
        else:
            print(
                f"[P2] {spec.path.name}: calculando caminhos mínimos da "
                "origem 10 aos vértices 20, 30, 40, 50 e 60.",
                file=sys.stderr,
            )
            result["source_10_paths"] = path_result(
                heap, 10, [(t, None) for t in (20, 30, 40, 50, 60)]
            )
            print(
                f"[P2] {spec.path.name}: caminhos da origem 10 concluídos "
                f"(status={result['source_10_paths']['status']}).",
                file=sys.stderr,
            )

    sources = rng.sample(range(1, n + 1), min(sample_size, n))
    if heap is None:
        print(
            f"[P2] {spec.path.name}: Dijkstra heap ignorado por "
            "should_run().",
            file=sys.stderr,
        )
        result["Dijkstra_heap"] = {
            "status": "skipped_memory_budget",
            "requested_samples": sample_size,
            "runs": 0,
        }
    else:
        heap_sample = sampled_benchmark.get(
            (spec.path.name, "Dijkstra_heap")
        )
        if heap_sample is not None:
            print(
                f"[P2] {spec.path.name}: usando amostras do benchmark.csv "
                f"para Dijkstra heap ({heap_sample['runs']}/"
                f"{heap_sample['requested_samples']}, média "
                f"{heap_sample.get('mean_seconds', 0):.6f} s).",
                file=sys.stderr,
            )
            result["Dijkstra_heap"] = heap_sample
        else:
            result["Dijkstra_heap"] = timing_result(
                heap,
                grafos.Dijkstra,
                "Dijkstra heap",
                spec.path.name,
                n,
                m,
                sources,
                sample_size,
            )

    vector_probe = grafos.VectorDijkstra(0)
    vector_sample = sampled_benchmark.get(
        (spec.path.name, "Dijkstra_vector")
    )
    if vector_sample is not None:
        print(
            f"[P2] {spec.path.name}: usando amostras do benchmark.csv "
            f"para Dijkstra vetor ({vector_sample['runs']}/"
            f"{vector_sample['requested_samples']}, status "
            f"{vector_sample['status']}).",
            file=sys.stderr,
        )
        result["Dijkstra_vector"] = vector_sample
    elif not vector_probe.should_run(n, m):
        print(
            f"[P2] {spec.path.name}: Dijkstra vetor ignorado por "
            "should_run() (orçamento de memória).",
            file=sys.stderr,
        )
        result["Dijkstra_vector"] = {
            "status": "skipped_memory_budget",
            "requested_samples": sample_size,
            "runs": 0,
        }
    else:
        vector = load_algorithm(spec, grafos.VectorDijkstra)
        result["Dijkstra_vector"] = timing_result(
            vector,
            grafos.VectorDijkstra,
            "Dijkstra vetor",
            spec.path.name,
            n,
            m,
            sources,
            sample_size,
        )

    return result


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--graphs", nargs="+", help="Filtrar por arquivo")
    parser.add_argument(
        "--output",
        type=Path,
        help="Arquivo JSONL (padrão: Dados/EstudosColetados/P2.jsonl)",
    )
    overwrite_group = parser.add_mutually_exclusive_group()
    overwrite_group.add_argument(
        "--overwrite",
        action="store_true",
        dest="overwrite",
        help="Substituir a saída existente (padrão)",
    )
    overwrite_group.add_argument(
        "--no-overwrite",
        action="store_false",
        dest="overwrite",
        help="Falhar se a saída já existir",
    )
    parser.set_defaults(overwrite=True)
    parser.add_argument("--samples", type=int, default=100)
    parser.add_argument("--seed", type=int, default=42)
    args = parser.parse_args()
    output = args.output or OUTPUT / "P2.jsonl"
    overwrite = args.overwrite
    if args.samples < 1:
        parser.error("--samples deve ser pelo menos 1")

    try:
        specs = select_graphs(specs_p2(), args.graphs)
    except ValueError as error:
        parser.error(str(error))
    if not specs:
        parser.error("nenhum grafo P2 encontrado em Dados/Entrada")
    if output.exists() and not overwrite:
        parser.error(f"{output} já existe; use --overwrite para substituí-lo")

    print(
        f"[P2] Lendo benchmark CSV para {len(specs)} grafos selecionados.",
        file=sys.stderr,
    )
    benchmark = load_benchmark({spec.path.name for spec in specs})
    sampled_benchmark = load_sample_benchmark(
        {spec.path.name for spec in specs}, args.samples, args.seed
    )
    print(
        f"[P2] CSV carregado: {len(benchmark)} conjuntos de medições "
        "de grafo completo e "
        f"{len(sampled_benchmark)} conjuntos de amostras compatíveis.",
        file=sys.stderr,
    )
    rng = random.Random(args.seed)

    def records():
        for index, spec in enumerate(specs, start=1):
            print(f"[P2] {index}/{len(specs)} {spec.path.name}", file=sys.stderr)
            yield collect(
                spec, args.samples, rng, benchmark, sampled_benchmark
            )

    write_jsonl(output, records(), overwrite)
    print(f"Resultados P2: {output}", file=sys.stderr)


if __name__ == "__main__":
    main()
