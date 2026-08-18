#pragma once

#include <vector>

#include "../edge/edge.hpp"
#include "../node/node.hpp"

namespace tspd::utils {
    std::vector<node::Node> calculateNodesFromDistances(
        const std::vector<edge::Edge>& edges,
        int dimension);
}
