#include "greedy_heuristic.hpp"


namespace tspd::greedyHeuristic{

    enum Label{
        simple,
        combined,
        truck, 
        drone
    };

    enum Operation{
        MakeFly,
        PushRight,
        PushLeft,
    }

    struct bestApplication{
        int currentNode;
        Operation op;
    }

    int GreedyHeuristic::_makeFlySavings(int currentNode, TSPD& instance){
        return instance.getDistance(currentNode, currentNode - 1)
                + instance.getDistance(currentNode, currentNode + 1);
                std::max(instance.getDistance(currentNode - 1, currentNode) + instance.getDistance(currentNode, currentNode + 1),
                        instance.getDistance(currentNode - 1, currentNode + 1));
    }

    int GreedyHeuristic::_pushLeftSavings(int currentNode, int droneCurrentNode, TSPD& instance){
        return instance.getDistance(currentNode - 1, currentNode) 
        + instance.getDistance(droneCurrentNode, currentNode - 1) 
        - instance.getDistance(droneCurrentNode, currentNode);
    }

    int GreedyHeuristic::_pushRightSavings(int currentNode, int droneCurrentNode, TSPD& instance){
        return instance.getDistance(currentNode + 1, currentNode) 
        + instance.getDistance(droneCurrentNode, currentNode + 1) 
        - instance.getDistance(droneCurrentNode, currentNode);
    }

    bool GreedyHeuristic::_anySimpleNode(vector<Label>& nodes){
        auto anyNode = std::find_if(nodes.begin(), nodes.end(), [](const Label& l){
            return l == Label::simple;
        });
        return anyNode != nodes.end();
    }

    void GreedyHeuristic::_greedyHeuristic(TSPD& instance, vector<int>& truckRoute, vector<int>& droneRoute){
        vector<Label> nodes (instance.getDimension(), Label::simple); 
        bestApplication ba;

        while(_anySimpleNode(nodes)){
            for(int i = 0; i < instance.getDimension() - 1; i++){
                int makeFlySavings = _makeFlySavings(i, instance);
                int pushLeftSavings = _pushLeftSavings(i, d, instance);
                int pushRightSavings = _pushRightSavings(i, d, instance);
                
            }
        }
        
    }

    Solution GreedyHeuristic::getGreedyHeuristicSolution(TSPD& instance){

        // Gera MST
        std::vector<node::Node> nodes = instance.getNodes();
        std::vector<edge::Edge> edges = instance.getEdges();
        vector<Edge> mst = Kruskal::kruskal(nodes, edges);

        // Fazemos um DFS
        vector<int> truckRoute = dfs::DFS::getDFS(nodes, mst);
        vector<int> droneRoute = {}

        // Aplicamos a heuristica greedy
        _greedyHeuristic(instance, truckRoute, droneRoute);


        return Solution{truckRoute, {}, 0, 0, 0};
    }
}