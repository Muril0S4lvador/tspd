#pragma once

#include "../solution/solution_strategy.hpp"
#include "block_dp.hpp"

#include <cstdint>

namespace tspd::hgvns {
    enum class PDPlacement { Final, Initial, AfterShake, AfterRVND };
    struct HGVNSStatistics {
        std::size_t pdCalls = 0;
        std::size_t ordersEvaluated = 0;
        std::size_t blocksAccepted = 0;
    };
    class HGVNS : public ::tspd::solution::SolutionStrategy {
        public:
            explicit HGVNS(std::uint32_t seed = 0, BlockSelection selection = BlockSelection::None,
                           BlockParameters parameters = {}, PDPlacement placement = PDPlacement::Final)
                : seed_(seed), selection_(selection), parameters_(parameters), placement_(placement) {}
            std::string getName() const override {
                if (selection_ == BlockSelection::None) return "HGVNS";
                const std::string name = selection_ == BlockSelection::Waiting ? "HGVNS PD Waiting" : "HGVNS PD Random";
                return name + (placement_ == PDPlacement::Initial ? " Initial"
                             : placement_ == PDPlacement::AfterShake ? " After Shake"
                             : placement_ == PDPlacement::AfterRVND ? " After RVND" : " Final");
            }
            solution::Solution Optimize(::tspd::tspd::TSPD& instance) override;
            solution::Solution Optimize(::tspd::tspd::TSPD& instance,
                                        const solution::Solution& tspSolution) override;
            const HGVNSStatistics& statistics() const { return statistics_; }
        private:
            std::uint32_t seed_;
            BlockSelection selection_;
            BlockParameters parameters_;
            PDPlacement placement_;
            HGVNSStatistics statistics_;
    };
}
