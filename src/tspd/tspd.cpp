#include "tspd.hpp"

namespace tspd::tspd {
    TSPD::TSPD(string name, string comment, string type, int dimension, string edge_weight_type, vector<node::Node> nodes) 
        : name_(name), comment_(comment), type_(type), dimension_(dimension), edge_weight_type_(edge_weight_type), nodes_(nodes) {}

    string TSPD::getName() const {return name_;}
    string TSPD::getComment() const {return comment_;}
    string TSPD::getType() const {return type_;}
    int TSPD::getDimension() const {return dimension_;}
    string TSPD::getEdgeWeightType() const {return edge_weight_type_;}
    vector<node::Node> TSPD::getNodes() const {return nodes_;}
}