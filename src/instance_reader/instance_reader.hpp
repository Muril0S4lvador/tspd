#pragma once

#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

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

        private:

            static string _trim(const string& value);
            static string _toUpper(const string& value);
            static bool _isSectionMarker(const string& line);
            static bool _parseHeaderLine(const string& line, string& key, string& value);
            static string _getValue(const string& line);

            static int _parseDimension(const string& line);

            static void _readCoordinateSection(const vector<string>& lines,
                                               size_t& lineIndex,
                                               vector<node::Node>& nodes);
            static void _readEdgeWeightSection(const vector<string>& lines,
                                               size_t& lineIndex,
                                               vector<Edge>& edges,
                                               const string& edgeWeightFormat,
                                               int dimension);
            static vector<Edge> _readExplicitEdges(const vector<Edge>& edges,
                                                   int dimension);
            static vector<Edge> _calculateEdges(
                const vector<node::Node>& nodes,
                const string& edgeWeightType);
            static double _calculateRawDistance(const node::Node& firstNode,
                                                const node::Node& secondNode,
                                                const string& edgeWeightType);
            static node::Node _parseNode(const string& line);
    };
}
