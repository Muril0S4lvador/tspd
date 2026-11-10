#pragma once

#include "../solution/solution_strategy.hpp"
#include <cstdint>
#include <limits>
#include <vector>

namespace tspd::hga_tac {
// Table 1 and Algorithm 4 of Mahmoudinazlou & Kwon (2024).
struct Parameters {
    int population = 15;
    int offspring = 25;
    int tournament = 5;
    int noImprovement = 2500;
    int diversifyAfter = 100;
    int escapeAfter = 1000;
    int escapeIterations = 10000;
    int bufferSize = 40;
    double escapeGap = 0.05;
    double mutationProbability = 0.1;
    double repairProbability = 0.5;
    double closeFraction = 0.3;
    // Optional TSPD endurance; the current instance file format supplies none.
    double endurance = std::numeric_limits<double>::infinity();
};

struct Statistics {
    int iterations = 0;
    int diversifications = 0;
    int escapes = 0;
    int repaired = 0;
    int infeasibleOffspring = 0;
};

class HGATAC : public ::tspd::solution::SolutionStrategy {
public:
    explicit HGATAC(std::uint32_t seed = 0, bool escape = false,
                    Parameters parameters = {});
    std::string getName() const override { return escape_ ? "HGA-TAC+" : "HGA-TAC"; }
    solution::Solution Optimize(::tspd::tspd::TSPD& instance) override;
    solution::Solution Optimize(::tspd::tspd::TSPD& instance,
                                const solution::Solution& tspSolution) override;
    const Statistics& statistics() const { return statistics_; }
private:
    std::uint32_t seed_;
    bool escape_;
    Parameters parameters_;
    Statistics statistics_;
};
}
