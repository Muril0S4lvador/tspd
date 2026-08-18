#include "edge.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <stdexcept>

namespace tspd::edge {
    Edge::Edge(int idA, int idB, int weight)
        : idA_(std::min(idA, idB)),
          idB_(std::max(idA, idB)),
          weight_(weight) {
        if (idA <= 0 || idB <= 0 || idA == idB)
            throw std::invalid_argument("An edge must connect two distinct positive node ids");
        if (weight < 0)
            throw std::invalid_argument("An edge weight cannot be negative");
    }

    int Edge::getAId() const { return idA_; }
    int Edge::getBId() const { return idB_; }
    int Edge::getWeight() const { return weight_; }

    int Edge::convertWeight(double weight) {
        if (!std::isfinite(weight) || weight < 0.0)
            throw std::invalid_argument("Invalid edge weight");

        const double convertedWeight = std::ceil(weight);
        if (convertedWeight > std::numeric_limits<int>::max())
            throw std::invalid_argument("Edge weight does not fit in int");

        return static_cast<int>(convertedWeight);
    }

    std::size_t Edge::compactIndex(int dimension,
                                   int firstNodeId,
                                   int secondNodeId) {
        if (dimension <= 0 || firstNodeId < 1 || secondNodeId < 1 ||
            firstNodeId > dimension || secondNodeId > dimension ||
            firstNodeId == secondNodeId) {
            throw std::out_of_range("Invalid node ids for compact distance storage");
        }

        std::size_t first = static_cast<std::size_t>(firstNodeId - 1);
        std::size_t second = static_cast<std::size_t>(secondNodeId - 1);
        if (first > second)
            std::swap(first, second);

        const std::size_t nodeCount = static_cast<std::size_t>(dimension);
        return first * (2 * nodeCount - first - 1) / 2 + second - first - 1;
    }

    int Edge::getWeight(const std::vector<Edge>& edges,
                        int dimension,
                        int firstNodeId,
                        int secondNodeId) {
        if (firstNodeId == secondNodeId)
            return 0;

        const std::size_t index = compactIndex(
            dimension, firstNodeId, secondNodeId);
        if (index >= edges.size())
            throw std::out_of_range("Edge is not available");

        return edges[index].getWeight();
    }
}
