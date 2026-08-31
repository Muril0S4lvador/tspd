#include "greedy_heuristic.hpp"

namespace tspd::greedyHeuristic{
    Solution GreedyHeuristic::greedyHeuristic(TSPD& instance){
    // tspd::solution::Solution s = tspd::kruskal::Kruskal::kruskal(nodes, edges);
std::cout << "criando solucao gulosa ui\n\n";
        // Gera MST
        std::vector<node::Node> nodes = instance.getNodes();
        std::vector<edge::Edge> edges = instance.getEdges();
        vector<Edge> mst = Kruskal::kruskal(nodes, edges);

        // Fazemos um DFS
        vector<int> truckRoute = dfs::DFS::getDFS(nodes, mst);

        std::cout << '\n';
        for(int& i : truckRoute)
            std::cout << i << ' '; 
        std::cout << '\n';

        // Aplicamos a heuristica greedy
        return Solution{truckRoute, {}, 0, 0, 0};
    }
}