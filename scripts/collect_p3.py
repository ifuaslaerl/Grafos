#!/usr/bin/env python3
"""Collect P3 results available through the C++ API."""

import argparse
import sys
from pathlib import Path

import grafos

from study_common import (
    OUTPUT,
    inspect_graph,
    load_algorithm,
    select_graphs,
    specs_p3,
    write_jsonl,
    distance,
)


def collect(spec):
    info = inspect_graph(spec)
    n, arcs = info["vertices"], info["arcs"]
    result = {
        "part": "P3",
        "graph": spec.path.name,
        "performance_data_source": "Dados/benchmark.csv",
        "summary": info,
    }

    result["bellman_ford_to_100"] = {
        "status": "pending_src_api",
        "reason": "multiple_source_runs_need_state_reset_or_reuse",
    }
    result["bellman_ford_negative_cycle"] = {
        "status": "pending_src_api",
        "reason": "multiple_source_runs_need_state_reset_or_reuse",
    }

    if not grafos.FloydWarshall(0).should_run(n, arcs):
        result["floyd_warshall_pairs"] = {
            "status": "skipped",
            "reason": "algorithm_memory_budget",
        }
        result["floyd_warshall_negative_cycle"] = None
    else:
        floyd = load_algorithm(spec, grafos.FloydWarshall)
        floyd.run()
        targets = (10, 20, 30, 40, 50)
        result["floyd_warshall_pairs"] = {
            str(source): {
                str(target): distance(floyd.get_dists(source)[target])
                for target in targets
                if target <= n
            }
            for source in targets
            if source <= n
        }
        result["floyd_warshall_negative_cycle"] = floyd.has_negative_cycle()
        del floyd

    return result


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--graphs", nargs="+", help="Filtrar por arquivo")
    parser.add_argument("--output", type=Path, default=OUTPUT / "P3.jsonl")
    parser.add_argument("--overwrite", action="store_true")
    args = parser.parse_args()

    try:
        specs = select_graphs(specs_p3(), args.graphs)
    except ValueError as error:
        parser.error(str(error))
    if not specs:
        parser.error("nenhum grafo P3 encontrado em Dados/Entrada")
    if args.output.exists() and not args.overwrite:
        parser.error(f"{args.output} já existe; use --overwrite para substituí-lo")

    def records():
        for index, spec in enumerate(specs, start=1):
            print(f"[P3] {index}/{len(specs)} {spec.path.name}", file=sys.stderr)
            yield collect(spec)

    write_jsonl(args.output, records(), args.overwrite)
    print(f"Resultados P3: {args.output}", file=sys.stderr)


if __name__ == "__main__":
    main()
