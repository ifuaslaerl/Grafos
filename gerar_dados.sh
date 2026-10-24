#!/usr/bin/env bash

set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
INPUT_DIR="$ROOT_DIR/Dados/Entrada"
modo="todos"

usage(){
    echo "Uso: $0 [--graficos-only]"
    echo "  --graficos-only  Regenera os gráficos dos dados JSON existentes, sem processar grafos."
}

case "${1:-}" in
    "")
        ;;
    --graficos-only)
        modo="graficos"
        ;;
    -h|--help)
        usage
        exit 0
        ;;
    *)
        usage >&2
        exit 2
        ;;
esac

if (($# > 1)); then
    usage >&2
    exit 2
fi

python="$ROOT_DIR/.venv/bin/python"
if [[ ! -x "$python" ]]; then
    python=python3
fi

if [[ "$modo" == "graficos" ]]; then
    shopt -s nullglob
    json_files=("$ROOT_DIR"/Dados/JSON/*.json)
    if ((${#json_files[@]} == 0)); then
        echo "Erro: nenhum JSON encontrado em Dados/JSON/. Gere os dados antes dos gráficos." >&2
        exit 1
    fi
    exec "$python" "$ROOT_DIR/Analysers/main.py"
fi

shopt -s nullglob
entradas=(
    "$INPUT_DIR"/grafo_[1-6].txt
    "$INPUT_DIR"/grafo_W_[3-5].txt
    "$INPUT_DIR"/rede_colaboracao.txt
)
if ((${#entradas[@]} != 10)); then
    echo "Erro: esperadas 6 entradas grafo_[1-6], 3 grafo_W_[3-5] e rede_colaboracao em Dados/Entrada/." >&2
    exit 1
fi

make -C "$ROOT_DIR"
mkdir -p "$ROOT_DIR/Dados/JSON" "$ROOT_DIR/Dados/Grafos" \
    "$ROOT_DIR/Dados/Graus" "$ROOT_DIR/Dados/Graficos"

for entrada in "${entradas[@]}"; do
    nome="$(basename "$entrada" .txt)"
    id="$nome"
    if [[ "$nome" == grafo_* ]]; then
        id="${nome#grafo_}"
    fi
    weighted_args=()
    case "$nome" in
        grafo_W_*|rede_colaboracao) weighted_args=(--weighted) ;;
    esac

    echo "Processando $nome..."
    "$ROOT_DIR/export_json" "${weighted_args[@]}" < "$entrada" > "$ROOT_DIR/Dados/JSON/grafo_$id.json"
    "$ROOT_DIR/export_grafo" bfs "${weighted_args[@]}" < "$entrada" > "$ROOT_DIR/Dados/Grafos/bfs_$id.csv"
    "$ROOT_DIR/export_grafo" dfs "${weighted_args[@]}" < "$entrada" > "$ROOT_DIR/Dados/Grafos/dfs_$id.csv"
    "$ROOT_DIR/export_grafo" dijkstra "${weighted_args[@]}" < "$entrada" > "$ROOT_DIR/Dados/Grafos/dijkstra_$id.csv"
    "$ROOT_DIR/export_graus" "${weighted_args[@]}" < "$entrada" > "$ROOT_DIR/Dados/Graus/grafo_$id.csv"
    done

    "$python" "$ROOT_DIR/Analysers/main.py"
