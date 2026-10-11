#pragma once

#include <vector>

#include "../../solution/solution_strategy.hpp"
#include "../../tspd/tspd.hpp"

namespace tspd::greedyHeuristic {
    /**
     * @brief Route-first/cluster-second greedy heuristic from Agatz et al.
     *
     * The current project exposes one distance matrix only. Consequently,
     * the same distances are divided by each vehicle speed by the
     * implementation in the .cpp file.
     */
    class GreedyHeuristic : public ::tspd::solution::SolutionStrategy {
        private:
            static solution::Solution _greedyHeuristic(
                ::tspd::tspd::TSPD& instance,
                const std::vector<int>& initialRoute);

        public:
            solution::Solution Optimize(
                ::tspd::tspd::TSPD& instance) override;
    };
}
