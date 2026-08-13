#pragma once 

#include <string>
#include <vector>
#include <iostream>
#include <stdexcept>

using std::string;
using std::vector;

#include "../node/node.hpp"

namespace tspd::tspd {
    class TSPD {
        private:
            string name_;
            string comment_;
            string type_;
            int dimension_;
            string edge_weight_type_;
            vector<node::Node> nodes_;
            vector<int> distances_;

        public:
            TSPD(string name,
                 string comment,
                 string type,
                 int dimension,
                 string edge_weight_type,
                 vector<node::Node> nodes,
                 vector<int> distances = {});

            string getName() const;
            string getComment() const;
            string getType() const;
            int getDimension() const;
            string getEdgeWeightType() const;
            vector<node::Node> getNodes() const;
            vector<int> getDistances() const;
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
