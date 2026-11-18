#pragma once

#include "../solution/solution_strategy.hpp"

namespace tspd::tsp {
    class TSPSolver : public ::tspd::solution::SolutionStrategy {
        private:
            static std::vector<int> improveRoute(const ::tspd::tspd::TSPD& instance,
                                                 std::vector<int> route);
        public:
            std::string getName() const override { return "TSP Solver"; }
            solution::Solution Optimize(::tspd::tspd::TSPD& instance) override;
    };
}
