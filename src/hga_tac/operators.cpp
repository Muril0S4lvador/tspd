#include "join.hpp"
#include <algorithm>
#include <cmath>
#include <numeric>
#include <stdexcept>

namespace tspd::hga_tac::detail {
namespace {
int pick(int size, std::mt19937& rng) { return std::uniform_int_distribution<int>(0, size-1)(rng); }
double unit(std::mt19937& rng) { return std::generate_canonical<double, 32>(rng); }
void relocate(Genes& c, int from, int count, int destination) {
    Genes block(c.begin()+from, c.begin()+from+count);
    c.erase(c.begin()+from, c.begin()+from+count);
    if (destination > from) destination -= count;
    c.insert(c.begin()+destination, block.begin(), block.end());
}
}
Genes crossover(const Genes& a, const Genes& b, Crossover type, std::mt19937& rng) {
    if (a.size() != b.size()) throw std::invalid_argument("Pais de tamanhos diferentes.");
    const int n = static_cast<int>(a.size());
    if (n < 2) return a;
    int left = pick(n, rng), right = pick(n, rng);
    if (left > right) std::swap(left, right);
    Genes child(n, 0);
    std::vector<bool> used(n+1, false);
    if (type == Crossover::OX2) {
        // Selected positions in A impose their order on the matching slots in B.
        Genes selected;
        for (int gene : a) if (unit(rng) < 0.5) { selected.push_back(gene); used[std::abs(gene)] = true; }
        int j = 0;
        for (int p = 0; p < n; ++p) child[p] = used[std::abs(b[p])] ? selected[j++] : b[p];
        return child;
    }
    const bool truck = unit(rng) < 0.5;
    for (int p = left; p <= right; ++p) {
        if (type != Crossover::TOX1 || (a[p] > 0) == truck) {
            child[p] = a[p]; used[std::abs(a[p])] = true;
        }
    }
    // Classical OX1 uses circular filling; type-aware methods use linear filling.
    int position = type == Crossover::OX1 ? (right+1)%n : 0;
    for (int offset = 0; offset < n; ++offset) {
        const int gene = b[type == Crossover::OX1 ? (right+1+offset)%n : offset];
        if (used[std::abs(gene)]) continue;
        while (child[position]) position = (position+1)%n;
        child[position] = gene;
        used[std::abs(gene)] = true;
    }
    if (type == Crossover::TOX2) {
        std::vector<int> signA(n+1), signB(n+1);
        for (int gene : a) signA[std::abs(gene)] = gene > 0 ? 1 : -1;
        for (int gene : b) signB[std::abs(gene)] = gene > 0 ? 1 : -1;
        for (int p = 0; p < n; ++p) {
            int id = std::abs(child[p]);
            child[p] = id * (p >= left && p <= right ? signB[id] : signA[id]);
        }
    }
    return child;
}
Genes mutate(Genes genes, std::mt19937& rng) {
    if (unit(rng) < 0.5) {
        for (int& gene : genes) if (unit(rng) < 0.1) gene = -gene;
    } else {
        std::vector<int> positions(genes.size());
        std::iota(positions.begin(), positions.end(), 0);
        std::shuffle(positions.begin(), positions.end(), rng);
        positions.resize(static_cast<std::size_t>(std::ceil(0.2 * genes.size())));
        Genes values;
        for (int p : positions) values.push_back(genes[p]);
        std::shuffle(values.begin(), values.end(), rng);
        for (std::size_t i = 0; i < positions.size(); ++i) genes[positions[i]] = values[i];
    }
    return genes;
}

Genes neighbor(const Context& context, const Genes& genes, const Decoded& decoded,
               int kind, double closeFraction, std::mt19937& rng) {
    if (kind < 0 || kind >= 22) throw std::invalid_argument("Vizinhanca HGA-TAC invalida.");
    Genes c = genes;
    const int n = static_cast<int>(c.size());
    if (!n) return c;
    std::vector<int> truck, drone, free, pairs;
    std::vector<bool> rendezvous(context.size(), false);
    std::vector<int> position(context.size(), -1);
    for (int p = 0; p < n; ++p) position[std::abs(c[p])] = p;
    for (const auto& f : decoded.flights) {
        rendezvous[decoded.truck[f.launch]] = true;
        rendezvous[decoded.truck[f.recovery]] = true;
    }
    for (int p = 0; p < n; ++p) {
        if (c[p] > 0) {
            truck.push_back(p);
            if (!rendezvous[c[p]]) free.push_back(p);
            if (p+1 < n && c[p+1] > 0) pairs.push_back(p);
        } else drone.push_back(p);
    }
    auto choose = [&](const std::vector<int>& v) { return v[pick(static_cast<int>(v.size()), rng)]; };
    // Restrict candidate partners to the closest ceil(0.3*N) customer IDs,
    // with deterministic ID tie breaks. This is not a full-neighborhood scan.
    auto nearby = [&](int id, const std::vector<int>& candidates, bool /* aerial */ = false) {
        std::vector<int> out;
        for (int p : candidates)
            if (context.close(id,std::abs(c[p]),closeFraction)) out.push_back(p);
        return out;
    };
    if (kind == 0 || kind == 1) { // N1/N2: relocate one/two free truck customers.
        std::vector<int> sources;
        for (int p : free)
            if (kind == 0 || (p+1 < n && c[p+1] > 0 && !rendezvous[c[p+1]])) sources.push_back(p);
        if (sources.empty()) return c;
        int from = choose(sources), count = kind+1;
        auto destinations = nearby(c[from], truck);
        destinations.erase(std::remove_if(destinations.begin(), destinations.end(), [&](int p) {
            return p >= from && p <= from+count;
        }), destinations.end());
        if (!destinations.empty()) relocate(c, from, count, choose(destinations));
    } else if (kind == 2) { // N3: exchange two truck nodes.
        if (truck.empty()) return c;
        int a = choose(truck); auto choices = nearby(c[a], truck);
        if (!choices.empty()) std::swap(c[a], c[choose(choices)]);
    } else if (kind == 3 || kind == 4) { // N4/N5: exchange a pair and one truck node.
        auto sources = pairs;
        if (kind == 3) sources.erase(std::remove_if(sources.begin(), sources.end(), [&](int p) {
            return rendezvous[c[p]] || rendezvous[c[p+1]];
        }), sources.end());
        if (sources.empty()) return c;
        int a = choose(sources); auto choices = nearby(c[a], truck);
        choices.erase(std::remove_if(choices.begin(), choices.end(), [&](int b) { return b == a || b == a+1; }), choices.end());
        if (choices.empty()) return c;
        int b = choose(choices);
        // Build the exchanged blocks without ambiguous erase/insert indices.
        Genes out;
        for (int p = 0; p < n; ++p) {
            if (p == a) { out.push_back(c[b]); ++p; }
            else if (p == b) { out.push_back(c[a]); out.push_back(c[a+1]); }
            else out.push_back(c[p]);
        }
        c = std::move(out);
    } else if (kind >= 5 && kind <= 8) { // N6..N9: pair reconnections.
        if (pairs.empty()) return c;
        int a = choose(pairs); auto choices = nearby(c[a], pairs);
        choices.erase(std::remove_if(choices.begin(), choices.end(), [&](int b) { return std::abs(a-b) < 2; }), choices.end());
        if (choices.empty()) return c;
        int b = choose(choices);
        if (kind == 5 || kind == 8) {
            int x = c[a], y = c[a+1];
            c[a] = c[b+(kind == 8 ? 1 : 0)]; c[a+1] = c[b+(kind == 8 ? 0 : 1)];
            c[b] = x; c[b+1] = y;
        } else if (kind == 6) {
            std::swap(c[b], c[b+1]); std::swap(c[a+1], c[b]);
        } else std::swap(c[a+1], c[b+1]);
    } else if (kind >= 9 && kind <= 12) { // N10..N13: changes within a decoded sortie.
        if (decoded.flights.empty()) return c;
        const auto& f = decoded.flights[pick(static_cast<int>(decoded.flights.size()), rng)];
        if (f.customers.size() != 1) return c;
        int d = position[f.customers.front()];
        int a = position[decoded.truck[f.launch]], b = position[decoded.truck[f.recovery]];
        if (kind == 12) { if (a >= 0 && b >= 0) std::swap(c[a],c[b]); }
        else {
            int t = kind == 10 ? a : b;
            if (kind == 9) {
                std::vector<int> choices;
                for (int k = f.launch; k <= f.recovery; ++k)
                    if (decoded.truck[k]) choices.push_back(position[decoded.truck[k]]);
                if (choices.empty()) return c;
                t = choose(choices);
            }
            if (t >= 0) { int id = c[t]; c[t] = -c[d]; c[d] = -id; }
        }
    } else if (kind == 13 || kind == 14 || kind == 16) { // N14, N15, L2.
        const auto& sources = kind == 13 ? free : drone;
        if (sources.empty()) return c;
        int from = choose(sources), gene = c[from];
        auto choices = nearby(std::abs(gene), pairs, kind != 14);
        choices.erase(std::remove_if(choices.begin(), choices.end(), [&](int p) { return from == p || from == p+1; }), choices.end());
        if (choices.empty()) return c;
        int destination = choose(choices)+1;
        c[from] = kind == 14 ? std::abs(gene) : -std::abs(gene);
        relocate(c, from, 1, destination);
    } else if (kind == 15) { // L1: middle of three consecutive truck nodes -> drone.
        std::vector<int> choices;
        for (int p = 1; p+1 < n; ++p) if (c[p-1] > 0 && c[p] > 0 && c[p+1] > 0) choices.push_back(p);
        if (!choices.empty()) { int p = choose(choices); c[p] = -c[p]; }
    } else if (kind == 17) { // L3: exchange IDs, preserving types at positions.
        if (truck.empty() || drone.empty()) return c;
        int a = choose(drone); auto choices = nearby(-c[a], truck, true);
        if (!choices.empty()) { int b = choose(choices), id = c[b]; c[b] = -c[a]; c[a] = -id; }
    } else if (kind == 18) { // L4: 2-opt on truck arcs, including intervening drone genes.
        if (truck.size() < 2) return c;
        int a = choose(truck); auto choices = nearby(c[a], truck);
        if (choices.empty()) return c;
        int b = choose(choices); if (a > b) std::swap(a,b);
        std::reverse(c.begin()+a, c.begin()+b+1);
    } else if (kind == 19 || kind == 20) { // L5/L6: exchange drone nodes and convert.
        if (drone.size() < 2) return c;
        int a = choose(drone); auto choices = nearby(-c[a], drone, true);
        if (choices.empty()) return c;
        int b = choose(choices); std::swap(c[a],c[b]);
        if (kind == 19) { c[a] = -c[a]; c[b] = -c[b]; }
        else { int p = unit(rng) < 0.5 ? a : b; c[p] = -c[p]; }
    } else if (kind == 21) { // L7: replace a sortie's drone customer, retain old one by truck.
        if (drone.size() < 2 || decoded.flights.empty()) return c;
        int from = choose(drone), d = c[from];
        std::vector<int> targets;
        for (int p : drone) if (p != from) targets.push_back(p);
        auto choices = nearby(-d, targets, true);
        if (choices.empty()) return c;
        int target = choose(choices);
        c[target] = -c[target];
        c.erase(c.begin()+from);
        if (from < target) --target;
        c.insert(c.begin()+target+(unit(rng)<0.5 ? 0 : 1), d);
    }
    return c;
}
}
