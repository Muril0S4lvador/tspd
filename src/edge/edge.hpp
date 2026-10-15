#pragma once

#include <cstddef>
#include <vector>

namespace tspd::edge {
    class Edge {
        private:
            int idA_;
            int idB_;
            double weight_; // Preserva a distancia euclidiana sem arredondar cada aresta.

        public:
            Edge(int idA, int idB, double weight); // Aceita pesos fracionarios desde a leitura.

            int getAId() const;
            int getBId() const;
            double getWeight() const; // Devolve o peso sem truncamento.

            static double convertWeight(double weight); // Valida o peso sem converter para inteiro.
            static std::size_t compactIndex(int dimension,
                                            int firstNodeId,
                                            int secondNodeId);
            static double getWeight(const std::vector<Edge>& edges, // Preserva decimais nas consultas do armazenamento compacto.
                                 int dimension,
                                 int firstNodeId,
                                 int secondNodeId);

            static void sortEdgesAscendingWeights(std::vector<Edge>& edges);
    };
}
