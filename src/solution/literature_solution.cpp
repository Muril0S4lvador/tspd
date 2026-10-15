#include "literature_solution.hpp"

#include <utility>

namespace tspd::solution {
    LiteratureSolution::LiteratureSolution(std::vector<edge::Edge> truckEdges,
                                           std::vector<edge::Edge> droneEdges,
                                           std::optional<double> totalCost)
        : truckEdges_(std::move(truckEdges)),
          droneEdges_(std::move(droneEdges)),
          totalCost_(totalCost) {}

    const std::vector<edge::Edge>& LiteratureSolution::getTruckEdges() const {
        return truckEdges_;
    }

    const std::vector<edge::Edge>& LiteratureSolution::getDroneEdges() const {
        return droneEdges_;
    }

    bool LiteratureSolution::hasTotalCost() const {
        return totalCost_.has_value();
    }

    double LiteratureSolution::getTotalCost() const {
        return totalCost_.value();
    }
}
