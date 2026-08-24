#include "kruskal.hpp"

namespace tspd::kruskal{

    void Kruskal::_addMST(vector<int>& mst, Node& u, Node& v){
        auto mstUIterator = std::find_if(mst.begin(), mst.end(), [id = u.getId()](const int& n){
            return n == id;
        });

        auto mstVIterator = std::find_if(mst.begin(), mst.end(), [id = v.getId()](const int& n){
            return n == id;
        });

        if(mstUIterator == mst.end()) mst.push_back(u.getId());
        if(mstVIterator == mst.end()) mst.push_back(v.getId());
    }

    void Kruskal::_setUnion(vector<int>& set, Node& u, Node& v){
        int minIdxNode = std::min(u.getId(), v.getId()) - 1;
        int maxIdxNode = std::max(u.getId(), v.getId()) - 1;

        bool isAlone = set[maxIdxNode] == -1 ? true : false;

        set[maxIdxNode] = minIdxNode; // Antiga Head vira corpo
        set[minIdxNode]--; // Head aumenta o tamanho
        
        if (isAlone) return; // Early return para não rodar desnecessariamente

        for(int& i : set){
            if(i == maxIdxNode){
                i = minIdxNode;
                set[minIdxNode]--;
            }
        }
    }

    Solution Kruskal::kruskal(vector<Node>& nodes, vector<Edge>& edges){
        vector<int> mst{};

        // Negativos representam tamanho e positivo representa o pai
        vector<int>set(nodes.size(), -1);

        Edge::sortEdgesAscendingWeights(edges);

        for(const auto& e : edges)
            std::cout << "A: " << e.getAId() << " - " << e.getBId() << " : " << e.getWeight() << '\n';

        for(const auto& e : edges){
            if( set[e.getAId() - 1] < 0 || set[e.getBId() - 1] < 0 )
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

        return Solution(mst, {}, 0, 0, 0);
    }
}