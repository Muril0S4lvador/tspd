#pragma once

#include <chrono>
#include <string>

#include "solution.hpp"
#include "../tspd/tspd.hpp"

namespace tspd::solution {
    class SolutionStrategy {
        public:
            using Clock = std::chrono::high_resolution_clock;

            virtual ~SolutionStrategy() = default;

            Solution Solve(::tspd::tspd::TSPD& instance);
            Solution Solve(::tspd::tspd::TSPD& instance, const Solution& tspSolution);
            virtual Solution Optimize(::tspd::tspd::TSPD& instance) = 0;
            // Falha explicitamente se uma nova estrategia nao implementar esta entrada.
            virtual Solution Optimize(::tspd::tspd::TSPD& instance, const Solution& tspSolution);

            Clock::time_point getStartTime() const { return start_; }
            Clock::time_point getEndTime() const { return end_; }
            std::chrono::duration<double> getDuration() const { return end_ - start_; }
            virtual std::string getName() const { return "[Default]"; }

        protected:
            static const vector<int>& requireTspRoute(
                const ::tspd::tspd::TSPD& instance, const Solution& tspSolution);

        private:
            Solution solve(::tspd::tspd::TSPD& instance, const Solution* tspSolution);
            Clock::time_point start_{};
            Clock::time_point end_{};
    };
}
