#pragma once

#include "../edge/edge.hpp"
#include "../node/node.hpp"

#include "../../solution/solution.hpp"
using tspd::solution::Solution;

#include "../kruskal/kruskal.hpp"
using tspd::kruskal::Kruskal;

#include "../../tspd/tspd.hpp"
using tspd::tspd::TSPD;

#include "../dfs/dfs.hpp"
using tspd::dfs::DFS;

#include <vector>
using std::vector;

namespace tspd::greedyHeuristic{
    /**
     * @brief 
     * Class that represents a greedy heuristic solution
     * proposed by Agatz (2016)
     */
    class GreedyHeuristic{
        public:
            static Solution greedyHeuristic(TSPD& instance);
    };
}