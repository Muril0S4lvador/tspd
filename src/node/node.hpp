#pragma once

#include <iostream>
#include <vector>

namespace tspd::node {
    class Node {
        private:
            int id_ = -1;
            double x_ = 0.0;
            double y_ = 0.0;
            std::string name_;
        
        public:
            Node() = default;
            Node(double x, double y, int id, std::string name);

            int getId() const;
            double getX() const;
            double getY() const;
            std::string getName() const;

            // Sobrecarga de Operador <<
            friend std::ostream& operator<<(std::ostream& os, const Node& node){
                os << "Node[" << node.id_ << "] (" << node.x_ << ", " << node.y_ << ")\t" << node.name_;
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