#include "greedy_heuristic.hpp"

#include "../dfs/dfs.hpp"
#include "../kruskal/kruskal.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <queue>
#include <set>
#include <stdexcept>
#include <vector>

namespace tspd::greedyHeuristic {
namespace {

    using Edge = ::tspd::edge::Edge;
    using Kruskal = ::tspd::kruskal::Kruskal;
    using Node = ::tspd::node::Node;
    using Solution = ::tspd::solution::Solution;
    using TSPD = ::tspd::tspd::TSPD;

    enum class Label {
        Simple,
        Combined,
        Truck,
        Drone
    };

    enum class Operation {
        MakeFly,
        PushLeft,
        PushRight
    };

    struct SegmentCost {
        double duration = 0;
        double truck = 0;
        double drone = 0;
        bool feasible = true;
    };

    struct Candidate {
        int position = -1;
        Operation operation = Operation::MakeFly;
        double saving = std::numeric_limits<double>::lowest();
        std::size_t version = 0;
    };

    int operationRank(Operation operation) {
        switch (operation) {
            case Operation::MakeFly: return 0;
            case Operation::PushLeft: return 1;
            case Operation::PushRight: return 2;
        }
        return 3;
    }

    bool isBetterCandidate(const Candidate& candidate,
                           const Candidate& best) {
        if (candidate.saving != best.saving)
            return candidate.saving > best.saving;

        if (operationRank(candidate.operation) != operationRank(best.operation))
            return operationRank(candidate.operation) < operationRank(best.operation);

        return candidate.position < best.position;
    }

    struct CandidatePriority {
        bool operator()(const Candidate& left,
                        const Candidate& right) const {
            return isBetterCandidate(right, left);
        }
    };

    int toSolutionCost(double cost) {
        if (cost <= 0) return 0;
        const double maximum = std::numeric_limits<int>::max();
        return static_cast<int>(std::ceil(std::min(cost, maximum)));
    }

    bool isPermutation(const std::vector<int>& route, int dimension) {
        if (dimension <= 0 || route.size() != static_cast<std::size_t>(dimension))
            return false;

        std::vector<bool> seen(static_cast<std::size_t>(dimension) + 1, false);
        for (const int nodeId : route) {
            if (nodeId < 1 || nodeId > dimension || seen[nodeId])
                return false;
            seen[nodeId] = true;
        }
        return true;
    }

    std::vector<int> naturalRoute(int dimension) {
        std::vector<int> route;
        if (dimension <= 0) return route;

        route.reserve(static_cast<std::size_t>(dimension));
        for (int nodeId = 1; nodeId <= dimension; ++nodeId)
            route.push_back(nodeId);
        return route;
    }

    std::vector<Node> nodesWithValidIds(const TSPD& instance) {
        const int dimension = instance.getDimension();
        std::vector<Node> nodes = instance.getNodes();

        bool idsAreValid = nodes.size() == static_cast<std::size_t>(dimension);
        if (idsAreValid) {
            std::vector<bool> seen(static_cast<std::size_t>(dimension) + 1, false);
            for (const Node& node : nodes) {
                const int id = node.getId();
                if (id < 1 || id > dimension || seen[id]) {
                    idsAreValid = false;
                    break;
                }
                seen[id] = true;
            }
        }

        // EXPLICIT TSPLIB instances may not have coordinates. The MST only
        // needs the vertex ids in that case; distances still come from TSPD.
        if (!idsAreValid) {
            nodes.clear();
            nodes.reserve(static_cast<std::size_t>(std::max(0, dimension)));
            for (int nodeId = 1; nodeId <= dimension; ++nodeId)
                nodes.emplace_back(0.0, 0.0, nodeId, "");
        }

        return nodes;
    }

    std::vector<int> buildInitialRoute(TSPD& instance) {
        const int dimension = instance.getDimension();
        if (dimension <= 0) return {};
        if (dimension == 1) return {1};

        std::vector<Node> nodes = nodesWithValidIds(instance);
        std::vector<Edge> edges;
        for (int first = 1; first <= dimension; ++first)
            for (int second = first + 1; second <= dimension; ++second)
                edges.emplace_back(first, second, instance.getDistance(first, second));

        std::vector<int> route;
        if (!edges.empty()) {
            std::vector<Edge> mst = Kruskal::kruskal(nodes, edges);
            route = ::tspd::dfs::DFS::getDFS(nodes, mst);
        }

        // The normal path is the MST preorder. The fallback keeps the
        // heuristic usable for an incomplete graph and respects the
        // project's 1..N vertex-id convention.
        if (!isPermutation(route, dimension))
            route = naturalRoute(dimension);

        return route;
    }

    class PartitionState {
        public:
            const TSPD& instance;
            const std::vector<int>& route;
            std::vector<Label> labels;
            std::set<int> boundaries;
            std::set<int> dronePositions;
            std::vector<double> prefixRouteDistance;
            std::vector<std::size_t> candidateVersions;
            int simpleCount;

            PartitionState(const TSPD& instanceToUse,
                           const std::vector<int>& closedRoute)
                : instance(instanceToUse),
                  route(closedRoute),
                  labels(closedRoute.size(), Label::Simple),
                  prefixRouteDistance(closedRoute.size(), 0),
                  candidateVersions(closedRoute.size(), 0),
                  simpleCount(static_cast<int>(closedRoute.size()) - 2) {
                if (route.size() < 2)
                    throw std::invalid_argument("A closed route needs at least two positions");

                labels.front() = Label::Combined;
                labels.back() = Label::Combined;

                for (int position = 0;
                     position < static_cast<int>(route.size());
                     ++position) {
                    boundaries.insert(position);
                }

                for (std::size_t position = 0; position + 1 < route.size(); ++position) {
                    prefixRouteDistance[position + 1] =
                        prefixRouteDistance[position] +
                        distance(static_cast<int>(position), static_cast<int>(position + 1));
                }
            }

            double distance(int firstPosition, int secondPosition) const {
                // Route positions map directly to vertex ids in 1..N; the
                // final position is the repeated depot.
                return static_cast<double>(instance.getDistance(
                    route[static_cast<std::size_t>(firstPosition)],
                    route[static_cast<std::size_t>(secondPosition)]));
            }

            int previousBoundary(int position) const {
                const auto current = boundaries.find(position);
                if (current == boundaries.begin() || current == boundaries.end())
                    return -1;
                return *std::prev(current);
            }

            int nextBoundary(int position) const {
                const auto current = boundaries.find(position);
                if (current == boundaries.end()) return -1;
                const auto next = std::next(current);
                if (next == boundaries.end()) return -1;
                return *next;
            }

            int numberOfDroneNodes(int begin,
                                   int end,
                                   int extraDrone = -1) const {
                int count = 0;
                const auto first = dronePositions.lower_bound(begin + 1);
                for (auto it = first;
                     it != dronePositions.end() && *it < end;
                     ++it) {
                    ++count;
                }

                if (extraDrone > begin && extraDrone < end &&
                    dronePositions.find(extraDrone) == dronePositions.end()) {
                    ++count;
                }
                return count;
            }

            SegmentCost segmentCost(int begin,
                                    int end,
                                    int extraDrone = -1) const {
                SegmentCost result;
                if (begin < 0 || end >= static_cast<int>(route.size()) || begin >= end)
                    return SegmentCost{0, 0, 0, false};

                const int droneCount = numberOfDroneNodes(begin, end, extraDrone);
                if (droneCount > 1)
                    return SegmentCost{0, 0, 0, false};

                int dronePosition = -1;
                const auto first = dronePositions.lower_bound(begin + 1);
                if (first != dronePositions.end() && *first < end)
                    dronePosition = *first;
                if (extraDrone > begin && extraDrone < end) {
                    if (dronePosition != -1 && dronePosition != extraDrone)
                        return SegmentCost{0, 0, 0, false};
                    dronePosition = extraDrone;
                }

                if (dronePosition == -1) {
                    result.truck = prefixRouteDistance[static_cast<std::size_t>(end)] -
                                   prefixRouteDistance[static_cast<std::size_t>(begin)];
                    result.truck /= instance.getTruckSpeed();
                    result.duration = result.truck;
                    return result;
                }

                if (dronePosition - 1 < begin || dronePosition + 1 > end)
                    return SegmentCost{0, 0, 0, false};

                // The truck skips the drone customer. The drone skips any
                // truck-only customers inside the operation.
                result.truck =
                    prefixRouteDistance[static_cast<std::size_t>(dronePosition - 1)] -
                    prefixRouteDistance[static_cast<std::size_t>(begin)] +
                    distance(dronePosition - 1, dronePosition + 1) +
                    prefixRouteDistance[static_cast<std::size_t>(end)] -
                    prefixRouteDistance[static_cast<std::size_t>(dronePosition + 1)];
                result.drone = distance(begin, dronePosition) +
                               distance(dronePosition, end);
                result.truck /= instance.getTruckSpeed();
                result.drone /= instance.getDroneSpeed();
                result.duration = std::max(result.truck, result.drone);
                return result;
            }

            SegmentCost totalCost() const {
                SegmentCost result;
                if (boundaries.size() < 2)
                    return SegmentCost{0, 0, 0, false};

                auto current = boundaries.begin();
                auto next = std::next(current);
                for (; next != boundaries.end(); ++current, ++next) {
                    const SegmentCost segment = segmentCost(*current, *next);
                    if (!segment.feasible)
                        return SegmentCost{0, 0, 0, false};

                    result.duration += segment.duration;
                    result.truck += segment.truck;
                    result.drone += segment.drone;
                }
                return result;
            }

            bool canMakeFly(int position) const {
                const int lastPosition = static_cast<int>(route.size()) - 1;
                if (position <= 0 || position >= lastPosition ||
                    labels[static_cast<std::size_t>(position)] != Label::Simple)
                    return false;

                const Label predecessor = labels[static_cast<std::size_t>(position - 1)];
                const Label successor = labels[static_cast<std::size_t>(position + 1)];
                if (predecessor == Label::Truck || predecessor == Label::Drone ||
                    successor == Label::Truck || successor == Label::Drone)
                    return false;

                const int previous = previousBoundary(position);
                const int next = nextBoundary(position);
                if (previous != position - 1 || next != position + 1)
                    return false;

                const SegmentCost merged = segmentCost(previous, next, position);
                return merged.feasible;
            }

            bool canPushLeft(int position) const {
                if (position <= 1 ||
                    labels[static_cast<std::size_t>(position)] != Label::Simple ||
                    labels[static_cast<std::size_t>(position - 1)] != Label::Combined)
                    return false;

                const int combinedPosition = position - 1;
                const int previous = previousBoundary(combinedPosition);
                const int next = nextBoundary(combinedPosition);
                if (previous < 0 || next != position)
                    return false;

                // A push extends an existing operation containing one drone
                // delivery; otherwise it would only create a needless label.
                const SegmentCost merged = segmentCost(previous, position);
                return merged.feasible && numberOfDroneNodes(previous, position) == 1;
            }

            bool canPushRight(int position) const {
                const int lastPosition = static_cast<int>(route.size()) - 1;
                if (position <= 0 || position + 1 >= lastPosition ||
                    labels[static_cast<std::size_t>(position)] != Label::Simple ||
                    labels[static_cast<std::size_t>(position + 1)] != Label::Combined)
                    return false;

                const int combinedPosition = position + 1;
                const int previous = previousBoundary(combinedPosition);
                const int next = nextBoundary(combinedPosition);
                if (previous != position || next < 0)
                    return false;

                const SegmentCost merged = segmentCost(position, next);
                return merged.feasible && numberOfDroneNodes(position, next) == 1;
            }

            double savingForMakeFly(int position) const {
                const int previous = previousBoundary(position);
                const int next = nextBoundary(position);
                const SegmentCost before = segmentCost(previous, position);
                const SegmentCost after = segmentCost(position, next);
                const SegmentCost candidate = segmentCost(previous, next, position);
                if (!before.feasible || !after.feasible || !candidate.feasible)
                    return std::numeric_limits<double>::lowest();
                return before.duration + after.duration - candidate.duration;
            }

            double savingForPushLeft(int position) const {
                const int combinedPosition = position - 1;
                const int previous = previousBoundary(combinedPosition);
                const SegmentCost before = segmentCost(previous, combinedPosition);
                const SegmentCost after = segmentCost(combinedPosition, position);
                const SegmentCost candidate = segmentCost(previous, position);
                if (!before.feasible || !after.feasible || !candidate.feasible)
                    return std::numeric_limits<double>::lowest();
                return before.duration + after.duration - candidate.duration;
            }

            double savingForPushRight(int position) const {
                const int combinedPosition = position + 1;
                const int next = nextBoundary(combinedPosition);
                const SegmentCost before = segmentCost(position, combinedPosition);
                const SegmentCost after = segmentCost(combinedPosition, next);
                const SegmentCost candidate = segmentCost(position, next);
                if (!before.feasible || !after.feasible || !candidate.feasible)
                    return std::numeric_limits<double>::lowest();
                return before.duration + after.duration - candidate.duration;
            }

            void apply(const Candidate& candidate) {
                const int position = candidate.position;
                switch (candidate.operation) {
                    case Operation::MakeFly:
                        if (labels[static_cast<std::size_t>(position)] == Label::Simple)
                            --simpleCount;
                        if (labels[static_cast<std::size_t>(position - 1)] == Label::Simple)
                            --simpleCount;
                        if (labels[static_cast<std::size_t>(position + 1)] == Label::Simple)
                            --simpleCount;
                        labels[static_cast<std::size_t>(position)] = Label::Drone;
                        labels[static_cast<std::size_t>(position - 1)] = Label::Combined;
                        labels[static_cast<std::size_t>(position + 1)] = Label::Combined;
                        boundaries.erase(position);
                        dronePositions.insert(position);
                        break;

                    case Operation::PushLeft:
                        if (labels[static_cast<std::size_t>(position)] == Label::Simple)
                            --simpleCount;
                        labels[static_cast<std::size_t>(position - 1)] = Label::Truck;
                        labels[static_cast<std::size_t>(position)] = Label::Combined;
                        boundaries.erase(position - 1);
                        break;

                    case Operation::PushRight:
                        if (labels[static_cast<std::size_t>(position)] == Label::Simple)
                            --simpleCount;
                        labels[static_cast<std::size_t>(position + 1)] = Label::Truck;
                        labels[static_cast<std::size_t>(position)] = Label::Combined;
                        boundaries.erase(position + 1);
                        break;
                }
            }

            void markCombined(int position) {
                if (labels[static_cast<std::size_t>(position)] == Label::Simple)
                    --simpleCount;
                labels[static_cast<std::size_t>(position)] = Label::Combined;
            }

            bool hasSimpleNodes() const {
                return simpleCount > 0;
            }
    };

    using CandidateQueue = std::priority_queue<
        Candidate,
        std::vector<Candidate>,
        CandidatePriority>;

    void addCandidatesForPosition(const PartitionState& state,
                                   int position,
                                   CandidateQueue& queue) {
        const int lastPosition = static_cast<int>(state.route.size()) - 1;
        if (position <= 0 || position >= lastPosition ||
            state.labels[static_cast<std::size_t>(position)] != Label::Simple)
            return;

        const std::size_t version =
            state.candidateVersions[static_cast<std::size_t>(position)];
        if (state.canMakeFly(position)) {
            queue.push(Candidate{
                position,
                Operation::MakeFly,
                state.savingForMakeFly(position),
                version
            });
        }
        if (state.canPushLeft(position)) {
            queue.push(Candidate{
                position,
                Operation::PushLeft,
                state.savingForPushLeft(position),
                version
            });
        }
        if (state.canPushRight(position)) {
            queue.push(Candidate{
                position,
                Operation::PushRight,
                state.savingForPushRight(position),
                version
            });
        }
    }

    void refreshCandidatesAround(PartitionState& state,
                                 int center,
                                 CandidateQueue& queue) {
        // An extended flight can affect candidates beyond adjacent positions.
        (void)center;
        queue = CandidateQueue{};
        const int firstPosition = 1;
        const int finalPosition = static_cast<int>(state.route.size()) - 2;

        for (int position = firstPosition; position <= finalPosition; ++position) {
            ++state.candidateVersions[static_cast<std::size_t>(position)];
            addCandidatesForPosition(state, position, queue);
        }
    }

    void initializeCandidates(PartitionState& state,
                              CandidateQueue& queue) {
        const int lastPosition = static_cast<int>(state.route.size()) - 1;
        for (int position = 1; position < lastPosition; ++position) {
            ++state.candidateVersions[static_cast<std::size_t>(position)];
            addCandidatesForPosition(state, position, queue);
        }
    }

    bool isCurrentCandidate(const PartitionState& state,
                            const Candidate& candidate) {
        const int position = candidate.position;
        if (position <= 0 || position >= static_cast<int>(state.route.size()) - 1)
            return false;
        if (candidate.version !=
            state.candidateVersions[static_cast<std::size_t>(position)])
            return false;

        double saving = std::numeric_limits<double>::lowest();
        bool feasible = false;
        switch (candidate.operation) {
            case Operation::MakeFly:
                feasible = state.canMakeFly(position);
                if (feasible) saving = state.savingForMakeFly(position);
                break;
            case Operation::PushLeft:
                feasible = state.canPushLeft(position);
                if (feasible) saving = state.savingForPushLeft(position);
                break;
            case Operation::PushRight:
                feasible = state.canPushRight(position);
                if (feasible) saving = state.savingForPushRight(position);
                break;
        }
        return feasible && saving == candidate.saving;
    }

} // namespace

Solution partitionRoute(
    TSPD& instance,
    const std::vector<int>& initialRoute) {
    if (initialRoute.empty())
        return Solution{{}, {}, 0, 0, 0};

    std::vector<int> closedRoute = initialRoute;
    closedRoute.push_back(initialRoute.front());
    PartitionState state(instance, closedRoute);
    CandidateQueue candidates;
    initializeCandidates(state, candidates);

    // Every iteration consumes at least one Simple label. A candidate is
    // selected by its local time saving, as in Agatz et al. If no move is
    // feasible, the node remains combined (the constrained-case fallback).
    while (state.hasSimpleNodes()) {
        Candidate best;
        bool foundCandidate = false;
        while (!candidates.empty()) {
            const Candidate candidate = candidates.top();
            candidates.pop();
            if (isCurrentCandidate(state, candidate)) {
                best = candidate;
                foundCandidate = true;
                break;
            }
        }

        if (!foundCandidate || best.saving <= 0.0) {
            for (int position = 1;
                 position < static_cast<int>(state.route.size()) - 1;
                 ++position) {
                if (state.labels[static_cast<std::size_t>(position)] == Label::Simple) {
                    state.markCombined(position);
                    refreshCandidatesAround(state, position, candidates);
                    break;
                }
            }
            continue;
        }

        state.apply(best);
        refreshCandidatesAround(state, best.position, candidates);
    }

    const SegmentCost total = state.totalCost();
    if (!total.feasible)
        throw std::logic_error("Greedy heuristic produced an infeasible partition");

    std::vector<int> truckRoute;
    std::vector<int> droneRoute;
    truckRoute.reserve(initialRoute.size());
    droneRoute.reserve(initialRoute.size());

    const bool hasDroneFlight = !state.dronePositions.empty();
    for (std::size_t position = 0; position + 1 < state.route.size(); ++position) {
        const Label label = state.labels[position];
        if (label != Label::Drone)
            truckRoute.push_back(state.route[position]);

        // Keep the drone route empty for a truck-only solution; Graphic uses
        // that to hide the dashed route.
        if (hasDroneFlight && label != Label::Truck)
            droneRoute.push_back(state.route[position]);
    }

    return Solution{
        truckRoute,
        droneRoute,
        toSolutionCost(total.duration),
        toSolutionCost(total.truck),
        toSolutionCost(total.drone)
    };
}

std::vector<int> iterativeImprovement(
    TSPD& instance,
    const std::vector<int>& initialRoute) {
    if (initialRoute.size() < 3)
        return initialRoute;

    std::vector<int> currentRoute = initialRoute;
    double currentCost = partitionRoute(instance, currentRoute).getTotalCost();

    // The paper evaluates the complete neighborhood. That is practical for
    // small routes, while the current public API has no separate fast
    // evaluator for a neighbor route. For larger instances we keep all three
    // Section 6.3 neighborhoods, using a bounded first-improvement scan for
    // each one so the solver remains usable with the repository's larger
    // TSPLIB files.
    constexpr std::size_t exhaustiveRouteLimit = 60;
    constexpr std::size_t boundedMovesPerNeighborhood = 500;
    const bool exhaustiveNeighborhood = currentRoute.size() <= exhaustiveRouteLimit;
    const std::size_t moveLimit = exhaustiveNeighborhood
                                      ? std::numeric_limits<std::size_t>::max()
                                      : boundedMovesPerNeighborhood;

    bool improved = true;
    while (improved) {
        improved = false;
        std::vector<int> bestRoute = currentRoute;
        double bestCost = currentCost;

        const auto evaluateCandidate = [&](std::vector<int> candidateRoute) {
            const double candidateCost =
                partitionRoute(instance, candidateRoute).getTotalCost();
            if (candidateCost < bestCost) {
                bestCost = candidateCost;
                bestRoute = std::move(candidateRoute);
            }
            return !exhaustiveNeighborhood && candidateCost < currentCost;
        };

        // Section 6.3: 2-point move (swap two customer positions).
        std::size_t evaluatedMoves = 0;
        bool stopNeighborhood = false;
        for (int first = 1;
             first < static_cast<int>(currentRoute.size()) - 1;
             ++first) {
            for (int second = first + 1;
                 second < static_cast<int>(currentRoute.size());
                 ++second) {
                if (evaluatedMoves >= moveLimit) {
                    stopNeighborhood = true;
                    break;
                }
                std::vector<int> candidateRoute = currentRoute;
                std::swap(candidateRoute[first], candidateRoute[second]);
                ++evaluatedMoves;
                if (evaluateCandidate(std::move(candidateRoute))) {
                    stopNeighborhood = true;
                    break;
                }
            }
            if (stopNeighborhood) break;
        }

        // Section 6.3: 2-opt move (reverse a customer subsequence).
        evaluatedMoves = 0;
        stopNeighborhood = false;
        for (int first = 1;
             first < static_cast<int>(currentRoute.size()) - 1;
             ++first) {
            for (int second = first + 1;
                 second < static_cast<int>(currentRoute.size());
                 ++second) {
                if (evaluatedMoves >= moveLimit) {
                    stopNeighborhood = true;
                    break;
                }
                std::vector<int> candidateRoute = currentRoute;
                std::reverse(candidateRoute.begin() + first,
                             candidateRoute.begin() + second + 1);
                ++evaluatedMoves;
                if (evaluateCandidate(std::move(candidateRoute))) {
                    stopNeighborhood = true;
                    break;
                }
            }
            if (stopNeighborhood) break;
        }

        // Section 6.3: 1-point move (remove one customer and relocate it).
        evaluatedMoves = 0;
        stopNeighborhood = false;
        for (int from = 1;
             from < static_cast<int>(currentRoute.size());
             ++from) {
            for (int to = 1;
                 to < static_cast<int>(currentRoute.size());
                 ++to) {
                if (from == to) continue;
                if (evaluatedMoves >= moveLimit) {
                    stopNeighborhood = true;
                    break;
                }

                std::vector<int> candidateRoute = currentRoute;
                const int node = candidateRoute[from];
                candidateRoute.erase(candidateRoute.begin() + from);
                candidateRoute.insert(candidateRoute.begin() + to, node);

                ++evaluatedMoves;
                if (evaluateCandidate(std::move(candidateRoute))) {
                    stopNeighborhood = true;
                    break;
                }
            }
            if (stopNeighborhood) break;
        }

        if (bestCost < currentCost) {
            currentRoute = std::move(bestRoute);
            currentCost = bestCost;
            improved = true;
        }
    }

    return currentRoute;
}

Solution GreedyHeuristic::_greedyHeuristic(
    TSPD& instance,
    const std::vector<int>& initialRoute) {
    const std::vector<int> improvedRoute = iterativeImprovement(instance, initialRoute);
    return partitionRoute(instance, improvedRoute);
}

Solution GreedyHeuristic::Optimize(TSPD& instance) {
    std::cout << "\nInitializing Greedy Heuristic\n";
    if (!std::isfinite(instance.getTruckSpeed()) || instance.getTruckSpeed() <= 0.0 ||
        !std::isfinite(instance.getDroneSpeed()) || instance.getDroneSpeed() <= 0.0)
        throw std::invalid_argument("Truck and drone speeds must be positive and finite");
    const std::vector<int> initialRoute = buildInitialRoute(instance);
    return _greedyHeuristic(instance, initialRoute);
}

} // namespace tspd::greedyHeuristic
