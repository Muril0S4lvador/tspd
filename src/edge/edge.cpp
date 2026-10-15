#include "edge.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <stdexcept>

namespace tspd::edge {
    Edge::Edge(int idA, int idB, double weight) // Armazena o peso real em vez de receber um inteiro.
        : idA_(std::min(idA, idB)),
          idB_(std::max(idA, idB)),
          weight_(weight) {
        if (idA < 0 || idB < 0 || idA == idB)
            throw std::invalid_argument("An edge must connect two distinct nonnegative node ids");
        if (!std::isfinite(weight) || weight < 0.0) // Rejeita tambem NaN e infinito em pesos decimais.
            throw std::invalid_argument("An edge weight cannot be negative");
    }

    int Edge::getAId() const { return idA_; }
    int Edge::getBId() const { return idB_; }
    double Edge::getWeight() const { return weight_; } // Nao descarta a parte fracionaria.

    double Edge::convertWeight(double weight) { // Mantem a validacao usada pelo leitor sem arredondamento.
        if (!std::isfinite(weight) || weight < 0.0)
            throw std::invalid_argument("Invalid edge weight");

        return weight; // Mantem a distancia euclidiana original para comparar com a referencia.
    }

    std::size_t Edge::compactIndex(int dimension,
                                   int firstNodeId,
                                   int secondNodeId) {
        if (dimension <= 0 || firstNodeId < 0 || secondNodeId < 0 ||
            firstNodeId >= dimension || secondNodeId >= dimension ||
            firstNodeId == secondNodeId) {
            throw std::out_of_range("Invalid node ids for compact distance storage");
        }

        std::size_t first = static_cast<std::size_t>(firstNodeId);
        std::size_t second = static_cast<std::size_t>(secondNodeId);
        if (first > second)
            std::swap(first, second);

        const std::size_t nodeCount = static_cast<std::size_t>(dimension);
        return first * (2 * nodeCount - first - 1) / 2 + second - first - 1;
    }

    double Edge::getWeight(const std::vector<Edge>& edges, // Preserva o peso real ao consultar uma aresta.
                        int dimension,
                        int firstNodeId,
                        int secondNodeId) {
        if (dimension <= 0 || firstNodeId < 0 || secondNodeId < 0 ||
            firstNodeId >= dimension || secondNodeId >= dimension) {
            throw std::out_of_range("Invalid node ids for compact distance storage");
        }
        if (firstNodeId == secondNodeId)
            return 0;

        const std::size_t index = compactIndex(
            dimension, firstNodeId, secondNodeId);
        if (index >= edges.size())
            throw std::out_of_range("Edge is not available");

        return edges[index].getWeight();
    }

    void Edge::sortEdgesAscendingWeights(std::vector<Edge>& edges){
        std::sort(edges.begin(), edges.end(), [](const Edge& a, const Edge& b) {
            return a.getWeight() < b.getWeight(); 
        });
    }
}
