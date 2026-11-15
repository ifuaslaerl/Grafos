#include "algorithm.hpp"

#include <queue>
#include <vector>

template<typename T = int>
class Bfs : public IAlgorithm<T>{
    using Base = IAlgorithm<T>;
    std::vector<int> parent_;
    std::vector<T> distance_;

public:
    using Base::Base;

    void run(int source) override{
        this->validate_vertex(source);
        std::vector<std::vector<int>> adjacency(this->vertex_count_ + 1);
        for(const auto& arc : this->arcs()){
            adjacency[arc.from].push_back(arc.to);
        }

        parent_.assign(this->vertex_count_ + 1, 0);
        distance_.assign(this->vertex_count_ + 1, Base::infinity());
        std::queue<int> queue;
        parent_[source] = source;
        distance_[source] = T{};
        queue.push(source);

        while(!queue.empty()){
            const int vertex = queue.front();
            queue.pop();
            for(int neighbor : adjacency[vertex]){
                if(parent_[neighbor] != 0){
                    continue;
                }
                parent_[neighbor] = vertex;
                distance_[neighbor] = distance_[vertex] + T{1};
                queue.push(neighbor);
            }
        }
    }

    T distance_to(int vertex) const override{
        this->validate_vertex(vertex);
        return distance_.at(vertex);
    }

    int parent_of(int vertex) const override{
        this->validate_vertex(vertex);
        return parent_.at(vertex);
    }
};
