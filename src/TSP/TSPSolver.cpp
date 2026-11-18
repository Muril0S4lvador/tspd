#include "TSPSolver.hpp"

#include "../agatz_solution/dfs/dfs.hpp"
#include "../agatz_solution/kruskal/kruskal.hpp"

#include <stdexcept>
#include <utility>
#include <algorithm>
#include <cmath>

namespace tspd::tsp {
    std::vector<int> TSPSolver::improveRoute(const ::tspd::tspd::TSPD& instance,
                                            std::vector<int> route) {
        double currentCost = instance.calculateTspCost(route);
        const int lastCustomer = static_cast<int>(route.size()) - 2;
        int neighborhood = 0;
        // Mesmo VND do Agatz: best improvement em 2-opt, relocate e swap.
        // Aqui o objetivo e o TSP puro, sem particionamento ou entregas do drone.
        while (neighborhood < 3) {
            auto bestRoute = route;
            double bestCost = currentCost;
            const double tolerance = 1e-9 * std::max(1.0, std::abs(currentCost));
            for (int first = 1; first <= lastCustomer; ++first) {
                const int firstDestination = neighborhood == 1 ? 1 : first + 1;
                for (int last = firstDestination; last <= lastCustomer; ++last) {
                    if (first == last) continue;
                    auto candidate = route;
                    if (neighborhood == 0) {
                        std::reverse(candidate.begin() + first, candidate.begin() + last + 1);
                    } else if (neighborhood == 1) {
                        const int customer = candidate[first];
                        candidate.erase(candidate.begin() + first);
                        candidate.insert(candidate.begin() + last, customer);
                    } else {
                        std::swap(candidate[first], candidate[last]);
                    }
                    const double candidateCost = instance.calculateTspCost(candidate);
                    if (candidateCost + tolerance < bestCost) {
                        bestCost = candidateCost;
                        bestRoute = std::move(candidate);
                    }
                }
            }
            if (bestCost + tolerance < currentCost) {
                route = std::move(bestRoute);
                currentCost = bestCost;
                neighborhood = 0;
            } else {
                ++neighborhood;
            }
        }
        return route;
    }

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
        route = improveRoute(instance, std::move(route));

        // Valida fechamento, cobertura e custo antes de disponibilizar o TSP.
        const double cost = instance.calculateTspCost(route);
        solution::Solution result{route, {}, cost, cost, 0.0};
        result.setTspRoute(std::move(route), cost);
        return result;
    }
}
