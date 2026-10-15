#include "tspd.hpp"

#include <utility>

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
}
