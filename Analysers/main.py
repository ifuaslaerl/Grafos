import os
import json
import glob
import re
from config import JSON_DIR, GRAUS_DIR, GRAFOS_DIR
import plotters

def extrair_id_grafo(filename):
    """Extrai o número 'X' de um arquivo como 'grafo_X.json' ou 'bfs_X.csv'."""
    base = os.path.basename(filename)
    name, _ = os.path.splitext(base)
    graph_match = re.search(r"^grafo_(.+)$", name)
    if graph_match:
        return graph_match.group(1)
    match = re.search(r'_(\d+)$', name)
    return match.group(1) if match else name

def carregar_dados():
    json_files = glob.glob(os.path.join(JSON_DIR, "*.json"))
    data_list = []
    
    for f in json_files:
        with open(f, "r", encoding="utf-8") as file:
            data = json.load(file)
            if "benchmark_get_components" not in data:
                raise ValueError(
                    f"JSON antigo ou incompatível: {f}. "
                    "Execute novamente gerar_dados.sh para gerar os benchmarks "
                    "de get_components()."
                )
            graph_id = extrair_id_grafo(f)
            data["graph_id"] = graph_id
            
            data["v_plus_e"] = data.get("v_plus_e", data["n"] + data["m"])
            
            data["csv_graus"] = os.path.join(GRAUS_DIR, f"grafo_{graph_id}.csv")
            data["csv_bfs"] = os.path.join(GRAFOS_DIR, f"bfs_{graph_id}.csv")
            data["csv_dfs"] = os.path.join(GRAFOS_DIR, f"dfs_{graph_id}.csv")
            data["csv_dijkstra"] = os.path.join(
                GRAFOS_DIR, f"dijkstra_{graph_id}.csv"
            )
            
            data_list.append(data)
            
    data_list.sort(key=lambda x: x["v_plus_e"])
    return data_list


def componente_topologia(graph):
    component = graph.get("componente_topologia")
    if component and component["id"] > 0:
        return component["id"], component["v_plus_e"]
    if not graph.get("tamanhos_componentes"):
        return None, graph["v_plus_e"]
    return None, graph["v_plus_e"]


if __name__ == "__main__":
    print("Iniciando leitura dos arquivos na estrutura Dados/...")
    dados = carregar_dados()
    
    if not dados:
        print(f"❌ Nenhum arquivo JSON encontrado em '{JSON_DIR}'. Verifique a estrutura de pastas.")
    else:
        print(f"✅ {len(dados)} grafos encontrados e processados. Gerando gráficos comparativos...")
        
        plotters.plot_performance_curve(dados)
        plotters.plot_memory_curve(dados)
        plotters.plot_time_profiling(dados)
        
        for graph in dados:
            g_id = graph["graph_id"]
            component_id, component_ve = componente_topologia(graph)
            
            print(
                f"   -> Desenhando topologias e histogramas para o Grafo {g_id}..."
            )
            
            plotters.plot_degree_histogram(
                graph["csv_graus"], g_id, graph["v_plus_e"]
            )
            
            # Gráficos Individuais
            plotters.draw_network_tree_csv(
                graph["csv_bfs"],
                g_id,
                component_ve,
                mode="BFS",
                component_id=component_id,
            )
            plotters.draw_network_tree_csv(
                graph["csv_dfs"],
                g_id,
                component_ve,
                mode="DFS",
                component_id=component_id,
            )
            plotters.draw_network_tree_csv(
                graph["csv_dijkstra"],
                g_id,
                component_ve,
                mode="Dijkstra",
                component_id=component_id,
            )
            
            plotters.draw_overlay_trees_csv(
                graph["csv_bfs"],
                graph["csv_dfs"],
                g_id,
                component_ve,
                graph["csv_dijkstra"],
                component_id,
            )
            
        print("\n🚀 Todos os gráficos foram gerados com sucesso na pasta 'Dados/Graficos'!")