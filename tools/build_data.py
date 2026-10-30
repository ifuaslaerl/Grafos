"""Extrai de Dados/ (cerca de 2 GB) apenas o que o site precisa.

Gera:
  site/data/dados.js   -> window.DADOS (resumos, benchmarks, graus, amostras de arvores)
  site/img/*.jpg       -> versoes reduzidas dos PNGs de sobreposicao

Os arquivos sao .js (e nao .json) para o site funcionar abrindo o
index.html direto, sem servidor (file:// bloqueia fetch).

Uso (na raiz do projeto):
  python site/tools/build_data.py
"""

import csv
import json
import os
import sys
from glob import glob

ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", ".."))
DADOS = os.path.join(ROOT, "Dados")
SITE = os.path.join(ROOT, "site")
AMOSTRA_ARVORE = 220  # primeiros vertices visitados de cada arvore (2D)
AMOSTRA_3D = 1500     # amostra maior para a visualizacao 3D

NOMES = {
    "rede_colaboracao": "Rede de colaboração",
}


def nome_grafo(gid):
    if gid in NOMES:
        return NOMES[gid]
    return f"Grafo {gid}"


def resumo_componentes(tamanhos):
    hist = {}
    for t in tamanhos:
        hist[t] = hist.get(t, 0) + 1
    return {
        "quantidade": len(tamanhos),
        "maiores": tamanhos[:12],
        "histograma": sorted([[t, c] for t, c in hist.items()]),
    }


def ler_graus(gid):
    caminho = os.path.join(DADOS, "CSV", f"grafo_{gid}.csv")
    with open(caminho, encoding="utf-8") as f:
        leitor = csv.DictReader(f)
        return [[int(r["Grau"]), int(r["Frequencia"])] for r in leitor]


def amostra_arvore(gid, alg, limite=AMOSTRA_ARVORE):
    """Primeiros N vertices pela ordem de visita.

    Em BFS, DFS e Dijkstra o pai sempre e visitado antes do filho,
    entao esse prefixo e sempre uma arvore conexa enraizada na origem.
    """
    caminho = os.path.join(DADOS, "CSV", f"{alg}_{gid}.csv")
    linhas = []
    with open(caminho, encoding="utf-8") as f:
        next(f)
        for linha in f:
            v, pai, dist, ordem, _comp = linha.rstrip("\n").split(",")
            ordem = int(ordem)
            if 0 < ordem <= limite:
                linhas.append([ordem, int(v), int(pai), float(dist)])
    linhas.sort()
    return [[v, pai, round(d, 4), ordem] for ordem, v, pai, d in linhas]


def compactar_3d(linhas):
    """[v, pai] em ordem de visita, achatado: [v1, p1, v2, p2, ...]."""
    out = []
    for v, pai, _d, _o in linhas:
        out += [v, pai]
    return out


def reduzir_imagens():
    try:
        from PIL import Image
    except ImportError:
        print("PIL ausente: imagens nao reduzidas")
        return []

    saida = []
    for png in sorted(glob(os.path.join(DADOS, "06_overlay_*.png"))):
        base = os.path.basename(png)
        gid = base.split("_grafo_")[1].split("_VE_")[0]
        destino = os.path.join(SITE, "img", f"overlay_{gid}.jpg")
        if not os.path.exists(destino):
            img = Image.open(png).convert("RGB")
            img.thumbnail((1800, 1800))
            img.save(destino, "JPEG", quality=84, optimize=True)
        saida.append({"id": gid, "arquivo": f"img/overlay_{gid}.jpg"})
    return saida


def main():
    grafos = []
    graus = {}
    arvores = {}
    arvores3d = {}

    for caminho in sorted(glob(os.path.join(DADOS, "JSON", "grafo_*.json"))):
        gid = os.path.basename(caminho)[len("grafo_"):-len(".json")]
        with open(caminho, encoding="utf-8") as f:
            d = json.load(f)

        bench = {}
        for alg, info in d["benchmark_get_components"].items():
            if isinstance(info, dict):
                bench[alg] = {
                    "media": info["mean_ms"],
                    "mediana": info["median_ms"],
                    "execucoes": info["raw_ms"],
                }

        grafos.append({
            "id": gid,
            "nome": nome_grafo(gid),
            "ponderado": d["ponderado"],
            "n": d["n"],
            "m": d["m"],
            "v_mais_e": d["v_plus_e"],
            "graus": d["graus"],
            "telemetria": d["telemetria"],
            "execucoes": d["benchmark_get_components"]["numero_execucoes"],
            "benchmark": bench,
            "componentes": resumo_componentes(d["tamanhos_componentes"]),
        })

        graus[gid] = ler_graus(gid)

        arvores[gid] = {}
        arvores3d[gid] = {}
        for alg in ("bfs", "dfs", "dijkstra"):
            print(f"  amostra {alg}_{gid}...", flush=True)
            grande = amostra_arvore(gid, alg, AMOSTRA_3D)
            arvores[gid][alg] = grande[:AMOSTRA_ARVORE]
            arvores3d[gid][alg] = compactar_3d(grande)

    grafos.sort(key=lambda g: g["v_mais_e"])

    dados = {
        "grafos": grafos,
        "graus": graus,
        "arvores": arvores,
        "sobreposicoes": reduzir_imagens(),
        "amostra_arvore": AMOSTRA_ARVORE,
    }

    destino = os.path.join(SITE, "data", "dados.js")
    with open(destino, "w", encoding="utf-8") as f:
        f.write("// Gerado por site/tools/build_data.py - nao editar a mao.\n")
        f.write("window.DADOS = ")
        json.dump(dados, f, ensure_ascii=False, separators=(",", ":"))
        f.write(";\n")

    print(f"ok: {destino} ({os.path.getsize(destino) / 1024:.0f} KB)")

    destino3d = os.path.join(SITE, "data", "dados3d.js")
    with open(destino3d, "w", encoding="utf-8") as f:
        f.write("// Gerado por site/tools/build_data.py - nao editar a mao.\n")
        f.write(f"window.DADOS3D = {{\"amostra\":{AMOSTRA_3D},\"arvores\":")
        json.dump(arvores3d, f, separators=(",", ":"))
        f.write("};\n")
    print(f"ok: {destino3d} ({os.path.getsize(destino3d) / 1024:.0f} KB)")


if __name__ == "__main__":
    sys.exit(main())
