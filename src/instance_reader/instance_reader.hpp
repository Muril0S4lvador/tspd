#pragma once

#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <chrono>

#include "../edge/edge.hpp"
#include "../tspd/tspd.hpp"

using tspd::edge::Edge;
using tspd::tspd::TSPD;
using std::ifstream;
using std::size_t;
using std::string;
using std::stringstream;
using std::vector;

namespace tspd::instance_reader {
    class InstanceReader {
        public:
            InstanceReader() = delete;

            static TSPD readInstance(const string& filePath);
            static vector<Edge> calculateEdges(const vector<node::Node>& nodes);
        private:
            static node::Node readDepot(string& line);
            static vector<node::Node> readNodes(ifstream& file, int& dimension);
    };
}
