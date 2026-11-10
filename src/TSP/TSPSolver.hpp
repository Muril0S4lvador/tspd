#pragma once

#include "../solution/solution_strategy.hpp"

namespace tspd::tsp {
    class TSPSolver : public ::tspd::solution::SolutionStrategy {
        public:
            std::string getName() const override { return "TSP Solver"; }
            solution::Solution Optimize(::tspd::tspd::TSPD& instance) override;
    };
}
