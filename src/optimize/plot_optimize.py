#!/usr/bin/env python3
"""Gráficos do relatório a partir de resultados/*.csv (gerados por bench_optimize).

Uso: python3 plot_optimize.py [pasta_resultados] [pasta_graficos]
"""

import csv
import statistics
import sys
from collections import defaultdict
from pathlib import Path

import matplotlib

matplotlib.use("Agg")
import matplotlib.pyplot as plt  # noqa: E402
from matplotlib.ticker import FuncFormatter  # noqa: E402

BASE = Path(__file__).resolve().parent
RES = Path(sys.argv[1]) if len(sys.argv) > 1 else BASE / "resultados"
OUT = Path(sys.argv[2]) if len(sys.argv) > 2 else BASE / "graficos"
OUT.mkdir(parents=True, exist_ok=True)

# Cor segue a entidade (mesma cor em todos os gráficos).
INK = "#0b0b0b"
INK2 = "#52514e"
GRID = "#e4e3df"
COR = {
    "original": "#8a8984",  # neutro: linha de base
    "double": "#2a78d6",
    "float": "#eb6834",
    "float16": "#1baf7a",
}
ROTULO = {
    "original": "Original (sequencial, double)",
    "double": "Paralelo double (8 B)",
    "float": "Paralelo float (4 B)",
    "float16": "Paralelo float16 (2 B)",
}
TIPOS = ["double", "float", "float16"]
BYTES_TIPO = {"double": 8, "float": 4, "float16": 2}
# Rampa sequencial (azul) para várias curvas ordenadas por tamanho.
RAMPA = ["#a9c8ef", "#7eaee8", "#5594de", "#2a78d6", "#1d5ea9", "#13437a"]

plt.rcParams.update({
    "font.size": 10,
    "axes.edgecolor": INK2,
    "axes.labelcolor": INK,
    "axes.titlesize": 12,
    "axes.titleweight": "bold",
    "axes.spines.top": False,
    "axes.spines.right": False,
    "axes.grid": True,
    "grid.color": GRID,
    "grid.linewidth": 0.8,
    "xtick.color": INK2,
    "ytick.color": INK2,
    "legend.frameon": False,
    "figure.dpi": 150,
    "savefig.bbox": "tight",
    "lines.linewidth": 2,
})


def ler(nome):
    caminho = RES / nome
    if not caminho.exists():
        return []
    with open(caminho, newline="", encoding="utf-8") as f:
        return list(csv.DictReader(f))


def medianas(linhas, chave):
    grupos = defaultdict(list)
    for linha in linhas:
        grupos[chave(linha)].append(float(linha["tempo_ms"]))
    return {k: statistics.median(v) for k, v in grupos.items()}


def fmt_ms(v, _=None):
    if v >= 1000:
        return f"{v / 1000:g} s"
    return f"{v:g} ms"


def salvar(fig, nome):
    fig.savefig(OUT / nome)
    plt.close(fig)
    print("gerado", OUT / nome)


def nome_grafo(g):
    g = g.replace(".txt", "")
    if g.startswith("random_n"):
        n, m = g[len("random_n"):].split("_m")
        return f"aleatório\nn={int(n):,}".replace(",", ".")
    return g.replace("_", " ")


# ---------------------------------------------------------------- Floyd-Warshall
fw = ler("fw_tempos.csv")
if fw:
    hw = max(int(r["threads"]) for r in fw)
    med = medianas(fw, lambda r: (r["algoritmo"], r["tipo"],
                                  int(r["threads"]), int(r["n"])))
    ns = sorted({int(r["n"]) for r in fw})

    # 1) Tempo x n
    fig, ax = plt.subplots(figsize=(7, 4.2))
    series = [
        ("original", ("FloydWarshall_original", "double", 1)),
        ("double", ("FloydWarshall_paralelo", "double", hw)),
        ("float", ("FloydWarshall_paralelo", "float", hw)),
        ("float16", ("FloydWarshall_paralelo", "float16", hw)),
    ]
    for chave, (alg, tipo, t) in series:
        xs = [n for n in ns if (alg, tipo, t, n) in med]
        if not xs:
            continue
        ys = [med[(alg, tipo, t, n)] for n in xs]
        rot = ROTULO[chave] + ("" if chave == "original" else f", {t} threads")
        ax.plot(xs, ys, marker="o", markersize=5, color=COR[chave], label=rot)
    ax.set_yscale("log")
    ax.yaxis.set_major_formatter(FuncFormatter(fmt_ms))
    ax.set_xlabel("número de vértices n")
    ax.set_ylabel("tempo (mediana)")
    ax.set_title("Floyd-Warshall: tempo de execução")
    ax.legend(loc="upper left")
    ax.text(1.0, -0.2, "O original só roda até n≈1144 (limite de 1,5·10⁹ iterações).",
            transform=ax.transAxes, ha="right", fontsize=8, color=INK2)
    salvar(fig, "01_fw_tempo_vs_n.png")

    # 2) Speedup x threads (double), uma curva por n
    fig, ax = plt.subplots(figsize=(7, 4.2))
    threads = sorted({int(r["threads"]) for r in fw
                      if r["algoritmo"] == "FloydWarshall_paralelo"
                      and r["tipo"] == "double"})
    ax.plot(threads, threads, ls="--", lw=1, color=INK2, label="ideal (linear)")
    for cor, n in zip(RAMPA[-len(ns):], ns):
        base = med.get(("FloydWarshall_paralelo", "double", 1, n))
        if not base:
            continue
        ys = [base / med[("FloydWarshall_paralelo", "double", t, n)] for t in threads]
        ax.plot(threads, ys, marker="o", markersize=5, color=cor, label=f"n = {n}")
    ax.set_xticks(threads)
    ax.set_xlabel("threads")
    ax.set_ylabel("speedup em relação a 1 thread")
    ax.set_title("Floyd-Warshall paralelo (double): escalabilidade")
    ax.legend(loc="upper left", ncol=2)
    salvar(fig, "02_fw_speedup_threads.png")

    # 3) Ganho total sobre o original, por n
    comuns = [n for n in ns if ("FloydWarshall_original", "double", 1, n) in med]
    if comuns:
        fig, ax = plt.subplots(figsize=(7, 4.2))
        largura = 0.8 / 3
        for i, tipo in enumerate(TIPOS):
            ys = []
            for n in comuns:
                p = med.get(("FloydWarshall_paralelo", tipo, hw, n))
                ys.append(med[("FloydWarshall_original", "double", 1, n)] / p if p else 0)
            xs = [j + (i - 1) * largura for j in range(len(comuns))]
            barras = ax.bar(xs, ys, largura * 0.92, color=COR[tipo], label=ROTULO[tipo])
            ax.bar_label(barras, fmt="%.0f×", fontsize=8, color=INK2, padding=2)
        ax.set_xticks(range(len(comuns)), [f"n = {n}" for n in comuns])
        ax.set_ylabel("vezes mais rápido que o original")
        ax.set_title(f"Floyd-Warshall: ganho sobre o original ({hw} threads)")
        ax.legend(loc="upper left")
        ax.grid(axis="x", visible=False)
        salvar(fig, "03_fw_ganho_vs_original.png")

    # 4) Tipos: tempo com 1 thread e com todas
    nmax = max(ns)
    fig, ax = plt.subplots(figsize=(7, 4.2))
    largura = 0.8 / 3
    grupos = [1, hw]
    for i, tipo in enumerate(TIPOS):
        ys = [med.get(("FloydWarshall_paralelo", tipo, t, nmax), 0) for t in grupos]
        xs = [j + (i - 1) * largura for j in range(len(grupos))]
        barras = ax.bar(xs, ys, largura * 0.92, color=COR[tipo], label=ROTULO[tipo])
        ax.bar_label(barras, labels=[fmt_ms(y) for y in ys], fontsize=8,
                     color=INK2, padding=2)
    ax.set_xticks(range(len(grupos)), [f"{t} thread" + ("s" if t > 1 else "") for t in grupos])
    ax.yaxis.set_major_formatter(FuncFormatter(fmt_ms))
    ax.set_ylabel("tempo (mediana)")
    ax.set_title(f"Floyd-Warshall n = {nmax}: efeito do tipo numérico")
    ax.legend(loc="upper right")
    ax.grid(axis="x", visible=False)
    salvar(fig, "04_fw_tempo_por_tipo.png")

# ------------------------------------------------------------------ Bellman-Ford
bf = ler("bf_tempos.csv")
if bf:
    hw = max(int(r["threads"]) for r in bf)
    med = medianas(bf, lambda r: (r["grafo"], r["algoritmo"], r["tipo"], int(r["threads"])))
    grafos = list(dict.fromkeys(r["grafo"] for r in bf))

    # 5) Tempo por grafo
    fig, ax = plt.subplots(figsize=(9, 4.5))
    series = [
        ("original", "BellmanFord_original", "double", 1),
        ("double", "BellmanFord_paralelo", "double", hw),
        ("float", "BellmanFord_paralelo", "float", hw),
        ("float16", "BellmanFord_paralelo", "float16", hw),
    ]
    largura = 0.8 / len(series)
    for i, (chave, alg, tipo, t) in enumerate(series):
        ys = [med.get((g, alg, tipo, t), 0) for g in grafos]
        xs = [j + (i - 1.5) * largura for j in range(len(grafos))]
        rot = ROTULO[chave] + ("" if chave == "original" else f", {t} threads")
        ax.bar(xs, ys, largura * 0.92, color=COR[chave], label=rot)
    ax.set_yscale("log")
    ax.yaxis.set_major_formatter(FuncFormatter(fmt_ms))
    ax.set_xticks(range(len(grafos)), [nome_grafo(g) for g in grafos], fontsize=8)
    ax.set_ylabel("tempo (mediana, escala log)")
    ax.set_title("Bellman-Ford a partir do vértice 1")
    ax.legend(loc="upper left", fontsize=8)
    ax.grid(axis="x", visible=False)
    salvar(fig, "05_bf_tempo_por_grafo.png")

    # 6) Speedup x threads (double), uma curva por grafo (ordenado por arcos)
    arcos = {r["grafo"]: int(r["arcos"]) for r in bf}
    ordem = sorted(grafos, key=lambda g: arcos[g])
    threads = sorted({int(r["threads"]) for r in bf
                      if r["algoritmo"] == "BellmanFord_paralelo" and r["tipo"] == "double"})
    fig, ax = plt.subplots(figsize=(7, 4.2))
    ax.plot(threads, threads, ls="--", lw=1, color=INK2, label="ideal (linear)")
    rampa = RAMPA if len(ordem) <= len(RAMPA) else RAMPA + ["#0b2c52"] * (len(ordem) - len(RAMPA))
    for cor, g in zip(rampa[-len(ordem):] if len(ordem) <= len(rampa) else rampa, ordem):
        base = med.get((g, "BellmanFord_paralelo", "double", 1))
        if not base:
            continue
        ys = [base / med[(g, "BellmanFord_paralelo", "double", t)] for t in threads]
        ax.plot(threads, ys, marker="o", markersize=5, color=cor,
                label=f"{nome_grafo(g).replace(chr(10), ' ')} ({arcos[g] / 1e6:.1f} M arcos)")
    ax.set_xticks(threads)
    ax.set_xlabel("threads")
    ax.set_ylabel("speedup em relação a 1 thread")
    ax.set_title("Bellman-Ford paralelo (double): escalabilidade")
    ax.legend(loc="upper left", fontsize=7)
    salvar(fig, "06_bf_speedup_threads.png")

    # 7) Memória por tipo (maior grafo)
    maior = ordem[-1]
    mem = {}
    for r in bf:
        if r["grafo"] != maior:
            continue
        chave = "original" if r["algoritmo"] == "BellmanFord_original" else r["tipo"]
        mem[chave] = int(r["memoria_bytes"]) / 2**20
    fw_mem = {}
    if fw:
        nmax = max(int(r["n"]) for r in fw)
        for r in fw:
            if int(r["n"]) == nmax and r["algoritmo"] == "FloydWarshall_paralelo":
                fw_mem[r["tipo"]] = int(r["memoria_bytes"]) / 2**20
    paineis = [(f"Bellman-Ford — {nome_grafo(maior).replace(chr(10), ' ')}", mem)]
    if fw_mem:
        paineis.append((f"Floyd-Warshall — n = {nmax}", fw_mem))
    fig, axs = plt.subplots(1, len(paineis), figsize=(4.6 * len(paineis), 4))
    axs = axs if len(paineis) > 1 else [axs]
    for ax, (titulo, dados) in zip(axs, paineis):
        chaves = [k for k in ["original", *TIPOS] if k in dados]
        barras = ax.bar(range(len(chaves)), [dados[k] for k in chaves], 0.6,
                        color=[COR[k] for k in chaves])
        ax.bar_label(barras, fmt="%.0f MiB", fontsize=8, color=INK2, padding=2)
        ax.set_xticks(range(len(chaves)),
                      ["original\n(double)" if k == "original" else f"{k}\n({BYTES_TIPO[k]} B)"
                       for k in chaves], fontsize=8)
        ax.set_title(titulo, fontsize=10)
        ax.set_ylabel("memória das estruturas (MiB)")
        ax.grid(axis="x", visible=False)
    fig.suptitle("Memória ocupada conforme o tipo numérico", fontweight="bold")
    fig.tight_layout()
    salvar(fig, "07_memoria_por_tipo.png")

# ------------------------------------------------------------------------- Erro
erros = ler("fw_erros.csv") + ler("bf_erros.csv")
erros = [e for e in erros if e["tipo"] in ("float", "float16") and "original" not in e["algoritmo"]]
if erros:
    fig, axs = plt.subplots(1, 2, figsize=(10, 4))
    for ax, alg in zip(axs, ["FloydWarshall", "BellmanFord"]):
        linhas = [e for e in erros if e["algoritmo"] == alg]
        rotulos = list(dict.fromkeys(
            (f"n = {e['n']}" if alg == "FloydWarshall" else nome_grafo(e["grafo"]))
            for e in linhas))
        largura = 0.38
        for i, tipo in enumerate(["float", "float16"]):
            brutos = [float(e["erro_rel_max"]) for e in linhas if e["tipo"] == tipo]
            ys = [y if y > 0 else float("nan") for y in brutos]
            xs = [j + (i - 0.5) * largura for j in range(len(ys))]
            ax.bar(xs, ys, largura * 0.92, color=COR[tipo], label=ROTULO[tipo])
            for x, y in zip(xs, brutos):
                if y == 0:  # pesos inteiros: resultado exato
                    ax.text(x, 1.4e-8, "exato", rotation=90, ha="center",
                            va="bottom", fontsize=7, color=INK2)
        ax.set_yscale("log")
        ax.set_ylim(1e-8, 1e-2)
        rotulos = [r.replace(chr(10), " ") for r in rotulos]
        if alg == "FloydWarshall":
            ax.set_xticks(range(len(rotulos)), rotulos, fontsize=7)
        else:
            ax.set_xticks(range(len(rotulos)), rotulos, fontsize=7,
                          rotation=25, ha="right")
        ax.set_title(alg.replace("FloydWarshall", "Floyd-Warshall").replace("BellmanFord", "Bellman-Ford"),
                     fontsize=10)
        ax.set_ylabel("erro relativo máximo vs double")
        ax.grid(axis="x", visible=False)
    fig.suptitle("Precisão perdida ao reduzir o tipo numérico", fontweight="bold")
    handles, labels = axs[0].get_legend_handles_labels()
    fig.legend(handles, labels, loc="upper center", ncol=2, fontsize=8,
               bbox_to_anchor=(0.5, 0.93))
    fig.tight_layout(rect=(0, 0, 1, 0.88))
    salvar(fig, "08_erro_por_tipo.png")
