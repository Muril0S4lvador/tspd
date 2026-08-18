#include "instance_reader.hpp"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <stdexcept>

namespace tspd::instance_reader {
    string InstanceReader::_trim(const string& value) {
        const size_t first = value.find_first_not_of(" \t\r\n");
        if (first == string::npos) return "";
        const size_t last = value.find_last_not_of(" \t\r\n");
        return value.substr(first, last - first + 1);
    }

    string InstanceReader::_toUpper(const string& value) {
        string result = value;
        for (char& character : result)
            character = static_cast<char>(std::toupper(static_cast<unsigned char>(character)));
        return result;
    }

    bool InstanceReader::_isSectionMarker(const string& line) {
        const string marker = _toUpper(_trim(line));
        if (marker == "EOF") return true;

        const string suffix = "_SECTION";
        if (marker.size() <= suffix.size() ||
            marker.compare(marker.size() - suffix.size(), suffix.size(), suffix) != 0) {
            return false;
        }

        for (const char character : marker) {
            if (!(character == '_' || character == '-' ||
                  (character >= 'A' && character <= 'Z') ||
                  (character >= '0' && character <= '9'))) return false;
        }
        return true;
    }

    bool InstanceReader::_parseHeaderLine(const string& line, string& key, string& value) {
        const string cleanedLine = _trim(line);
        if (cleanedLine.empty() || _isSectionMarker(cleanedLine)) return false;

        stringstream stream(cleanedLine);
        string rawKey;
        stream >> rawKey;
        if (rawKey.empty()) return false;
        if (rawKey.back() == ':') rawKey.pop_back();

        const string normalizedKey = _toUpper(rawKey);
        if (normalizedKey != "NAME" && normalizedKey != "COMMENT" &&
            normalizedKey != "TYPE" && normalizedKey != "DIMENSION" &&
            normalizedKey != "EDGE_WEIGHT_TYPE" && normalizedKey != "EDGE_WEIGHT_FORMAT") {
            return false;
        }

        const size_t colon = cleanedLine.find(':');
        if (colon != string::npos) {
            value = _trim(cleanedLine.substr(colon + 1));
        } else {
            string remainder;
            std::getline(stream, remainder);
            value = _trim(remainder);
        }
        key = normalizedKey;
        return true;
    }

    string InstanceReader::_getValue(const string& line) {
        string key;
        string value;
        return _parseHeaderLine(line, key, value) ? value : "";
    }

    int InstanceReader::_parseDimension(const string& line) {
        const string value = _getValue(line);
        size_t parsedCharacters = 0;
        int dimension = 0;
        try {
            dimension = std::stoi(value, &parsedCharacters);
        } catch (const std::exception&) {
            throw std::invalid_argument("Invalid DIMENSION value: " + value);
        }
        if (parsedCharacters != value.size() || dimension <= 0)
            throw std::invalid_argument("Invalid DIMENSION value: " + value);
        return dimension;
    }

    node::Node InstanceReader::_parseNode(const string& line) {
        stringstream streamLine(line);
        int id = 0;
        double x = 0.0;
        double y = 0.0;
        if (!(streamLine >> id >> x >> y))
            throw std::invalid_argument("Invalid coordinate line: " + _trim(line));
        return node::Node(id, x, y);
    }

    void InstanceReader::_readCoordinateSection(const vector<string>& lines,
                                               size_t& lineIndex,
                                               vector<node::Node>& nodes) {
        ++lineIndex;
        while (lineIndex < lines.size()) {
            const string line = _trim(lines[lineIndex]);
            if (_isSectionMarker(line)) return;
            if (!line.empty()) nodes.push_back(_parseNode(line));
            ++lineIndex;
        }
    }

    void InstanceReader::_readEdgeWeightSection(const vector<string>& lines,
                                               size_t& lineIndex,
                                               vector<Edge>& edges,
                                               const string& edgeWeightFormat,
                                               int dimension) {
        const size_t nodeCount = static_cast<size_t>(dimension);
        const size_t compactSize = nodeCount * (nodeCount - 1) / 2;
        edges.clear();
        edges.reserve(compactSize);
        for (int firstNodeId = 1; firstNodeId <= dimension; ++firstNodeId) {
            for (int secondNodeId = firstNodeId + 1; secondNodeId <= dimension; ++secondNodeId)
                edges.emplace_back(firstNodeId, secondNodeId, 0);
        }

        const string format = _toUpper(_trim(edgeWeightFormat));
        size_t row = 0;
        size_t column = 0;
        size_t expectedWeights = 0;

        if (format == "FULL_MATRIX") {
            expectedWeights = nodeCount * nodeCount;
        } else if (format == "UPPER_ROW") {
            column = nodeCount > 1 ? 1 : 0;
            expectedWeights = compactSize;
        } else if (format == "LOWER_ROW") {
            row = nodeCount > 1 ? 1 : 0;
            expectedWeights = compactSize;
        } else if (format == "UPPER_DIAG_ROW" || format == "LOWER_DIAG_ROW" ||
                   format == "UPPER_DIAG_COL" || format == "LOWER_DIAG_COL") {
            expectedWeights = nodeCount * (nodeCount + 1) / 2;
        } else if (format == "UPPER_COL") {
            column = nodeCount > 1 ? 1 : 0;
            expectedWeights = compactSize;
        } else if (format == "LOWER_COL") {
            row = nodeCount > 1 ? 1 : 0;
            expectedWeights = compactSize;
        } else {
            throw std::invalid_argument("Unsupported EDGE_WEIGHT_FORMAT: " + edgeWeightFormat);
        }

        const auto advancePosition = [&]() {
            if (format == "FULL_MATRIX") {
                if (++column == nodeCount) { column = 0; ++row; }
            } else if (format == "UPPER_ROW") {
                if (++column == nodeCount) { ++row; column = row + 1; }
            } else if (format == "LOWER_ROW") {
                if (++column == row) { ++row; column = 0; }
            } else if (format == "UPPER_DIAG_ROW") {
                if (++column == nodeCount) { ++row; column = row; }
            } else if (format == "LOWER_DIAG_ROW") {
                if (++column > row) { ++row; column = 0; }
            } else if (format == "UPPER_COL") {
                if (++row == column) { ++column; row = 0; }
            } else if (format == "LOWER_COL") {
                if (++row == nodeCount) { ++column; row = column + 1; }
            } else if (format == "UPPER_DIAG_COL") {
                if (++row > column) { ++column; row = 0; }
            } else if (format == "LOWER_DIAG_COL") {
                if (++row == nodeCount) { ++column; row = column; }
            }
        };

        size_t consumedWeights = 0;
        const auto consumeWeight = [&](double weight) {
            if (!std::isfinite(weight) || weight < 0.0)
                throw std::invalid_argument("Invalid edge weight");
            if (consumedWeights >= expectedWeights)
                throw std::invalid_argument("EDGE_WEIGHT_SECTION has more values than expected");

            // Em FULL_MATRIX, a metade inferior e a diagonal são descartadas imediatamente.
            const bool retainWeight = row != column &&
                (format != "FULL_MATRIX" || row < column);
            if (retainWeight) {
                const int firstNodeId = static_cast<int>(row + 1);
                const int secondNodeId = static_cast<int>(column + 1);
                edges[Edge::compactIndex(dimension, firstNodeId, secondNodeId)] =
                    Edge(firstNodeId, secondNodeId, Edge::convertWeight(weight));
            }
            ++consumedWeights;
            advancePosition();
        };

        ++lineIndex;
        while (lineIndex < lines.size()) {
            const string line = _trim(lines[lineIndex]);
            if (_isSectionMarker(line)) break;
            if (!line.empty()) {
                stringstream streamLine(line);
                double weight = 0.0;
                while (streamLine >> weight) consumeWeight(weight);
                if (!streamLine.eof())
                    throw std::invalid_argument("Invalid edge weight line: " + line);
            }
            ++lineIndex;
        }

        if (consumedWeights != expectedWeights)
            throw std::invalid_argument("EDGE_WEIGHT_SECTION has fewer values than expected");
    }

    vector<Edge> InstanceReader::_readExplicitEdges(const vector<Edge>& edges, int dimension) {
        const size_t nodeCount = static_cast<size_t>(dimension);
        if (edges.size() != nodeCount * (nodeCount - 1) / 2)
            throw std::invalid_argument("Compact EDGE_WEIGHT_SECTION size differs from DIMENSION");
        return edges;
    }

    double InstanceReader::_calculateRawDistance(const node::Node& firstNode,
                                                const node::Node& secondNode,
                                                const string& edgeWeightType) {
        const string type = _toUpper(_trim(edgeWeightType));
        if (type == "GEO") {
            constexpr double pi = 3.14159265358979323846;
            constexpr double earthRadius = 6378.388;
            const auto geoToRadians = [pi](double coordinate) {
                const double degrees = std::trunc(coordinate);
                return pi * (degrees + 5.0 * (coordinate - degrees) / 3.0) / 180.0;
            };
            const double latitude1 = geoToRadians(firstNode.getX());
            const double longitude1 = geoToRadians(firstNode.getY());
            const double latitude2 = geoToRadians(secondNode.getX());
            const double longitude2 = geoToRadians(secondNode.getY());
            const double cosine = std::sin(latitude1) * std::sin(latitude2) +
                std::cos(latitude1) * std::cos(latitude2) * std::cos(longitude1 - longitude2);
            return earthRadius * std::acos(std::clamp(cosine, -1.0, 1.0));
        }

        const double deltaX = firstNode.getX() - secondNode.getX();
        const double deltaY = firstNode.getY() - secondNode.getY();
        const double squaredDistance = deltaX * deltaX + deltaY * deltaY;
        if (type == "ATT") return std::sqrt(squaredDistance / 10.0);
        if (type == "EUC_2D" || type == "CEIL_2D") return std::sqrt(squaredDistance);
        throw std::invalid_argument("Cannot calculate distances for EDGE_WEIGHT_TYPE: " + edgeWeightType);
    }

    vector<Edge> InstanceReader::_calculateEdges(const vector<node::Node>& nodes,
                                                const string& edgeWeightType) {
        const size_t nodeCount = nodes.size();
        vector<Edge> edges;
        edges.reserve(nodeCount * (nodeCount - 1) / 2);
        for (size_t first = 0; first < nodeCount; ++first) {
            for (size_t second = first + 1; second < nodeCount; ++second) {
                edges.emplace_back(static_cast<int>(first + 1), static_cast<int>(second + 1),
                    Edge::convertWeight(_calculateRawDistance(nodes[first], nodes[second], edgeWeightType)));
            }
        }
        return edges;
    }

    TSPD InstanceReader::readInstance(const string& filePath) {
        ifstream file(filePath);
        if (!file.is_open()) throw std::invalid_argument("Fail to open " + filePath);

        vector<string> lines;
        string line;
        while (std::getline(file, line)) lines.push_back(line);

        string name;
        string comment;
        string type;
        string edgeWeightType;
        string edgeWeightFormat;
        int dimension = 0;
        vector<node::Node> coordinateNodes;
        vector<node::Node> displayNodes;
        vector<Edge> edges;
        bool hasCoordinateSection = false;
        bool hasDisplaySection = false;

        for (size_t lineIndex = 0; lineIndex < lines.size();) {
            const string currentLine = _trim(lines[lineIndex]);
            const string marker = _toUpper(currentLine);
            if (marker == "EOF") break;
            if (marker == "NODE_COORD_SECTION") {
                hasCoordinateSection = true;
                _readCoordinateSection(lines, lineIndex, coordinateNodes);
                continue;
            }
            if (marker == "DISPLAY_DATA_SECTION") {
                hasDisplaySection = true;
                _readCoordinateSection(lines, lineIndex, displayNodes);
                continue;
            }
            if (marker == "EDGE_WEIGHT_SECTION") {
                _readEdgeWeightSection(lines, lineIndex, edges, edgeWeightFormat, dimension);
                continue;
            }
            if (_isSectionMarker(currentLine)) {
                ++lineIndex;
                while (lineIndex < lines.size() && !_isSectionMarker(lines[lineIndex])) ++lineIndex;
                continue;
            }

            string key;
            string value;
            if (_parseHeaderLine(currentLine, key, value)) {
                if (key == "NAME") name = value;
                else if (key == "COMMENT") {
                    if (!comment.empty()) comment += "\t";
                    comment += value;
                } else if (key == "DIMENSION") dimension = _parseDimension(currentLine);
                else if (key == "EDGE_WEIGHT_TYPE") edgeWeightType = value;
                else if (key == "EDGE_WEIGHT_FORMAT") edgeWeightFormat = value;
                else if (key == "TYPE") {
                    type = value;
                    string typeToken;
                    stringstream typeStream(_toUpper(value));
                    typeStream >> typeToken;
                    if (typeToken == "TOUR")
                        throw std::invalid_argument("File must not be a solution");
                }
            }
            ++lineIndex;
        }

        if (dimension == 0) throw std::invalid_argument("Missing DIMENSION in " + filePath);
        if (hasCoordinateSection && static_cast<int>(coordinateNodes.size()) != dimension)
            throw std::invalid_argument("NODE_COORD_SECTION size differs from DIMENSION in " + filePath);

        if (_toUpper(_trim(edgeWeightType)) == "EXPLICIT") {
            edges = _readExplicitEdges(edges, dimension);
        } else {
            if (!hasCoordinateSection && hasDisplaySection &&
                static_cast<int>(displayNodes.size()) == dimension) {
                coordinateNodes = std::move(displayNodes);
            } else if (!hasCoordinateSection && hasDisplaySection) {
                throw std::invalid_argument("DISPLAY_DATA_SECTION size differs from DIMENSION in " + filePath);
            }
            edges = _calculateEdges(coordinateNodes, edgeWeightType);
        }

        return TSPD(name, comment, type, dimension, edgeWeightType, coordinateNodes, edges);
    }
}
