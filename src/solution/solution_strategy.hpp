#pragma once

#include <chrono>

#include "solution.hpp"
#include "../tspd/tspd.hpp"

namespace tspd::solution {
    class SolutionStrategy {
        public:
            using Clock = std::chrono::high_resolution_clock;

            virtual ~SolutionStrategy() = default;

            Solution Solve(::tspd::tspd::TSPD& instance);
            virtual Solution Optimize(::tspd::tspd::TSPD& instance) = 0;

            Clock::time_point getStartTime() const { return start_; }
            Clock::time_point getEndTime() const { return end_; }
            std::chrono::duration<double> getDuration() const { return end_ - start_; }

        private:
            Clock::time_point start_{};
            Clock::time_point end_{};
    };
}
