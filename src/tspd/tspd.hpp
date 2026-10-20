#pragma once 

#include <string>
#include <vector>
#include <iostream>
#include <optional>

using std::string;
using std::vector;

#include "../edge/edge.hpp"
#include "../node/node.hpp"
#include "../solution/literature_solution.hpp"

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
            std::optional<solution::LiteratureSolution> literatureSolution_;
            std::optional<solution::LiteratureSolution> literatureTspSolution_;

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
            double getDistance(int firstNodeId, int secondNodeId) const; // Propaga distancias decimais aos algoritmos e a referencia.
            vector<node::Node> getNodes() const;
            vector<edge::Edge> getEdges() const;
            bool hasLiteratureSolution() const;
            const solution::LiteratureSolution& getLiteratureSolution() const;
            void setLiteratureSolution(solution::LiteratureSolution literatureSolution);
            bool hasLiteratureTspSolution() const;
            const solution::LiteratureSolution& getLiteratureTspSolution() const;
            void setLiteratureTspSolution(solution::LiteratureSolution literatureSolution);
            double calculateTspCost(const vector<int>& route) const;

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
