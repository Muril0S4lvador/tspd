#include "solution_strategy.hpp"
#include <iostream>

namespace tspd::solution {
    Solution SolutionStrategy::Solve(::tspd::tspd::TSPD& instance) {
        start_ = Clock::now();
        end_ = start_;
        try {
            Solution result = Optimize(instance);
            end_ = Clock::now();
            std::cout << "Optimization " << this->getName() << " Finalized.\n";
            return result;
        } catch (...) {
            end_ = Clock::now();
            throw;
        }
    }
}
