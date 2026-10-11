#include "node.hpp"

namespace tspd::node {
    Node::Node(double x, double y, int id, std::string name) : id_(id), x_(x), y_(y), name_(name) {}

    int Node::getId() const {return id_;}
    double Node::getX() const {return x_;}
    double Node::getY() const {return y_;}
    std::string Node::getName() const {return name_;}
}