#pragma once

#include <vector>
using std::vector;

#include <iostream>

namespace tspd::solution{
    class Solution{
        private:
            vector<int> truckNodes_;
            vector<int> droneNodes_;
            int totalCost_;
            int truckCost_;
            int droneCost_;

        public:
            Solution(vector<int> truckNodes, 
                    vector<int> droneNodes,
                    int totalCost, int truckCost, int droneCost);

            int getTotalCost() const;
            int getTruckCost() const;
            int getDroneCost() const;

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