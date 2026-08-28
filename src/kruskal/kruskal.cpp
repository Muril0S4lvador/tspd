#include "kruskal.hpp"

namespace tspd::kruskal{

    void Kruskal::_addMST(vector<int>& mst, Node& u, Node& v){
        mst.push_back(u.getId());
        mst.push_back(v.getId());
    }

    void Kruskal::_setUnion(vector<int>& set, Node& u, Node& v){
        int uId = u.getId() - 1;
        int vId = v.getId() - 1;
        while(set[uId] >= 0 || set[vId] >= 0){
            if(set[uId] >= 0) uId = set[uId];
            if(set[vId] >= 0) vId = set[vId];
        }

        int minIdxNode = std::min(uId, vId);
        int maxIdxNode = std::max(uId, vId);

        if(minIdxNode == 6 && maxIdxNode == 11)
            minIdxNode = minIdxNode;


        int head = set[maxIdxNode];
        while(head > 0){
            set[maxIdxNode] = set[set[maxIdxNode]];
            head = set[maxIdxNode];
        }

        set[minIdxNode] += set[maxIdxNode]; // Head aumenta o tamanho
        set[maxIdxNode] = minIdxNode; // Antiga Head vira corpo
        
        // if (isAlone) return; // Early return para não rodar desnecessariamente

        // for(int& i : set){
        //     if(i == maxIdxNode){
        //         i = minIdxNode;
        //         set[minIdxNode]--;
        //     }
        // }

        // std::cout << "\n\nEdges: ";
        // for(int& i : set)
        //     std::cout << i << ' ';
    }

    bool Kruskal::_findUnion(vector<int>& set, int uId, int vId){
        int u = uId - 1, v = vId - 1;
        bool res = false;

        while(set[u] >= 0 || set[v] >= 0){

            if(set[u] >= 0) u = set[u];
            if(set[v] >= 0) v = set[v];
        }

        // if(uId == 2 && vId == 11)
        //     std::cout << "Cheguei ao FIMMM\n\n";

        if(u == v)
            res = true;

        // if(set[set[u]] > 0) set[u] = set[set[u]];
        // if(set[set[v]] > 0) set[v] = set[set[v]];

        // std::cout << "\nFind Union: Nodes " << uId << " and " << vId << " are " << (!res ? "not " : "") << "conected.\n";
        return res;
    }

    Solution Kruskal::kruskal(vector<Node>& nodes, vector<Edge>& edges){
        vector<int> mst{};

        // Negativos representam tamanho e positivo representa o pai
        vector<int>set(nodes.size(), -1);

        Edge::sortEdgesAscendingWeights(edges);

        // for(const auto& e : edges)
        //     std::cout << "A: " << e.getAId() << " - " << e.getBId() << " : " << e.getWeight() << '\n';

        for(const auto& e : edges){
            if( !_findUnion(set, e.getAId(), e.getBId()) )
            {
                auto itUNode = std::find_if(nodes.begin(), nodes.end(), [id = e.getAId()](const Node& n){
                    return n.getId() == id;
                });
                auto itVNode = std::find_if(nodes.begin(), nodes.end(), [id = e.getBId()](const Node& n){
                    return n.getId() == id;
                });

                if(itUNode == nodes.end() || itVNode == nodes.end())
                    exit(-1);

                Node u = *itUNode, 
                    v = *itVNode;

                _addMST(mst, u, v);
                _setUnion(set, u, v);
            }
        }

        // for(const auto& i : mst){
        //     std::cout << i << ' ';
        // }

        return Solution(mst, {}, 0, 0, 0);
    }
}