#include "neighborhoods.hpp"

#include <algorithm>
#include <cstdint>
#include <functional>
#include <limits>
#include <numeric>
#include <stdexcept>
#include <tuple>

namespace tspd::hgvns::detail {
namespace {
    struct Move { Neighborhood kind; std::size_t first; std::size_t second; };
    struct Range { std::size_t first; std::size_t last; };
    struct Addition { std::size_t customer; std::size_t launch; std::size_t recovery; double saving; Range range; };

    std::vector<Range> freeRanges(const State& state) {
        std::vector<Range> result;
        std::size_t cursor = 0;
        for (const auto& flight : state.flights) {
            if (flight.launch > cursor) result.push_back({cursor, flight.launch});
            cursor = flight.recovery;
        }
        if (cursor + 1 < state.truck.size()) result.push_back({cursor, state.truck.size() - 1});
        return result;
    }

    bool anchor(const State& state, std::size_t position) {
        return std::any_of(state.flights.begin(), state.flights.end(), [&](const Flight& flight) {
            return flight.launch == position || flight.recovery == position;
        });
    }

    void blocks(const Move& move, std::size_t& p, std::size_t& a, std::size_t& q, std::size_t& b) {
        p = move.first;
        q = move.second;
        a = move.kind == Neighborhood::Exchange ? 1 : 2;
        b = move.kind == Neighborhood::Exchange22 ? 2 : 1;
        if (p > q) { std::swap(p, q); std::swap(a, b); }
    }

    std::vector<int> apply(const State& state, const Move& move) {
        auto route = state.truck;
        const auto p = move.first, q = move.second;
        if (move.kind == Neighborhood::Reinsertion) {
            const int customer = route[p];
            route.erase(route.begin() + p);
            route.insert(route.begin() + q, customer);
        } else if (move.kind == Neighborhood::OrOpt2) {
            const std::vector<int> pair{route[p], route[p + 1]};
            route.erase(route.begin() + p, route.begin() + p + 2);
            route.insert(route.begin() + q, pair.begin(), pair.end());
        } else if (move.kind == Neighborhood::TwoOpt) {
            std::reverse(route.begin() + p, route.begin() + q + 1);
        } else {
            std::size_t first, a, second, b;
            blocks(move, first, a, second, b);
            route.clear();
            route.insert(route.end(), state.truck.begin(), state.truck.begin() + first);
            route.insert(route.end(), state.truck.begin() + second, state.truck.begin() + second + b);
            route.insert(route.end(), state.truck.begin() + first + a, state.truck.begin() + second);
            route.insert(route.end(), state.truck.begin() + first, state.truck.begin() + first + a);
            route.insert(route.end(), state.truck.begin() + second + b, state.truck.end());
        }
        return route;
    }

    // Delta apenas das arestas alteradas; tambem serve como limite inferior do tempo sincronizado.
    double truckDelta(const Context& context, const State& state, const Move& move) {
        const auto& t = state.truck;
        const auto d = [&](std::size_t a, std::size_t b) { return context.truck(t[a], t[b]); };
        const auto p = move.first, q = move.second;
        if (move.kind == Neighborhood::Reinsertion) {
            const auto u = q < p ? q - 1 : q;
            const auto v = u + 1;
            return -d(p - 1, p) - d(p, p + 1) + d(p - 1, p + 1)
                   -d(u, v) + d(u, p) + d(p, v);
        }
        if (move.kind == Neighborhood::OrOpt2) {
            const auto u = q < p ? q - 1 : q + 1;
            const auto v = u + 1;
            return -d(p - 1, p) - d(p + 1, p + 2) + d(p - 1, p + 2)
                   -d(u, v) + d(u, p) + d(p + 1, v);
        }
        if (move.kind == Neighborhood::TwoOpt)
            return -d(p - 1, p) - d(q, q + 1) + d(p - 1, q) + d(p, q + 1);
        std::size_t first, a, second, b;
        blocks(move, first, a, second, b);
        const auto aEnd = first + a - 1, bEnd = second + b - 1;
        double delta = -d(first - 1, first) - d(bEnd, bEnd + 1)
                       + d(first - 1, second) + d(aEnd, bEnd + 1);
        if (first + a == second)
            delta += -d(aEnd, second) + d(bEnd, first);
        else
            delta += -d(aEnd, first + a) - d(second - 1, second)
                     + d(bEnd, first + a) + d(second - 1, first);
        return delta;
    }

    template<class Visitor>
    void truckMoves(const State& state, Neighborhood kind, Visitor visit, bool initialOnly = false) {
        const std::size_t last = state.truck.size() - 1;
        for (std::size_t p = 1; p < last; ++p) {
            if ((kind == Neighborhood::OrOpt2 || kind == Neighborhood::Exchange21 ||
                 kind == Neighborhood::Exchange22) && p + 1 >= last) continue;
            if (initialOnly && anchor(state, p)) continue;
            const std::size_t qBegin = kind == Neighborhood::Exchange || kind == Neighborhood::TwoOpt
                                      ? p + 1 : kind == Neighborhood::Exchange22 ? p + 2 : 1;
            const std::size_t qEnd = kind == Neighborhood::OrOpt2 || kind == Neighborhood::Exchange22
                                    ? last - 1 : last;
            for (std::size_t q = qBegin; q < qEnd; ++q) {
                if (p == q || (kind == Neighborhood::Exchange21 && q == p + 1)) continue;
                if (kind == Neighborhood::TwoOpt && p == 1 && q + 1 == last) continue;
                if (initialOnly) {
                    // A construcao inicial reinsere clientes em subrotas ja acompanhadas por voo.
                    const std::size_t gap = q < p ? q : q + 1;
                    const bool paired = std::any_of(state.flights.begin(), state.flights.end(), [&](const Flight& f) {
                        return f.launch < gap && gap <= f.recovery;
                    });
                    if (!paired) continue;
                }
                visit(Move{kind, p, q});
            }
        }
    }

    std::optional<State> bestTruck(const Context& context, const State& source, Neighborhood kind,
                                   double bestCost, bool initialOnly = false) {
        const auto current = evaluate(context, source, false);
        std::optional<State> best;
        truckMoves(source, kind, [&](const Move& move) {
            const double bound = current.truck + truckDelta(context, source, move);
            if (!improves(bound, bestCost)) return;
            auto candidate = rebindFlights(context, source, apply(source, move));
            if (!candidate) return;
            const double cost = evaluate(context, *candidate, false).total;
            if (improves(cost, bestCost)) {
                bestCost = cost;
                best = std::move(candidate);
            }
        }, initialOnly);
        return best;
    }

    std::optional<State> bestAddition(const Context& context, const State& source, double bestCost) {
        const auto current = evaluate(context, source, false);
        std::vector<Addition> removals;
        for (const auto& range : freeRanges(source)) {
            for (std::size_t j = range.first + 1; j < range.last; ++j) {
                const double saving = context.truck(source.truck[j - 1], source.truck[j])
                                    + context.truck(source.truck[j], source.truck[j + 1])
                                    - context.truck(source.truck[j - 1], source.truck[j + 1]);
                removals.push_back({j, 0, 0, saving, range});
            }
        }
        std::stable_sort(removals.begin(), removals.end(), [](const Addition& a, const Addition& b) {
            return a.saving > b.saving;
        });
        std::optional<Addition> best;
        for (auto removal : removals) {
            const double lowerBound = current.total - removal.saving;
            if (!improves(lowerBound, bestCost)) continue;
            bool reachedBound = false;
            for (std::size_t i = removal.range.first; i <= removal.range.last && !reachedBound; ++i) {
                if (i == removal.customer || i + 1 == source.truck.size()) continue;
                for (std::size_t k = i; k <= removal.range.last; ++k) {
                    if (k == removal.customer) continue;
                    double truckTime = current.prefix[k] - current.prefix[i];
                    if (i < removal.customer && removal.customer < k) truckTime -= removal.saving;
                    truckTime = std::max(0.0, truckTime);
                    const double droneTime = context.drone(source.truck[i], source.truck[removal.customer])
                                           + context.drone(source.truck[removal.customer], source.truck[k]);
                    const double cost = lowerBound + std::max(0.0, droneTime - truckTime);
                    if (improves(cost, bestCost)) {
                        bestCost = cost;
                        removal.launch = i;
                        removal.recovery = k;
                        best = removal;
                    }
                    // Zero espera atinge o limite inferior: os demais pares deste cliente nao melhoram.
                    if (droneTime <= truckTime) { reachedBound = true; break; }
                }
            }
        }
        if (!best) return std::nullopt;
        return addFlight(source, best->customer, best->launch, best->recovery);
    }

    std::optional<State> randomAddition(const State& source, std::mt19937& random) {
        struct Group { Range range; std::uint64_t pairs; std::uint64_t count; };
        std::vector<Group> groups;
        std::uint64_t total = 0;
        for (const auto& range : freeRanges(source)) {
            const auto points = static_cast<std::uint64_t>(range.last - range.first);
            if (points < 2) continue;
            const auto pairs = points * (points + 1) / 2 - (range.last + 1 == source.truck.size() ? 1 : 0);
            const auto count = (points - 1) * pairs;
            groups.push_back({range, pairs, count});
            total += count;
        }
        if (!total) return std::nullopt;
        auto rank = std::uniform_int_distribution<std::uint64_t>(0, total - 1)(random);
        for (const auto& group : groups) {
            if (rank >= group.count) { rank -= group.count; continue; }
            const auto customer = group.range.first + 1 + static_cast<std::size_t>(rank / group.pairs);
            rank %= group.pairs;
            std::vector<std::size_t> endpoints;
            for (std::size_t p = group.range.first; p <= group.range.last; ++p)
                if (p != customer) endpoints.push_back(p);
            for (std::size_t i = 0; i < endpoints.size(); ++i) {
                if (endpoints[i] + 1 == source.truck.size()) continue;
                const auto count = endpoints.size() - i;
                if (rank >= count) { rank -= count; continue; }
                return addFlight(source, customer, endpoints[i], endpoints[i + static_cast<std::size_t>(rank)]);
            }
        }
        throw std::logic_error("Falha no sorteio de uma viagem HGVNS.");
    }
}

    std::optional<State> rebindFlights(const Context& context, const State& source, std::vector<int> truck) {
        State result{std::move(truck), {}};
        std::vector<std::size_t> position(static_cast<std::size_t>(context.dimension()), 0);
        for (std::size_t p = 1; p + 1 < result.truck.size(); ++p) position[result.truck[p]] = p;
        const auto map = [&](std::size_t p) {
            return p + 1 == source.truck.size() ? result.truck.size() - 1 : position[source.truck[p]];
        };
        struct Link { Flight current; Flight previous; };
        std::vector<Link> links;
        for (const auto& flight : source.flights) {
            const auto a = map(flight.launch), b = map(flight.recovery);
            links.push_back({{std::min(a, b), flight.customer, std::max(a, b)}, flight});
        }
        std::sort(links.begin(), links.end(), [](const Link& a, const Link& b) {
            return std::tie(a.current.launch, a.current.recovery, a.current.customer)
                 < std::tie(b.current.launch, b.current.recovery, b.current.customer);
        });
        std::vector<bool> conflict(links.size(), false);
        bool needsRepair = false;
        std::size_t component = 0, furthest = 0;
        for (std::size_t i = 0; i <= links.size(); ++i) {
            if (i < links.size() && (i == component || links[i].current.launch < furthest)) {
                furthest = std::max(furthest, links[i].current.recovery);
                continue;
            }
            if (i - component > 1) {
                std::fill(conflict.begin() + component, conflict.begin() + i, true);
                needsRepair = true;
            }
            component = i;
            if (i < links.size()) furthest = links[i].current.recovery;
        }
        if (!needsRepair) {
            for (const auto& link : links) result.flights.push_back(link.current);
            return result;
        }

        // Adaptacao documentada: preserva viagens validas; nos conflitos, considera
        // os extremos remapeados e as posicoes anteriores. Nao reatribui clientes.
        const auto prefix = evaluate(context, State{result.truck, {}}, false).prefix;
        struct Option { Flight flight; double cost; int predecessor = -1; };
        std::vector<std::vector<Option>> choices;
        for (std::size_t i = 0; i < links.size(); ++i) {
            const auto& link = links[i];
            std::vector<Flight> alternatives{link.current};
            if (conflict[i]) {
                std::vector<std::size_t> endpoints{link.current.launch, link.current.recovery,
                                                 link.previous.launch, link.previous.recovery};
                std::sort(endpoints.begin(), endpoints.end());
                endpoints.erase(std::unique(endpoints.begin(), endpoints.end()), endpoints.end());
                alternatives.clear();
                for (auto a : endpoints)
                    for (auto b : endpoints)
                        if (a <= b && a + 1 < result.truck.size() && b < result.truck.size())
                            alternatives.push_back({a, link.current.customer, b});
            }
            std::vector<Option> options;
            for (const auto& flight : alternatives) {
                double previousCost = i == 0 ? 0 : std::numeric_limits<double>::infinity();
                int predecessor = -1;
                if (i > 0) {
                    for (std::size_t j = 0; j < choices.back().size(); ++j) {
                        const auto& previous = choices.back()[j];
                        if (previous.flight.recovery <= flight.launch && previous.cost < previousCost) {
                            previousCost = previous.cost;
                            predecessor = static_cast<int>(j);
                        }
                    }
                }
                const double droneTime = context.drone(result.truck[flight.launch], flight.customer)
                                       + context.drone(flight.customer, result.truck[flight.recovery]);
                const double truckTime = prefix[flight.recovery] - prefix[flight.launch];
                options.push_back({flight, previousCost + std::max(0.0, droneTime - truckTime), predecessor});
            }
            choices.push_back(std::move(options));
        }
        int best = -1;
        double bestCost = std::numeric_limits<double>::infinity();
        for (std::size_t j = 0; j < choices.back().size(); ++j)
            if (choices.back()[j].cost < bestCost) { bestCost = choices.back()[j].cost; best = static_cast<int>(j); }
        if (best < 0) return std::nullopt;
        result.flights.resize(links.size());
        for (std::size_t i = links.size(); i-- > 0;) {
            const auto& chosen = choices[i][static_cast<std::size_t>(best)];
            result.flights[i] = chosen.flight;
            best = chosen.predecessor;
        }
        sortFlights(result);
        return result;
    }

    State addFlight(const State& source, std::size_t customer, std::size_t launch, std::size_t recovery) {
        State result = source;
        const int node = source.truck[customer];
        const auto shifted = [&](std::size_t p) { return p > customer ? p - 1 : p; };
        result.truck.erase(result.truck.begin() + customer);
        for (auto& flight : result.flights) {
            flight.launch = shifted(flight.launch);
            flight.recovery = shifted(flight.recovery);
        }
        result.flights.push_back({shifted(launch), node, shifted(recovery)});
        sortFlights(result);
        return result;
    }

    std::optional<State> bestNeighbor(const Context& context, const State& source, Neighborhood neighborhood) {
        const double cost = evaluate(context, source, false).total;
        return neighborhood == Neighborhood::RelocateCustomer ? bestAddition(context, source, cost)
                                                               : bestTruck(context, source, neighborhood, cost);
    }

    std::optional<State> randomNeighbor(const Context& context, const State& source,
                                       Neighborhood neighborhood, std::mt19937& random) {
        if (neighborhood == Neighborhood::RelocateCustomer) return randomAddition(source, random);
        // Um movimento cru uniforme, quando viavel, evita reparar toda a vizinhanca.
        // Se ele falhar, o reservatorio abaixo sorteia uniformemente entre os viaveis.
        // A mistura continua uniforme: 1/M + ((M-F)/M)/F = 1/F para cada movimento viavel.
        std::uint64_t moves = 0;
        truckMoves(source, neighborhood, [&](const Move&) { ++moves; });
        if (!moves) return std::nullopt;
        const auto rank = std::uniform_int_distribution<std::uint64_t>(0, moves - 1)(random);
        std::uint64_t index = 0;
        std::optional<Move> drawn;
        truckMoves(source, neighborhood, [&](const Move& move) {
            if (index++ == rank) drawn = move;
        });
        auto direct = rebindFlights(context, source, apply(source, *drawn));
        if (direct) return direct;
        std::uint64_t count = 0;
        std::optional<State> selected;
        truckMoves(source, neighborhood, [&](const Move& move) {
            auto candidate = rebindFlights(context, source, apply(source, move));
            if (!candidate) return;
            ++count;
            if (std::uniform_int_distribution<std::uint64_t>(1, count)(random) == 1)
                selected = std::move(candidate);
        });
        return selected;
    }

    State createInitialSolution(const Context& context, const std::vector<int>& tour) {
        State current{tour, {}};
        validate(context, current);
        while (true) {
            double cost = evaluate(context, current, false).total;
            auto best = bestAddition(context, current, cost);
            if (best) cost = evaluate(context, *best, false).total;
            auto insertion = bestTruck(context, current, Neighborhood::Reinsertion, cost, true);
            if (insertion) best = std::move(insertion);
            if (!best) return current;
            validate(context, *best);
            current = std::move(*best);
        }
    }
}
