#pragma once

#include "../tspd/tspd.hpp"
#include "../solution/solution.hpp"

#include <cstddef>
#include <vector>

namespace tspd::hgvns::detail {
    // Extremidades sao posicoes do caminhao; customer e um ID de cliente.
    // Assim as duas ocorrencias do deposito continuam sendo eventos distintos.
    struct Flight {
        std::size_t launch;
        int customer;
        std::size_t recovery;
    };

    struct State {
        std::vector<int> truck;
        std::vector<Flight> flights;
    };

    struct Evaluation {
        double total = 0;
        double truck = 0;
        double drone = 0;
        std::vector<double> prefix;
    };

    class Context {
        public:
            explicit Context(const ::tspd::tspd::TSPD& instance);
            int dimension() const { return dimension_; }
            double truck(int first, int second) const {
                return truck_[static_cast<std::size_t>(first) * dimension_ + second];
            }
            double drone(int first, int second) const {
                return drone_[static_cast<std::size_t>(first) * dimension_ + second];
            }
        private:
            int dimension_;
            std::vector<double> truck_;
            std::vector<double> drone_;
    };

    bool improves(double candidate, double incumbent);
    void sortFlights(State& state);
    void validate(const Context& context, const State& state);
    Evaluation evaluate(const Context& context, const State& state, bool check = true);
    ::tspd::solution::Solution toSolution(const Context& context, const State& state);
}
