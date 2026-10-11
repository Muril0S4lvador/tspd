#include "solution_strategy.hpp"

namespace tspd::solution {
    Solution SolutionStrategy::Solve(::tspd::tspd::TSPD& instance) {
        start_ = Clock::now();
        end_ = start_;
        try {
            Solution result = Optimize(instance);
            end_ = Clock::now();
            return result;
        } catch (...) {
            end_ = Clock::now();
            throw;
        }
    }
}
