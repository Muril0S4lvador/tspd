#pragma once
#include "HGATAC.hpp"
#include <random>

namespace tspd::hga_tac::detail {
using Genes = std::vector<int>;
// Endpoints are truck route POSITIONS. Customers are node IDs.
struct Flight { int launch; std::vector<int> customers; int recovery; };
struct Decoded {
    double cost = std::numeric_limits<double>::infinity();
    double truckCost = 0;
    double droneCost = 0;
    std::vector<int> truck;
    std::vector<Flight> flights;
};
class Context {
public:
    explicit Context(const ::tspd::tspd::TSPD& instance);
    int size() const { return size_; }
    double truck(int a, int b) const { return truck_[a * size_ + b]; }
    double drone(int a, int b) const { return drone_[a * size_ + b]; }
    bool close(int a, int b, double fraction) const;
private:
    int size_;
    std::vector<double> truck_, drone_;
    std::vector<int> proximityRank_;
};
void validateGenes(const Context&, const Genes&);
bool multipleDeliveries(const Genes&);
bool improves(double candidate, double incumbent);
// Strict Join returns infinity for infeasible chromosomes. Penalized Join allows
// multi-customer flights and endurance excess, solely for population exploration.
Decoded join(const Context&, const Genes&, double endurance,
             bool penalized = false, double multiplePenalty = 2, double rangePenalty = 2);
Genes partition(const Context&, const std::vector<int>& tsp, double endurance);
solution::Solution toSolution(const Context&, const Genes&, const Decoded&);
enum class Crossover { OX1, OX2, TOX1, TOX2 };
Genes crossover(const Genes&, const Genes&, Crossover, std::mt19937&);
Genes mutate(Genes, std::mt19937&);
// N1..N15 = 0..14; L1..L7 = 15..21. One random admissible move.
Genes neighbor(const Context&, const Genes&, const Decoded&, int kind,
               double closeFraction, std::mt19937&);
}
