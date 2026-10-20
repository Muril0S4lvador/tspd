#pragma once

#include <optional>
#include <string>

#include "../solution/literature_solution.hpp"
#include "../tspd/tspd.hpp"

namespace tspd::solution_reader {
    class SolutionReader {
        public:
            SolutionReader() = delete;

            static std::optional<::tspd::solution::LiteratureSolution> readSolution(
                const std::string& instancePath,
                const ::tspd::tspd::TSPD& instance);
            static std::optional<::tspd::solution::LiteratureSolution> readTspSolution(
                const std::string& instancePath,
                const ::tspd::tspd::TSPD& instance);
        private:
            static ::tspd::solution::LiteratureSolution readSolutionFile(
                const std::string& solutionPath,
                const ::tspd::tspd::TSPD& instance,
                bool isTsp);
    };
}
