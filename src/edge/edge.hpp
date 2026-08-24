#pragma once

#include <cstddef>
#include <vector>

namespace tspd::edge {
    class Edge {
        private:
            int idA_;
            int idB_;
            int weight_;

        public:
            Edge(int idA, int idB, int weight);

            int getAId() const;
            int getBId() const;
            int getWeight() const;

            static int convertWeight(double weight);
            static std::size_t compactIndex(int dimension,
                                            int firstNodeId,
                                            int secondNodeId);
            static int getWeight(const std::vector<Edge>& edges,
                                 int dimension,
                                 int firstNodeId,
                                 int secondNodeId);

            static void sortEdgesAscendingWeights(std::vector<Edge>& edges);
    };
}
