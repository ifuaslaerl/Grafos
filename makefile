# Compilador e flags de otimização máxima
CXX = g++
CXXFLAGS = -std=c++17 -O3 -march=native -flto -Wall -Wextra
LDFLAGS = -flto

# Nomes dos executáveis
TARGET_MAIN = main.out
TARGET_EXPORT = export.out

# Regra padrão: compila os dois executáveis
all: $(TARGET_MAIN) $(TARGET_EXPORT)

# Linkagem do main.out
$(TARGET_MAIN): main.o src/graph.o
	$(CXX) $(LDFLAGS) -o $@ $^

# Linkagem do export.out
$(TARGET_EXPORT): export_json.o src/graph.o
	$(CXX) $(LDFLAGS) -o $@ $^

# Regra genérica para transformar .cpp em .o
%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Limpeza dos arquivos compilados
clean:
	rm -f *.o src/*.o $(TARGET_MAIN) $(TARGET_EXPORT)

.PHONY: all clean