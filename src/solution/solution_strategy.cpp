#include "solution_strategy.hpp"
#include <algorithm>
#include <cmath>
#include <iostream>
#include <stdexcept>

namespace tspd::solution {
    Solution SolutionStrategy::Solve(::tspd::tspd::TSPD& instance) {
        return solve(instance, nullptr);
    }

    Solution SolutionStrategy::Solve(::tspd::tspd::TSPD& instance, const Solution& tspSolution) {
        return solve(instance, &tspSolution);
    }

    Solution SolutionStrategy::Optimize(::tspd::tspd::TSPD&, const Solution&) {
        throw std::logic_error(getName() + " nao implementa Optimize(instance, tspSolution).");
    }

    const vector<int>& SolutionStrategy::requireTspRoute(
        const ::tspd::tspd::TSPD& instance, const Solution& tspSolution) {
        const auto& route = tspSolution.getTspNodes();
        const auto& cost = tspSolution.getTspCost();
        if (route.empty() || !cost || !std::isfinite(*cost))
            throw std::invalid_argument("Solucao TSP previa ausente ou sem rota e custo validos.");
        const double expectedCost = instance.calculateTspCost(route);
        if (std::abs(*cost - expectedCost) > 1e-9 * std::max(1.0, std::abs(expectedCost)))
            throw std::invalid_argument("Custo do TSP fornecido nao corresponde a rota nesta instancia.");
        return route;
    }

    Solution SolutionStrategy::solve(::tspd::tspd::TSPD& instance, const Solution* tspSolution) {
        start_ = Clock::now();
        end_ = start_;
        try {
            std::cout << "Optimization " << this->getName() << " Initialized.\n";
            Solution result = tspSolution ? Optimize(instance, *tspSolution) : Optimize(instance);
            end_ = Clock::now();
            std::cout << "Optimization " << this->getName() << " Finalized.\n";
            return result;
        } catch (...) {
            end_ = Clock::now();
            throw;
        }
    }
}
