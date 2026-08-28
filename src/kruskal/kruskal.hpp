#pragma once

#include <vector>
using std::vector;

#include <algorithm>

#include "../solution/solution.hpp"
using tspd::solution::Solution;

#include "../edge/edge.hpp"
using tspd::edge::Edge;

#include "../node/node.hpp"
using tspd::node::Node;


namespace tspd::kruskal{
    class Kruskal{
        private:
            static void _addMST(vector<int>& mst, Node& u, Node& v);
            static void _setUnion(vector<int>& set, Node& u, Node& v);
            static bool _findUnion(vector<int>& set, int uId, int vId);

        public:
            static Solution kruskal(vector<Node>& nodes, vector<Edge>& edges);
    };
}