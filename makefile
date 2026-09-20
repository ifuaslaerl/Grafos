CXX = g++
CXXFLAGS = -O2 -std=c++17 -Wall

a.out: main.cpp src/graph.cpp src/graph.hpp
	$(CXX) $(CXXFLAGS) main.cpp src/graph.cpp -o a.out

clean:
	rm -f a.out

.PHONY: clean