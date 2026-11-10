#include "HGATAC.hpp"
#include "join.hpp"
#include <algorithm>
#include <array>
#include <cmath>
#include <deque>
#include <numeric>
#include <stdexcept>

namespace tspd::hga_tac {
namespace {
using namespace detail;
double unit(std::mt19937& rng) { return std::generate_canonical<double,32>(rng); }
int pick(int n, std::mt19937& rng) { return std::uniform_int_distribution<int>(0,n-1)(rng); }
struct Individual { Genes genes; Decoded decoded; int type; double fitness = 0; };
using Population = std::vector<Individual>;

class Search {
    const Context& context;
    const Parameters& p;
    std::mt19937 rng;
    bool escape;
    Statistics& stats;
    std::array<Population,3> populations;
    std::deque<int> recent;
    double penaltyM = 2, penaltyR = 2;
    Individual base, best;

    Individual evaluate(Genes genes) const {
        const bool multiple = multipleDeliveries(genes);
        auto decoded = join(context, genes, p.endurance, false);
        int type = multiple ? 1 : (std::isfinite(decoded.cost) ? 0 : 2);
        if (type) decoded = join(context, genes, p.endurance, true, penaltyM, penaltyR);
        return {std::move(genes), std::move(decoded), type, 0};
    }
    void remember(const Individual& x) {
        if (!x.type && improves(x.decoded.cost, best.decoded.cost)) best = x;
    }
    void add(Individual x) {
        remember(x);
        auto& population = populations[x.type];
        population.push_back(std::move(x));
        if (population.size() >= static_cast<std::size_t>(p.population+p.offspring)) {
            std::stable_sort(population.begin(), population.end(), [](const Individual& a, const Individual& b) {
                return a.decoded.cost < b.decoded.cost;
            });
            population.resize(p.population);
        }
    }
    Genes perturb() {
        Genes genes = base.genes;
        const int n = static_cast<int>(genes.size());
        if (!n) return genes;
        if (unit(rng) < 0.5) {
            for (int i = 0; i < n; ++i) {
                double r = unit(rng);
                if (r < 0.1) genes[i] = -genes[i];
                else if (r < 0.2 && n > 1) std::swap(genes[i],genes[i+1<n ? i+1 : i-1]);
            }
        } else {
            int a = pick(n,rng), b = pick(n,rng); if (a>b) std::swap(a,b);
            switch (pick(3,rng)) {
                case 0: std::reverse(genes.begin()+a,genes.begin()+b+1); break;
                case 1: for (int i=a;i<=b;++i) genes[i] = -genes[i]; break;
                default: std::shuffle(genes.begin()+a,genes.begin()+b+1,rng);
            }
        }
        return genes;
    }
    void fill() {
        const int types = std::isfinite(p.endurance) ? 3 : 2;
        // Degenerate instances can have no members of an infeasible class.
        // Bound only initialization attempts, never the evolutionary search.
        for (int attempt=0;attempt<100*p.population;++attempt) {
            bool full = true;
            for (int type=0;type<types;++type)
                if (populations[type].size()<static_cast<std::size_t>(p.population) &&
                    !(type==1 && context.size()<3)) full=false;
            if (full) break;
            auto x = evaluate(perturb());
            remember(x);
            if (populations[x.type].size()<static_cast<std::size_t>(p.population))
                populations[x.type].push_back(std::move(x));
        }
        for (auto& population:populations) {
            const auto original = population.size();
            if (original) for (std::size_t i=0;population.size()<static_cast<std::size_t>(p.population);++i)
                population.push_back(population[i%original]);
        }
    }
    std::vector<Individual*> rank() {
        std::vector<Individual*> all;
        for (auto& pop:populations) for (auto& x:pop) all.push_back(&x);
        for (auto* x:all) {
            double d1=1,d2=1;
            for (auto* y:all) if (x!=y) {
                int mismatch=0;
                for (std::size_t i=0;i<x->genes.size();++i) mismatch += x->genes[i]!=y->genes[i];
                double d = x->genes.empty() ? 0 : static_cast<double>(mismatch)/x->genes.size();
                if (d<d1) { d2=d1; d1=d; } else if (d<d2) d2=d;
            }
            x->fitness = x->decoded.cost * std::pow(0.8,(d1+d2)/2);
        }
        std::stable_sort(all.begin(),all.end(),[](const Individual* a,const Individual* b) { return a->fitness<b->fitness; });
        return all;
    }
    Genes parent(const std::vector<Individual*>& all) {
        std::vector<int> indices(all.size()); std::iota(indices.begin(),indices.end(),0);
        const int count = std::min(p.tournament,static_cast<int>(all.size()));
        int winner=static_cast<int>(all.size());
        for (int i=0;i<count;++i) {
            int j=i+pick(static_cast<int>(indices.size())-i,rng);
            std::swap(indices[i],indices[j]); winner=std::min(winner,indices[i]);
        }
        return all[winner]->genes;
    }
    Individual localSearch(Individual current) {
        std::array<int,22> order; std::iota(order.begin(),order.end(),0);
        std::shuffle(order.begin(),order.end(),rng);
        std::size_t k=0;
        while(k<order.size()) {
            auto genes = neighbor(context,current.genes,current.decoded,order[k],p.closeFraction,rng);
            if (genes==current.genes) { ++k; continue; }
            auto candidate=evaluate(std::move(genes));
            if (!candidate.type && improves(candidate.decoded.cost,current.decoded.cost)) {
                current=std::move(candidate);
                std::shuffle(order.begin(),order.end(),rng); k=0;
            } else ++k;
        }
        return current;
    }
    Individual repair(Individual x) {
        bool previous=false;
        for (int& gene:x.genes) {
            if (gene<0 && previous) gene=-gene;
            previous=gene<0;
        }
        x=evaluate(std::move(x.genes));
        // On a restricted instance, progressively return aerial customers to
        // the truck; termination is guaranteed by the all-truck chromosome.
        while(x.type) {
            auto it=std::find_if(x.genes.begin(),x.genes.end(),[](int g){return g<0;});
            if(it==x.genes.end()) throw std::logic_error("Reparo HGA-TAC inconsistente.");
            *it=-*it; x=evaluate(std::move(x.genes));
        }
        ++stats.repaired;
        return x;
    }
    void adjustPenalties() {
        int feasible=static_cast<int>(std::count(recent.begin(),recent.end(),0));
        const double fraction=static_cast<double>(feasible)/recent.size();
        int m=static_cast<int>(std::count(recent.begin(),recent.end(),1));
        int r=static_cast<int>(recent.size())-feasible-m;
        double oldM=penaltyM,oldR=penaltyR;
        if (!std::isfinite(p.endurance)) {
            if (fraction<0.15) penaltyM=std::min(8.0,penaltyM*1.1);
            if (fraction>0.25) penaltyM=std::max(3.0,penaltyM*0.9);
        } else {
            if(fraction<0.15) {
                if(r<m) penaltyM=std::min(8.0,penaltyM*1.1);
                else penaltyR=std::min(5.0,penaltyR*1.1);
            }
            if(fraction>0.25) {
                if(r<m) penaltyR=std::max(1.5,penaltyR*0.9);
                else penaltyM=std::max(3.0,penaltyM*0.9);
            }
        }
        // A penalty change invalidates every stored infeasible objective.
        if(oldM!=penaltyM || oldR!=penaltyR)
            for(int type=1;type<3;++type) for(auto& x:populations[type])
                x.decoded=join(context,x.genes,p.endurance,true,penaltyM,penaltyR);
    }
    void diversify() {
        rank();
        for(auto& pop:populations) {
            std::stable_sort(pop.begin(),pop.end(),[](const Individual& a,const Individual& b){ return a.fitness<b.fitness; });
            pop.resize(std::min(pop.size(),static_cast<std::size_t>(std::max(1,static_cast<int>(std::ceil(0.3*p.population))))));
        }
        fill(); ++stats.diversifications;
    }
    void escapeLocal() {
        const double original=best.decoded.cost;
        std::vector<Individual> buffer{best};
        for(int i=0;i<p.escapeIterations;++i) {
            const auto& source=buffer[pick(static_cast<int>(buffer.size()),rng)];
            auto genes=neighbor(context,source.genes,source.decoded,pick(22,rng),p.closeFraction,rng);
            if(std::any_of(buffer.begin(),buffer.end(),[&](const Individual& x){return x.genes==genes;})) continue;
            auto candidate=evaluate(std::move(genes));
            if(candidate.type) continue;
            const bool better=improves(candidate.decoded.cost,best.decoded.cost);
            const bool close=best.decoded.cost>0 && (candidate.decoded.cost-best.decoded.cost)/best.decoded.cost<p.escapeGap;
            if(!better && !close) continue;
            remember(candidate);
            if(buffer.size()<static_cast<std::size_t>(p.bufferSize)) buffer.push_back(std::move(candidate));
            else *std::max_element(buffer.begin(),buffer.end(),[](const Individual& a,const Individual& b){return a.decoded.cost<b.decoded.cost;})=std::move(candidate);
        }
        for(auto& x:buffer) if(improves(x.decoded.cost,original)) add(std::move(x));
        ++stats.escapes;
    }
public:
    Search(const Context& c,const Parameters& parameters,std::uint32_t seed,bool useEscape,
           Statistics& statistics,const std::vector<int>& tour)
        : context(c),p(parameters),rng(seed),escape(useEscape),stats(statistics) {
        base=evaluate(partition(context,tour,p.endurance));
        best=base;
        populations[0].push_back(base);
    }
    solution::Solution run() {
        // No search dimensions for an empty or single-customer problem.
        if(context.size()<=2) return toSolution(context,best.genes,best.decoded);
        fill();
        int stagnant=0;
        while(stagnant<p.noImprovement) {
            const double before=best.decoded.cost;
            auto all=rank();
            // Explicit sequencing keeps RNG consumption stable across compilers.
            auto a=parent(all), b=parent(all);
            auto genes=crossover(a,b,static_cast<Crossover>(pick(4,rng)),rng);
            if(unit(rng)<p.mutationProbability) genes=mutate(std::move(genes),rng);
            auto child=evaluate(std::move(genes));
            recent.push_back(child.type); if(recent.size()>100) recent.pop_front();
            if(child.type) {
                ++stats.infeasibleOffspring;
                if(unit(rng)<p.repairProbability) child=repair(std::move(child));
            }
            if(!child.type) child=localSearch(std::move(child));
            add(std::move(child)); adjustPenalties();
            ++stats.iterations;
            stagnant=improves(best.decoded.cost,before) ? 0 : stagnant+1;
            if(stagnant && stagnant%p.diversifyAfter==0 && stagnant<p.noImprovement) diversify();
            if(improves(best.decoded.cost,before)) stagnant=0;
            if(escape && stagnant && stagnant%p.escapeAfter==0 && stagnant<p.noImprovement) escapeLocal();
            if(improves(best.decoded.cost,before)) stagnant=0;
        }
        // Decode the incumbent strictly again; no penalized state reaches output.
        return toSolution(context,best.genes,join(context,best.genes,p.endurance));
    }
};
}
HGATAC::HGATAC(std::uint32_t seed,bool escape,Parameters parameters)
    : seed_(seed),escape_(escape),parameters_(parameters) {
    const auto& p=parameters_;
    auto probability=[](double v){return std::isfinite(v) && v>=0 && v<=1;};
    if(p.population<1 || p.offspring<1 || p.tournament<1 || p.noImprovement<1 ||
       p.diversifyAfter<1 || p.escapeAfter<1 || p.escapeIterations<1 || p.bufferSize<1 ||
       !probability(p.mutationProbability) || !probability(p.repairProbability) ||
       !probability(p.closeFraction) || p.closeFraction==0 ||
       !std::isfinite(p.escapeGap) || p.escapeGap<0 || std::isnan(p.endurance) || p.endurance<=0 ||
       p.population>100000 || p.offspring>100000)
        throw std::invalid_argument("Parametros invalidos para HGA-TAC.");
}
solution::Solution HGATAC::Optimize(::tspd::tspd::TSPD&) {
    throw std::logic_error("HGA-TAC requer TSP previo. Use Solve(instance, tspSolution).");
}
solution::Solution HGATAC::Optimize(::tspd::tspd::TSPD& instance,const solution::Solution& tspSolution) {
    const auto& tour=requireTspRoute(instance,tspSolution);
    statistics_={};
    detail::Context context(instance);
    Search search(context,parameters_,seed_,escape_,statistics_,tour);
    return search.run();
}
}
