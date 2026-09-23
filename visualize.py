import os
import json
import glob
import matplotlib.pyplot as plt
import seaborn as sns
import networkx as nx
import numpy as np

plt.style.use("dark_background")
sns.set_theme(style="darkgrid", rc={"axes.facecolor": "#121212", "figure.facecolor": "#121212"})
COLOR_BFS = "#00ffcc"
COLOR_DFS = "#ff007f"
OUTPUT_DIR = "graficos_saida"

def load_all_data():
    files = glob.glob("*.json")
    data_list = []
    for f in files:
        with open(f, "r") as file:
            data = json.load(file)
            data["filename"] = f
            data_list.append(data)
    data_list.sort(key=lambda x: x["v_plus_e"])
    return data_list

def plot_performance_curve(data_list, scale="linear"):
    if not data_list: return
    v_plus_e = [d["v_plus_e"] for d in data_list]
    bfs_means = [d["benchmark"]["BFS"]["mean"] for d in data_list]
    bfs_medians = [d["benchmark"]["BFS"]["median"] for d in data_list]
    dfs_means = [d["benchmark"]["DFS"]["mean"] for d in data_list]
    dfs_medians = [d["benchmark"]["DFS"]["median"] for d in data_list]

    plt.figure(figsize=(12, 7))
    for d in data_list:
        x = d["v_plus_e"]
        plt.scatter([x]*len(d["benchmark"]["BFS"]["raw"]), d["benchmark"]["BFS"]["raw"], color=COLOR_BFS, alpha=0.3, s=15)
        plt.scatter([x]*len(d["benchmark"]["DFS"]["raw"]), d["benchmark"]["DFS"]["raw"], color=COLOR_DFS, alpha=0.3, s=15)

    plt.plot(v_plus_e, bfs_means, color=COLOR_BFS, marker='o', linestyle='-', linewidth=2, label="BFS (Média)")
    plt.plot(v_plus_e, bfs_medians, color=COLOR_BFS, marker='s', linestyle='--', linewidth=2, label="BFS (Mediana)")
    plt.plot(v_plus_e, dfs_means, color=COLOR_DFS, marker='o', linestyle='-', linewidth=2, label="DFS (Média)")
    plt.plot(v_plus_e, dfs_medians, color=COLOR_DFS, marker='s', linestyle='--', linewidth=2, label="DFS (Mediana)")

    suffix = "_linear"
    xlabel_suf = ""
    ylabel_suf = ""
    if scale == "log":
        plt.xscale("log")
        plt.yscale("log")
        suffix = "_log"
        xlabel_suf = " [Escala Log]"
        ylabel_suf = " [Escala Log]"

    plt.xlabel(f"Quantidade de Vértices + Arestas (V + E){xlabel_suf}", fontsize=12, color="lightgray")
    plt.ylabel(f"Tempo de Execução (ms){ylabel_suf}", fontsize=12, color="lightgray")
    plt.legend(facecolor="#222222", edgecolor="white", labelcolor="white")
    plt.tight_layout()
    plt.savefig(os.path.join(OUTPUT_DIR, f"01_performance_curve{suffix}.png"), dpi=300)
    plt.close()

def plot_memory_curve(data_list, scale="linear"):
    if not data_list or "telemetria" not in data_list[0]: return
    v_plus_e = [d["v_plus_e"] for d in data_list]
    mem = [d["telemetria"]["peak_memory_mb"] for d in data_list]

    plt.figure(figsize=(12, 7))
    plt.plot(v_plus_e, mem, color="#ffcc00", marker='D', linestyle='-', linewidth=2, label="Consumo Lista de Adjacência")
    plt.fill_between(v_plus_e, mem, color="#ffcc00", alpha=0.1)

    suffix = "_linear"
    xlabel_suf = ""
    ylabel_suf = ""
    if scale == "log":
        plt.xscale("log")
        plt.yscale("log")
        suffix = "_log"
        xlabel_suf = " [Escala Log]"
        ylabel_suf = " [Escala Log]"

    plt.xlabel(f"Quantidade de Vértices + Arestas (V + E){xlabel_suf}", fontsize=12, color="lightgray")
    plt.ylabel(f"Pico de Memória RAM (MB){ylabel_suf}", fontsize=12, color="lightgray")
    plt.legend(facecolor="#222222", edgecolor="white", labelcolor="white")
    plt.tight_layout()
    plt.savefig(os.path.join(OUTPUT_DIR, f"02_memory_curve{suffix}.png"), dpi=300)
    plt.close()

def plot_time_profiling(data_list, scale="linear"):
    if not data_list or "telemetria" not in data_list[0]: return
    
    labels = [f"G{i+1}" for i in range(len(data_list))]
    build = np.array([d["telemetria"]["tempo_build_ms"] for d in data_list])
    grau = np.array([d["telemetria"]["tempo_grau_ms"] for d in data_list])
    comp = np.array([d["telemetria"]["tempo_comp_ms"] for d in data_list])
    bfs = np.array([d["benchmark"]["BFS"]["mean"] for d in data_list])

    plt.figure(figsize=(12, 7))
    plt.bar(labels, build, color="#555555", label="Construção (I/O)")
    plt.bar(labels, grau, bottom=build, color="#888888", label="Cálculo de Graus")
    plt.bar(labels, comp, bottom=build+grau, color="#ff007f", label="Extração Componentes")
    plt.bar(labels, bfs, bottom=build+grau+comp, color="#00ffcc", label="BFS (Média)")

    suffix = "_linear"
    ylabel_suf = ""
    if scale == "log":
        plt.yscale("log")
        suffix = "_log"
        ylabel_suf = " [Escala Log]"

    plt.xlabel("Grafos Analisados", fontsize=12, color="lightgray")
    plt.ylabel(f"Tempo Acumulado (ms){ylabel_suf}", fontsize=12, color="lightgray")
    plt.legend(facecolor="#222222", edgecolor="white", labelcolor="white", loc="upper left")
    plt.tight_layout()
    plt.savefig(os.path.join(OUTPUT_DIR, f"03_time_profiling{suffix}.png"), dpi=300)
    plt.close()

def plot_component_sizes(comp_sizes, graph_id, v_plus_e):
    if not comp_sizes: return
    
    plt.figure(figsize=(10, 6))
    sizes = sorted(comp_sizes, reverse=True)
    plt.plot(range(1, len(sizes) + 1), sizes, marker='o', linestyle='', color="#ff007f", alpha=0.7)
    
    plt.yscale("log")
    plt.xscale("log")
    
    plt.xlabel("Rank da Componente (Escala Log)", fontsize=12, color="lightgray")
    plt.ylabel("Tamanho da Componente (Escala Log)", fontsize=12, color="lightgray")
    plt.tight_layout()
    plt.savefig(os.path.join(OUTPUT_DIR, f"04_rank_componentes_grafo_{graph_id}_VE_{v_plus_e}_log.png"), dpi=300)
    plt.close()

def plot_degree_histogram(degrees, graph_id, v_plus_e):
    plt.figure(figsize=(10, 6))
    sns.histplot(degrees, bins=50, color="#00ffcc", alpha=0.7, edgecolor="#121212")
    
    plt.yscale("linear")
    
    plt.xlabel("Grau do Vértice", fontsize=12, color="lightgray")
    plt.ylabel("Frequência Absoluta", fontsize=12, color="lightgray")
    plt.tight_layout()
    plt.savefig(os.path.join(OUTPUT_DIR, f"05_histograma_graus_grafo_{graph_id}_VE_{v_plus_e}_linear.png"), dpi=300)
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

def draw_network_tree(graph_data, graph_id, v_plus_e, mode="BFS"):
    nodes = graph_data.get("nos", [])
    if not nodes: return
    
    bridges = [tuple(e) for e in graph_data.get("pontes", [])]
    articulations = graph_data.get("articulacoes", [])
    
    tree_key = "bfs" if mode == "BFS" else "dfs"
    if tree_key not in graph_data: return
    
    tree_edges = graph_data[tree_key].get("tree_edges", [])
    other_edges_key = "cross_edges" if mode == "BFS" else "back_edges"
    other_edges = graph_data[tree_key].get(other_edges_key, [])

    G = nx.Graph()
    G.add_nodes_from(nodes)
    G.add_edges_from(tree_edges)
    G.add_edges_from(other_edges)

    pos = get_tree_pos(nodes, tree_edges)
    
    degrees = dict(G.degree())
    node_sizes = [v * 30 + 10 for v in degrees.values()]
    node_colors = list(degrees.values())
    
    plt.figure(figsize=(16, 12)) 
    
    nx.draw_networkx_nodes(G, pos, node_size=node_sizes, node_color=node_colors, cmap=plt.cm.magma, edgecolors="none")
    
    if articulations:
        art_sizes = [degrees[node] * 30 + 10 for node in articulations]
        nx.draw_networkx_nodes(G, pos, nodelist=articulations, node_size=art_sizes, node_color="none", edgecolors="#FFD700", linewidths=3)
    
    nx.draw_networkx_edges(G, pos, edgelist=other_edges, alpha=0.3, edge_color="#666666", style="dashed", width=1.0)
    
    tree_color = COLOR_BFS if mode == "BFS" else COLOR_DFS
    normal_tree_edges = [e for e in tree_edges if (e[0], e[1]) not in bridges and (e[1], e[0]) not in bridges]
    nx.draw_networkx_edges(G, pos, edgelist=normal_tree_edges, alpha=0.9, edge_color=tree_color, width=2.0)
    
    if bridges:
        nx.draw_networkx_edges(G, pos, edgelist=bridges, alpha=1.0, edge_color="#FFD700", width=3.5)
    
    plt.axis("off")
    plt.tight_layout()
    plt.savefig(os.path.join(OUTPUT_DIR, f"06_arvore_topologia_grafo_{graph_id}_VE_{v_plus_e}_{mode}.png"), dpi=300)
    plt.close()

def plot_dfs_comparison(data_list, scale="linear"):
    if not data_list: return
    
    # Extrai o valor de V+E real caso o JSON exista
    v_plus_e = [d["v_plus_e"] for d in data_list]
    
    # Valores de tempo hardcodados enviados por si
    iterative_times = [1.082, 0.236, 28.425, 137.469, 987.050, 739.389]
    recursive_times = [0.563, 0.138, 28.220, 109.485, 830.702, 553.370]

    # Previne quebra caso haja menos de 6 arquivos JSON carregados
    limit = min(len(v_plus_e), len(iterative_times))
    v_plus_e = v_plus_e[:limit]
    iterative_times = iterative_times[:limit]
    recursive_times = recursive_times[:limit]

    plt.figure(figsize=(12, 7))
    
    # Segue exatamente o estilo visual da figura 1 (Linhas, cores contrastantes, marcadores)
    plt.plot(v_plus_e, iterative_times, color=COLOR_DFS, marker='o', linestyle='-', linewidth=2, label="DFS Iterativa (Média)")
    plt.plot(v_plus_e, recursive_times, color="#ffcc00", marker='s', linestyle='--', linewidth=2, label="DFS Recursiva (Média)")

    suffix = "_linear"
    xlabel_suf = ""
    ylabel_suf = ""
    if scale == "log":
        plt.xscale("log")
        plt.yscale("log")
        suffix = "_log"
        xlabel_suf = " [Escala Log]"
        ylabel_suf = " [Escala Log]"

    plt.xlabel(f"Quantidade de Vértices + Arestas (V + E){xlabel_suf}", fontsize=12, color="lightgray")
    plt.ylabel(f"Tempo de Execução (ms){ylabel_suf}", fontsize=12, color="lightgray")
    plt.legend(facecolor="#222222", edgecolor="white", labelcolor="white")
    plt.tight_layout()
    plt.savefig(os.path.join(OUTPUT_DIR, f"07_dfs_comparacao_tempos{suffix}.png"), dpi=300)
    plt.close()

if __name__ == "__main__":
    os.makedirs(OUTPUT_DIR, exist_ok=True)
    data_list = load_all_data()
    
    if not data_list:
        print("Nenhum arquivo .json encontrado no diretório.")
    else:
        print(f"Processando {len(data_list)} grafos encontrados...\n")
        
        plot_performance_curve(data_list, scale="linear")
        plot_performance_curve(data_list, scale="log")
        
        plot_memory_curve(data_list, scale="linear")
        plot_memory_curve(data_list, scale="log")
        
        plot_time_profiling(data_list, scale="linear")
        plot_time_profiling(data_list, scale="log")
        
        # Gera o novo gráfico de comparação DFS em ambas as escalas
        plot_dfs_comparison(data_list, scale="linear")
        plot_dfs_comparison(data_list, scale="log")
        
        for idx, graph in enumerate(data_list):
            graph_id = idx + 1
            ve_total = graph["v_plus_e"]
            
            print(f"Gerando artefatos visuais: Grafo {graph_id}...")
            plot_component_sizes(graph["tamanhos_componentes"], graph_id, ve_total)
            plot_degree_histogram(graph["distribuicao_graus"], graph_id, ve_total)
            
            draw_network_tree(graph["desenho_grafo"], graph_id, ve_total, mode="BFS")
            draw_network_tree(graph["desenho_grafo"], graph_id, ve_total, mode="DFS")
            
        print(f"\nTodos os gráficos foram gerados com sucesso na pasta '{OUTPUT_DIR}'!")