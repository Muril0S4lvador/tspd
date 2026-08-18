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
            string comment_;
            string type_;
            int dimension_;
            string edge_weight_type_;
            vector<node::Node> nodes_;
            vector<Edge> edges_;

        public:
            TSPD(string name,
                 string comment,
                 string type,
                 int dimension,
                 string edge_weight_type,
                 vector<node::Node> nodes,
                 vector<Edge> edges = {});

            string getName() const;
            string getComment() const;
            string getType() const;
            int getDimension() const;
            string getEdgeWeightType() const;
            vector<node::Node> getNodes() const;
            vector<Edge> getEdges() const;
            int getDistance(int firstNodeId, int secondNodeId) const;

            // Sobrecarga de Operador <<
            friend std::ostream& operator<<(std::ostream& os, const TSPD& tspd) {
                os << "\nInstância " << tspd.name_ << '\n';
                os << "Comment(s): " << tspd.comment_ << '\n';
                os << "Type: " << tspd.type_ << '\n';
                os << "Edge Weight Type: " << tspd.edge_weight_type_ << '\n';
                os << "Dimension: " << tspd.dimension_ << '\n';
                // os << "Nodes: \n" << tspd.nodes_;
                return os;
            }
    };
}
