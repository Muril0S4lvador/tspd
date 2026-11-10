#include "hgvns/HGVNS.hpp"
#include "hgvns/neighborhoods.hpp"
#include "instance_reader/instance_reader.hpp"
#include "TSP/TSPSolver.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <iostream>
#include <limits>
#include <numeric>
#include <random>
#include <set>
#include <stdexcept>

using namespace tspd::hgvns::detail;
using Instance = tspd::tspd::TSPD;
using Node = tspd::node::Node;

void check(bool value, const char* message) {
    if (!value) throw std::runtime_error(message);
}

void near(double actual, double expected) {
    check(std::abs(actual - expected) < 1e-7 * std::max(1.0, std::abs(expected)), "Independent cost mismatch");
}

template<class Function>
void rejects(Function function) {
    try { function(); }
    catch (const std::invalid_argument&) { return; }
    throw std::runtime_error("Invalid HGVNS state was accepted");
}

Instance makeInstance(int n, unsigned seed, double truck = 1, double drone = 0.5) {
    std::mt19937 random(seed);
    std::uniform_real_distribution<double> coordinate(-20, 20);
    std::vector<Node> nodes;
    for (int i = 0; i < n; ++i) nodes.emplace_back(coordinate(random), coordinate(random), i, std::to_string(i));
    return Instance("hgvns-fixture", "TSP-D", n, truck, drone, nodes.front(), nodes,
                    tspd::instance_reader::InstanceReader::calculateEdges(nodes));
}

// Independent event-by-event simulation from coordinates, without cached matrices,
// prefix sums, edge deltas, or the evaluator's truck-plus-waiting formula.
Evaluation independent(const Instance& instance, const State& state) {
    const auto nodes = instance.getNodes();
    const auto distance = [&](int a, int b) {
        return std::hypot(nodes[a].getX() - nodes[b].getX(), nodes[a].getY() - nodes[b].getY());
    };
    const auto truckSegment = [&](std::size_t first, std::size_t last) {
        double value = 0;
        for (auto p = first; p < last; ++p)
            value += distance(state.truck[p], state.truck[p + 1]) * instance.getTruckSpeed();
        return value;
    };
    Evaluation result;
    result.truck = truckSegment(0, state.truck.size() - 1);
    std::size_t cursor = 0;
    for (const auto& flight : state.flights) {
        result.total += truckSegment(cursor, flight.launch);
        const double air = (distance(state.truck[flight.launch], flight.customer)
                          + distance(flight.customer, state.truck[flight.recovery])) * instance.getDroneSpeed();
        result.drone += air;
        result.total += std::max(air, truckSegment(flight.launch, flight.recovery));
        cursor = flight.recovery;
    }
    result.total += truckSegment(cursor, state.truck.size() - 1);
    return result;
}

std::vector<std::vector<int>> oracleTruckRoutes(const State& source, Neighborhood kind) {
    std::vector<std::vector<int>> result;
    const int n = static_cast<int>(source.truck.size()) - 2;
    for (int p = 1; p <= n; ++p) {
        for (int q = 1; q <= n; ++q) {
            if (p == q) continue;
            auto route = source.truck;
            if (kind == Neighborhood::Reinsertion) {
                const int node = route[p];
                route.erase(route.begin() + p);
                route.insert(route.begin() + q, node);
            } else if (kind == Neighborhood::OrOpt2) {
                if (p == n || q == n) continue;
                std::vector<int> pair{route[p], route[p + 1]};
                route.erase(route.begin() + p, route.begin() + p + 2);
                route.insert(route.begin() + q, pair.begin(), pair.end());
            } else if (kind == Neighborhood::TwoOpt) {
                if (q < p || (p == 1 && q == n)) continue;
                std::reverse(route.begin() + p, route.begin() + q + 1);
            } else if (kind == Neighborhood::Exchange) {
                if (q < p) continue;
                std::swap(route[p], route[q]);
            } else {
                const int a = 2, b = kind == Neighborhood::Exchange22 ? 2 : 1;
                if (p + a - 1 > n || q + b - 1 > n || (p < q + b && q < p + a)) continue;
                // Assemble the exchanged sequence by labeling the two blocks first.
                route.clear();
                for (int i = 0; i < static_cast<int>(source.truck.size());) {
                    if (i == p) {
                        route.insert(route.end(), source.truck.begin() + q, source.truck.begin() + q + b);
                        i += a;
                    } else if (i == q) {
                        route.insert(route.end(), source.truck.begin() + p, source.truck.begin() + p + a);
                        i += b;
                    } else {
                        route.push_back(source.truck[i++]);
                    }
                }
            }
            result.push_back(std::move(route));
        }
    }
    return result;
}

double oracleBest(const Instance& instance, const Context& context, const State& source, Neighborhood kind) {
    double best = independent(instance, source).total;
    if (kind != Neighborhood::RelocateCustomer) {
        for (auto route : oracleTruckRoutes(source, kind)) {
            auto candidate = rebindFlights(context, source, std::move(route));
            if (!candidate) continue;
            validate(context, *candidate);
            const auto expected = independent(instance, *candidate);
            near(evaluate(context, *candidate).total, expected.total);
            best = std::min(best, expected.total);
        }
    } else {
        for (std::size_t j = 1; j + 1 < source.truck.size(); ++j) {
            bool endpoint = false;
            for (const auto& f : source.flights) endpoint |= j == f.launch || j == f.recovery;
            if (endpoint) continue;
            for (std::size_t i = 0; i + 1 < source.truck.size(); ++i) {
                if (i == j) continue;
                for (std::size_t k = i; k < source.truck.size(); ++k) {
                    if (k == j) continue;
                    const auto first = std::min(j, i), last = std::max(j, k);
                    bool crosses = false;
                    for (const auto& f : source.flights) crosses |= first < f.recovery && last > f.launch;
                    if (crosses) continue;
                    State candidate = source;
                    const int customer = candidate.truck[j];
                    candidate.truck.erase(candidate.truck.begin() + j);
                    for (auto& f : candidate.flights) {
                        if (f.launch > j) --f.launch;
                        if (f.recovery > j) --f.recovery;
                    }
                    candidate.flights.push_back({i > j ? i - 1 : i, customer, k > j ? k - 1 : k});
                    sortFlights(candidate);
                    validate(context, candidate);
                    const auto expected = independent(instance, candidate);
                    near(evaluate(context, candidate).total, expected.total);
                    best = std::min(best, expected.total);
                }
            }
        }
    }
    return best;
}

// Exhaustive operation enumeration, without DP tables or prefix-sum costs.
double enumeratePartitions(const Instance& instance, const std::vector<int>& order, std::size_t i) {
    if (i + 1 == order.size()) return 0;
    auto distance = [&](int a, int b, double factor) {
        const auto nodes = instance.getNodes();
        return std::hypot(nodes[a].getX() - nodes[b].getX(), nodes[a].getY() - nodes[b].getY()) * factor;
    };
    double best = distance(order[i], order[i + 1], instance.getTruckSpeed())
                + enumeratePartitions(instance, order, i + 1);
    for (auto j = i + 2; j < order.size(); ++j) {
        for (auto k = i + 1; k < j; ++k) {
            double truck = 0;
            auto previous = i;
            for (auto p = i + 1; p <= j; ++p) if (p != k) {
                truck += distance(order[previous], order[p], instance.getTruckSpeed());
                previous = p;
            }
            const double drone = distance(order[i], order[k], instance.getDroneSpeed())
                               + distance(order[k], order[j], instance.getDroneSpeed());
            best = std::min(best, std::max(truck, drone) + enumeratePartitions(instance, order, j));
        }
    }
    return best;
}

void blockTests() {
    using namespace tspd::hgvns;
    std::size_t accepted = 0;
    std::size_t orderGains = 0;
    std::set<std::size_t> randomChoices;
    for (unsigned seed = 0; seed < 30; ++seed) {
        auto instance = makeInstance(9, seed);
        Context context(instance);
        std::vector<int> order{0, 1, 2, 3, 4, 5, 6, 0};
        const auto decoded = partitionPath(context, order);
        near(evaluate(context, decoded, false).total, enumeratePartitions(instance, order, 0));
        State source{{0, 1, 3, 5, 7, 0}, {{0, 2, 1}, {1, 4, 2}, {2, 6, 3}, {3, 8, 4}}};
        BlockStatistics waiting, random, repeated;
        const auto a = postprocessBlocks(context, source, BlockSelection::Waiting, seed, {3, 2}, &waiting);
        const auto b = postprocessBlocks(context, source, BlockSelection::Random, seed, {3, 2}, &random);
        const auto c = postprocessBlocks(context, source, BlockSelection::Random, seed, {3, 2}, &repeated);
        check(waiting.selectedOperations.size() == 2 && random.selectedOperations.size() == 2,
              "Policies used different DP budgets");
        check(waiting.selectedWaiting[0] >= waiting.selectedWaiting[1], "Waiting priority reversed");
        check(random.selectedOperations == repeated.selectedOperations, "Block seed not reproducible");
        randomChoices.insert(random.selectedOperations.front());
        check(b.truck == c.truck, "Reconstruction not reproducible");
        for (const auto& result : {a, b}) {
            validate(context, result);
            near(evaluate(context, result).total, independent(instance, result).total);
            check(evaluate(context, result).total <= evaluate(context, source).total + 1e-8,
                  "Block DP worsened solution");
        }
        accepted += waiting.accepted + random.accepted;
        State stationary{{0, 1, 3, 5, 7, 0}, {{0, 2, 0}, {0, 4, 0}, {2, 6, 2}, {4, 8, 5}}};
        validate(context, postprocessBlocks(context, stationary, BlockSelection::Waiting, seed, {5, 3}));
        for (auto policy : {BlockSelection::Waiting, BlockSelection::Random}) {
            for (std::size_t limit : {3u, 4u, 5u, 8u, 20u}) {
                for (const auto& input : {source, stationary}) {
                    const auto result = postprocessBlocks(context, input, policy, seed, {limit, 2});
                    validate(context, result);
                    near(evaluate(context, result).total, independent(instance, result).total);
                    check(evaluate(context, result).total <= evaluate(context, input).total + 1e-8,
                          "Mixed/stationary block worsened");
                }
            }
        }
        auto truckOnly = State{{0, 1, 2, 3, 4, 5, 6, 7, 8, 0}, {}};
        BlockStatistics improvement;
        const auto improved = postprocessBlocks(context, truckOnly, BlockSelection::Waiting, seed, {}, &improvement);
        const auto single = postprocessBlocks(context, truckOnly, BlockSelection::Waiting, seed, {20, 5, 1});
        check(evaluate(context, improved).total <= evaluate(context, single).total + 1e-8,
              "Multiple orders lost the single-order incumbent");
        orderGains += improves(evaluate(context, improved).total, evaluate(context, single).total);
        check(improvement.ordersEvaluated > 1 && improvement.ordersEvaluated <= 32,
              "Order budget was ignored");
        const auto repeatOrders = postprocessBlocks(context, truckOnly, BlockSelection::Waiting, seed);
        check(improved.truck == repeatOrders.truck && improved.flights.size() == repeatOrders.flights.size(),
              "Multiple-order search not reproducible");
        accepted += improvement.accepted;
        check(evaluate(context, improved).total <= evaluate(context, truckOnly).total + 1e-8,
              "Truck-only block worsened");
    }
    check(randomChoices.size() > 1, "Random selection ignored seed");
    check(accepted > 0, "No fixture exercised accepting an improved block");
    check(orderGains > 0, "Multiple orders never improved the fixed-order partition");
    for (auto placement : {PDPlacement::Initial, PDPlacement::AfterShake, PDPlacement::AfterRVND, PDPlacement::Final}) {
        auto instance = makeInstance(8, 123);
        tspd::tsp::TSPSolver tsp;
        const auto tour = tsp.Solve(instance);
        HGVNS baseline(42);
        const auto reference = baseline.Solve(instance, tour);
        for (auto policy : {BlockSelection::Waiting, BlockSelection::Random}) {
            HGVNS disabled(42, policy, {20, 0, 8}, placement);
            const auto same = disabled.Solve(instance, tour);
            near(same.getTotalCost(), reference.getTotalCost());
            check(same.getTruckNodes() == reference.getTruckNodes(), "Disabled PD changed GVNS random stream");
            check(disabled.statistics().pdCalls == 0, "Disabled PD was executed");
            HGVNS enabled(42, policy, {6, 2, 8}, placement), repeat(42, policy, {6, 2, 8}, placement);
            const auto a = enabled.Solve(instance, tour), b = repeat.Solve(instance, tour);
            near(a.getTotalCost(), b.getTotalCost());
            check(a.getTruckNodes() == b.getTruckNodes() && a.getDroneNodes() == b.getDroneNodes(),
                  "PD placement not reproducible");
            check(enabled.statistics().pdCalls > 0 && enabled.statistics().ordersEvaluated > 0,
                  "PD placement was not invoked");
            if (placement == PDPlacement::Final || placement == PDPlacement::Initial)
                check(enabled.statistics().pdCalls == 1, "Single-pass placement repeated PD");
            if (placement == PDPlacement::Final)
                check(a.getTotalCost() <= reference.getTotalCost() + 1e-8, "Final PD worsened baseline HGVNS");
        }
    }
}

int main() {
    try {
        blockTests();
        std::vector<Node> triangleNodes{{0, 0, 0, "depot"}, {3, 0, 1, "a"}, {0, 4, 2, "b"}};
        Instance triangle("triangle", "TSP-D", 3, 2, 1, triangleNodes.front(), triangleNodes,
                          tspd::instance_reader::InstanceReader::calculateEdges(triangleNodes));
        Context triangleContext(triangle);
        near(evaluate(triangleContext, {{0, 1, 0}, {{0, 2, 1}}}).total, 15);
        near(evaluate(triangleContext, {{0, 1, 0}, {{0, 2, 2}}}).total, 12);
        near(evaluate(triangleContext, {{0, 1, 0}, {{1, 2, 1}}}).total, 22);
        near(evaluate(triangleContext, {{0, 0}, {{0, 1, 0}, {0, 2, 0}}}).total, 14);
        near(evaluate(triangleContext, {{0, 1, 2, 0}, {}}).total, 24);
        rejects([&] { evaluate(triangleContext, {{0, 1, 0}, {}}); });
        rejects([&] { evaluate(triangleContext, {{0, 1, 2}, {}}); });
        rejects([&] { evaluate(triangleContext, {{0, 1, 0}, {{0, 1, 2}}}); });
        rejects([&] { evaluate(triangleContext, {{0, 1, 0}, {{2, 2, 2}}}); });

        auto eight = makeInstance(8, 8);
        Context eightContext(eight);
        rejects([&] { evaluate(eightContext, {{0, 1, 2, 3, 4, 5, 0}, {{1, 6, 4}, {2, 7, 5}}}); });
        rejects([&] { evaluate(eightContext, {{0, 1, 2, 3, 4, 5, 0}, {{1, 6, 5}, {2, 7, 4}}}); });

        // The article's mixed-node examples: all six truck transformations preserve
        // deliveries and produce feasible flights under the documented repair policy.
        const State figure{{0, 5, 3, 1, 6, 2, 0}, {{1, 7, 3}, {3, 4, 4}}};
        validate(eightContext, figure);
        for (auto route : std::vector<std::vector<int>>{
            {0, 5, 1, 6, 2, 3, 0}, {0, 1, 6, 2, 5, 3, 0},
            {0, 5, 3, 2, 6, 1, 0}, {0, 5, 2, 6, 3, 1, 0},
            {0, 6, 2, 1, 5, 3, 0}, {0, 5, 2, 6, 1, 3, 0}}) {
            auto candidate = rebindFlights(eightContext, figure, std::move(route));
            check(candidate.has_value(), "Mixed-node repair rejected a figure example");
            validate(eightContext, *candidate);
            const auto actual = evaluate(eightContext, *candidate);
            const auto expected = independent(eight, *candidate);
            near(actual.total, expected.total);
            near(actual.truck, expected.truck);
            near(actual.drone, expected.drone);
        }

        // Check the repair recurrence against all combinations of its documented
        // endpoint alternatives for the Exchange example, without reusing its DP.
        const std::vector<int> exchanged{0, 5, 3, 2, 6, 1, 0};
        double exhaustiveRepair = std::numeric_limits<double>::infinity();
        for (std::size_t a : {1u, 3u, 5u}) for (std::size_t b : {1u, 3u, 5u})
            for (std::size_t c : {3u, 4u, 5u}) for (std::size_t d : {3u, 4u, 5u}) {
                if (a > b || b > c || c > d) continue;
                State alternative{exchanged, {{a, 7, b}, {c, 4, d}}};
                validate(eightContext, alternative);
                exhaustiveRepair = std::min(exhaustiveRepair, independent(eight, alternative).total);
            }
        near(evaluate(eightContext, *rebindFlights(eightContext, figure, exchanged)).total, exhaustiveRepair);

        constexpr std::array<Neighborhood, 7> kinds{Neighborhood::Reinsertion, Neighborhood::OrOpt2,
            Neighborhood::Exchange, Neighborhood::Exchange21, Neighborhood::Exchange22,
            Neighborhood::TwoOpt, Neighborhood::RelocateCustomer};
        int checked = 0;
        for (unsigned seed = 0; seed < 10; ++seed) {
            auto instance = makeInstance(8, seed, 1.3, seed % 2 ? 0.4 : 0.9);
            Context context(instance);
            State truckOnly{{0, 1, 2, 3, 4, 5, 6, 7, 0}, {}};
            std::mt19937 random(seed);
            std::shuffle(truckOnly.truck.begin() + 1, truckOnly.truck.end() - 1, random);
            for (const State& state : {
                truckOnly,
                State{{0, 1, 2, 3, 4, 5, 6, 0}, {{2, 7, 4}}},
                State{{0, 1, 2, 3, 4, 5, 6, 0}, {{3, 7, 3}}},
                State{{0, 1, 2, 3, 4, 5, 0}, {{1, 6, 2}, {3, 7, 4}}}
            }) {
                for (auto kind : kinds) {
                    const double expected = oracleBest(instance, context, state, kind);
                    auto best = bestNeighbor(context, state, kind);
                    near(best ? independent(instance, *best).total : independent(instance, state).total, expected);
                    for (int repeat = 0; repeat < 5; ++repeat) {
                        auto neighbor = randomNeighbor(context, state, kind, random);
                        if (neighbor) {
                            validate(context, *neighbor);
                            near(evaluate(context, *neighbor).total, independent(instance, *neighbor).total);
                        }
                    }
                    ++checked;
                }
                auto solution = toSolution(context, state);
                near(*solution.getTspCost(), instance.calculateTspCost(solution.getTspNodes()));
                const auto initial = createInitialSolution(context, truckOnly.truck);
                validate(context, initial);
                check(evaluate(context, initial).total <= evaluate(context, truckOnly).total + 1e-8,
                      "Initial construction increased the TSP cost");
            }
            tspd::tsp::TSPSolver tsp;
            const auto baseline = tsp.Solve(instance);
            tspd::hgvns::HGVNS first(seed), second(seed);
            const auto a = first.Solve(instance, baseline), b = second.Solve(instance, baseline);
            near(a.getTotalCost(), b.getTotalCost());
            check(a.getTruckNodes() == b.getTruckNodes() && a.getDroneNodes() == b.getDroneNodes(),
                  "Same HGVNS seed did not reproduce routes");
            check(a.getTotalCost() <= baseline.getTotalCost() + 1e-8, "HGVNS worsened its input TSP");
            near(*a.getTspCost(), instance.calculateTspCost(a.getTspNodes()));
        }
        tspd::tsp::TSPSolver tsp;
        const auto baseline = tsp.Solve(triangle);
        tspd::hgvns::HGVNS hgvns;
        near(hgvns.Solve(triangle, baseline).getTotalCost(), 12);
        std::cout << "HGVNS tests passed: " << checked
                  << " independent best-neighborhood checks, simulation, repairs, and seeded runs\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
