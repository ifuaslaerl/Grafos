import os

# Resolve o caminho absoluto da pasta raiz (um nível acima de Analysers/)
ROOT_DIR = os.path.abspath(os.path.join(os.path.dirname(__file__), '..'))

# Estrutura de Diretórios
BASE_DIR = os.path.join(ROOT_DIR, "Dados")
OUTPUT_DIR = os.path.join(BASE_DIR, "Graficos")
JSON_DIR = os.path.join(BASE_DIR, "JSON")
GRAUS_DIR = os.path.join(BASE_DIR, "Graus")
GRAFOS_DIR = os.path.join(BASE_DIR, "Grafos")

# Paleta de Cores do Projeto
COLOR_BFS = "#00ffcc"
COLOR_DFS_ITER = "#ff007f"
COLOR_DFS_REC = "#ffcc00"
COLOR_BG = "#121212"

# Configurações de Renderização e Limites
MAX_NODES = 3000

# Garante que a pasta de saída exista
os.makedirs(OUTPUT_DIR, exist_ok=True)