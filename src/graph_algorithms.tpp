#ifndef GRAFO_ALGORITHMS_TPP
#define GRAFO_ALGORITHMS_TPP

#include <algorithm>
#include <functional>
#include <queue>
#include <stack>
#include <tuple>
#include <utility>

using namespace std;

namespace graph_lib{

template <typename T>
struct FrontierEntry{
    T cost;
    int vertex;
    int parent;
};

template <typename T>
class IFronteira{
public:
    virtual void push(T cost, int vertex, int parent) = 0;
    virtual FrontierEntry<T> pop() = 0;
    virtual bool empty() const = 0;
    virtual bool process(
        const FrontierEntry<T>& entry,
        vector<int>& parent,
        vector<T>& distance
    ) = 0;
    virtual void explore(
        int vertex,
        const Edge<T>& edge,
        vector<int>& parent,
        vector<T>& distance
    ) = 0;
    virtual ~IFronteira() = default;
};

template <typename T>
class BfsFrontier : public IFronteira<T>{
    queue<FrontierEntry<T>> queue_;

public:
    void push(T cost, int vertex, int parent) override{
        queue_.push({cost, vertex, parent});
    }

    FrontierEntry<T> pop() override{
        FrontierEntry<T> entry = queue_.front();
        queue_.pop();
        return entry;
    }

    bool empty() const override{
        return queue_.empty();
    }

    bool process(
        const FrontierEntry<T>& entry,
        vector<int>& parent,
        vector<T>& distance
    ) override{
        if(entry.vertex == entry.parent){
            return true;
        }
        if(parent[entry.vertex] != 0){
            return false;
        }

        parent[entry.vertex] = entry.parent;
        distance[entry.vertex] = distance[entry.parent] + T{1};
        return true;
    }

    void explore(
        int vertex,
        const Edge<T>& edge,
        vector<int>& parent,
        vector<T>&
    ) override{
        if(parent[edge.v] == 0){
            push(T{}, edge.v, vertex);
        }
    }
};

template <typename T>
class DfsFrontier : public IFronteira<T>{
    stack<FrontierEntry<T>> stack_;

public:
    void push(T cost, int vertex, int parent) override{
        stack_.push({cost, vertex, parent});
    }

    FrontierEntry<T> pop() override{
        FrontierEntry<T> entry = stack_.top();
        stack_.pop();
        return entry;
    }

    bool empty() const override{
        return stack_.empty();
    }

    bool process(
        const FrontierEntry<T>& entry,
        vector<int>& parent,
        vector<T>& distance
    ) override{
        if(entry.vertex == entry.parent){
            return true;
        }
        if(parent[entry.vertex] != 0){
            return false;
        }

        parent[entry.vertex] = entry.parent;
        distance[entry.vertex] = distance[entry.parent] + T{1};
        return true;
    }

    void explore(
        int vertex,
        const Edge<T>& edge,
        vector<int>& parent,
        vector<T>&
    ) override{
        if(parent[edge.v] == 0){
            push(T{}, edge.v, vertex);
        }
    }
};

template <typename T>
struct FrontierEntryGreater{
    bool operator()(
        const FrontierEntry<T>& lhs,
        const FrontierEntry<T>& rhs
    ) const{
        if(lhs.cost != rhs.cost){
            return lhs.cost > rhs.cost;
        }
        if(lhs.vertex != rhs.vertex){
            return lhs.vertex > rhs.vertex;
        }
        return lhs.parent > rhs.parent;
    }
};

template <typename T>
class DijkstraHeapFrontier : public IFronteira<T>{
    priority_queue<
        FrontierEntry<T>,
        vector<FrontierEntry<T>>,
        FrontierEntryGreater<T>
    > queue_;

public:
    void push(T cost, int vertex, int parent) override{
        queue_.push({cost, vertex, parent});
    }

    FrontierEntry<T> pop() override{
        FrontierEntry<T> entry = queue_.top();
        queue_.pop();
        return entry;
    }

    bool empty() const override{
        return queue_.empty();
    }

    bool process(
        const FrontierEntry<T>& entry,
        vector<int>&,
        vector<T>& distance
    ) override{
        return entry.cost == distance[entry.vertex];
    }

    void explore(
        int vertex,
        const Edge<T>& edge,
        vector<int>& parent,
        vector<T>& distance
    ) override{
        const T candidate = distance[vertex] + edge.w;
        if(candidate < distance[edge.v]){
            distance[edge.v] = candidate;
            parent[edge.v] = vertex;
            push(candidate, edge.v, vertex);
        }
    }
};

template <typename T>
class DijkstraVectorFrontier : public IFronteira<T>{
    vector<T> costs_;
    vector<int> parents_;
    vector<bool> pending_;
    int pending_count_;

public:
    explicit DijkstraVectorFrontier(int n)
        : costs_(n + 1),
          parents_(n + 1, 0),
          pending_(n + 1, false),
          pending_count_(0){}

    void push(T cost, int vertex, int parent) override{
        if(!pending_[vertex]){
            pending_[vertex] = true;
            ++pending_count_;
        }
        costs_[vertex] = cost;
        parents_[vertex] = parent;
    }

    FrontierEntry<T> pop() override{
        int best = 0;
        for(int vertex = 1; vertex < static_cast<int>(pending_.size()); ++vertex){
            if(!pending_[vertex]){
                continue;
            }
            if(best == 0 || costs_[vertex] < costs_[best] ||
                (costs_[vertex] == costs_[best] && vertex < best)){
                best = vertex;
            }
        }

        pending_[best] = false;
        --pending_count_;
        return {costs_[best], best, parents_[best]};
    }

    bool empty() const override{
        return pending_count_ == 0;
    }

    bool process(
        const FrontierEntry<T>& entry,
        vector<int>&,
        vector<T>& distance
    ) override{
        return entry.cost == distance[entry.vertex];
    }

    void explore(
        int vertex,
        const Edge<T>& edge,
        vector<int>& parent,
        vector<T>& distance
    ) override{
        const T candidate = distance[vertex] + edge.w;
        if(candidate < distance[edge.v] &&
            (parent[edge.v] == 0 || pending_[edge.v])){
            distance[edge.v] = candidate;
            parent[edge.v] = vertex;
            push(candidate, edge.v, vertex);
        }
    }
};

} // namespace graph_lib

template <typename T>
vector<int> Grafo<T>::traverse_impl(
    int start,
    vector<int>& parent,
    vector<T>& distance,
    graph_lib::IFronteira<T>& frontier,
    int& undirected_edge_count
){
    vector<int> order;
    int edge_visits = 0;
    undirected_edge_count = 0;
    if(start < 1 || start > n){
        return order;
    }

    parent[start] = start;
    distance[start] = T{};
    frontier.push(T{}, start, start);
    while(!frontier.empty()){
        const auto entry = frontier.pop();
        if(!frontier.process(entry, parent, distance)){
            continue;
        }

        order.push_back(entry.vertex);
        for(const Edge<T>& edge : adj[entry.vertex]){
            ++edge_visits;
            frontier.explore(entry.vertex, edge, parent, distance);
        }
    }

    undirected_edge_count = edge_visits / 2;
    return order;
}

template <typename T>
void Grafo<T>::get_components(
    vector<vector<int>>& components,
    vector<int>& component_edge_counts,
    graph_lib::IFronteira<T>& frontier
){
    components.clear();
    component_edge_counts.clear();
    vector<int> parent(n + 1, 0);
    vector<T> distance(n + 1, numeric_limits<T>::infinity());
    vector<pair<vector<int>, int>> components_with_edges;

    for(int vertex = 1; vertex <= n; ++vertex){
        if(parent[vertex] != 0){
            continue;
        }

        int edge_count = 0;
        vector<int> component =
            traverse_impl(vertex, parent, distance, frontier, edge_count);
        components_with_edges.push_back({move(component), edge_count});
    }

    sort(
        components_with_edges.begin(),
        components_with_edges.end(),
        [](const auto& lhs, const auto& rhs){
            return lhs.first.size() > rhs.first.size();
        }
    );
    components.reserve(components_with_edges.size());
    component_edge_counts.reserve(components_with_edges.size());
    for(auto& component : components_with_edges){
        components.push_back(move(component.first));
        component_edge_counts.push_back(component.second);
    }
}

template <typename T>
vector<int> Grafo<T>::dfs(
    int start,
    vector<int>& parent,
    vector<int>& distance
){
    parent.assign(n + 1, 0);
    distance.assign(n + 1, -1);

    vector<T> traversal_distance(n + 1, T{-1});
    graph_lib::DfsFrontier<T> frontier;
    int edge_count = 0;
    vector<int> order =
        traverse_impl(start, parent, traversal_distance, frontier, edge_count);
    for(int vertex : order){
        distance[vertex] = static_cast<int>(traversal_distance[vertex]);
    }
    return order;
}

template <typename T>
vector<int> Grafo<T>::bfs(
    int start,
    vector<int>& parent,
    vector<int>& distance
){
    parent.assign(n + 1, 0);
    distance.assign(n + 1, -1);

    vector<T> traversal_distance(n + 1, T{-1});
    graph_lib::BfsFrontier<T> frontier;
    int edge_count = 0;
    vector<int> order =
        traverse_impl(start, parent, traversal_distance, frontier, edge_count);
    for(int vertex : order){
        distance[vertex] = static_cast<int>(traversal_distance[vertex]);
    }
    return order;
}

template <typename T>
vector<int> Grafo<T>::dijkstra_vector(
    int start,
    vector<int>& parent,
    vector<T>& distance
){
    static_assert(
        numeric_limits<T>::has_infinity,
        "Dijkstra requires a weight type with infinity"
    );
    parent.assign(n + 1, 0);
    distance.assign(n + 1, numeric_limits<T>::infinity());
    if(start < 1 || start > n){
        return {};
    }

    parent[start] = start;
    distance[start] = T{};
    graph_lib::DijkstraVectorFrontier<T> frontier(n);
    int edge_count = 0;
    return traverse_impl(start, parent, distance, frontier, edge_count);
}

template <typename T>
vector<int> Grafo<T>::dijkstra_heap(
    int start,
    vector<int>& parent,
    vector<T>& distance
){
    static_assert(
        numeric_limits<T>::has_infinity,
        "Dijkstra requires a weight type with infinity"
    );
    parent.assign(n + 1, 0);
    distance.assign(n + 1, numeric_limits<T>::infinity());
    if(start < 1 || start > n){
        return {};
    }

    parent[start] = start;
    distance[start] = T{};
    graph_lib::DijkstraHeapFrontier<T> frontier;
    int edge_count = 0;
    return traverse_impl(start, parent, distance, frontier, edge_count);
}

template <typename T>
T Grafo<T>::get_dist(
    int u,
    int v,
    graph_lib::IFronteira<T>& frontier
){
    if(u < 1 || u > n || v < 1 || v > n){
        return numeric_limits<T>::infinity();
    }

    vector<int> parent(n + 1, 0);
    vector<T> distance(n + 1, numeric_limits<T>::infinity());
    distance[u] = T{};
    int edge_count = 0;
    traverse_impl(u, parent, distance, frontier, edge_count);
    return distance[v];
}

template <typename T>
int Grafo<T>::get_diameter(){
    int diameter = 0;
    vector<int> parent, distance;
    vector<vector<int>> components;
    vector<int> component_edge_counts;
    graph_lib::BfsFrontier<T> frontier;
    get_components(components, component_edge_counts, frontier);

    for(const vector<int>& component : components){
        if(component.empty()){
            continue;
        }

        bfs(component.front(), parent, distance);
        int farthest = component.front();
        for(int vertex : component){
            if(distance[vertex] > distance[farthest]){
                farthest = vertex;
            }
        }

        bfs(farthest, parent, distance);
        for(int vertex : component){
            diameter = max(diameter, distance[vertex]);
        }
    }
    return diameter;
}

#endif
