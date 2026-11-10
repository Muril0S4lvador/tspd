#pragma once

#include "solution_state.hpp"

#include <optional>
#include <random>

namespace tspd::hgvns::detail {
    enum class Neighborhood { Reinsertion, OrOpt2, Exchange, Exchange21, Exchange22, TwoOpt, RelocateCustomer };

    // Mantem extremos por ID e, quando necessario, aplica a politica local de reparo documentada.
    std::optional<State> rebindFlights(const Context& context, const State& source, std::vector<int> truck);
    State addFlight(const State& source, std::size_t customer, std::size_t launch, std::size_t recovery);
    std::optional<State> bestNeighbor(const Context& context, const State& source, Neighborhood neighborhood);
    std::optional<State> randomNeighbor(const Context& context, const State& source,
                                       Neighborhood neighborhood, std::mt19937& random);
    State createInitialSolution(const Context& context, const std::vector<int>& tour);
}
