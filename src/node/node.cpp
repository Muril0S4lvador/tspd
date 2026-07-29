#include "node.hpp"

namespace tspd::node {
    Node::Node(int id, double x, double y) : id_(id), x_(x), y_(y) {}

    int Node::getId() const {return id_;}
    double Node::getX() const {return x_;}
    double Node::getY() const {return y_;}
}