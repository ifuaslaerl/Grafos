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
    match = re.search(r'_(\d+)$', name)
    return match.group(1) if match else name

def carregar_dados():
    json_files = glob.glob(os.path.join(JSON_DIR, "*.json"))
    data_list = []
    
    for f in json_files:
        with open(f, "r", encoding="utf-8") as file:
            data = json.load(file)
            graph_id = extrair_id_grafo(f)
            data["graph_id"] = graph_id
            
            data["v_plus_e"] = data.get("v_plus_e", data["n"] + data["m"])
            
            data["csv_graus"] = os.path.join(GRAUS_DIR, f"grafo_{graph_id}.csv")
            data["csv_bfs"] = os.path.join(GRAFOS_DIR, f"bfs_{graph_id}.csv")
            data["csv_dfs"] = os.path.join(GRAFOS_DIR, f"dfs_{graph_id}.csv")
            
            data_list.append(data)
            
    data_list.sort(key=lambda x: x["v_plus_e"])
    return data_list

if __name__ == "__main__":
    print("Iniciando leitura dos arquivos na estrutura Dados/...")
    dados = carregar_dados()
    
    if not dados:
        print(f"❌ Nenhum arquivo JSON encontrado em '{JSON_DIR}'. Verifique a estrutura de pastas.")
    else:
        print(f"✅ {len(dados)} grafos encontrados e processados. Gerando gráficos comparativos...")
        
        plotters.plot_performance_curve(dados, scale="linear")
        plotters.plot_performance_curve(dados, scale="log")
        
        plotters.plot_memory_curve(dados, scale="linear")
        plotters.plot_memory_curve(dados, scale="log")
        
        plotters.plot_time_profiling(dados, scale="linear")
        plotters.plot_time_profiling(dados, scale="log")
        
        for graph in dados:
            g_id = graph["graph_id"]
            ve_total = graph["v_plus_e"]
            
            print(f"   -> Desenhando topologias e histogramas para o Grafo {g_id}...")
            
            plotters.plot_degree_histogram(graph["csv_graus"], g_id, ve_total)
            
            # Gráficos Individuais
            plotters.draw_network_tree_csv(graph["csv_bfs"], g_id, ve_total, mode="BFS")
            plotters.draw_network_tree_csv(graph["csv_dfs"], g_id, ve_total, mode="DFS")
            
            # Novo Gráfico de Sobreposição
            plotters.draw_overlay_trees_csv(graph["csv_bfs"], graph["csv_dfs"], g_id, ve_total)
            
        print("\n🚀 Todos os gráficos foram gerados com sucesso na pasta 'Dados/Graficos'!")