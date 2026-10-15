#pragma once

#include <vector>
using std::vector;

#include <iostream>

namespace tspd::solution{
    class Solution{
        private:
            vector<int> truckNodes_;
            vector<int> droneNodes_;
            double totalCost_; // Preserva custos fracionarios calculados pelo particionamento.
            double truckCost_; // Preserva custos fracionarios calculados pelo particionamento.
            double droneCost_; // Preserva custos fracionarios calculados pelo particionamento.

        public:
            Solution(vector<int> truckNodes, 
                    vector<int> droneNodes,
                    double totalCost, double truckCost, double droneCost); // Preserva custos fracionarios calculados pelo particionamento.

            double getTotalCost() const; // Preserva custos fracionarios calculados pelo particionamento.
            double getTruckCost() const; // Preserva custos fracionarios calculados pelo particionamento.
            double getDroneCost() const; // Preserva custos fracionarios calculados pelo particionamento.

            vector<int> getTruckNodes() const;
            vector<int> getDroneNodes() const;
            
            
            // Sobrecarga de Operador <<
            friend std::ostream& operator<<(std::ostream& os, const Solution& s) {
                os << "There isn't a << operation defined for Solution class yet.\n" << s.getDroneCost();
                for(auto n : s.truckNodes_){
                    os << n;
                    os << ' ';
                }
                os << '\n';
                return os;
            }
    };
}
