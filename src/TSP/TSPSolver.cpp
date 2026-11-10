#include "TSPSolver.hpp"

#include "../agatz_solution/dfs/dfs.hpp"
#include "../agatz_solution/kruskal/kruskal.hpp"

#include <stdexcept>
#include <utility>

namespace tspd::tsp {
    solution::Solution TSPSolver::Optimize(::tspd::tspd::TSPD& instance) {
        auto nodes = instance.getNodes();
        const int dimension = instance.getDimension();
        if (dimension < 1 || nodes.size() != static_cast<std::size_t>(dimension))
            throw std::invalid_argument("TSP requer uma instancia com deposito e dimensao consistente.");

        // Kruskal e DFS indexam os vetores pelos IDs dos nos.
        std::vector<bool> seen(nodes.size(), false);
        for (const auto& node : nodes) {
            const int id = node.getId();
            if (id < 0 || id >= dimension || seen[id])
                throw std::invalid_argument("TSP requer IDs unicos entre zero e a dimensao menos um.");
            seen[id] = true;
        }

        auto edges = instance.getEdges();
        if (edges.size() != nodes.size() * (nodes.size() - 1) / 2)
            throw std::invalid_argument("TSP requer as arestas do grafo completo da instancia.");
        for (const auto& edge : edges)
            if (edge.getAId() >= dimension || edge.getBId() >= dimension)
                throw std::invalid_argument("TSP recebeu uma aresta com ID fora da instancia.");

        std::vector<int> route{0};
        if (dimension > 1) {
            auto mst = ::tspd::kruskal::Kruskal::kruskal(nodes, edges);
            if (mst.size() != nodes.size() - 1)
                throw std::runtime_error("Kruskal nao produziu uma arvore cobrindo todos os nos.");
            route = ::tspd::dfs::DFS::getDFS(nodes, mst);
        }
        route.push_back(0);

        // Valida fechamento, cobertura e custo antes de disponibilizar o TSP.
        const double cost = instance.calculateTspCost(route);
        solution::Solution result{route, {}, cost, cost, 0.0};
        result.setTspRoute(std::move(route), cost);
        return result;
    }
}
