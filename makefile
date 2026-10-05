# Compilador e flags de otimização máxima
CXX = g++
# A flag -I. avisa ao compilador que a raiz do projeto faz parte do path de includes
CXXFLAGS = -std=c++17 -O3 -march=native -flto -Wall -Wextra -I.
LDFLAGS = -flto

# Executáveis alvo
TARGETS = export_json export_grafo export_graus export_memoria
UNIT_TEST_BUILD_DIR = unit_tests/.build
JSON_OUTPUTS = Dados/JSON/*.json
CSV_OUTPUTS = Dados/Grafos/*.csv Dados/Graus/*.csv
ANALYSER_OUTPUTS = Dados/Graficos/*.png

# Regra padrão: compila todos os executáveis
all: $(TARGETS)

# Executável simples para testar a biblioteca ponderada
test_graph: src/main.o
	$(CXX) $(LDFLAGS) -o $@ $^

# Regras de linkagem buscando os objetos na pasta Exporters
export_json: Exporters/export_json.o
	$(CXX) $(LDFLAGS) -o $@ $^

export_grafo: Exporters/export_grafo.o
	$(CXX) $(LDFLAGS) -o $@ $^

export_graus: Exporters/export_graus.o
	$(CXX) $(LDFLAGS) -o $@ $^

export_memoria: Exporters/export_memoria.o
	$(CXX) $(LDFLAGS) -o $@ $^

# Regra genérica para transformar .cpp em .o respeitando as pastas
%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Limpeza dos binários/objetos e artefatos gerados por testes e análise
clean:
	rm -f Exporters/*.o src/*.o $(TARGETS) test_graph $(JSON_OUTPUTS) $(CSV_OUTPUTS) $(ANALYSER_OUTPUTS)
	rm -rf $(UNIT_TEST_BUILD_DIR)

.PHONY: all clean