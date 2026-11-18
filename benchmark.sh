#!/usr/bin/env bash

set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
make -C "$ROOT_DIR" pipe

if ! ulimit -s unlimited; then
    echo "Erro: nao foi possivel definir a pilha como ilimitada." >&2
    exit 1
fi

cd "$ROOT_DIR"
exec "$ROOT_DIR/pipe" "$@"
