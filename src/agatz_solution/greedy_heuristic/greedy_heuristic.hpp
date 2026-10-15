#pragma once

#include <vector>
#include <string>

#include "../../solution/solution_strategy.hpp"
#include "../../tspd/tspd.hpp"

namespace tspd::greedyHeuristic {
    /**
     * @brief Route-first/cluster-second greedy heuristic from Agatz et al.
     *
     * The current project exposes one distance matrix only. Consequently,
     * the same distances are multiplied by each vehicle cost factor by the
     * implementation in the .cpp file.
     */
    class GreedyHeuristic : public ::tspd::solution::SolutionStrategy {
        private:
            static solution::Solution _greedyHeuristic(
                ::tspd::tspd::TSPD& instance,
                const std::vector<int>& initialRoute);

        public:
            std::string getName() const override { return "Greedy Heuristic"; }

            solution::Solution Optimize(
                ::tspd::tspd::TSPD& instance) override;
    };
}
