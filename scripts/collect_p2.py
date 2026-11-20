#!/usr/bin/env python3
"""Collect P2 results available through the C++ API."""

import argparse
import sys
from pathlib import Path

from study_common import (
    OUTPUT,
    inspect_graph,
    select_graphs,
    specs_p2,
    write_jsonl,
)


def collect(spec):
    info = inspect_graph(spec)
    result = {
        "part": "P2",
        "graph": spec.path.name,
        "performance_data_source": "Dados/benchmark.csv",
        "summary": info,
    }

    result["source_10_paths"] = {
        "status": "unavailable",
        "reason": "src_does_not_expose_path_query",
    }
    if spec.path.name == "rede_colaboracao.txt":
        result["researcher_paths"] = {
            "status": "unavailable",
            "reason": "src_does_not_expose_path_query",
        }
    result["Dijkstra_vector"] = {
        "status": "unavailable",
        "reason": "src_does_not_implement_vector_dijkstra",
    }
    return result


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--graphs", nargs="+", help="Filtrar por arquivo")
    parser.add_argument("--output", type=Path, default=OUTPUT / "P2.jsonl")
    parser.add_argument("--overwrite", action="store_true")
    args = parser.parse_args()

    try:
        specs = select_graphs(specs_p2(), args.graphs)
    except ValueError as error:
        parser.error(str(error))
    if not specs:
        parser.error("nenhum grafo P2 encontrado em Dados/Entrada")
    if args.output.exists() and not args.overwrite:
        parser.error(f"{args.output} já existe; use --overwrite para substituí-lo")

    def records():
        for index, spec in enumerate(specs, start=1):
            print(f"[P2] {index}/{len(specs)} {spec.path.name}", file=sys.stderr)
            yield collect(spec)

    write_jsonl(args.output, records(), args.overwrite)
    print(f"Resultados P2: {args.output}", file=sys.stderr)


if __name__ == "__main__":
    main()
