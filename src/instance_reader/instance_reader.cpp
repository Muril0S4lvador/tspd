#include "instance_reader.hpp"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <stdexcept>

namespace tspd::instance_reader {

    node::Node InstanceReader::readDepot(string& line){
        std::istringstream iss(line);
        double x = 0.0, y = 0.0;
        string name;
        iss >> x >> y >> name;
        return node::Node(x, y, 1, name);
    }

    vector<node::Node> InstanceReader::readNodes(ifstream& file, int& dimension){
        vector<node::Node> nodes {};
        string line;
        for(int i = 1; i < dimension; i++){
            std::getline(file, line);
            double x = 0.0, y = 0.0;
            string name;
            std::istringstream iss(line);
            iss >> x >> y >> name;
            nodes.push_back(node::Node(x, y, i + 1, name));
        }
        return nodes;
    }

    vector<Edge> InstanceReader::calculateEdges(const vector<node::Node>& nodes) {
        vector<Edge> edges;
        for (size_t first = 0; first < nodes.size(); ++first) {
            for (size_t second = first + 1; second < nodes.size(); ++second) {
                const auto& a = nodes[first];
                const auto& b = nodes[second];
                edges.emplace_back(a.getId(), b.getId(), Edge::convertWeight(
                    std::hypot(a.getX() - b.getX(), a.getY() - b.getY())));
            }
        }
        return edges;
    }

    TSPD InstanceReader::readInstance(const string& filePath) {
        ifstream file(filePath);
        if (!file.is_open()) throw std::invalid_argument("Fail to open " + filePath);
        else std::cout << "Reading instance " << filePath << '\n';
        
        double truckSpeed = 0.0, droneSpeed = 0.0;
        int dimension = 0;

        node::Node depot;
        vector<node::Node> nodes;

        string line;
        while (std::getline(file, line)) {
            if(line.find("/*The speed of the Truck*/") != std::string::npos)
            {
                std::getline(file, line);
                std::istringstream iss(line);
                iss >> truckSpeed;
            }
            else if(line.find("/*The speed of the Drone*/") != std::string::npos)
            {
                std::getline(file, line);
                std::istringstream iss(line);
                iss >> droneSpeed;
            }
            else if(line.find("/*Number of Nodes*/") != std::string::npos)
            {
                std::getline(file, line);
                dimension = std::stoi(line);
            }
            else if(line.find("/*The Depot*/") != std::string::npos)
            {
                std::getline(file, line);
                depot = readDepot(line);
            }
            else if(line.find("/*The Locations (x_coor y_coor name)*/") != std::string::npos)
            {
                nodes = readNodes(file, dimension);
                nodes.insert(nodes.begin(), depot);
            }
        }

        return TSPD("name", "type", dimension, truckSpeed, droneSpeed, depot, nodes, calculateEdges(nodes));
    }
}
