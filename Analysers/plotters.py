import os
import sys
import pandas as pd
import matplotlib.pyplot as plt
import matplotlib.lines as mlines
import seaborn as sns
import networkx as nx
import numpy as np
from config import (
    OUTPUT_DIR,
    COLOR_BFS,
    COLOR_DFS_ITER,
    COLOR_DIJKSTRA,
    COLOR_BG,
    MAX_NODES,
)

PLOT_COLORS = {
    "BFS": "#008c72",
    "DFS_Iterativa": "#c2185b",
    "Dijkstra_Heap": "#3159b8",
}

sys.setrecursionlimit(max(3000, 10*MAX_NODES))

plt.style.use("dark_background")
sns.set_theme(style="darkgrid", rc={"axes.facecolor": COLOR_BG, "figure.facecolor": COLOR_BG})

def _style_standard_axes(figure, axis):
    figure.set_facecolor("white")
    axis.set_facecolor("white")
    axis.tick_params(colors="#222222")
    for spine in axis.spines.values():
        spine.set_color("#444444")


def plot_performance_curve(data_list):
    if not data_list:
        return
    algorithms = (
        ("BFS", PLOT_COLORS["BFS"], "o"),
        ("DFS_Iterativa", PLOT_COLORS["DFS_Iterativa"], "s"),
        ("Dijkstra_Heap", PLOT_COLORS["Dijkstra_Heap"], "D"),
    )

    figure, axis = plt.subplots(figsize=(14, 8), facecolor="white")
    _style_standard_axes(figure, axis)
    for algorithm, color, marker in algorithms:
        values = []
        for graph in data_list:
            stats = graph.get("benchmark_get_components", {}).get(algorithm, {})
            mean = stats.get("mean_ms", 0)
            median = stats.get("median_ms", 0)
            if mean > 0 and median > 0:
                values.append((graph["n"] + graph["m"], mean, median))
        values.sort(key=lambda item: item[0])
        if values:
            x_values = [item[0] for item in values]
            axis.plot(
                x_values,
                [item[1] for item in values],
                color=color,
                marker=marker,
                linewidth=2,
                label=f"{algorithm} (média)",
            )
            axis.plot(
                x_values,
                [item[2] for item in values],
                color=color,
                marker=marker,
                linestyle="--",
                linewidth=1.5,
                alpha=0.75,
                label=f"{algorithm} (mediana)",
            )

    axis.set_xscale("log")
    axis.set_yscale("log")
    axis.set_xlabel("Vértices + arestas do grafo (V + E)", color="#222222")
    axis.set_ylabel("Tempo de execução (ms)", color="#222222")
    axis.legend(
        facecolor="white",
        edgecolor="#444444",
        labelcolor="#222222",
        bbox_to_anchor=(1.04, 1),
        loc="upper left",
    )
    figure.tight_layout()
    figure.savefig(
        os.path.join(OUTPUT_DIR, "01_performance_curve_log.png"), dpi=300
    )
    plt.close(figure)

def plot_memory_curve(data_list):
    if not data_list:
        return
    x_values, memory_values = [], []
    for graph in data_list:
        value = graph["telemetria"].get("memoria_get_components_delta_mb", 0)
        if value > 0:
            x_values.append(graph.get("v_plus_e", graph["n"] + graph["m"]))
            memory_values.append(value)

    figure, axis = plt.subplots(figsize=(12, 7), facecolor="white")
    _style_standard_axes(figure, axis)
    axis.plot(
        x_values,
        memory_values,
        color="#a66b00",
        marker="D",
        linewidth=2,
        label="Variação de RSS em get_components()",
    )
    axis.set_xscale("log")
    axis.set_yscale("log")
    axis.set_xlabel("Quantidade de vértices + arestas do grafo (V + E)", color="#222222")
    axis.set_ylabel("Variação de RSS em get_components() (MB)", color="#222222")
    axis.legend(facecolor="white", edgecolor="#444444", labelcolor="#222222")
    figure.tight_layout()
    figure.savefig(os.path.join(OUTPUT_DIR, "02_memory_curve_log.png"), dpi=300)
    plt.close(figure)

def plot_time_profiling(data_list):
    if not data_list:
        return
    figure, axis = plt.subplots(figsize=(12, 7), facecolor="white")
    _style_standard_axes(figure, axis)
    for algorithm, color, marker in (
        ("BFS", PLOT_COLORS["BFS"], "o"),
        ("DFS_Iterativa", PLOT_COLORS["DFS_Iterativa"], "s"),
        ("Dijkstra_Heap", PLOT_COLORS["Dijkstra_Heap"], "D"),
    ):
        values = []
        for graph in data_list:
            value = (
                graph.get("benchmark_get_components", {})
                .get(algorithm, {})
                .get("mean_ms", 0)
            )
            if value > 0:
                values.append((graph["n"] + graph["m"], value))
        values.sort(key=lambda item: item[0])
        if values:
            axis.plot(
                [item[0] for item in values],
                [item[1] for item in values],
                color=color,
                marker=marker,
                linewidth=2,
                label=f"{algorithm} (média de get_components)",
            )

    axis.set_xscale("log")
    axis.set_yscale("log")
    axis.set_xlabel("Vértices + arestas do grafo (V + E)", color="#222222")
    axis.set_ylabel("Tempo de get_components() (ms)", color="#222222")
    axis.legend(
        facecolor="white",
        edgecolor="#444444",
        labelcolor="#222222",
        loc="upper left",
    )
    figure.tight_layout()
    figure.savefig(os.path.join(OUTPUT_DIR, "03_time_profiling_log.png"), dpi=300)
    plt.close(figure)


def plot_degree_histogram(csv_path, graph_id, v_plus_e):
    if not csv_path or not os.path.exists(csv_path): return
    
    df = pd.read_csv(csv_path)
    if df.empty: return
    
    figure, axis = plt.subplots(figsize=(10, 6), facecolor="white")
    _style_standard_axes(figure, axis)
    x_values = df["Grau"]
    axis.bar(
        x_values,
        df["Frequencia"],
        color=PLOT_COLORS["BFS"],
        alpha=0.8,
        edgecolor="white",
    )
    axis.set_xlabel("Grau do vértice", color="#222222")
    axis.set_ylabel("Frequência absoluta", color="#222222")
    axis.set_title(f"Distribuição de Graus - Grafo {graph_id}", color="#222222")
    figure.tight_layout()
    figure.savefig(
        os.path.join(OUTPUT_DIR, f"04_histograma_graus_grafo_{graph_id}.png"),
        dpi=300,
    )
    plt.close(figure)

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

def draw_network_tree_csv(
    csv_path, graph_id, v_plus_e, mode="BFS", component_id=None
):
    if not csv_path or not os.path.exists(csv_path): return
    
    df = pd.read_csv(csv_path)
    if df.empty: return
    if component_id is not None and "componente_conexa" in df.columns:
        df = df[df["componente_conexa"] == component_id]
        if df.empty:
            return

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
    
    plt.figure(figsize=(16, 12), facecolor=COLOR_BG)
    nx.draw_networkx_nodes(G, pos, node_size=node_sizes, node_color=node_colors, cmap=plt.cm.magma, edgecolors="none")
    
    tree_color = {
        "BFS": COLOR_BFS,
        "DFS": COLOR_DFS_ITER,
        "Dijkstra": COLOR_DIJKSTRA,
    }.get(mode, COLOR_DFS_ITER)
    nx.draw_networkx_edges(G, pos, edgelist=tree_edges, alpha=0.9, edge_color=tree_color, width=2.0)
    
    plt.axis("off")
    plt.tight_layout()
    plt.savefig(os.path.join(OUTPUT_DIR, f"05_arvore_topologia_grafo_{graph_id}_VE_{v_plus_e}_{mode}.png"), dpi=300)
    plt.close()

def draw_overlay_trees_csv(
    csv_bfs,
    csv_dfs,
    graph_id,
    v_plus_e,
    csv_dijkstra=None,
    component_id=None,
):
    paths = {"BFS": csv_bfs, "DFS": csv_dfs}
    if csv_dijkstra and os.path.exists(csv_dijkstra):
        paths["Dijkstra"] = csv_dijkstra
    if any(not path or not os.path.exists(path) for path in paths.values()):
        return

    frames = {
        name: pd.read_csv(path).sort_values(by="ordem_visitacao")
        for name, path in paths.items()
    }
    if component_id is not None:
        frames = {
            name: frame[
                frame["componente_conexa"] == component_id
            ].copy()
            if "componente_conexa" in frame.columns
            else frame
            for name, frame in frames.items()
        }
        if any(frame.empty for frame in frames.values()):
            return
    vertex_lists = {
        name: frame["vertice"].tolist() for name, frame in frames.items()
    }
    selected = {name: set() for name in frames}
    indices = {name: 0 for name in frames}

    while len(set.union(*selected.values())) < MAX_NODES:
        advanced = False
        for name, vertices in vertex_lists.items():
            if indices[name] >= len(vertices):
                continue
            selected[name].add(vertices[indices[name]])
            indices[name] += 1
            advanced = True
            if len(set.union(*selected.values())) >= MAX_NODES:
                break
        if not advanced:
            break

    def extract_tree_edges(frame, selected_vertices):
        edges = set()
        for _, row in frame.iterrows():
            parent, vertex = int(row["pai"]), int(row["vertice"])
            if (
                parent != 0
                and parent != vertex
                and parent in selected_vertices
                and vertex in selected_vertices
            ):
                edges.add(tuple(sorted((parent, vertex))))
        return edges

    edge_sets = {
        name: extract_tree_edges(frames[name], selected[name])
        for name in frames
    }
    valid_nodes = set.union(*selected.values())
    graph = nx.Graph()
    graph.add_nodes_from(valid_nodes)
    for edges in edge_sets.values():
        graph.add_edges_from(edges)
    graph.remove_nodes_from(list(nx.isolates(graph)))
    if not graph.nodes:
        return

    positions = nx.spring_layout(graph, seed=42, k=0.15, iterations=50)
    plt.figure(figsize=(16, 12), facecolor=COLOR_BG)
    nx.draw_networkx_nodes(
        graph,
        positions,
        node_size=20,
        node_color="#171717",
        edgecolors="none",
        alpha=0.8,
    )

    edge_owners = {}
    for algorithm, edges in edge_sets.items():
        for edge in edges:
            edge_owners.setdefault(edge, set()).add(algorithm)
    shared_edges = [
        edge for edge, owners in edge_owners.items() if len(owners) >= 2
    ]
    exclusive_edges = {
        algorithm: [
            edge for edge in edges if edge_owners[edge] == {algorithm}
        ]
        for algorithm, edges in edge_sets.items()
    }
    algorithm_colors = {
        "BFS": COLOR_BFS,
        "DFS": COLOR_DFS_ITER,
        "Dijkstra": COLOR_DIJKSTRA,
    }
    legend_handles = []
    if shared_edges:
        nx.draw_networkx_edges(
            graph,
            positions,
            edgelist=shared_edges,
            edge_color="white",
            width=2.0,
            alpha=0.85,
        )
        legend_handles.append(
            mlines.Line2D(
                [], [], color="white", linewidth=2, label="Compartilhada"
            )
        )
    for algorithm, edges in exclusive_edges.items():
        if not edges:
            continue
        color = algorithm_colors[algorithm]
        nx.draw_networkx_edges(
            graph,
            positions,
            edgelist=edges,
            edge_color=color,
            width=1.5,
            alpha=0.85,
            style="dashed" if algorithm == "DFS" else "solid",
        )
        legend_handles.append(
            mlines.Line2D(
                [],
                [],
                color=color,
                linewidth=1.5,
                linestyle="dashed" if algorithm == "DFS" else "solid",
                label=f"Exclusivas {algorithm}",
            )
        )

    plt.legend(
        handles=legend_handles,
        facecolor=COLOR_BG,
        edgecolor="white",
        labelcolor="white",
        loc="upper right",
        fontsize=12,
    )
    plt.axis("off")
    has_dijkstra = "Dijkstra" in paths
    algorithms = "BFS x DFS x Dijkstra" if has_dijkstra else "BFS x DFS"
    output_name = (
        "06_overlay_bfs_dfs_dijkstra"
        if has_dijkstra
        else "06_overlay_bfs_dfs"
    )
    plt.title(
        f"Sobreposição Topológica {algorithms} - Grafo {graph_id}",
        color="white",
        fontsize=16,
    )
    plt.tight_layout()
    plt.savefig(
        os.path.join(
            OUTPUT_DIR,
            f"{output_name}_grafo_{graph_id}_VE_{v_plus_e}.png",
        ),
        dpi=300,
    )
    plt.close()