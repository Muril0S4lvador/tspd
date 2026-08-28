#pragma once

#include <vector>
using std::vector;

#include <algorithm>

#include "../../edge/edge.hpp"
using tspd::edge::Edge;

#include "../../node/node.hpp"
using tspd::node::Node;

namespace tspd::dfs{
    class DFS{
        private:
            static vector<vector<int>> _getAdjacencyList(vector<Node>& nodes, vector<Edge>& edges);
            static void _recurseDFS(int nodeId, vector<bool>& visited, vector<int>& path, vector<vector<int>>& adjacencyList);
            
        public:
            static vector<int> getDFS(vector<Node>& nodes, vector<Edge>& edges);
    };
}