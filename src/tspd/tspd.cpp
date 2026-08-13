#include "tspd.hpp"

#include <algorithm>

namespace tspd::tspd {
    TSPD::TSPD(string name,
               string comment,
               string type,
               int dimension,
               string edge_weight_type,
               vector<node::Node> nodes,
               vector<int> distances)
        : name_(name),
          comment_(comment),
          type_(type),
          dimension_(dimension),
          edge_weight_type_(edge_weight_type),
          nodes_(nodes),
          distances_(distances) {}

    string TSPD::getName() const {return name_;}
    string TSPD::getComment() const {return comment_;}
    string TSPD::getType() const {return type_;}
    int TSPD::getDimension() const {return dimension_;}
    string TSPD::getEdgeWeightType() const {return edge_weight_type_;}
    vector<node::Node> TSPD::getNodes() const {return nodes_;}
    vector<int> TSPD::getDistances() const {return distances_;}

    int TSPD::getDistance(int firstNodeId, int secondNodeId) const {
        if (firstNodeId < 1 || secondNodeId < 1 ||
            firstNodeId > dimension_ || secondNodeId > dimension_) {
            throw std::out_of_range("Node id outside distance matrix");
        }

        if (firstNodeId == secondNodeId)
            return 0;

        size_t first = static_cast<size_t>(firstNodeId - 1);
        size_t second = static_cast<size_t>(secondNodeId - 1);
        if (first > second)
            std::swap(first, second);

        const size_t index =
            first * (2 * static_cast<size_t>(dimension_) - first - 1) / 2 +
            second - first - 1;
        if (index >= distances_.size())
            throw std::out_of_range("Distance is not available");

        return distances_[index];
    }
}
