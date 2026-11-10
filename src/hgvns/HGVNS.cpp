#include "HGVNS.hpp"
#include "neighborhoods.hpp"

#include <algorithm>
#include <array>
#include <random>
#include <stdexcept>

namespace tspd::hgvns {
namespace {
    using namespace detail;
    constexpr std::array<Neighborhood, 7> neighborhoods{
        Neighborhood::Reinsertion, Neighborhood::OrOpt2, Neighborhood::Exchange,
        Neighborhood::Exchange21, Neighborhood::Exchange22, Neighborhood::TwoOpt,
        Neighborhood::RelocateCustomer
    };

    State rvnd(const Context& context, State current, std::mt19937& random) {
        auto order = neighborhoods;
        std::shuffle(order.begin(), order.end(), random);
        std::size_t k = 0;
        while (k < order.size()) {
            auto candidate = bestNeighbor(context, current, order[k]);
            if (candidate) {
                validate(context, *candidate);
                current = std::move(*candidate);
                std::shuffle(order.begin(), order.end(), random);
                k = 0;
            } else {
                ++k;
            }
        }
        return current;
    }
}

    solution::Solution HGVNS::Optimize(::tspd::tspd::TSPD&) {
        throw std::logic_error("HGVNS requer um TSP previamente calculado. Use Solve(instance, tspSolution).");
    }

    solution::Solution HGVNS::Optimize(::tspd::tspd::TSPD& instance,
                                      const solution::Solution& tspSolution) {
        const auto& tour = requireTspRoute(instance, tspSolution);
        detail::Context context(instance);
        statistics_ = {};
        std::mt19937 pdRandom(seed_);
        auto applyPD = [&](detail::State state) {
            if (selection_ == BlockSelection::None || parameters_.maxBlocks == 0) return state;
            BlockStatistics stats;
            const auto pdSeed = statistics_.pdCalls == 0 ? seed_ : pdRandom();
            auto improved = detail::postprocessBlocks(context, state, selection_, pdSeed, parameters_, &stats);
            ++statistics_.pdCalls;
            statistics_.ordersEvaluated += stats.ordersEvaluated;
            statistics_.blocksAccepted += stats.accepted;
            return improved;
        };
        auto current = detail::createInitialSolution(context, tour);
        if (placement_ == PDPlacement::Initial) current = applyPD(std::move(current));
        double cost = detail::evaluate(context, current).total;
        std::mt19937 random(seed_);
        std::size_t k = 0;
        while (k < neighborhoods.size()) {
            auto perturbed = detail::randomNeighbor(context, current, neighborhoods[k], random);
            if (!perturbed) { ++k; continue; }
            if (placement_ == PDPlacement::AfterShake) *perturbed = applyPD(std::move(*perturbed));
            auto candidate = rvnd(context, std::move(*perturbed), random);
            if (placement_ == PDPlacement::AfterRVND) candidate = applyPD(std::move(candidate));
            const double candidateCost = detail::evaluate(context, candidate).total;
            if (detail::improves(candidateCost, cost)) {
                current = std::move(candidate);
                cost = candidateCost;
                k = 0;
            } else {
                ++k;
            }
        }
        if (placement_ == PDPlacement::Final) current = applyPD(std::move(current));
        return detail::toSolution(context, current);
    }
}
