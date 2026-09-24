# Compilador e flags de otimização máxima
CXX = g++
# A flag -I. avisa ao compilador que a raiz do projeto faz parte do path de includes
CXXFLAGS = -std=c++17 -O3 -march=native -flto -Wall -Wextra -I.
LDFLAGS = -flto

# Executáveis alvo
TARGETS = export_json export_grafo export_graus

# Regra padrão: compila todos os executáveis
all: $(TARGETS)

# Regras de linkagem buscando os objetos na pasta Exporters
export_json: Exporters/export_json.o src/graph.o
	$(CXX) $(LDFLAGS) -o $@ $^

export_grafo: Exporters/export_grafo.o src/graph.o
	$(CXX) $(LDFLAGS) -o $@ $^

export_graus: Exporters/export_graus.o src/graph.o
	$(CXX) $(LDFLAGS) -o $@ $^

# Regra genérica para transformar .cpp em .o respeitando as pastas
%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Limpeza dos arquivos compilados nas respectivas pastas
clean:
	rm -f Exporters/*.o src/*.o $(TARGETS)

.PHONY: all clean