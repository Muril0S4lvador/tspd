#pragma once 

#include <string>
#include <vector>
#include <iostream>

using std::string;
using std::vector;

#include "../edge/edge.hpp"
#include "../node/node.hpp"

using tspd::edge::Edge;

namespace tspd::tspd {
    class TSPD {
        private:
            string name_;
            string type_;
            int dimension_;
            double truckSpeed_;
            double droneSpeed_;
            node::Node depot_;
            vector<node::Node> nodes_;
            vector<Edge> edges_;

        public:
            TSPD(string name,
                string type,
                int dimension,
                double truckSpeed, 
                double droneSpeed,
                node::Node depot,
                vector<node::Node> nodes,
                vector<Edge> edges = {});

            string getName() const;
            string getType() const;
            int getDimension() const;
            double getTruckSpeed() const;
            double getDroneSpeed() const;
            int getDistance(int firstNodeId, int secondNodeId) const;
            vector<node::Node> getNodes() const;
            vector<edge::Edge> getEdges() const;

            // Sobrecarga de Operador <<
            friend std::ostream& operator<<(std::ostream& os, const TSPD& tspd) {
                os << "\nInstância " << tspd.getName() << '\n';
                os << "Type: " << tspd.getType() << '\n';
                os << "Dimension: " << tspd.getDimension() << '\n';
                os << "Truck Speed: " << tspd.getTruckSpeed() << '\n';
                os << "Drone Speed: " << tspd.getDroneSpeed() << '\n';
                // os << "Nodes: \n" << tspd.getNodes();
                return os;
            }
    };
}
