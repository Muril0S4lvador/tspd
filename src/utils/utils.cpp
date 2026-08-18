#include "utils.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <stdexcept>

namespace tspd::utils {
    std::vector<node::Node> calculateNodesFromDistances(
        const std::vector<edge::Edge>& edges,
        int dimension) {
        if (dimension <= 0)
            return {};

        const std::size_t nodeCount = static_cast<std::size_t>(dimension);
        const std::size_t expectedEdgeCount = nodeCount * (nodeCount - 1) / 2;
        if (edges.size() != expectedEdgeCount)
            throw std::invalid_argument("Compact edge list size differs from DIMENSION");

        std::vector<node::Node> nodes;
        nodes.reserve(nodeCount);
        if (nodeCount == 1) {
            nodes.emplace_back(1, 0.0, 0.0);
            return nodes;
        }

        const auto squared = [](double value) { return value * value; };
        const double epsilon = std::numeric_limits<double>::epsilon();

        std::size_t secondAnchor = 1;
        for (std::size_t index = 2; index < nodeCount; ++index) {
            if (edge::Edge::getWeight(edges, dimension, 1, static_cast<int>(index + 1)) >
                edge::Edge::getWeight(edges, dimension, 1,
                                      static_cast<int>(secondAnchor + 1))) {
                secondAnchor = index;
            }
        }

        const double firstSecondDistance = edge::Edge::getWeight(
            edges, dimension, 1, static_cast<int>(secondAnchor + 1));
        if (firstSecondDistance <= epsilon) {
            for (std::size_t index = 0; index < nodeCount; ++index)
                nodes.emplace_back(static_cast<int>(index + 1), 0.0, 0.0);
            return nodes;
        }

        const auto xFromTwoAnchors = [&](std::size_t index) {
            const double firstDistance = edge::Edge::getWeight(
                edges, dimension, 1, static_cast<int>(index + 1));
            const double secondDistance = edge::Edge::getWeight(
                edges, dimension, static_cast<int>(secondAnchor + 1),
                static_cast<int>(index + 1));
            return (squared(firstDistance) + squared(firstSecondDistance) -
                    squared(secondDistance)) /
                   (2.0 * firstSecondDistance);
        };

        std::size_t thirdAnchor = 0;
        double thirdAnchorHeightSquared = 0.0;
        for (std::size_t index = 0; index < nodeCount; ++index) {
            if (index == 0 || index == secondAnchor)
                continue;

            const double x = xFromTwoAnchors(index);
            const double firstDistance = edge::Edge::getWeight(
                edges, dimension, 1, static_cast<int>(index + 1));
            const double heightSquared = squared(firstDistance) - squared(x);
            if (thirdAnchor == 0 || heightSquared > thirdAnchorHeightSquared) {
                thirdAnchor = index;
                thirdAnchorHeightSquared = heightSquared;
            }
        }

        const double thirdAnchorX =
            thirdAnchor == 0 ? 0.0 : xFromTwoAnchors(thirdAnchor);
        const double thirdAnchorY =
            std::sqrt(std::max(0.0, thirdAnchorHeightSquared));

        for (std::size_t index = 0; index < nodeCount; ++index) {
            const double x = xFromTwoAnchors(index);
            double y = 0.0;

            if (thirdAnchor != 0 && thirdAnchorY > epsilon) {
                const double firstDistance = edge::Edge::getWeight(
                    edges, dimension, 1, static_cast<int>(index + 1));
                const double thirdDistance = edge::Edge::getWeight(
                    edges, dimension, static_cast<int>(thirdAnchor + 1),
                    static_cast<int>(index + 1));
                const double rightSide = squared(firstDistance) +
                                         squared(thirdAnchorX) +
                                         squared(thirdAnchorY) -
                                         squared(thirdDistance);
                y = (rightSide / 2.0 - x * thirdAnchorX) / thirdAnchorY;
            }

            nodes.emplace_back(static_cast<int>(index + 1), x, y);
        }

        return nodes;
    }
}
