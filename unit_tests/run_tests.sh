#!/usr/bin/env bash

set -u

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
TASKS_DIR="$ROOT_DIR/unit_tests/mains"
CASES_DIR="$ROOT_DIR/unit_tests/cases"
BUILD_DIR="$ROOT_DIR/unit_tests/.build"
VALIDATORS_DIR="$ROOT_DIR/unit_tests/validators"
TMP_DIR="$(mktemp -d)"
CXX="${CXX:-g++}"
FAILED=0
RAN=0

cleanup(){
    rm -rf "$TMP_DIR"
}
trap cleanup EXIT

mkdir -p "$BUILD_DIR"

run_task(){
    local task="$1"
    local source="$TASKS_DIR/$task/main.cpp"
    local input_dir="$CASES_DIR/$task/input"
    local expected_dir="$CASES_DIR/$task/expected"
    local binary="$BUILD_DIR/$task"
    local input expected actual case_name

    if [[ ! -f "$source" ]]; then
        echo "Erro: não existe main para a tarefa '$task': $source" >&2
        FAILED=$((FAILED + 1))
        return
    fi

    if ! "$CXX" -std=c++17 -O2 -Wall -Wextra -I"$ROOT_DIR/src" "$source" -o "$binary"; then
        echo "Falha ao compilar: $task" >&2
        FAILED=$((FAILED + 1))
        return
    fi

    if [[ ! -d "$input_dir" || ! -d "$expected_dir" ]]; then
        echo "Erro: faltam as pastas input/expected para '$task'." >&2
        FAILED=$((FAILED + 1))
        return
    fi

    local found_case=0
    for input in "$input_dir"/*.in; do
        [[ -f "$input" ]] || continue
        found_case=1
        case_name="$(basename "$input" .in)"
        expected="$expected_dir/$case_name.ans"
        actual="$TMP_DIR/$task-$case_name.out"

        if [[ ! -f "$expected" ]]; then
            echo "FALHOU $task/$case_name: falta saída esperada $expected"
            FAILED=$((FAILED + 1))
            continue
        fi

        if ! "$binary" < "$input" > "$actual"; then
            echo "FALHOU $task/$case_name: o programa terminou com erro"
            FAILED=$((FAILED + 1))
            continue
        fi

        RAN=$((RAN + 1))
        if [[ -f "$VALIDATORS_DIR/$task.py" ]]; then
            if python3 "$VALIDATORS_DIR/$task.py" "$input" "$expected" "$actual"; then
                echo "OK $task/$case_name"
            else
                echo "FALHOU $task/$case_name: saída inválida"
                FAILED=$((FAILED + 1))
            fi
        elif diff -u "$expected" "$actual"; then
            echo "OK $task/$case_name"
        else
            echo "FALHOU $task/$case_name: saída diferente"
            FAILED=$((FAILED + 1))
        fi
    done

    if [[ "$found_case" -eq 0 ]]; then
        echo "Sem casos de teste para '$task' (adicione arquivos .in e .ans)."
    fi
}

if [[ "$#" -gt 1 ]]; then
    echo "Uso: $0 [nome_da_tarefa]" >&2
    exit 2
fi

if [[ "$#" -eq 1 ]]; then
    run_task "$1"
else
    found_task=0
    for source in "$TASKS_DIR"/*/main.cpp; do
        [[ -f "$source" ]] || continue
        found_task=1
        run_task "$(basename "$(dirname "$source")")"
    done
    if [[ "$found_task" -eq 0 ]]; then
        echo "Erro: nenhum main de tarefa encontrado em $TASKS_DIR" >&2
        exit 2
    fi
fi

if [[ "$FAILED" -gt 0 ]]; then
    echo "Resultado: $FAILED falha(s), $RAN teste(s) executado(s)."
    exit 1
fi

echo "Resultado: $RAN teste(s) executado(s), sem falhas."
