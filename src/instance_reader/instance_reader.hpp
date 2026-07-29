#pragma once

#include <string>
#include <iostream>
#include <fstream>
#include <sstream>

#include "../tspd/tspd.hpp"

using tspd::tspd::TSPD;
using std::string;
using std::ifstream;
using std::stringstream;

namespace tspd::instance_reader {
    class InstanceReader {
        public:
            InstanceReader() = delete; // Para uma classe estática

            static TSPD readInstance(const string& filePath);

        private:
            static string getValue(const string& line);

            static void parseStrValue(const string& line, string& var);
            static int parseDimension(const string& line);

            static void readNodeCoordSector(ifstream& file, vector<node::Node>& node);
            static node::Node parseNode(const string& line);
    };
}
