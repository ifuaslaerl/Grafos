import os
import sys
import pandas as pd
import matplotlib.pyplot as plt
import matplotlib.lines as mlines
import seaborn as sns
import networkx as nx
import numpy as np
from config import OUTPUT_DIR, COLOR_BFS, COLOR_DFS_ITER, COLOR_DFS_REC, COLOR_BG, MAX_NODES

sys.setrecursionlimit(max(3000, 10*MAX_NODES))

plt.style.use("dark_background")
sns.set_theme(style="darkgrid", rc={"axes.facecolor": COLOR_BG, "figure.facecolor": COLOR_BG})

def plot_performance_curve(data_list, scale="linear"):
    if not data_list: return
    v_plus_e = [d["v_plus_e"] for d in data_list]
    
    bfs_means = [d["benchmark"]["BFS"]["mean_ms"] for d in data_list]
    bfs_medians = [d["benchmark"]["BFS"]["median_ms"] for d in data_list]
    
    dfs_i_means = [d["benchmark"]["DFS_Iterativa"]["mean_ms"] for d in data_list]
    dfs_i_medians = [d["benchmark"]["DFS_Iterativa"]["median_ms"] for d in data_list]
    
    dfs_r_means = [d["benchmark"]["DFS_Recursiva"]["mean_ms"] for d in data_list]
    dfs_r_medians = [d["benchmark"]["DFS_Recursiva"]["median_ms"] for d in data_list]

    plt.figure(figsize=(14, 8))

    num_runs = len(data_list[0]["benchmark"]["BFS"]["raw_ms"])
    for i in range(num_runs):
        bfs_raw = [d["benchmark"]["BFS"]["raw_ms"][i] for d in data_list]
        dfs_i_raw = [d["benchmark"]["DFS_Iterativa"]["raw_ms"][i] for d in data_list]
        dfs_r_raw = [d["benchmark"]["DFS_Recursiva"]["raw_ms"][i] for d in data_list]
        
        label_bfs = "BFS (Execuções)" if i == 0 else None
        label_dfs_i = "DFS Iter. (Execuções)" if i == 0 else None
        label_dfs_r = "DFS Rec. (Execuções)" if i == 0 else None
        
        plt.plot(v_plus_e, bfs_raw, color=COLOR_BFS, alpha=0.15, linewidth=1, label=label_bfs)
        plt.plot(v_plus_e, dfs_i_raw, color=COLOR_DFS_ITER, alpha=0.15, linewidth=1, label=label_dfs_i)
        plt.plot(v_plus_e, dfs_r_raw, color=COLOR_DFS_REC, alpha=0.15, linewidth=1, label=label_dfs_r)

    plt.plot(v_plus_e, bfs_medians, color=COLOR_BFS, marker='o', linestyle='--', linewidth=2, alpha=0.8, label="BFS (Mediana)")
    plt.plot(v_plus_e, dfs_i_medians, color=COLOR_DFS_ITER, marker='s', linestyle='--', linewidth=2, alpha=0.8, label="DFS Iter. (Mediana)")
    plt.plot(v_plus_e, dfs_r_medians, color=COLOR_DFS_REC, marker='^', linestyle='--', linewidth=2, alpha=0.8, label="DFS Rec. (Mediana)")

    plt.plot(v_plus_e, bfs_means, color=COLOR_BFS, marker='o', linestyle='-', linewidth=2.5, label="BFS (Média)")
    plt.plot(v_plus_e, dfs_i_means, color=COLOR_DFS_ITER, marker='s', linestyle='-', linewidth=2.5, label="DFS Iter. (Média)")
    plt.plot(v_plus_e, dfs_r_means, color=COLOR_DFS_REC, marker='^', linestyle='-', linewidth=2.5, label="DFS Rec. (Média)")

    suffix, xlabel_suf, ylabel_suf = "_linear", "", ""
    if scale == "log":
        plt.xscale("log")
        plt.yscale("log")
        suffix, xlabel_suf, ylabel_suf = "_log", " [Escala Log]", " [Escala Log]"

    plt.xlabel(f"Quantidade de Vértices + Arestas (V + E){xlabel_suf}", fontsize=12, color="lightgray")
    plt.ylabel(f"Tempo de Execução (ms){ylabel_suf}", fontsize=12, color="lightgray")
    
    plt.legend(facecolor="#222222", edgecolor="white", labelcolor="white", bbox_to_anchor=(1.04, 1), loc="upper left")
    
    plt.tight_layout()
    plt.savefig(os.path.join(OUTPUT_DIR, f"01_performance_curve{suffix}.png"), dpi=300)
    plt.close()

def plot_memory_curve(data_list, scale="linear"):
    if not data_list or "telemetria" not in data_list[0]: return
    v_plus_e = [d["v_plus_e"] for d in data_list]
    mem = [d["telemetria"]["peak_memory_mb"] for d in data_list]

    plt.figure(figsize=(12, 7))
    plt.plot(v_plus_e, mem, color="#ffcc00", marker='D', linestyle='-', linewidth=2, label="Consumo Global do Processo")
    plt.fill_between(v_plus_e, mem, color="#ffcc00", alpha=0.1)

    suffix, xlabel_suf, ylabel_suf = "_linear", "", ""
    if scale == "log":
        plt.xscale("log")
        plt.yscale("log")
        suffix, xlabel_suf, ylabel_suf = "_log", " [Escala Log]", " [Escala Log]"

    plt.xlabel(f"Quantidade de Vértices + Arestas (V + E){xlabel_suf}", fontsize=12, color="lightgray")
    plt.ylabel(f"Pico de Memória RAM (MB){ylabel_suf}", fontsize=12, color="lightgray")
    plt.legend(facecolor="#222222", edgecolor="white", labelcolor="white")
    plt.tight_layout()
    plt.savefig(os.path.join(OUTPUT_DIR, f"02_memory_curve{suffix}.png"), dpi=300)
    plt.close()

def plot_time_profiling(data_list, scale="linear"):
    if not data_list or "telemetria" not in data_list[0]: return
    
    labels = [f"G{d['graph_id']}" for d in data_list]
    build = np.array([d["telemetria"]["tempo_build_ms"] for d in data_list])
    grau = np.array([d["telemetria"]["tempo_grau_ms"] for d in data_list])
    comp = np.array([d["telemetria"]["tempo_comp_ms"] for d in data_list])
    bfs = np.array([d["benchmark"]["BFS"]["mean_ms"] for d in data_list])

    plt.figure(figsize=(12, 7))
    plt.bar(labels, build, color="#555555", label="Construção (I/O)")
    plt.bar(labels, grau, bottom=build, color="#888888", label="Cálculo de Graus")
    plt.bar(labels, comp, bottom=build+grau, color="#ff007f", label="Extração Componentes")
    plt.bar(labels, bfs, bottom=build+grau+comp, color="#00ffcc", label="BFS (Média)")

    suffix, ylabel_suf = "_linear", ""
    if scale == "log":
        plt.yscale("log")
        suffix, ylabel_suf = "_log", " [Escala Log]"

    plt.xlabel("Grafos Analisados", fontsize=12, color="lightgray")
    plt.ylabel(f"Tempo Acumulado (ms){ylabel_suf}", fontsize=12, color="lightgray")
    plt.legend(facecolor="#222222", edgecolor="white", labelcolor="white", loc="upper left")
    plt.tight_layout()
    plt.savefig(os.path.join(OUTPUT_DIR, f"03_time_profiling{suffix}.png"), dpi=300)
    plt.close()

def plot_degree_histogram(csv_path, graph_id, v_plus_e):
    if not csv_path or not os.path.exists(csv_path): return
    
    df = pd.read_csv(csv_path)
    if df.empty: return
    
    plt.figure(figsize=(10, 6))
    plt.bar(df['Grau'], df['Frequencia'], color="#00ffcc", alpha=0.8, edgecolor=COLOR_BG)
    
    plt.xlabel("Grau do Vértice", fontsize=12, color="lightgray")
    plt.ylabel("Frequência Absoluta", fontsize=12, color="lightgray")
    plt.title(f"Distribuição de Graus - Grafo {graph_id}", color="white")
    plt.tight_layout()
    plt.savefig(os.path.join(OUTPUT_DIR, f"04_histograma_graus_grafo_{graph_id}_VE_{v_plus_e}.png"), dpi=300)
    plt.close()

def get_tree_pos(nodes, tree_edges):
    DG = nx.DiGraph()
    DG.add_nodes_from(nodes)
    DG.add_edges_from(tree_edges) 
    
    leaves_count = {}
    def count_leaves(node):
        children = list(DG.successors(node))
        if not children:
            leaves_count[node] = 1
            return 1
        total = sum(count_leaves(c) for c in children)
        leaves_count[node] = total
        return total
    
    roots = [n for n, d in DG.in_degree() if d == 0]
    for root in roots:
        count_leaves(root)
        
    pos = {}
    def _layout(node, x_start, x_end, y):
        pos[node] = ((x_start + x_end) / 2.0, y)
        children = list(DG.successors(node))
        if not children: return
        
        current_x = x_start
        total_leaves = leaves_count[node]
        for child in children:
            child_width = (leaves_count[child] / total_leaves) * (x_end - x_start)
            _layout(child, current_x, current_x + child_width, y - 1)
            current_x += child_width
            
    current_x = 0
    for root in roots:
        root_width = leaves_count[root]
        _layout(root, current_x, current_x + root_width, 0)
        current_x += root_width
        
    return pos

def draw_network_tree_csv(csv_path, graph_id, v_plus_e, mode="BFS"):
    if not csv_path or not os.path.exists(csv_path): return
    
    df = pd.read_csv(csv_path)
    if df.empty: return

    df = df.sort_values(by="ordem_visitacao")

    if len(df) > MAX_NODES:
        df = df.head(MAX_NODES)

    valid_nodes = set(df['vertice'])
    
    valid_edges = df[(df['pai'] != 0) & (df['pai'] != df['vertice'])]
    valid_edges = valid_edges[valid_edges['pai'].isin(valid_nodes)]

    tree_edges = list(zip(valid_edges['pai'], valid_edges['vertice']))
    nodes = list(df['vertice'])

    G = nx.Graph()
    G.add_nodes_from(nodes)
    G.add_edges_from(tree_edges)

    isolated = list(nx.isolates(G))
    G.remove_nodes_from(isolated)
    
    if len(G.nodes) == 0: return

    pos = get_tree_pos(list(G.nodes), tree_edges)
    
    degrees = dict(G.degree())
    node_sizes = [min(v * 30 + 10, 300) for v in degrees.values()]
    node_colors = list(degrees.values())
    
    plt.figure(figsize=(16, 12)) 
    nx.draw_networkx_nodes(G, pos, node_size=node_sizes, node_color=node_colors, cmap=plt.cm.magma, edgecolors="none")
    
    tree_color = COLOR_BFS if mode == "BFS" else COLOR_DFS_ITER
    nx.draw_networkx_edges(G, pos, edgelist=tree_edges, alpha=0.9, edge_color=tree_color, width=2.0)
    
    plt.axis("off")
    plt.tight_layout()
    plt.savefig(os.path.join(OUTPUT_DIR, f"05_arvore_topologia_grafo_{graph_id}_VE_{v_plus_e}_{mode}.png"), dpi=300)
    plt.close()

def draw_overlay_trees_csv(csv_bfs, csv_dfs, graph_id, v_plus_e):
    if not csv_bfs or not os.path.exists(csv_bfs) or not csv_dfs or not os.path.exists(csv_dfs): return

    df_bfs = pd.read_csv(csv_bfs).sort_values(by="ordem_visitacao")
    df_dfs = pd.read_csv(csv_dfs).sort_values(by="ordem_visitacao")

    bfs_list = df_bfs['vertice'].tolist()
    dfs_list = df_dfs['vertice'].tolist()

    bfs_selected = set()
    dfs_selected = set()

    idx_bfs = 0
    idx_dfs = 0
    
    while len(bfs_selected | dfs_selected) < MAX_NODES and (idx_bfs < len(bfs_list) or idx_dfs < len(dfs_list)):
        if idx_bfs < len(bfs_list):
            bfs_selected.add(bfs_list[idx_bfs])
            idx_bfs += 1
            
        if len(bfs_selected | dfs_selected) >= MAX_NODES:
            break
            
        if idx_dfs < len(dfs_list):
            dfs_selected.add(dfs_list[idx_dfs])
            idx_dfs += 1

    valid_nodes = bfs_selected | dfs_selected

    def extract_tree_edges(df, selected_prefix):
        edges = []
        for _, row in df.iterrows():
            u, v = int(row['pai']), int(row['vertice'])
            if u != 0 and u != v and u in selected_prefix and v in selected_prefix:
                edges.append(tuple(sorted((u, v))))
        return set(edges)

    bfs_edges = extract_tree_edges(df_bfs, bfs_selected)
    dfs_edges = extract_tree_edges(df_dfs, dfs_selected)

    intersection_edges = list(bfs_edges.intersection(dfs_edges))
    only_bfs = list(bfs_edges - dfs_edges)
    only_dfs = list(dfs_edges - bfs_edges)

    G = nx.Graph()
    G.add_nodes_from(valid_nodes)
    G.add_edges_from(bfs_edges)
    G.add_edges_from(dfs_edges)

    isolated_nodes = list(nx.isolates(G))
    G.remove_nodes_from(isolated_nodes)

    if len(G.nodes) == 0: return

    pos = nx.spring_layout(G, seed=42, k=0.15, iterations=50)

    plt.figure(figsize=(16, 12)) 
    
    # Nós redesenhados para ficarem muito mais sutis (cinza escuro, pequenos e com leve transparência)
    nx.draw_networkx_nodes(G, pos, node_size=20, node_color="#171717", edgecolors="none", alpha=0.8)

    nx.draw_networkx_edges(G, pos, edgelist=intersection_edges, edge_color="white", width=2.5, alpha=0.9)
    nx.draw_networkx_edges(G, pos, edgelist=only_bfs, edge_color=COLOR_BFS, width=1.5, alpha=0.7, style="solid")
    nx.draw_networkx_edges(G, pos, edgelist=only_dfs, edge_color=COLOR_DFS_ITER, width=1.5, alpha=0.7, style="dashed")

    line_ambos = mlines.Line2D([], [], color='white', linewidth=2.5, label='Arestas Comuns (BFS e DFS)')
    line_bfs = mlines.Line2D([], [], color=COLOR_BFS, linewidth=1.5, label='Exclusivo BFS')
    line_dfs = mlines.Line2D([], [], color=COLOR_DFS_ITER, linewidth=1.5, linestyle='dashed', label='Exclusivo DFS')
    plt.legend(handles=[line_ambos, line_bfs, line_dfs], facecolor="#222222", edgecolor="white", labelcolor="white", loc="upper right", fontsize=12)

    plt.axis("off")
    plt.title(f"Sobreposição Topológica BFS x DFS - Grafo {graph_id}", color="white", fontsize=16)
    plt.tight_layout()
    plt.savefig(os.path.join(OUTPUT_DIR, f"06_overlay_bfs_dfs_grafo_{graph_id}_VE_{v_plus_e}.png"), dpi=300)
    plt.close()