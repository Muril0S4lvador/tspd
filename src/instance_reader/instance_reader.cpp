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
                                               vector<double>& edgeWeights) {
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
                    edgeWeights.push_back(weight);
                }

                if (!streamLine.eof())
                    throw std::invalid_argument(
                        "Invalid edge weight line: " + trim(line));
            }

            ++lineIndex;
        }
    }

    vector<node::Node> InstanceReader::calculateCoordinates(
        const vector<double>& edgeWeights,
        const string& edgeWeightFormat,
        int dimension) {
        if (dimension <= 0)
            return {};

        const size_t nodeCount = static_cast<size_t>(dimension);
        vector<double> distances(nodeCount * nodeCount, 0.0);
        size_t weightIndex = 0;

        const auto nextWeight = [&]() {
            if (weightIndex >= edgeWeights.size())
                throw std::invalid_argument(
                    "EDGE_WEIGHT_SECTION has fewer values than DIMENSION requires");
            return edgeWeights[weightIndex++];
        };

        const auto setDistance = [&](size_t row, size_t column, double distance) {
            distances[row * nodeCount + column] = distance;
            distances[column * nodeCount + row] = distance;
        };

        const string format = toUpper(trim(edgeWeightFormat));
        if (format == "FULL_MATRIX") {
            for (size_t row = 0; row < nodeCount; ++row) {
                for (size_t column = 0; column < nodeCount; ++column)
                    setDistance(row, column, nextWeight());
            }
        } else if (format == "UPPER_ROW") {
            for (size_t row = 0; row + 1 < nodeCount; ++row) {
                for (size_t column = row + 1; column < nodeCount; ++column)
                    setDistance(row, column, nextWeight());
            }
        } else if (format == "LOWER_ROW") {
            for (size_t row = 1; row < nodeCount; ++row) {
                for (size_t column = 0; column < row; ++column)
                    setDistance(row, column, nextWeight());
            }
        } else if (format == "UPPER_DIAG_ROW") {
            for (size_t row = 0; row < nodeCount; ++row) {
                for (size_t column = row; column < nodeCount; ++column)
                    setDistance(row, column, nextWeight());
            }
        } else if (format == "LOWER_DIAG_ROW") {
            for (size_t row = 0; row < nodeCount; ++row) {
                for (size_t column = 0; column <= row; ++column)
                    setDistance(row, column, nextWeight());
            }
        } else if (format == "UPPER_COL") {
            for (size_t column = 1; column < nodeCount; ++column) {
                for (size_t row = 0; row < column; ++row)
                    setDistance(row, column, nextWeight());
            }
        } else if (format == "LOWER_COL") {
            for (size_t column = 0; column + 1 < nodeCount; ++column) {
                for (size_t row = column + 1; row < nodeCount; ++row)
                    setDistance(row, column, nextWeight());
            }
        } else if (format == "UPPER_DIAG_COL") {
            for (size_t column = 0; column < nodeCount; ++column) {
                for (size_t row = 0; row <= column; ++row)
                    setDistance(row, column, nextWeight());
            }
        } else if (format == "LOWER_DIAG_COL") {
            for (size_t column = 0; column < nodeCount; ++column) {
                for (size_t row = column; row < nodeCount; ++row)
                    setDistance(row, column, nextWeight());
            }
        } else {
            throw std::invalid_argument(
                "Unsupported EDGE_WEIGHT_FORMAT: " + edgeWeightFormat);
        }

        if (weightIndex != edgeWeights.size())
            throw std::invalid_argument(
                "EDGE_WEIGHT_SECTION has more values than DIMENSION requires");

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
            if (distances[firstAnchor * nodeCount + index] >
                distances[firstAnchor * nodeCount + secondAnchor]) {
                secondAnchor = index;
            }
        }

        const double firstSecondDistance =
            distances[firstAnchor * nodeCount + secondAnchor];
        if (firstSecondDistance <= epsilon) {
            for (size_t index = 0; index < nodeCount; ++index)
                nodes.emplace_back(static_cast<int>(index + 1), 0.0, 0.0);
            return nodes;
        }

        const auto xFromTwoAnchors = [&](size_t index) {
            const double firstDistance =
                distances[firstAnchor * nodeCount + index];
            const double secondDistance =
                distances[secondAnchor * nodeCount + index];
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
                distances[firstAnchor * nodeCount + index];
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
                    distances[firstAnchor * nodeCount + index];
                const double thirdDistance =
                    distances[thirdAnchor * nodeCount + index];
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
                readEdgeWeightSection(lines, lineIndex, edgeWeights);
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

        if (!hasCoordinateSection && hasDisplaySection &&
            static_cast<int>(displayNodes.size()) == dimension) {
            coordinateNodes = std::move(displayNodes);
        } else if (!hasCoordinateSection && hasDisplaySection) {
            throw std::invalid_argument(
                "DISPLAY_DATA_SECTION size differs from DIMENSION in " + filePath);
        }

        if (coordinateNodes.empty() &&
            toUpper(trim(edgeWeightType)) == "EXPLICIT") {
            coordinateNodes = calculateCoordinates(
                edgeWeights, edgeWeightFormat, dimension);
        }

        return TSPD(name, comment, type, dimension, edgeWeightType, coordinateNodes);
    }
}
