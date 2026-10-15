#pragma once

#include <vector>
#include <optional>

#include "../edge/edge.hpp"

namespace tspd::solution {
    class LiteratureSolution {
        private:
            std::vector<edge::Edge> truckEdges_;
            std::vector<edge::Edge> droneEdges_;
            std::optional<double> totalCost_;

        public:
            LiteratureSolution(std::vector<edge::Edge> truckEdges,
                               std::vector<edge::Edge> droneEdges,
                               std::optional<double> totalCost = std::nullopt);

            const std::vector<edge::Edge>& getTruckEdges() const;
            const std::vector<edge::Edge>& getDroneEdges() const;
            bool hasTotalCost() const;
            double getTotalCost() const;
    };
}
