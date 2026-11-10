#include "solution_state.hpp"

#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <tuple>

namespace tspd::hgvns::detail {
    Context::Context(const ::tspd::tspd::TSPD& instance)
        : dimension_(instance.getDimension()) {
        if (dimension_ < 1 || !std::isfinite(instance.getTruckSpeed()) ||
            !std::isfinite(instance.getDroneSpeed()) || instance.getTruckSpeed() <= 0 ||
            instance.getDroneSpeed() <= 0)
            throw std::invalid_argument("HGVNS requer dimensao e fatores de deslocamento positivos e finitos.");
        const auto size = static_cast<std::size_t>(dimension_) * dimension_;
        truck_.assign(size, 0);
        drone_.assign(size, 0);
        for (int first = 0; first < dimension_; ++first) {
            for (int second = first + 1; second < dimension_; ++second) {
                const double distance = instance.getDistance(first, second);
                const double truckTime = distance * instance.getTruckSpeed();
                const double droneTime = distance * instance.getDroneSpeed();
                if (!std::isfinite(truckTime) || !std::isfinite(droneTime) || distance < 0)
                    throw std::invalid_argument("HGVNS recebeu tempos de deslocamento invalidos.");
                const auto forward = static_cast<std::size_t>(first) * dimension_ + second;
                const auto backward = static_cast<std::size_t>(second) * dimension_ + first;
                truck_[forward] = truck_[backward] = truckTime;
                drone_[forward] = drone_[backward] = droneTime;
            }
        }
    }

    bool improves(double candidate, double incumbent) {
        return candidate < incumbent - 1e-9 * std::max(1.0, std::abs(incumbent));
    }

    void sortFlights(State& state) {
        std::sort(state.flights.begin(), state.flights.end(), [](const Flight& a, const Flight& b) {
            return std::tie(a.launch, a.recovery, a.customer) < std::tie(b.launch, b.recovery, b.customer);
        });
    }

    void validate(const Context& context, const State& state) {
        if (state.truck.size() < 2 || state.truck.front() != 0 || state.truck.back() != 0)
            throw std::invalid_argument("Solucao HGVNS deve partir e retornar ao deposito.");
        std::vector<bool> served(static_cast<std::size_t>(context.dimension()), false);
        served[0] = true;
        for (std::size_t p = 1; p + 1 < state.truck.size(); ++p) {
            const int customer = state.truck[p];
            if (customer <= 0 || customer >= context.dimension() || served[customer])
                throw std::invalid_argument("Solucao HGVNS tem cliente do caminhao repetido ou invalido.");
            served[customer] = true;
        }
        std::size_t previousRecovery = 0;
        for (const auto& flight : state.flights) {
            if (flight.launch >= state.truck.size() - 1 || flight.recovery >= state.truck.size() ||
                flight.launch > flight.recovery || flight.launch < previousRecovery)
                throw std::invalid_argument("Solucao HGVNS tem viagens de drone sobrepostas ou extremos invalidos.");
            if (flight.customer <= 0 || flight.customer >= context.dimension() || served[flight.customer])
                throw std::invalid_argument("Solucao HGVNS tem cliente do drone repetido ou invalido.");
            served[flight.customer] = true;
            previousRecovery = flight.recovery;
        }
        if (std::find(served.begin(), served.end(), false) != served.end())
            throw std::invalid_argument("Solucao HGVNS nao atende todos os clientes.");
    }

    Evaluation evaluate(const Context& context, const State& state, bool check) {
        if (check) validate(context, state);
        Evaluation result;
        result.prefix.assign(state.truck.size(), 0);
        for (std::size_t p = 1; p < state.truck.size(); ++p)
            result.prefix[p] = result.prefix[p - 1] + context.truck(state.truck[p - 1], state.truck[p]);
        result.truck = result.total = result.prefix.back();
        for (const auto& flight : state.flights) {
            const double flightTime = context.drone(state.truck[flight.launch], flight.customer)
                                    + context.drone(flight.customer, state.truck[flight.recovery]);
            const double truckTime = result.prefix[flight.recovery] - result.prefix[flight.launch];
            result.drone += flightTime;
            // Os trechos de voo nao se sobrepoem: acrescenta somente a espera do caminhao.
            result.total += std::max(0.0, flightTime - truckTime);
        }
        if (!std::isfinite(result.total) || !std::isfinite(result.drone))
            throw std::invalid_argument("HGVNS produziu custo nao finito.");
        return result;
    }

    ::tspd::solution::Solution toSolution(const Context& context, const State& state) {
        const auto cost = evaluate(context, state);
        std::vector<int> droneRoute;
        if (!state.flights.empty()) {
            droneRoute.push_back(0);
            std::size_t cursor = 0;
            for (const auto& flight : state.flights) {
                for (std::size_t p = cursor + 1; p <= flight.launch; ++p)
                    droneRoute.push_back(state.truck[p]);
                droneRoute.push_back(flight.customer);
                droneRoute.push_back(state.truck[flight.recovery]);
                cursor = flight.recovery;
            }
            for (std::size_t p = cursor + 1; p < state.truck.size(); ++p)
                droneRoute.push_back(state.truck[p]);
        }

        // Convencao explicita para tsp_cost: insere cada cliente aereo apos seu lancamento.
        std::vector<int> tour;
        auto flight = state.flights.begin();
        for (std::size_t p = 0; p < state.truck.size(); ++p) {
            tour.push_back(state.truck[p]);
            while (flight != state.flights.end() && flight->launch == p) {
                tour.push_back(flight->customer);
                ++flight;
            }
        }
        double tourCost = 0;
        for (std::size_t p = 1; p < tour.size(); ++p)
            tourCost += context.truck(tour[p - 1], tour[p]);
        ::tspd::solution::Solution result{state.truck, droneRoute, cost.total, cost.truck, cost.drone};
        result.setTspRoute(std::move(tour), tourCost);
        return result;
    }
}
