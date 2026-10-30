"""Converte os resultados de Estudos/run_estudos.sh no arquivo do site.

Le Dados/Estudos/{caminhos,tempos}_*.json e gera site/data/estudos.js
(window.ESTUDOS). Estudos sem resultado ainda ficam null e o site mostra
um aviso no lugar.

Uso (na raiz do projeto):
  python site/tools/build_estudos.py
"""

import json
import os
import sys

ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", ".."))
ENTRADA = os.path.join(ROOT, "Dados", "Estudos")
REDE = os.path.join(ROOT, "Dados", "Entrada", "rede")
SITE = os.path.join(ROOT, "site")
GRAFOS = ["W_3", "W_4", "W_5", "W_6", "W_7"]


def ler(nome):
    caminho = os.path.join(ENTRADA, nome)
    if not os.path.exists(caminho) or os.path.getsize(caminho) == 0:
        return None
    with open(caminho, encoding="utf-8") as f:
        return json.loads(f.read())


def estudo1():
    grafos = []
    for gid in GRAFOS:
        d = ler(f"caminhos_{gid}.json")
        if d is None:
            continue
        g = {"id": gid, "nome": f"Grafo {gid}", "n": d["n"], "m": d["m"]}
        if "erro" in d:
            g["erro"] = d["erro"]
        else:
            g["tempo_s"] = d["tempo_dijkstra_s"]
            g["destinos"] = [{k: x[k] for k in ("destino", "distancia", "caminho")} for x in d["destinos"]]
        grafos.append(g)
    return {"origem": 10, "grafos": grafos} if grafos else None


def estudo2():
    grafos = []
    k_pedido = None
    for gid in GRAFOS:
        d = ler(f"tempos_{gid}.json")
        if d is None:
            continue
        k_pedido = d["k_pedido"]
        grafos.append({
            "id": gid,
            "nome": f"Grafo {gid}",
            "n": d["n"],
            "m": d["m"],
            "heap_ms": d["heap_ms"],
            "k_heap": d["k_heap"],
            "vetor_ms": d["vetor_ms"],
            "k_vetor": d["k_vetor"],
            "diferenca": d["diferenca_max_vetor_heap"],
        })
    return {"k_pedido": k_pedido, "grafos": grafos} if grafos else None


def vizinhos(indices):
    """Vizinhos de cada indice na rede (para explicar destinos inalcancaveis)."""
    alvo = set(indices)
    viz = {i: [] for i in indices}
    with open(os.path.join(REDE, "rede_colaboracao.txt"), encoding="utf-8") as f:
        next(f)
        for linha in f:
            u, v, _w = linha.split()
            u, v = int(u), int(v)
            if u in alvo:
                viz[u].append(v)
            if v in alvo:
                viz[v].append(u)
    return viz


def nomes(indices):
    alvo = set(indices)
    out = {}
    with open(os.path.join(REDE, "rede_colaboracao_vertices.txt"), encoding="utf-8") as f:
        for linha in f:
            i, _, nome = linha.rstrip("\r\n").partition(",")
            if int(i) in alvo:
                out[int(i)] = nome
    return out


def estudo3():
    """Uma entrada por origem: caminhos_rede_<indice>.json (Dijkstra primeiro)."""
    arquivos = sorted(f for f in os.listdir(ENTRADA) if f.startswith("caminhos_rede_") and f.endswith(".json"))
    execucoes = [d for d in map(ler, arquivos) if d is not None and "erro" not in d]
    if not execucoes:
        return None
    execucoes.sort(key=lambda d: d["origem"] != "Edsger W. Dijkstra")
    sem_caminho = {x["indice"] for d in execucoes for x in d["destinos"] if x["caminho"] is None}
    viz = vizinhos(sorted(sem_caminho)) if sem_caminho else {}
    nomes_viz = nomes([v for vs in viz.values() for v in vs])
    origens = []
    for d in execucoes:
        destinos = []
        for x in d["destinos"]:
            item = {"destino": x["destino"], "distancia": x["distancia"], "caminho": x["caminho"]}
            if x["caminho"] is None:
                item["coautores"] = [nomes_viz.get(v, str(v)) for v in viz.get(x["indice"], [])]
            destinos.append(item)
        origens.append({"origem": d["origem"], "tempo_s": d["tempo_dijkstra_s"], "destinos": destinos})
    return {"n": execucoes[0]["n"], "m": execucoes[0]["m"], "origens": origens}


def main():
    dados = {"estudo1": estudo1(), "estudo2": estudo2(), "estudo3": estudo3()}
    destino = os.path.join(SITE, "data", "estudos.js")
    with open(destino, "w", encoding="utf-8") as f:
        f.write("// Gerado por site/tools/build_estudos.py a partir de Dados/Estudos - nao editar a mao.\n")
        f.write("window.ESTUDOS = ")
        json.dump(dados, f, ensure_ascii=False, separators=(",", ":"))
        f.write(";\n")
    for k, v in dados.items():
        n = len(v.get("grafos", v.get("origens", []))) if v else 0
        print(f"  {k}: {'ok (' + str(n) + ')' if v else 'sem dados'}")
    print(f"ok: {destino}")


if __name__ == "__main__":
    sys.exit(main())
