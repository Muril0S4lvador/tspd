#include "TSP/TSPSolver.hpp"
#include "agatz_solution/exact_partiotioning_algorithm/exactPartitioning.hpp"
#include "agatz_solution/greedy_heuristic/greedy_heuristic.hpp"
#include "instance_reader/instance_reader.hpp"
#include "hgvns/HGVNS.hpp"

#include <cmath>
#include <iostream>
#include <limits>
#include <memory>
#include <stdexcept>

using Instance = tspd::tspd::TSPD;
using Solution = tspd::solution::Solution;
using Strategy = tspd::solution::SolutionStrategy;
using Node = tspd::node::Node;
using Edge = tspd::edge::Edge;

void check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

template<class Function>
void rejects(Function function, const std::string& message) {
    try {
        function();
    } catch (const std::exception& error) {
        check(std::string(error.what()).find(message) != std::string::npos,
              "Failure did not identify the missing or invalid TSP dependency");
        return;
    }
    throw std::runtime_error("Invalid or unimplemented TSP input was accepted");
}

Instance triangle(double truckFactor = 2.0) {
    std::vector<Node> nodes{{0, 0, 0, "depot"}, {3, 0, 1, "a"}, {0, 4, 2, "b"}};
    return Instance("triangle", "TSP-D", 3, truckFactor, 1.0, nodes.front(), nodes,
                    tspd::instance_reader::InstanceReader::calculateEdges(nodes));
}

Solution supplied(std::vector<int> route, double cost) {
    Solution result{route, {}, cost, cost, 0};
    result.setTspRoute(std::move(route), cost);
    return result;
}

// Represents a future heuristic that forgot to implement the precomputed input.
class MissingTspInput : public Strategy {
    public:
        Solution Optimize(Instance&) override { return Solution{{}, {}, 0, 0, 0}; }
};

int main() {
    try {
        auto instance = triangle();
        std::unique_ptr<Strategy> tspSolver = std::make_unique<tspd::tsp::TSPSolver>();
        const auto initial = tspSolver->Solve(instance);
        const auto initialRoute = initial.getTspNodes();
        check(initialRoute == std::vector<int>({0, 1, 2, 0}), "Unexpected MST preorder");
        // Independent 3-4-5 triangle perimeter with truck factor two.
        check(initial.getTspCost() && std::abs(*initial.getTspCost() - 24.0) < 1e-12,
              "TSP route cost is not the independently known perimeter");
        check(initial.getTotalCost() == 24 && initial.getTruckCost() == 24 &&
              initial.getDroneCost() == 0 && initial.getDroneNodes().empty(),
              "Standalone TSP is not represented as a truck-only Solution");

        std::vector<std::unique_ptr<Strategy>> heuristics;
        heuristics.push_back(std::make_unique<tspd::greedyHeuristic::GreedyHeuristic>());
        heuristics.push_back(std::make_unique<tspd::exactPartitioning::ExactPartitioning>());
        heuristics.push_back(std::make_unique<tspd::hgvns::HGVNS>());
        for (auto& heuristic : heuristics) {
            rejects([&] { heuristic->Solve(instance); }, "TSP previamente calculado");
            rejects([&] { heuristic->Optimize(instance); }, "TSP previamente calculado");
            rejects([&] { heuristic->Solve(instance, Solution{{}, {}, 0, 0, 0}); }, "Solucao TSP previa");
            for (const auto& invalid : {
                supplied({0, 1, 2}, 24),
                supplied({0, 1, 1, 0}, 24),
                supplied({0, 1, 3, 0}, 24),
                supplied({1, 0, 2, 1}, 24),
            }) {
                rejects([&] { heuristic->Solve(instance, invalid); }, "TSP route");
                rejects([&] { heuristic->Optimize(instance, invalid); }, "TSP route");
            }
            rejects([&] { heuristic->Solve(instance, supplied(initialRoute, 25)); }, "Custo do TSP");
            rejects([&] { heuristic->Solve(instance, supplied(initialRoute,
                std::numeric_limits<double>::infinity())); }, "Solucao TSP previa");
            auto differentInstance = triangle(3);
            rejects([&] { heuristic->Solve(differentInstance, initial); }, "Custo do TSP");

            const auto result = heuristic->Solve(instance, initial);
            check(std::isfinite(result.getTotalCost()) && result.getTotalCost() <= 24.0,
                  "Heuristic failed after receiving a valid precomputed TSP");
            check(initial.getTspNodes() == initialRoute && *initial.getTspCost() == 24.0,
                  "A heuristic mutated the shared initial TSP");
            check(heuristic->getDuration().count() >= 0, "Invalid optimization duration");
        }

        MissingTspInput missingInput;
        rejects([&] { missingInput.Solve(instance, initial); }, "nao implementa Optimize");

        const Node depot{0, 0, 0, "depot"};
        Instance empty("empty", "TSP-D", 0, 1, 1, depot, {});
        rejects([&] { tspSolver->Solve(empty); }, "dimensao consistente");
        Instance incomplete("incomplete", "TSP-D", 3, 2, 1, depot, instance.getNodes(), {});
        rejects([&] { tspSolver->Solve(incomplete); }, "grafo completo");
        Instance invalidIds("ids", "TSP-D", 3, 2, 1, depot,
            {depot, Node{3, 0, 1, "a"}, Node{0, 4, 1, "b"}}, instance.getEdges());
        rejects([&] { tspSolver->Solve(invalidIds); }, "IDs unicos");
        Instance invalidEdge("edge", "TSP-D", 3, 2, 1, depot, instance.getNodes(),
            {Edge{0, 1, 3}, Edge{0, 3, 4}, Edge{1, 2, 5}});
        rejects([&] { tspSolver->Solve(invalidEdge); }, "aresta com ID fora");

        Instance onlyDepot("depot", "TSP-D", 1, 1, 1, depot, {depot});
        const auto trivial = tspSolver->Solve(onlyDepot);
        check(trivial.getTspNodes() == std::vector<int>({0, 0}) && trivial.getTotalCost() == 0,
              "Depot-only TSP is not closed or has nonzero cost");
        std::cout << "TSP solver and precomputed-input safety tests passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
