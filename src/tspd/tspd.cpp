#include "tspd.hpp"

#include <utility>
#include <cmath>
#include <stdexcept>

namespace tspd::tspd {
    TSPD::TSPD(string name,
                string type,
                int dimension,
                double truckSpeed, 
                double droneSpeed,
                node::Node depot,
                vector<node::Node> nodes,
                vector<Edge> edges)
        :   name_(name),
            type_(type),
            dimension_(dimension),
            truckSpeed_(truckSpeed),
            droneSpeed_(droneSpeed),
            depot_(depot),
            nodes_(nodes),
            edges_(edges) {}

    string TSPD::getName() const {return name_;}
    string TSPD::getType() const {return type_;}
    int TSPD::getDimension() const {return dimension_;}
    double TSPD::getTruckSpeed() const {return truckSpeed_;}
    double TSPD::getDroneSpeed() const {return droneSpeed_;}
    vector<node::Node> TSPD::getNodes() const {return nodes_;}
    vector<edge::Edge> TSPD::getEdges() const {return edges_;}
    bool TSPD::hasLiteratureSolution() const { return literatureSolution_.has_value(); }

    const solution::LiteratureSolution& TSPD::getLiteratureSolution() const {
        return literatureSolution_.value();
    }

    void TSPD::setLiteratureSolution(solution::LiteratureSolution literatureSolution) {
        literatureSolution_ = std::move(literatureSolution);
    }

    double TSPD::getDistance(int firstNodeId, int secondNodeId) const { // Evita truncar o peso double retornado por Edge.
        return Edge::getWeight(edges_, dimension_, firstNodeId, secondNodeId);
    }

    bool TSPD::hasLiteratureTspSolution() const { return literatureTspSolution_.has_value(); }

    const solution::LiteratureSolution& TSPD::getLiteratureTspSolution() const {
        return literatureTspSolution_.value();
    }

    void TSPD::setLiteratureTspSolution(solution::LiteratureSolution literatureSolution) {
        literatureTspSolution_ = std::move(literatureSolution);
    }

    double TSPD::calculateTspCost(const vector<int>& route) const {
        if (!std::isfinite(truckSpeed_) || truckSpeed_ <= 0.0)
            throw std::invalid_argument("Truck speed must be positive and finite");
        if (nodes_.empty() || route.size() != nodes_.size() + 1 ||
            route.front() != depot_.getId() || route.back() != depot_.getId())
            throw std::invalid_argument("TSP route must cover all nodes and return to the depot");
        vector<bool> visited(nodes_.size(), false);
        for (std::size_t index = 0; index + 1 < route.size(); ++index) {
            const int node = route[index];
            if (node < 0 || static_cast<std::size_t>(node) >= visited.size() || visited[node])
                throw std::invalid_argument("TSP route must visit each node exactly once");
            visited[node] = true;
        }
        double cost = 0.0;
        for (std::size_t index = 1; index < route.size(); ++index)
            cost += getDistance(route[index - 1], route[index]) * truckSpeed_;
        if (!std::isfinite(cost))
            throw std::invalid_argument("TSP cost must be finite");
        return cost;
    }
}
