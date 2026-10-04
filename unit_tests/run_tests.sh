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
    local input expected actual case_name input_file expected_file

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
    for input in "$input_dir"/*.in "$input_dir"/*.txt; do
        [[ -f "$input" ]] || continue
        found_case=1
        input_file="$(basename "$input")"
        if [[ "$input_file" == test_input*.txt ]]; then
            expected_file="${input_file/test_input/test_output}"
            case_name="${input_file%.txt}"
        elif [[ "$input_file" == *.in ]]; then
            case_name="${input_file%.in}"
            expected_file="$case_name.ans"
        else
            case_name="${input_file%.txt}"
            expected_file="$input_file"
        fi
        expected="$expected_dir/$expected_file"
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
        elif [[ "$task" == 1671 ]] && python3 - "$expected" "$actual" <<'PY'
import sys

with open(sys.argv[1], encoding="utf-8") as file:
    expected_tokens = file.read().split()
with open(sys.argv[2], encoding="utf-8") as file:
    actual_tokens = file.read().split()

if expected_tokens == actual_tokens:
    sys.exit(0)

limit = min(len(expected_tokens), len(actual_tokens))
for index in range(limit):
    if expected_tokens[index] != actual_tokens[index]:
        print(
            f"Primeira diferença no índice {index}: "
            f"esperado={expected_tokens[index]}, obtido={actual_tokens[index]}"
        )
        break
else:
    print(
        f"Quantidade de valores diferente: "
        f"esperado={len(expected_tokens)}, obtido={len(actual_tokens)}"
    )
sys.exit(1)
PY
        then
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
