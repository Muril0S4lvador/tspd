#include "hga_tac/HGATAC.hpp"
#include "hga_tac/join.hpp"
#include "instance_reader/instance_reader.hpp"
#include <algorithm>
#include <cmath>
#include <functional>
#include <iostream>
#include <numeric>
#include <random>
#include <stdexcept>

using namespace tspd::hga_tac;
using namespace tspd::hga_tac::detail;
using Instance = tspd::tspd::TSPD;
using Node = tspd::node::Node;
void check(bool b,const char* message) { if(!b) throw std::runtime_error(message); }
void near(double a,double b) {
    if(!std::isfinite(a) || !std::isfinite(b)) {check(a==b,"infinity mismatch");return;}
    check(std::abs(a-b)<1e-7*std::max(1.0,std::abs(b)),"independent cost mismatch");
}
template<class F> void rejects(F f) {
    try {f();} catch(const std::exception&) {return;}
    throw std::runtime_error("invalid input accepted");
}
Instance instance(int n,unsigned seed,double drone=0.5) {
    std::mt19937 rng(seed); std::uniform_real_distribution<double> xy(-10,10);
    std::vector<Node> nodes;
    for(int i=0;i<n;++i) nodes.emplace_back(xy(rng),xy(rng),i,std::to_string(i));
    return Instance("hga-fixture","TSP-D",n,1,drone,nodes.front(),nodes,
        tspd::instance_reader::InstanceReader::calculateEdges(nodes));
}
double distance(const Instance& x,int a,int b) {
    auto nodes=x.getNodes();
    return std::hypot(nodes[a].getX()-nodes[b].getX(),nodes[a].getY()-nodes[b].getY());
}
// Exhaustively enumerate ALL launch/recovery combinations, independently of
// Join's DP, and simulate chronological events directly from coordinates.
double exhaustive(const Instance& x,const Genes& genes,double range=INFINITY) {
    std::vector<int> truck{0},drone,boundary;
    for(int gene:genes) {
        if(gene>0) truck.push_back(gene);
        else {drone.push_back(-gene);boundary.push_back(static_cast<int>(truck.size())-1);}
    }
    truck.push_back(0);
    auto drive=[&](int a,int b){double t=0;for(int k=a;k<b;++k)t+=distance(x,truck[k],truck[k+1]);return t;};
    double best=INFINITY;
    std::function<void(int,int,double)> visit=[&](int f,int cursor,double time) {
        if(f==static_cast<int>(drone.size())) {best=std::min(best,time+drive(cursor,static_cast<int>(truck.size())-1));return;}
        int last=f+1<static_cast<int>(drone.size())?boundary[f+1]:static_cast<int>(truck.size())-1;
        for(int a=cursor;a<=boundary[f];++a) for(int b=boundary[f];b<=last;++b) {
            double air=(distance(x,truck[a],drone[f])+distance(x,drone[f],truck[b]))*x.getDroneSpeed();
            if(air<=range) visit(f+1,b,time+drive(cursor,a)+std::max(drive(a,b),air));
        }
    };
    visit(0,0,0);return best;
}
void simulate(const Instance& x,const Genes& genes,const Decoded& d) {
    check(std::isfinite(d.cost),"nonfinite output");
    std::vector<int> served(x.getDimension(),0);served[0]=1;
    for(std::size_t i=1;i+1<d.truck.size();++i) ++served[d.truck[i]];
    auto drive=[&](int a,int b){double t=0;for(int k=a;k<b;++k)t+=distance(x,d.truck[k],d.truck[k+1])*x.getTruckSpeed();return t;};
    int cursor=0;double total=0,airTotal=0;
    for(const auto& f:d.flights) {
        check(f.launch>=cursor && f.recovery>=f.launch,"overlapping flights");
        check(f.customers.size()==1,"multiple deliveries in output");
        int customer=f.customers.front();++served[customer];
        double air=(distance(x,d.truck[f.launch],customer)+distance(x,customer,d.truck[f.recovery]))*x.getDroneSpeed();
        total+=drive(cursor,f.launch)+std::max(drive(f.launch,f.recovery),air);
        airTotal+=air;cursor=f.recovery;
    }
    total+=drive(cursor,static_cast<int>(d.truck.size())-1);
    for(int count:served) check(count==1,"customer omitted or duplicated");
    near(d.cost,total);near(d.truckCost,drive(0,static_cast<int>(d.truck.size())-1));near(d.droneCost,airTotal);
    auto s=toSolution(Context(x),genes,d);
    near(*s.getTspCost(),x.calculateTspCost(s.getTspNodes()));
}
int main() {
    try {
        std::mt19937 rng(61);
        for(int n=1;n<=8;++n) for(int trial=0;trial<80;++trial) {
            auto x=instance(n,trial+17*n);Context c(x);
            Genes genes(n-1);std::iota(genes.begin(),genes.end(),1);std::shuffle(genes.begin(),genes.end(),rng);
            bool previous=false;
            for(int& g:genes) {if(!previous && rng()%2) g=-g;previous=g<0;}
            for(double range:{static_cast<double>(INFINITY),5.0,20.0}) {
                auto d=join(c,genes,range);near(d.cost,exhaustive(x,genes,range));
                if(std::isfinite(d.cost)) simulate(x,genes,d);
            }
            for(int op=0;op<4;++op) {
                auto b=genes;std::shuffle(b.begin(),b.end(),rng);
                validateGenes(c,crossover(genes,b,static_cast<Crossover>(op),rng));
            }
            validateGenes(c,mutate(genes,rng));
            auto d=join(c,genes,INFINITY);
            for(int op=0;op<22;++op) {
                auto moved=neighbor(c,genes,d,op,0.3,rng);validateGenes(c,moved);
                auto md=join(c,moved,INFINITY);
                if(std::isfinite(md.cost)) simulate(x,moved,md);
            }
        }
        // A single aerial customer can depart/return at a stationary truck.
        auto x=instance(2,123);Context c(x);auto stationary=join(c,{-1},INFINITY);
        near(stationary.cost,2*distance(x,0,1)*x.getDroneSpeed());
        check(stationary.flights.front().launch==stationary.flights.front().recovery,"stationary sortie absent");
        auto y=instance(4,31);Context cy(y);
        check(!std::isfinite(join(cy,{-1,-2,3},INFINITY).cost),"Type 1 accepted");
        check(std::isfinite(join(cy,{-1,-2,3},INFINITY,true).cost),"Type 1 not evaluated");
        check(join(cy,{-1,-2,3},INFINITY,true,4).cost>=join(cy,{-1,-2,3},INFINITY,true,2).cost,"penalty not monotonic");
        rejects([&]{join(cy,{1,1,3},INFINITY);});
        rejects([&]{join(cy,{0,2,3},INFINITY);});
        rejects([&]{join(cy,{1,2,3},0);});
        Parameters params;params.population=4;params.offspring=4;params.noImprovement=18;
        params.diversifyAfter=4;params.escapeAfter=7;params.escapeIterations=50;
        for(double range:{static_cast<double>(INFINITY),1.0}) for(int n:{1,2,5,9}) {
            auto model=instance(n,27);Context ctx(model);
            std::vector<int> tour(n);std::iota(tour.begin(),tour.end(),0);tour.push_back(0);
            tspd::solution::Solution tsp(tour,{},0,0,0);tsp.setTspRoute(tour,model.calculateTspCost(tour));
            params.endurance=range;
            HGATAC algorithm(42,true,params);auto result=algorithm.Solve(model,tsp);
            auto repeated=algorithm.Solve(model,tsp);
            near(result.getTotalCost(),repeated.getTotalCost());
            check(result.getTruckNodes()==repeated.getTruckNodes() && result.getDroneNodes()==repeated.getDroneNodes(),"seed not reproducible");
            check(result.getTotalCost()<=*tsp.getTspCost()+1e-8,"worse than all-truck seed");
            near(*result.getTspCost(),model.calculateTspCost(result.getTspNodes()));
            if(n>2) {check(algorithm.statistics().escapes>0,"escape not executed");check(algorithm.statistics().diversifications>0,"diversification not executed");}
            rejects([&]{algorithm.Solve(model);});
            tsp.setTspRoute(tour,-1);rejects([&]{algorithm.Solve(model,tsp);});
        }
        Parameters invalid;invalid.population=0;rejects([&]{HGATAC bad(0,false,invalid);});
        std::cout<<"HGA-TAC tests passed\n";return 0;
    } catch(const std::exception& error) {std::cerr<<error.what()<<'\n';return 1;}
}
