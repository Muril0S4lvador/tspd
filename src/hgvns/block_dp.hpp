#pragma once
#include "solution_state.hpp"
#include <cstdint>

namespace tspd::hgvns {
    enum class BlockSelection { None, Waiting, Random };
    struct BlockParameters {
        std::size_t maxNodes = 20;
        std::size_t maxBlocks = 5;
        std::size_t maxOrders = 32;
    };
    struct BlockStatistics {
        std::vector<std::size_t> selectedOperations;
        std::vector<double> selectedWaiting;
        std::size_t candidates = 0;
        std::size_t accepted = 0;
        std::size_t ordersEvaluated = 0;
    };
    namespace detail {
        // Open path: endpoint IDs may coincide for a stationary operation.
        State partitionPath(const Context& context, const std::vector<int>& sequence);
        State postprocessBlocks(const Context& context, const State& source,
                                BlockSelection selection, std::uint32_t seed,
                                BlockParameters parameters = {}, BlockStatistics* statistics = nullptr);
    }
}
