CXX = g++
CXXFLAGS = -std=c++17 -O3 -march=native -flto -Wall -Wextra -I.
LDFLAGS = -flto

PIPE_SOURCES = \
	src/benchmark.cpp \
	src/algorithm.hpp \
	src/graph_reader.hpp \
	src/run_measurement.hpp \
	src/bfs.cpp \
	src/dfs.cpp \
	src/djikstra.cpp \
	src/bellman_ford.cpp \
	src/floyd_warshall.cpp

all: pipe

pipe: $(PIPE_SOURCES)
	$(CXX) $(CXXFLAGS) -pthread -o $@ src/benchmark.cpp $(LDFLAGS) -pthread

clean:
	rm -f pipe

.PHONY: all clean