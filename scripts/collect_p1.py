#!/usr/bin/env python3
"""Collect P1 results available through the C++ API."""

import argparse
import sys
from pathlib import Path

import grafos

from study_common import (
    OUTPUT,
    GraphSpec,
    inspect_graph,
    load_algorithm,
    select_graphs,
    specs_p1,
    write_jsonl,
)


def collect(spec: GraphSpec):
    info = inspect_graph(spec)
    n = info["vertices"]
    result = {
        "part": "P1",
        "graph": spec.path.name,
        "performance_data_source": "Dados/benchmark.csv",
        "summary": info,
        "degree_statistics": {
            "status": "unavailable",
            "reason": "src_does_not_expose_degree_statistics",
        },
    }

    bfs = load_algorithm(spec, grafos.BFS)
    bfs.run(1)
    result["components"] = {
        "status": "unavailable",
        "reason": "src_does_not_expose_component_query",
    }
    result["diameter_approximation"] = {
        "status": "pending_src_implementation",
        "reason": "waiting_for_approximate_diameter_api",
    }

    result["parents"] = {}
    for source in (1, 2, 3):
        if source > n:
            continue
        if source == 1:
            tree = bfs.get_tree()
            result["parents"][str(source)] = {
                str(target): tree[target] or None
                for target in (10, 20, 30)
                if target <= n
            }
        else:
            result["parents"][str(source)] = {
                "status": "pending_src_api",
                "reason": "source_specific_rerun_requires_state_reset_or_reuse",
            }

    result["pair_distances"] = {}
    for source, target in ((10, 20), (10, 30), (20, 30)):
        result["pair_distances"][f"{source}-{target}"] = {
            "status": "unavailable",
            "reason": "src_does_not_expose_pair_distance_query",
        }

    return result


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--graphs", nargs="+", help="Filtrar por arquivo")
    parser.add_argument("--output", type=Path, default=OUTPUT / "P1.jsonl")
    parser.add_argument("--overwrite", action="store_true")
    args = parser.parse_args()

    try:
        specs = select_graphs(specs_p1(), args.graphs)
    except ValueError as error:
        parser.error(str(error))
    if not specs:
        parser.error("nenhum grafo P1 encontrado em Dados/Entrada")
    if args.output.exists() and not args.overwrite:
        parser.error(f"{args.output} já existe; use --overwrite para substituí-lo")

    def records():
        for index, spec in enumerate(specs, start=1):
            print(f"[P1] {index}/{len(specs)} {spec.path.name}", file=sys.stderr)
            yield collect(spec)

    write_jsonl(args.output, records(), args.overwrite)
    print(f"Resultados P1: {args.output}", file=sys.stderr)


if __name__ == "__main__":
    main()
