#include "block_dp.hpp"
#include <algorithm>
#include <cmath>
#include <limits>
#include <numeric>
#include <random>
#include <set>
#include <stdexcept>

namespace tspd::hgvns::detail {
namespace {
    void append(State& target, const State& part) {
        if (target.truck.empty()) target.truck.push_back(part.truck.front());
        const auto offset = target.truck.size() - 1;
        target.truck.insert(target.truck.end(), part.truck.begin() + 1, part.truck.end());
        for (auto flight : part.flights) {
            flight.launch += offset;
            flight.recovery += offset;
            target.flights.push_back(flight);
        }
    }
    double duration(const Context& context, const State& state) {
        return evaluate(context, state, false).total;
    }
    std::vector<int> farthestInsertion(const Context& context, const std::vector<int>& sequence) {
        std::vector<int> path{sequence.front(), sequence.back()};
        std::vector<int> remaining(sequence.begin() + 1, sequence.end() - 1);
        std::sort(remaining.begin(), remaining.end());
        while (!remaining.empty()) {
            auto chosen = remaining.begin();
            double farthest = -1;
            for (auto it = remaining.begin(); it != remaining.end(); ++it) {
                double nearest = std::numeric_limits<double>::infinity();
                for (int node : path) nearest = std::min(nearest, context.truck(*it, node));
                if (nearest > farthest) { farthest = nearest; chosen = it; }
            }
            double best = std::numeric_limits<double>::infinity();
            std::size_t position = 1;
            for (std::size_t p = 1; p < path.size(); ++p) {
                const double delta = context.truck(path[p - 1], *chosen)
                                   + context.truck(*chosen, path[p]) - context.truck(path[p - 1], path[p]);
                if (delta < best) { best = delta; position = p; }
            }
            path.insert(path.begin() + position, *chosen);
            remaining.erase(chosen);
        }
        return path;
    }
    std::vector<std::vector<int>> orderNeighbors(const std::vector<int>& order, std::mt19937& random) {
        std::vector<std::vector<int>> moves;
        for (std::size_t i = 1; i + 1 < order.size(); ++i) {
            for (std::size_t j = i + 1; j + 1 < order.size(); ++j) {
                auto candidate = order;
                std::reverse(candidate.begin() + i, candidate.begin() + j + 1);
                moves.push_back(std::move(candidate));
            }
            for (std::size_t j = 1; j + 1 < order.size(); ++j) if (i != j) {
                auto candidate = order;
                const int node = candidate[i];
                candidate.erase(candidate.begin() + i);
                candidate.insert(candidate.begin() + j, node);
                moves.push_back(std::move(candidate));
            }
        }
        std::shuffle(moves.begin(), moves.end(), random);
        return moves;
    }
    std::vector<State> operations(const State& source) {
        std::vector<State> result;
        std::size_t position = 0;
        auto flight = source.flights.begin();
        while (position + 1 < source.truck.size() || flight != source.flights.end()) {
            if (flight != source.flights.end() && flight->launch == position) {
                const auto end = flight->recovery;
                result.push_back({std::vector<int>(source.truck.begin() + position,
                                                  source.truck.begin() + end + 1),
                                  {{0, flight->customer, end - position}}});
                position = end;
                ++flight;
            } else {
                result.push_back({{source.truck[position], source.truck[position + 1]}, {}});
                ++position;
            }
        }
        return result;
    }
}

    State partitionPath(const Context& context, const std::vector<int>& sequence) {
        if (sequence.size() < 2) throw std::invalid_argument("PD requer dois extremos.");
        const auto n = sequence.size();
        std::vector<double> prefix(n, 0), best(n, std::numeric_limits<double>::infinity());
        std::vector<std::size_t> previous(n), aerial(n, n);
        for (std::size_t p = 1; p < n; ++p)
            prefix[p] = prefix[p - 1] + context.truck(sequence[p - 1], sequence[p]);
        best[0] = 0;
        for (std::size_t j = 1; j < n; ++j) {
            best[j] = best[j - 1] + context.truck(sequence[j - 1], sequence[j]);
            previous[j] = j - 1;
            for (std::size_t i = 0; i + 1 < j; ++i) {
                for (std::size_t k = i + 1; k < j; ++k) {
                    const double truck = prefix[j] - prefix[i]
                        - context.truck(sequence[k - 1], sequence[k])
                        - context.truck(sequence[k], sequence[k + 1])
                        + context.truck(sequence[k - 1], sequence[k + 1]);
                    const double drone = context.drone(sequence[i], sequence[k])
                                       + context.drone(sequence[k], sequence[j]);
                    const double cost = best[i] + std::max(truck, drone);
                    if (cost < best[j]) {
                        best[j] = cost;
                        previous[j] = i;
                        aerial[j] = k;
                    }
                }
            }
        }
        std::vector<State> parts;
        for (auto j = n - 1; j > 0;) {
            const auto i = previous[j];
            State part{{sequence[i]}, {}};
            for (auto p = i + 1; p <= j; ++p)
                if (p != aerial[j]) part.truck.push_back(sequence[p]);
            if (aerial[j] != n)
                part.flights.push_back({0, sequence[aerial[j]], part.truck.size() - 1});
            parts.push_back(std::move(part));
            j = i;
        }
        State result;
        for (auto p = parts.rbegin(); p != parts.rend(); ++p) append(result, *p);
        // A stationary flight uses one truck position, rather than two copies of its ID.
        for (std::size_t p = 1; p < result.truck.size();) {
            if (result.truck[p] != result.truck[p - 1]) { ++p; continue; }
            result.truck.erase(result.truck.begin() + p);
            for (auto& flight : result.flights) {
                if (flight.launch >= p) --flight.launch;
                if (flight.recovery >= p) --flight.recovery;
            }
        }
        return result;
    }

    State postprocessBlocks(const Context& context, const State& source,
                            BlockSelection selection, std::uint32_t seed, BlockParameters parameters,
                            BlockStatistics* statistics) {
        if (statistics) *statistics = {};
        validate(context, source);
        if (parameters.maxNodes < 3) throw std::invalid_argument("Blocos da PD requerem maxNodes >= 3.");
        if (parameters.maxOrders == 0) throw std::invalid_argument("PD requer maxOrders >= 1.");
        if (selection == BlockSelection::None || parameters.maxBlocks == 0) return source;
        const auto atoms = operations(source);
        struct Block { std::size_t begin, end; double waiting; };
        std::vector<Block> blocks;
        // Same disjoint candidate pool and number of calls for both policies.
        for (std::size_t begin = 0; begin < atoms.size(); ++begin) {
            std::set<int> nodes;
            double waiting = 0;
            auto end = begin;
            for (; end < atoms.size(); ++end) {
                auto next = nodes;
                next.insert(atoms[end].truck.begin(), atoms[end].truck.end());
                for (const auto& flight : atoms[end].flights) next.insert(flight.customer);
                if (next.size() > parameters.maxNodes) break;
                nodes = std::move(next);
                for (const auto& flight : atoms[end].flights) {
                    const double air = context.drone(atoms[end].truck.front(), flight.customer)
                                     + context.drone(flight.customer, atoms[end].truck.back());
                    double truck = 0;
                    for (std::size_t p = 1; p < atoms[end].truck.size(); ++p)
                        truck += context.truck(atoms[end].truck[p - 1], atoms[end].truck[p]);
                    waiting += std::abs(air - truck); // Waiting of either vehicle.
                }
            }
            if (end > begin) {
                blocks.push_back({begin, end, waiting});
                begin = end - 1;
            }
        }
        std::vector<Block> selected;
        if (statistics) statistics->candidates = blocks.size();
        if (selection == BlockSelection::Waiting)
            std::stable_sort(blocks.begin(), blocks.end(), [](const Block& a, const Block& b) {
                return a.waiting > b.waiting;
            });
        else {
            std::mt19937 random(seed);
            std::shuffle(blocks.begin(), blocks.end(), random);
        }
        for (const auto& block : blocks) {
            if (selected.size() == parameters.maxBlocks) break;
            bool overlap = false;
            for (const auto& prior : selected)
                overlap = overlap || (block.begin < prior.end && prior.begin < block.end);
            if (!overlap) selected.push_back(block);
        }
        if (statistics) for (const auto& block : selected) {
            statistics->selectedOperations.push_back(block.begin);
            statistics->selectedWaiting.push_back(block.waiting);
        }
        std::sort(selected.begin(), selected.end(), [](const Block& a, const Block& b) {
            return a.begin < b.begin;
        });
        State result;
        std::size_t cursor = 0;
        for (const auto& block : selected) {
            while (cursor < block.begin) append(result, atoms[cursor++]);
            State original;
            for (; cursor < block.end; ++cursor) append(original, atoms[cursor]);
            const int first = original.truck.front(), last = original.truck.back();
            std::vector<int> sequence{first};
            std::set<int> seen{first, last};
            auto add = [&](int id) { if (seen.insert(id).second) sequence.push_back(id); };
            for (std::size_t p = 0; p < original.truck.size(); ++p) {
                add(original.truck[p]);
                for (const auto& flight : original.flights)
                    if (flight.launch == p) add(flight.customer);
            }
            sequence.push_back(last);
            // Returning to a block's launch ID by truck would revisit that customer
            // (or create an intermediate depot). Only a full depot tour may do so.
            const bool closedTour = first == 0 && block.begin == 0 && block.end == atoms.size();
            State best = original;
            double bestCost = duration(context, original);
            double bestOrderCost = std::numeric_limits<double>::infinity();
            auto bestOrder = sequence;
            std::set<std::vector<int>> visited;
            auto evaluateOrder = [&](const std::vector<int>& order) {
                if (visited.size() == parameters.maxOrders || !visited.insert(order).second) return;
                auto candidate = partitionPath(context, order);
                if (statistics) ++statistics->ordersEvaluated;
                const bool legal = first != last || candidate.truck.size() == 1 || closedTour;
                if (!legal) return;
                const double cost = duration(context, candidate);
                if (cost < bestOrderCost) { bestOrderCost = cost; bestOrder = order; }
                if (improves(cost, bestCost)) { bestCost = cost; best = std::move(candidate); }
            };
            evaluateOrder(sequence); // maxOrders=1 reproduces the original fixed-order method.
            if (parameters.maxOrders > 1) {
                auto inserted = farthestInsertion(context, sequence);
                evaluateOrder(inserted);
                auto reversed = sequence;
                std::reverse(reversed.begin() + 1, reversed.end() - 1);
                evaluateOrder(reversed);
                std::reverse(inserted.begin() + 1, inserted.end() - 1);
                evaluateOrder(inserted);
                // Independent stream: order search cannot change block selection or GVNS shaking.
                std::mt19937 random(seed ^ static_cast<std::uint32_t>(block.begin) ^ 0x9e3779b9u);
                while (visited.size() < parameters.maxOrders) {
                    const auto center = bestOrder;
                    const auto before = visited.size();
                    for (const auto& neighbor : orderNeighbors(center, random)) {
                        evaluateOrder(neighbor);
                        if (visited.size() == parameters.maxOrders) break;
                    }
                    if (before == visited.size() || center == bestOrder) break;
                }
            }
            const bool better = improves(bestCost, duration(context, original));
            if (statistics && better) ++statistics->accepted;
            append(result, best);
        }
        while (cursor < atoms.size()) append(result, atoms[cursor++]);
        if (result.truck.empty()) return source;
        if (result.truck.size() == 1) result.truck.push_back(result.truck.front());
        sortFlights(result);
        validate(context, result);
        return improves(duration(context, result), duration(context, source)) ? result : source;
    }
}
