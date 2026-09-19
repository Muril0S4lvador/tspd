#pragma once

#include <vector>

#include "../../solution/solution.hpp"
#include "../../tspd/tspd.hpp"

namespace tspd::greedyHeuristic {
    /**
     * @brief Route-first/cluster-second greedy heuristic from Agatz et al.
     *
     * The current project exposes one distance matrix only. Consequently,
     * the same matrix is used for truck and drone travel time by the
     * implementation in the .cpp file.
     */
    class GreedyHeuristic {
        private:
            static solution::Solution _greedyHeuristic(
                ::tspd::tspd::TSPD& instance,
                const std::vector<int>& initialRoute);

        public:
            static solution::Solution getGreedyHeuristicSolution(
                ::tspd::tspd::TSPD& instance);
    };
}
