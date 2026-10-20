#include "solution_reader.hpp"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <limits>
#include <locale>
#include <regex>
#include <sstream>
#include <stdexcept>
#include <unordered_set>
#include <utility>
#include <vector>

namespace tspd::solution_reader {
    namespace {
        struct SourceLine {
            std::size_t number;
            std::string text;
        };

        struct BlockComment {
            std::size_t firstLine;
            std::string text;
        };

        struct Operation {
            int start;
            int end;
            int fly;
            std::vector<int> internalNodes;
            std::size_t line;
        };

        [[noreturn]] void fail(const std::filesystem::path& path,
                               std::size_t line,
                               const std::string& message) {
            throw std::invalid_argument("Solution file '" + path.string() +
                "', line " + std::to_string(line) + ": " + message);
        }

        std::string trim(const std::string& text) {
            const auto first = std::find_if_not(text.begin(), text.end(), [](unsigned char c) {
                return std::isspace(c) != 0;
            });
            const auto last = std::find_if_not(text.rbegin(), text.rend(), [](unsigned char c) {
                return std::isspace(c) != 0;
            }).base();
            if (first >= last)
                return {};
            return std::string(first, last);
        }

        std::vector<SourceLine> removeComments(const std::filesystem::path& path,
                                               std::vector<BlockComment>& comments) {
            std::ifstream file(path);
            if (!file.is_open())
                fail(path, 1, "file exists but cannot be opened");

            std::vector<SourceLine> lines;
            std::string line;
            std::string currentComment;
            std::size_t lineNumber = 0;
            std::size_t commentStartLine = 0;
            bool insideBlockComment = false;

            while (std::getline(file, line)) {
                ++lineNumber;
                std::string code;
                for (std::size_t index = 0; index < line.size();) {
                    if (insideBlockComment) {
                        if (line[index] == '*' && index + 1 < line.size() &&
                            line[index + 1] == '/') {
                            comments.push_back({commentStartLine, std::move(currentComment)});
                            currentComment.clear();
                            insideBlockComment = false;
                            index += 2;
                        } else {
                            currentComment.push_back(line[index]);
                            ++index;
                        }
                    } else if (line[index] == '/' && index + 1 < line.size() &&
                               line[index + 1] == '/') {
                        break;
                    } else if (line[index] == '/' && index + 1 < line.size() &&
                               line[index + 1] == '*') {
                        insideBlockComment = true;
                        commentStartLine = lineNumber;
                        currentComment.clear();
                        code.push_back(' ');
                        index += 2;
                    } else {
                        code.push_back(line[index]);
                        ++index;
                    }
                }

                if (insideBlockComment)
                    currentComment.push_back('\n');
                lines.push_back({lineNumber, trim(code)});
            }

            if (insideBlockComment)
                fail(path, commentStartLine, "unterminated block comment");
            if (file.bad())
                fail(path, lineNumber + 1, "error while reading file");
            return lines;
        }

        std::size_t commentLine(const BlockComment& comment, std::size_t offset) {
            return comment.firstLine + static_cast<std::size_t>(std::count(
                comment.text.begin(), comment.text.begin() + static_cast<std::ptrdiff_t>(offset), '\n'));
        }

        std::optional<double> readTotalCost(const std::filesystem::path& path,
                             const std::vector<BlockComment>& comments) {
            static const std::regex labelPattern(R"(Total\s+cost)", std::regex_constants::icase);
            static const std::regex costPattern(
                R"(Total\s+cost\s*:\s*([+-]?(?:(?:[0-9]+(?:\.[0-9]*)?)|(?:\.[0-9]+))(?:[eE][+-]?[0-9]+)?))",
                std::regex_constants::icase);

            bool foundCost = false;
            double totalCost = 0.0;
            for (const auto& comment : comments) {
                for (std::sregex_iterator match(comment.text.begin(), comment.text.end(), labelPattern),
                     end; match != end; ++match) {
                    const std::size_t offset = static_cast<std::size_t>(match->position());
                    const std::string suffix = comment.text.substr(offset);
                    std::smatch costMatch;
                    if (!std::regex_search(suffix, costMatch, costPattern) ||
                        costMatch.position() != 0) {
                        fail(path, commentLine(comment, offset),
                             "malformed Total cost comment");
                    }

                    const std::string number = costMatch[1].str();
                    const std::string remainder = trim(suffix.substr(
                        static_cast<std::size_t>(costMatch.length())));
                    if (!remainder.empty()) {
                        fail(path, commentLine(comment, offset),
                             "malformed Total cost value");
                    }

                    if (foundCost)
                        fail(path, commentLine(comment, offset),
                             "more than one Total cost value was found");

                    std::istringstream costStream(number);
                    costStream.imbue(std::locale::classic());
                    costStream >> totalCost;
                    std::string extraValue;
                    if (!costStream || (costStream >> extraValue)) {
                        fail(path, commentLine(comment, offset),
                             "malformed Total cost value");
                    }
                    if (!std::isfinite(totalCost) ||
                        totalCost < 0.0) {
                        fail(path, commentLine(comment, offset),
                             "Total cost must be a finite nonnegative number");
                    }
                    foundCost = true;
                }
            }

            if (!foundCost)
                return std::nullopt;
            return totalCost;
        }

        int parseInteger(const std::string& token,
                         const std::filesystem::path& path,
                         std::size_t line,
                         const std::string& field) {
            std::size_t parsedLength = 0;
            long long value = 0;
            try {
                value = std::stoll(token, &parsedLength);
            } catch (const std::exception&) {
                fail(path, line, field + " must be an integer");
            }
            if (parsedLength != token.size() ||
                value < std::numeric_limits<int>::min() ||
                value > std::numeric_limits<int>::max()) {
                fail(path, line, field + " must be an integer in range");
            }
            return static_cast<int>(value);
        }

        std::vector<std::string> splitFields(const std::string& text) {
            std::istringstream stream(text);
            std::vector<std::string> fields;
            std::string field;
            while (stream >> field)
                fields.push_back(field);
            return fields;
        }

        void appendEdge(std::vector<::tspd::edge::Edge>& edges,
                        int first,
                        int second,
                        const ::tspd::tspd::TSPD& instance,
                        const std::filesystem::path& path,
                        std::size_t line,
                        const std::string& edgeType) {
            if (first == second)
                return;

            try {
                edges.emplace_back(first, second, instance.getDistance(first, second));
            } catch (const std::exception& error) {
                fail(path, line, "cannot build " + edgeType + " edge " +
                     std::to_string(first) + " -> " + std::to_string(second) +
                     ": " + error.what());
            }
        }
    }

    std::optional<::tspd::solution::LiteratureSolution> SolutionReader::readSolution(
        const std::string& instancePath,
        const ::tspd::tspd::TSPD& instance) {
        const std::filesystem::path instanceFile(instancePath);
        const std::filesystem::path solutionsDirectory = instanceFile.parent_path() / "solutions";
        const std::string stem = instanceFile.stem().string();
        const std::string extension = instanceFile.extension().string();
        const std::vector<std::filesystem::path> candidates {
            solutionsDirectory / (stem + "-DP" + extension),
            solutionsDirectory / (stem + "-tsp" + extension),
            solutionsDirectory / instanceFile.filename()
        };

        std::filesystem::path solutionPath;
        for (const auto& candidate : candidates) {
            std::cout << "Procurando solucao em: " << candidate.string() << '\n';
            std::error_code filesystemError;
            const bool solutionExists = std::filesystem::exists(candidate, filesystemError);
            if (filesystemError)
                fail(candidate, 1, "cannot inspect file: " + filesystemError.message());
            if (solutionExists) {
                solutionPath = candidate;
                break;
            }
        }
        if (solutionPath.empty())
            return std::nullopt;

        return readSolutionFile(solutionPath.string(), instance,
                                solutionPath.filename() == candidates[1].filename());
    }

    std::optional<::tspd::solution::LiteratureSolution> SolutionReader::readTspSolution(
        const std::string& instancePath,
        const ::tspd::tspd::TSPD& instance) {
        const std::filesystem::path instanceFile(instancePath);
        const auto path = instanceFile.parent_path() / "solutions" /
            (instanceFile.stem().string() + "-tsp" + instanceFile.extension().string());
        std::error_code error;
        const bool exists = std::filesystem::exists(path, error);
        if (error)
            fail(path, 1, "cannot inspect file: " + error.message());
        if (!exists)
            return std::nullopt;
        return readSolutionFile(path.string(), instance, true);
    }

    ::tspd::solution::LiteratureSolution SolutionReader::readSolutionFile(
        const std::string& path,
        const ::tspd::tspd::TSPD& instance,
        bool isTsp) {
        const std::filesystem::path solutionPath(path);
        std::vector<BlockComment> comments;
        const std::vector<SourceLine> sourceLines = removeComments(solutionPath, comments);
        std::optional<double> totalCost = readTotalCost(solutionPath, comments);
        if (!totalCost && !isTsp)
            fail(solutionPath, 1, "missing Total cost block comment");

        std::vector<SourceLine> dataLines;
        for (const auto& sourceLine : sourceLines) {
            if (!sourceLine.text.empty())
                dataLines.push_back(sourceLine);
        }
        if (dataLines.empty())
            fail(solutionPath, 1, "missing operation count");

        const std::vector<std::string> countFields = splitFields(dataLines.front().text);
        if (countFields.size() != 1)
            fail(solutionPath, dataLines.front().number,
                 "operation count line must contain exactly one integer");
        const int operationCount = parseInteger(
            countFields.front(), solutionPath, dataLines.front().number, "operation count");
        if (operationCount <= 0)
            fail(solutionPath, dataLines.front().number,
                 "operation count must be positive");

        const std::size_t actualOperationLines = dataLines.size() - 1;
        if (actualOperationLines != static_cast<std::size_t>(operationCount)) {
            const std::size_t errorLine = actualOperationLines >
                static_cast<std::size_t>(operationCount)
                ? dataLines[static_cast<std::size_t>(operationCount) + 1].number
                : sourceLines.back().number + 1;
            fail(solutionPath, errorLine,
                 "operation count does not match the number of operation lines");
        }

        std::vector<Operation> operations;
        operations.reserve(static_cast<std::size_t>(operationCount));
        for (std::size_t index = 1; index < dataLines.size(); ++index) {
            const SourceLine& sourceLine = dataLines[index];
            const std::vector<std::string> fields = splitFields(sourceLine.text);
            if (fields.size() < 4)
                fail(solutionPath, sourceLine.number,
                     "operation requires start, end, fly, internal count, and internal IDs");

            Operation operation {
                parseInteger(fields[0], solutionPath, sourceLine.number, "start ID"),
                parseInteger(fields[1], solutionPath, sourceLine.number, "end ID"),
                parseInteger(fields[2], solutionPath, sourceLine.number, "fly ID"),
                {},
                sourceLine.number
            };
            const int internalCount = parseInteger(
                fields[3], solutionPath, sourceLine.number, "internal count");
            if (internalCount < 0)
                fail(solutionPath, sourceLine.number, "internal count cannot be negative");
            if (fields.size() != static_cast<std::size_t>(internalCount) + 4) {
                fail(solutionPath, sourceLine.number,
                     "internal count does not match the number of internal IDs");
            }

            operation.internalNodes.reserve(static_cast<std::size_t>(internalCount));
            for (std::size_t fieldIndex = 4; fieldIndex < fields.size(); ++fieldIndex) {
                operation.internalNodes.push_back(parseInteger(
                    fields[fieldIndex], solutionPath, sourceLine.number, "internal ID"));
            }
            operations.push_back(std::move(operation));
        }

        const std::vector<::tspd::node::Node> nodes = instance.getNodes();
        if (nodes.empty())
            fail(solutionPath, operations.front().line,
                 "instance does not provide nodes for solution validation");

        std::unordered_set<int> knownNodeIds;
        for (const auto& node : nodes)
            knownNodeIds.insert(node.getId());

        const auto validateNode = [&](int nodeId, std::size_t line, const std::string& field) {
            if (knownNodeIds.find(nodeId) == knownNodeIds.end())
                fail(solutionPath, line, field + " " + std::to_string(nodeId) +
                     " is not a node in the instance");
        };

        const int depotId = nodes.front().getId();
        for (std::size_t index = 0; index < operations.size(); ++index) {
            const Operation& operation = operations[index];
            validateNode(operation.start, operation.line, "start ID");
            validateNode(operation.end, operation.line, "end ID");
            if (operation.fly != -1) {
                if (operation.fly < 0)
                    fail(solutionPath, operation.line,
                         "fly ID must be -1 or a node ID from the instance");
                validateNode(operation.fly, operation.line, "fly ID");
            }
            for (int internalId : operation.internalNodes)
                validateNode(internalId, operation.line, "internal ID");

            if (index == 0 && operation.start != depotId)
                fail(solutionPath, operation.line,
                     "operation sequence must start at the instance depot");
            if (index > 0 && operations[index - 1].end != operation.start)
                fail(solutionPath, operation.line,
                     "operation sequence is discontinuous with the preceding operation");
        }
        if (operations.back().end != depotId)
            fail(solutionPath, operations.back().line,
                 "operation sequence must return to the instance depot");

        std::vector<::tspd::edge::Edge> truckEdges;
        std::vector<::tspd::edge::Edge> droneEdges;
        std::vector<int> tspRoute{depotId};
        for (const Operation& operation : operations) {
            if (isTsp && operation.fly != -1)
                fail(solutionPath, operation.line, "TSP solution cannot contain drone deliveries");
            std::vector<int> truckPath;
            truckPath.reserve(operation.internalNodes.size() + 2);
            truckPath.push_back(operation.start);
            truckPath.insert(truckPath.end(), operation.internalNodes.begin(),
                             operation.internalNodes.end());
            truckPath.push_back(operation.end);
            if (isTsp) {
                tspRoute.insert(tspRoute.end(), operation.internalNodes.begin(),
                                operation.internalNodes.end());
                tspRoute.push_back(operation.end);
            }
            for (std::size_t index = 1; index < truckPath.size(); ++index) {
                appendEdge(truckEdges, truckPath[index - 1], truckPath[index], instance,
                           solutionPath, operation.line, "truck");
            }

            if (operation.fly != -1) {
                appendEdge(droneEdges, operation.start, operation.fly, instance,
                           solutionPath, operation.line, "drone");
                appendEdge(droneEdges, operation.fly, operation.end, instance,
                           solutionPath, operation.line, "drone");
            }
        }

        if (isTsp) {
            try {
                totalCost = instance.calculateTspCost(tspRoute);
            } catch (const std::exception& error) {
                fail(solutionPath, operations.back().line, error.what());
            }
        }
        return ::tspd::solution::LiteratureSolution(
            std::move(truckEdges), std::move(droneEdges), totalCost);
    }
}
