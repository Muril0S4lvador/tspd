#include "instance_reader.hpp"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <limits>
#include <stdexcept>
#include <utility>

namespace tspd::instance_reader {
    string InstanceReader::trim(const string& value) {
        const size_t first = value.find_first_not_of(" \t\r\n");
        if (first == string::npos)
            return "";

        const size_t last = value.find_last_not_of(" \t\r\n");
        return value.substr(first, last - first + 1);
    }

    string InstanceReader::toUpper(const string& value) {
        string result = value;
        for (char& character : result) {
            character = static_cast<char>(
                std::toupper(static_cast<unsigned char>(character)));
        }
        return result;
    }

    bool InstanceReader::isSectionMarker(const string& line) {
        const string marker = toUpper(trim(line));
        if (marker == "EOF")
            return true;

        const string suffix = "_SECTION";
        if (marker.size() <= suffix.size() ||
            marker.compare(marker.size() - suffix.size(), suffix.size(), suffix) != 0)
            return false;

        for (const char character : marker) {
            if (!(character == '_' || character == '-' ||
                  (character >= 'A' && character <= 'Z') ||
                  (character >= '0' && character <= '9'))) {
                return false;
            }
        }
        return true;
    }

    bool InstanceReader::parseHeaderLine(const string& line,
                                         string& key,
                                         string& value) {
        const string cleanedLine = trim(line);
        if (cleanedLine.empty() || isSectionMarker(cleanedLine))
            return false;

        stringstream stream(cleanedLine);
        string rawKey;
        stream >> rawKey;
        if (rawKey.empty())
            return false;

        if (!rawKey.empty() && rawKey.back() == ':')
            rawKey.pop_back();

        const string normalizedKey = toUpper(rawKey);
        if (normalizedKey != "NAME" && normalizedKey != "COMMENT" &&
            normalizedKey != "TYPE" && normalizedKey != "DIMENSION" &&
            normalizedKey != "EDGE_WEIGHT_TYPE" &&
            normalizedKey != "EDGE_WEIGHT_FORMAT")
            return false;

        const size_t colon = cleanedLine.find(':');
        if (colon != string::npos) {
            value = trim(cleanedLine.substr(colon + 1));
        } else {
            string remainder;
            std::getline(stream, remainder);
            value = trim(remainder);
        }

        key = normalizedKey;
        return true;
    }

    string InstanceReader::getValue(const string& line) {
        string key;
        string value;
        return parseHeaderLine(line, key, value) ? value : "";
    }

    int InstanceReader::parseDimension(const string& line) {
        const string value = getValue(line);
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

    node::Node InstanceReader::parseNode(const string& line) {
        stringstream streamLine(line);
        int id = 0;
        double x = 0.0;
        double y = 0.0;

        if (!(streamLine >> id >> x >> y))
            throw std::invalid_argument("Invalid coordinate line: " + trim(line));

        return node::Node(id, x, y);
    }

    void InstanceReader::readCoordinateSection(const vector<string>& lines,
                                               size_t& lineIndex,
                                               vector<node::Node>& nodes) {
        ++lineIndex;

        while (lineIndex < lines.size()) {
            const string line = trim(lines[lineIndex]);
            if (isSectionMarker(line))
                return;

            if (!line.empty())
                nodes.push_back(parseNode(line));

            ++lineIndex;
        }
    }

    void InstanceReader::readEdgeWeightSection(const vector<string>& lines,
                                               size_t& lineIndex,
                                               vector<double>& edgeWeights,
                                               const string& edgeWeightFormat,
                                               int dimension) {
        const size_t nodeCount = static_cast<size_t>(dimension);
        const size_t compactSize = nodeCount * (nodeCount - 1) / 2;
        edgeWeights.assign(compactSize, 0.0);

        const string format = toUpper(trim(edgeWeightFormat));
        size_t row = 0;
        size_t column = 0;
        size_t expectedWeights = 0;

        if (format == "FULL_MATRIX") {
            column = 0;
            expectedWeights = nodeCount * nodeCount;
        } else if (format == "UPPER_ROW") {
            column = nodeCount > 1 ? 1 : 0;
            expectedWeights = nodeCount * (nodeCount - 1) / 2;
        } else if (format == "LOWER_ROW") {
            row = nodeCount > 1 ? 1 : 0;
            column = 0;
            expectedWeights = nodeCount * (nodeCount - 1) / 2;
        } else if (format == "UPPER_DIAG_ROW") {
            expectedWeights = nodeCount * (nodeCount + 1) / 2;
        } else if (format == "LOWER_DIAG_ROW") {
            expectedWeights = nodeCount * (nodeCount + 1) / 2;
        } else if (format == "UPPER_COL") {
            column = nodeCount > 1 ? 1 : 0;
            expectedWeights = nodeCount * (nodeCount - 1) / 2;
        } else if (format == "LOWER_COL") {
            row = nodeCount > 1 ? 1 : 0;
            expectedWeights = nodeCount * (nodeCount - 1) / 2;
        } else if (format == "UPPER_DIAG_COL" || format == "LOWER_DIAG_COL") {
            expectedWeights = nodeCount * (nodeCount + 1) / 2;
        } else {
            throw std::invalid_argument(
                "Unsupported EDGE_WEIGHT_FORMAT: " + edgeWeightFormat);
        }

        size_t consumedWeights = 0;
        const auto compactIndex = [nodeCount](size_t first, size_t second) {
            if (first > second)
                std::swap(first, second);
            return first * (2 * nodeCount - first - 1) / 2 +
                   second - first - 1;
        };

        const auto advancePosition = [&]() {
            if (format == "FULL_MATRIX") {
                ++column;
                if (column == nodeCount) {
                    column = 0;
                    ++row;
                }
            } else if (format == "UPPER_ROW") {
                ++column;
                if (column == nodeCount) {
                    ++row;
                    column = row + 1;
                }
            } else if (format == "LOWER_ROW") {
                ++column;
                if (column == row) {
                    ++row;
                    column = 0;
                }
            } else if (format == "UPPER_DIAG_ROW") {
                ++column;
                if (column == nodeCount) {
                    ++row;
                    column = row;
                }
            } else if (format == "LOWER_DIAG_ROW") {
                ++column;
                if (column > row) {
                    ++row;
                    column = 0;
                }
            } else if (format == "UPPER_COL") {
                ++row;
                if (row == column) {
                    ++column;
                    row = 0;
                }
            } else if (format == "LOWER_COL") {
                ++row;
                if (row == nodeCount) {
                    ++column;
                    row = column + 1;
                }
            } else if (format == "UPPER_DIAG_COL") {
                ++row;
                if (row > column) {
                    ++column;
                    row = 0;
                }
            } else if (format == "LOWER_DIAG_COL") {
                ++row;
                if (row == nodeCount) {
                    ++column;
                    row = column;
                }
            }
        };

        const auto consumeWeight = [&](double weight) {
            if (consumedWeights >= expectedWeights)
                throw std::invalid_argument(
                    "EDGE_WEIGHT_SECTION has more values than expected");

            if (row != column)
                edgeWeights[compactIndex(row, column)] = weight;

            ++consumedWeights;
            advancePosition();
        };

        ++lineIndex;

        while (lineIndex < lines.size()) {
            const string line = trim(lines[lineIndex]);
            if (isSectionMarker(line))
                return;

            if (!line.empty()) {
                stringstream streamLine(line);
                double weight = 0.0;
                while (streamLine >> weight) {
                    if (!std::isfinite(weight) || weight < 0.0)
                        throw std::invalid_argument(
                            "Invalid edge weight: " + trim(line));
                    consumeWeight(weight);
                }

                if (!streamLine.eof())
                    throw std::invalid_argument(
                        "Invalid edge weight line: " + trim(line));
            }

            ++lineIndex;
        }

        if (consumedWeights != expectedWeights)
            throw std::invalid_argument(
                "EDGE_WEIGHT_SECTION has fewer values than expected");
    }

    int InstanceReader::convertDistanceToInteger(double distance) {
        const double roundedDistance = std::ceil(distance);
        if (roundedDistance > std::numeric_limits<int>::max() ||
            roundedDistance < std::numeric_limits<int>::min()) {
            throw std::invalid_argument("Distance does not fit in int");
        }

        return static_cast<int>(roundedDistance);
    }

    InstanceReader::CalculatedData InstanceReader::calculateCoordinates(
        const vector<double>& compactEdgeWeights,
        int dimension) {
        if (dimension <= 0)
            return {};

        const size_t nodeCount = static_cast<size_t>(dimension);
        const size_t compactSize = nodeCount * (nodeCount - 1) / 2;
        vector<int> distances(compactSize, 0);
        if (compactEdgeWeights.size() != compactSize)
            throw std::invalid_argument(
                "Compact EDGE_WEIGHT_SECTION size differs from DIMENSION");

        for (size_t index = 0; index < compactSize; ++index)
            distances[index] = convertDistanceToInteger(compactEdgeWeights[index]);

        CalculatedData result;
        result.distances = std::move(distances);
        result.nodes = calculateNodesFromDistances(result.distances, dimension);
        return result;
    }

    double InstanceReader::calculateRawDistance(
        const node::Node& firstNode,
        const node::Node& secondNode,
        const string& edgeWeightType) {
        const string type = toUpper(trim(edgeWeightType));
        const double firstX = firstNode.getX();
        const double firstY = firstNode.getY();
        const double secondX = secondNode.getX();
        const double secondY = secondNode.getY();

        if (type == "GEO") {
            constexpr double pi = 3.14159265358979323846;
            constexpr double earthRadius = 6378.388;
            const auto geoToRadians = [pi](double coordinate) {
                const double degrees = std::trunc(coordinate);
                const double minutes = coordinate - degrees;
                return pi * (degrees + 5.0 * minutes / 3.0) / 180.0;
            };

            const double latitude1 = geoToRadians(firstX);
            const double longitude1 = geoToRadians(firstY);
            const double latitude2 = geoToRadians(secondX);
            const double longitude2 = geoToRadians(secondY);
            const double cosine =
                std::sin(latitude1) * std::sin(latitude2) +
                std::cos(latitude1) * std::cos(latitude2) *
                    std::cos(longitude1 - longitude2);

            return earthRadius * std::acos(std::clamp(cosine, -1.0, 1.0));
        }

        const double deltaX = firstX - secondX;
        const double deltaY = firstY - secondY;
        const double squaredDistance = deltaX * deltaX + deltaY * deltaY;

        if (type == "ATT")
            return std::sqrt(squaredDistance / 10.0);

        if (type == "EUC_2D" || type == "CEIL_2D")
            return std::sqrt(squaredDistance);

        throw std::invalid_argument(
            "Cannot calculate distances for EDGE_WEIGHT_TYPE: " + edgeWeightType);
    }

    vector<int> InstanceReader::calculateDistances(
        const vector<node::Node>& nodes,
        const string& edgeWeightType) {
        const size_t nodeCount = nodes.size();
        vector<int> distances(nodeCount * (nodeCount - 1) / 2, 0);

        const auto compactIndex = [nodeCount](size_t row, size_t column) {
            if (row > column)
                std::swap(row, column);

            return row * (2 * nodeCount - row - 1) / 2 + column - row - 1;
        };

        for (size_t first = 0; first < nodeCount; ++first) {
            for (size_t second = first + 1; second < nodeCount; ++second) {
                distances[compactIndex(first, second)] = convertDistanceToInteger(
                    calculateRawDistance(nodes[first], nodes[second], edgeWeightType));
            }
        }

        return distances;
    }

    int InstanceReader::getCompactDistance(const vector<int>& distances,
                                           int dimension,
                                           size_t firstNode,
                                           size_t secondNode) {
        if (firstNode == secondNode)
            return 0;

        const size_t nodeCount = static_cast<size_t>(dimension);
        if (firstNode >= nodeCount || secondNode >= nodeCount)
            throw std::invalid_argument("Node index outside distance matrix");

        if (firstNode > secondNode)
            std::swap(firstNode, secondNode);

        const size_t index =
            firstNode * (2 * nodeCount - firstNode - 1) / 2 +
            secondNode - firstNode - 1;
        return distances.at(index);
    }

    vector<node::Node> InstanceReader::calculateNodesFromDistances(
        const vector<int>& distances,
        int dimension) {
        if (dimension <= 0)
            return {};

        const size_t nodeCount = static_cast<size_t>(dimension);
        vector<node::Node> nodes;
        nodes.reserve(nodeCount);
        if (nodeCount == 1) {
            nodes.emplace_back(1, 0.0, 0.0);
            return nodes;
        }

        const auto squared = [](double value) {
            return value * value;
        };
        const double epsilon = std::numeric_limits<double>::epsilon();
        const size_t firstAnchor = 0;

        size_t secondAnchor = 1;
        for (size_t index = 2; index < nodeCount; ++index) {
            if (getCompactDistance(distances, dimension, firstAnchor, index) >
                getCompactDistance(distances, dimension, firstAnchor, secondAnchor)) {
                secondAnchor = index;
            }
        }

        const double firstSecondDistance =
            getCompactDistance(distances, dimension, firstAnchor, secondAnchor);
        if (firstSecondDistance <= epsilon) {
            for (size_t index = 0; index < nodeCount; ++index)
                nodes.emplace_back(static_cast<int>(index + 1), 0.0, 0.0);
            return nodes;
        }

        const auto xFromTwoAnchors = [&](size_t index) {
            const double firstDistance =
                getCompactDistance(distances, dimension, firstAnchor, index);
            const double secondDistance =
                getCompactDistance(distances, dimension, secondAnchor, index);
            return (squared(firstDistance) + squared(firstSecondDistance) -
                    squared(secondDistance)) /
                   (2.0 * firstSecondDistance);
        };

        size_t thirdAnchor = firstAnchor;
        double thirdAnchorHeightSquared = 0.0;
        for (size_t index = 0; index < nodeCount; ++index) {
            if (index == firstAnchor || index == secondAnchor)
                continue;

            const double x = xFromTwoAnchors(index);
            const double firstDistance =
                getCompactDistance(distances, dimension, firstAnchor, index);
            const double heightSquared = squared(firstDistance) - squared(x);
            if (thirdAnchor == firstAnchor || heightSquared > thirdAnchorHeightSquared) {
                thirdAnchor = index;
                thirdAnchorHeightSquared = heightSquared;
            }
        }

        const double thirdAnchorX =
            thirdAnchor == firstAnchor ? 0.0 : xFromTwoAnchors(thirdAnchor);
        const double thirdAnchorY =
            std::sqrt(std::max(0.0, thirdAnchorHeightSquared));

        for (size_t index = 0; index < nodeCount; ++index) {
            const double x = xFromTwoAnchors(index);
            double y = 0.0;

            if (thirdAnchor != firstAnchor && thirdAnchorY > epsilon) {
                const double firstDistance =
                    getCompactDistance(distances, dimension, firstAnchor, index);
                const double thirdDistance =
                    getCompactDistance(distances, dimension, thirdAnchor, index);
                const double rightSide =
                    squared(firstDistance) +
                    squared(thirdAnchorX) +
                    squared(thirdAnchorY) -
                    squared(thirdDistance);
                y = (rightSide / 2.0 - x * thirdAnchorX) / thirdAnchorY;
            }

            nodes.emplace_back(static_cast<int>(index + 1), x, y);
        }

        return nodes;
    }

    TSPD InstanceReader::readInstance(const string& filePath) {
        ifstream file(filePath);
        if (!file.is_open())
            throw std::invalid_argument("Fail to open " + filePath);

        vector<string> lines;
        string line;
        while (std::getline(file, line))
            lines.push_back(line);
        file.close();

        string name = "";
        string comment = "";
        string type = "";
        string edgeWeightType = "";
        string edgeWeightFormat = "";
        int dimension = 0;
        vector<node::Node> coordinateNodes;
        vector<node::Node> displayNodes;
        vector<double> edgeWeights;
        vector<int> distances;
        bool hasCoordinateSection = false;
        bool hasDisplaySection = false;

        for (size_t lineIndex = 0; lineIndex < lines.size();) {
            const string currentLine = trim(lines[lineIndex]);
            const string marker = toUpper(currentLine);

            if (marker == "EOF")
                break;

            if (marker == "NODE_COORD_SECTION") {
                hasCoordinateSection = true;
                readCoordinateSection(lines, lineIndex, coordinateNodes);
                continue;
            }

            if (marker == "DISPLAY_DATA_SECTION") {
                hasDisplaySection = true;
                readCoordinateSection(lines, lineIndex, displayNodes);
                continue;
            }

            if (marker == "EDGE_WEIGHT_SECTION") {
                readEdgeWeightSection(lines, lineIndex, edgeWeights,
                                      edgeWeightFormat, dimension);
                continue;
            }

            if (isSectionMarker(currentLine)) {
                ++lineIndex;
                while (lineIndex < lines.size() &&
                       !isSectionMarker(lines[lineIndex])) {
                    ++lineIndex;
                }
                continue;
            }

            string key;
            string value;
            if (parseHeaderLine(currentLine, key, value)) {
                if (key == "NAME") {
                    name = value;
                } else if (key == "COMMENT") {
                    if (!comment.empty())
                        comment += "\t";
                    comment += value;
                } else if (key == "DIMENSION") {
                    dimension = parseDimension(currentLine);
                } else if (key == "EDGE_WEIGHT_TYPE") {
                    edgeWeightType = value;
                } else if (key == "EDGE_WEIGHT_FORMAT") {
                    edgeWeightFormat = value;
                } else if (key == "TYPE") {
                    type = value;
                    string typeToken;
                    stringstream typeStream(toUpper(value));
                    typeStream >> typeToken;
                    if (typeToken == "TOUR")
                        throw std::invalid_argument("File must not be a solution");
                }
            }

            ++lineIndex;
        }

        if (dimension == 0)
            throw std::invalid_argument("Missing DIMENSION in " + filePath);

        if (hasCoordinateSection &&
            static_cast<int>(coordinateNodes.size()) != dimension) {
            throw std::invalid_argument(
                "NODE_COORD_SECTION size differs from DIMENSION in " + filePath);
        }

        const string normalizedEdgeWeightType = toUpper(trim(edgeWeightType));
        if (normalizedEdgeWeightType == "EXPLICIT") {
            const CalculatedData calculated = calculateCoordinates(
                edgeWeights, dimension);
            coordinateNodes = calculated.nodes;
            distances = calculated.distances;
        } else {
            if (!hasCoordinateSection && hasDisplaySection &&
                static_cast<int>(displayNodes.size()) == dimension) {
                coordinateNodes = std::move(displayNodes);
            } else if (!hasCoordinateSection && hasDisplaySection) {
                throw std::invalid_argument(
                    "DISPLAY_DATA_SECTION size differs from DIMENSION in " + filePath);
            }

            distances = calculateDistances(coordinateNodes, edgeWeightType);
        }

        return TSPD(name, comment, type, dimension, edgeWeightType,
                    coordinateNodes, distances);
    }
}
