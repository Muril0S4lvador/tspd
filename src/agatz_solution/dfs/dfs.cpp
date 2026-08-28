#include "dfs.hpp"

namespace tspd::dfs{

    vector<vector<int>> DFS::_getAdjacencyList(vector<Node>& nodes, vector<Edge>& edges){
        vector<vector<int>> adj(nodes.size());

        for(const Edge& e : edges){
            adj[e.getAId() - 1].push_back(e.getBId());
            adj[e.getBId() - 1].push_back(e.getAId());
        }

        for (auto& neighbors : adj)
            std::sort(neighbors.begin(), neighbors.end());


        return adj;
    }

    void DFS::_recurseDFS(int nodeId, vector<bool>& visited, vector<int>& path, vector<vector<int>>& adjacencyList){
        visited[nodeId - 1] = true;

        path.push_back(nodeId);

        for(int& neighbor : adjacencyList[nodeId - 1])
            if(!visited[neighbor - 1])
                _recurseDFS(neighbor, visited, path, adjacencyList);
    }

    vector<int> DFS::getDFS(vector<Node>& nodes, vector<Edge>& edges){
        vector<bool> visited(nodes.size(), false);

        vector<vector<int>> adjacencyList = _getAdjacencyList(nodes, edges);

        vector<int> path = {};
        _recurseDFS(1, visited, path, adjacencyList);
        
        return path;
    }
}


/*

DFS(G, inicio):
    para cada vértice v de G:
        visitado[v] ← falso

    para cada vértice v de G:
        ordenar G[v] em ordem crescente

    DFS_VISITA(inicio)


DFS_VISITA(u):
    visitado[u] ← verdadeiro
    imprimir u

    para cada v em G[u]:
        se visitado[v] = falso:
            DFS_VISITA(v)

*/