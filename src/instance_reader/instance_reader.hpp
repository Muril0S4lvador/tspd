#pragma once

#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include "../tspd/tspd.hpp"

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
            static string trim(const string& value);
            static string toUpper(const string& value);
            static bool isSectionMarker(const string& line);
            static bool parseHeaderLine(const string& line, string& key, string& value);
            static string getValue(const string& line);

            static int parseDimension(const string& line);

            static void readCoordinateSection(const vector<string>& lines,
                                              size_t& lineIndex,
                                              vector<node::Node>& nodes);
            static void readEdgeWeightSection(const vector<string>& lines,
                                              size_t& lineIndex,
                                              vector<double>& edgeWeights);
            static vector<node::Node> calculateCoordinates(
                const vector<double>& edgeWeights,
                const string& edgeWeightFormat,
                int dimension);
            static node::Node parseNode(const string& line);
    };
}
