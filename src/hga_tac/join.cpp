#include "join.hpp"
#include <algorithm>
#include <cmath>
#include <numeric>
#include <stdexcept>

namespace tspd::hga_tac::detail {
Context::Context(const ::tspd::tspd::TSPD& instance) : size_(instance.getDimension()) {
    if (size_ < 1 || instance.getNodes().size() != static_cast<std::size_t>(size_) ||
        !std::isfinite(instance.getTruckSpeed()) || instance.getTruckSpeed() <= 0 ||
        !std::isfinite(instance.getDroneSpeed()) || instance.getDroneSpeed() <= 0)
        throw std::invalid_argument("HGA-TAC requer dimensao e fatores positivos e finitos.");
    truck_.resize(static_cast<std::size_t>(size_) * size_);
    drone_.resize(truck_.size());
    for (int a = 0; a < size_; ++a) for (int b = 0; b < size_; ++b) {
        double distance = instance.getDistance(a, b);
        double t = distance * instance.getTruckSpeed(), d = distance * instance.getDroneSpeed();
        if (distance < 0 || !std::isfinite(t) || !std::isfinite(d))
            throw std::invalid_argument("HGA-TAC recebeu tempos de deslocamento invalidos.");
        truck_[a * size_ + b] = t;
        drone_[a * size_ + b] = d;
    }
    // Both vehicles share the same metric in the repository. Positive scaling
    // preserves nearest-neighbor order, so a single ranking serves both.
    proximityRank_.assign(truck_.size(),size_);
    for(int a=0;a<size_;++a) {
        std::vector<int> ids;
        for(int b=1;b<size_;++b) if(b!=a) ids.push_back(b);
        std::sort(ids.begin(),ids.end(),[&](int b,int d) {
            return truck(a,b)!=truck(a,d) ? truck(a,b)<truck(a,d) : b<d;
        });
        for(std::size_t rank=0;rank<ids.size();++rank)
            proximityRank_[a*size_+ids[rank]]=static_cast<int>(rank);
    }
}
bool Context::close(int a,int b,double fraction) const {
    return proximityRank_[a*size_+b]<std::max(1,static_cast<int>(std::ceil(fraction*size_)));
}
void validateGenes(const Context& c, const Genes& genes) {
    if (genes.size() != static_cast<std::size_t>(c.size() - 1))
        throw std::invalid_argument("Cromossomo HGA-TAC com tamanho invalido.");
    std::vector<bool> seen(c.size(), false);
    for (int gene : genes) {
        // Check before abs(), including INT_MIN.
        if (gene == 0 || gene <= -c.size() || gene >= c.size())
            throw std::invalid_argument("ID invalido no cromossomo HGA-TAC.");
        int id = std::abs(gene);
        if (seen[id]) throw std::invalid_argument("Cliente repetido no cromossomo HGA-TAC.");
        seen[id] = true;
    }
}
bool multipleDeliveries(const Genes& genes) {
    for (std::size_t i = 1; i < genes.size(); ++i)
        if (genes[i] < 0 && genes[i-1] < 0) return true;
    return false;
}
bool improves(double a, double b) {
    return std::isfinite(a) && (!std::isfinite(b) || a < b - 1e-9 * std::max(1.0, std::abs(b)));
}

Decoded join(const Context& c, const Genes& genes, double endurance,
             bool penalized, double multiplePenalty, double rangePenalty) {
    validateGenes(c, genes);
    if (std::isnan(endurance) || endurance <= 0 ||
        !std::isfinite(multiplePenalty) || multiplePenalty < 1 ||
        !std::isfinite(rangePenalty) || rangePenalty < 0)
        throw std::invalid_argument("Parametros invalidos para Join.");
    Decoded result;
    result.truck = {0};
    std::vector<std::vector<int>> groups;
    std::vector<int> boundary;
    bool previousDrone = false;
    for (int gene : genes) {
        if (gene > 0) { result.truck.push_back(gene); previousDrone = false; }
        else {
            if (!previousDrone) {
                groups.emplace_back();
                boundary.push_back(static_cast<int>(result.truck.size()) - 1);
            }
            groups.back().push_back(-gene);
            previousDrone = true;
        }
    }
    result.truck.push_back(0);
    const int m = static_cast<int>(result.truck.size());
    const int h = static_cast<int>(groups.size());
    std::vector<double> prefix(m, 0);
    for (int t = 1; t < m; ++t)
        prefix[t] = prefix[t-1] + c.truck(result.truck[t-1], result.truck[t]);
    result.truckCost = prefix.back();
    if (!penalized && multipleDeliveries(genes)) return result;
    const double inf = std::numeric_limits<double>::infinity();
    std::vector<std::vector<double>> dp(h + 1, std::vector<double>(m, inf));
    std::vector<std::vector<int>> recovery(h, std::vector<int>(m, -1));
    // State (g,t): g drone groups consumed, truck at position t. The extra
    // consumed-group state avoids C(i)->C(i) for a stationary truck sortie.
    const int tailStart = h ? boundary.back() : 0;
    for (int t = tailStart; t < m; ++t) dp[h][t] = prefix.back() - prefix[t];
    for (int g = h - 1; g >= 0; --g) {
        const int lower = g ? boundary[g-1] : 0;
        const int upper = boundary[g];
        const int last = g + 1 < h ? boundary[g+1] : m - 1;
        double internal = 0, weight = multiplePenalty;
        for (std::size_t j = 1; j < groups[g].size(); ++j) {
            internal += weight * c.drone(groups[g][j-1], groups[g][j]);
            weight *= multiplePenalty;
        }
        for (int t = upper; t >= lower; --t) {
            if (t < upper) dp[g][t] = c.truck(result.truck[t], result.truck[t+1]) + dp[g][t+1];
            for (int k = upper; k <= last; ++k) {
                const double air = c.drone(result.truck[t], groups[g].front()) + internal
                                 + c.drone(groups[g].back(), result.truck[k]);
                if (!penalized && air > endurance) continue;
                const double penalty = penalized ? rangePenalty * std::max(0.0, air - endurance) : 0;
                const double cost = std::max(prefix[k] - prefix[t], air + penalty) + dp[g+1][k];
                if (cost < dp[g][t]) { dp[g][t] = cost; recovery[g][t] = k; }
            }
        }
    }
    result.cost = dp[0][0];
    if (!std::isfinite(result.cost)) return result;
    int t = 0;
    for (int g = 0; g < h; ++g) {
        while (recovery[g][t] < 0) ++t;
        const int k = recovery[g][t];
        result.flights.push_back({t, groups[g], k});
        int previous = result.truck[t];
        for (int customer : groups[g]) {
            result.droneCost += c.drone(previous, customer);
            previous = customer;
        }
        result.droneCost += c.drone(previous, result.truck[k]);
        t = k;
    }
    return result;
}

Genes partition(const Context& c, const std::vector<int>& tour, double endurance) {
    // Exact fixed-order partition, not the exponential global DP and not VND.
    const int n = static_cast<int>(tour.size());
    std::vector<double> prefix(n, 0), best(n, std::numeric_limits<double>::infinity());
    std::vector<int> predecessor(n, -1), drone(n, -1);
    for (int k = 1; k < n; ++k) prefix[k] = prefix[k-1] + c.truck(tour[k-1], tour[k]);
    best[0] = 0;
    for (int k = 1; k < n; ++k) {
        best[k] = best[k-1] + c.truck(tour[k-1], tour[k]);
        predecessor[k] = k-1;
        for (int i = 0; i + 1 < k; ++i) for (int j = i+1; j < k; ++j) {
            const double air = c.drone(tour[i], tour[j]) + c.drone(tour[j], tour[k]);
            if (air > endurance) continue;
            const double truck = prefix[k] - prefix[i] - c.truck(tour[j-1], tour[j])
                               - c.truck(tour[j], tour[j+1]) + c.truck(tour[j-1], tour[j+1]);
            const double value = best[i] + std::max(truck, air);
            if (improves(value, best[k])) { best[k] = value; predecessor[k] = i; drone[k] = j; }
        }
    }
    Genes genes(tour.begin()+1, tour.end()-1);
    for (int k = n-1; k > 0; k = predecessor[k])
        if (drone[k] >= 0) genes[drone[k]-1] *= -1;
    return genes;
}

solution::Solution toSolution(const Context& c, const Genes& genes, const Decoded& d) {
    if (!std::isfinite(d.cost) || multipleDeliveries(genes))
        throw std::logic_error("HGA-TAC nao pode devolver uma solucao inviavel.");
    std::vector<int> route;
    if (!d.flights.empty()) {
        route.push_back(0);
        int cursor = 0;
        for (const auto& f : d.flights) {
            for (int p = cursor+1; p <= f.launch; ++p) route.push_back(d.truck[p]);
            route.push_back(f.customers.front());
            route.push_back(d.truck[f.recovery]);
            cursor = f.recovery;
        }
        for (int p = cursor+1; p < static_cast<int>(d.truck.size()); ++p) route.push_back(d.truck[p]);
    }
    solution::Solution result(d.truck, route, d.cost, d.truckCost, d.droneCost);
    std::vector<int> tsp{0};
    for (int gene : genes) tsp.push_back(std::abs(gene));
    tsp.push_back(0);
    double cost = 0;
    for (std::size_t p = 1; p < tsp.size(); ++p) cost += c.truck(tsp[p-1], tsp[p]);
    result.setTspRoute(std::move(tsp), cost);
    return result;
}
}
