#pragma once

#include <iostream>
#include <vector>

namespace tspd::node {
    class Node {
        private:
            int id_;
            double x_;
            double y_;
        
        public:
            Node(int id, double x, double y);

            int getId() const;
            double getX() const;
            double getY() const;

            // Sobrecarga de Operador <<
            friend std::ostream& operator<<(std::ostream& os, const Node& node){
                os << "Node[" << node.id_ << "] (" << node.x_ << ", " << node.y_ << ")";
                return os;
            }
        };
        
    // Sobrecarga de Operador << para vetores desse tipo
    inline std::ostream& operator<<(std::ostream& os, const std::vector<Node>& vec) {
        for (size_t i = 0; i < vec.size(); ++i) {
            os << vec[i];
            if (i + 1 < vec.size()) {
                os << '\n';
            }
        }
        return os;
    }
}