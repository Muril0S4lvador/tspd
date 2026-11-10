#include "instance_reader/instance_reader.hpp"
#include "solution_reader/solution_reader.hpp"
#include "agatz_solution/exact_partiotioning_algorithm/exactPartitioning.hpp"
#include "agatz_solution/greedy_heuristic/greedy_heuristic.hpp"
#include "TSP/TSPSolver.hpp"

#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>

namespace fs = std::filesystem;
using Instance = tspd::tspd::TSPD;

void check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

void near(double actual, double expected) {
    check(std::abs(actual - expected) < 1e-9 * std::max(1.0, std::abs(expected)),
          "Unexpected cost");
}

void write(const fs::path& path, const std::string& content) {
    std::ofstream file(path);
    file << content;
    check(static_cast<bool>(file), "Cannot write test fixture");
}

template<class Function>
void rejects(Function function) {
    bool rejected = false;
    try { function(); }
    catch (const std::invalid_argument&) { rejected = true; }
    check(rejected, "Invalid TSP accepted");
}

// Independent calculation from coordinates, rather than the stored edge matrix.
void checkComputedRoute(const Instance& instance, const tspd::solution::Solution& solution) {
    check(solution.getTspCost().has_value(), "Generated TSP cost missing");
    const auto nodes = instance.getNodes();
    const auto& route = solution.getTspNodes();
    check(route.size() == nodes.size() + 1 && route.front() == 0 && route.back() == 0,
          "Generated TSP is not closed");
    double expected = 0.0;
    for (std::size_t i = 1; i < route.size(); ++i) {
        const auto& first = nodes.at(route[i - 1]);
        const auto& second = nodes.at(route[i]);
        expected += std::hypot(first.getX() - second.getX(), first.getY() - second.getY())
            * instance.getTruckSpeed();
    }
    near(*solution.getTspCost(), expected);
    check(solution.getTotalCost() <= expected + 1e-8, "TSP-D exceeds its full truck route");
}

int main(int argc, char** argv) {
    try {
        const auto directory = fs::path("build") / "tsp-reference-fixtures";
        fs::create_directories(directory / "solutions");
        const auto instancePath = directory / "triangle.txt";
        const auto tspPath = directory / "solutions" / "triangle-tsp.txt";
        const auto dpPath = directory / "solutions" / "triangle-DP.txt";
        write(instancePath,
            "/*The speed of the Truck*/\n2\n/*The speed of the Drone*/\n1\n"
            "/*Number of Nodes*/\n3\n/*The Depot*/\n0 0 depot\n"
            "/*The Locations (x_coor y_coor name)*/\n3 0 a\n0 4 b\n");
        // All fixture mutations are confined to these two files under build/.
        fs::remove(dpPath);
        fs::remove(tspPath);
        auto instance = tspd::instance_reader::InstanceReader::readInstance(instancePath.string());
        check(!instance.hasLiteratureSolution() && !instance.hasLiteratureTspSolution(),
              "Unexpected reference without files");
        near(instance.calculateTspCost({0, 1, 2, 0}), 24.0);
        rejects([&] { instance.calculateTspCost({0, 1, 0}); });
        rejects([&] { instance.calculateTspCost({0, 1, 1, 0}); });
        rejects([&] { instance.calculateTspCost({0, 1, 2, 1}); });
        rejects([&] { instance.calculateTspCost({0, 1, 8, 0}); });

        write(tspPath, "1\n0 0 -1 2 1 2\n");
        instance = tspd::instance_reader::InstanceReader::readInstance(instancePath.string());
        check(instance.hasLiteratureTspSolution(), "TSP-only reference missing");
        near(instance.getLiteratureTspSolution().getTotalCost(), 24.0);
        near(instance.getLiteratureSolution().getTotalCost(), 24.0);
        check(instance.getLiteratureTspSolution().getTruckEdges().size() == 3,
              "Internal edges or return edge missing");

        write(dpPath, "/* Total cost: 8 */\n1\n0 0 2 1 1\n");
        instance = tspd::instance_reader::InstanceReader::readInstance(instancePath.string());
        near(instance.getLiteratureSolution().getTotalCost(), 8.0);
        near(instance.getLiteratureTspSolution().getTotalCost(), 24.0);
        write(tspPath, "/* Total cost: 999 */\n3\n0 1 -1 0\n1 2 -1 0\n2 0 -1 0\n");
        near(tspd::solution_reader::SolutionReader::readTspSolution(
            instancePath.string(), instance)->getTotalCost(), 24.0);

        for (const auto& invalid : {
            "1\n0 0 2 1 1\n", // Drone in a TSP file.
            "1\n0 0 -1 2 1 1\n", // Duplicate and omitted customer.
            "1\n0 0 -1 1 1\n", // Missing customer.
            "1\n0 1 -1 1 2\n", // No return to depot.
            "1\n0 0 -1 2 1 9\n"}) { // Unknown ID.
            write(tspPath, invalid);
            rejects([&] { tspd::solution_reader::SolutionReader::readTspSolution(
                instancePath.string(), instance); });
        }
        fs::remove(tspPath);
        instance = tspd::instance_reader::InstanceReader::readInstance(instancePath.string());
        check(instance.hasLiteratureSolution() && !instance.hasLiteratureTspSolution(),
              "DP-only reference changed");

        tspd::exactPartitioning::ExactPartitioning exact;
        tspd::greedyHeuristic::GreedyHeuristic greedy;
        tspd::tsp::TSPSolver tspSolver;
        const auto initialTsp = tspSolver.Solve(instance);
        checkComputedRoute(instance, initialTsp);
        checkComputedRoute(instance, exact.Solve(instance, initialTsp));
        checkComputedRoute(instance, greedy.Solve(instance, initialTsp));
        if (argc > 1) {
            instance = tspd::instance_reader::InstanceReader::readInstance(argv[1]);
            check(instance.hasLiteratureTspSolution(), "Dataset TSP missing");
            const auto tsp = instance.getLiteratureTspSolution();
            double independentCost = 0.0;
            const auto nodes = instance.getNodes();
            for (const auto& edge : tsp.getTruckEdges()) {
                const auto& first = nodes.at(edge.getAId());
                const auto& second = nodes.at(edge.getBId());
                independentCost += std::hypot(first.getX() - second.getX(),
                    first.getY() - second.getY()) * instance.getTruckSpeed();
            }
            near(tsp.getTotalCost(), independentCost);
            const auto datasetTsp = tspSolver.Solve(instance);
            const auto exactResult = exact.Solve(instance, datasetTsp);
            const auto greedyResult = greedy.Solve(instance, datasetTsp);
            checkComputedRoute(instance, exactResult);
            checkComputedRoute(instance, greedyResult);
            std::cout << "Dataset: reference TSP=" << tsp.getTotalCost()
                << ", exact route TSP=" << *exactResult.getTspCost()
                << ", greedy route TSP=" << *greedyResult.getTspCost() << '\n';
        }
        std::cout << "TSP reference tests passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
