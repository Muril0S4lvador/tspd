#include "solution.hpp"

namespace tspd::solution{
    Solution::Solution(vector<int> truckNodes, 
                    vector<int> droneNodes,
                    double totalCost, double truckCost, double droneCost) :  // Preserva custos fracionarios calculados pelo particionamento.
                    truckNodes_(truckNodes), droneNodes_(droneNodes),
                    totalCost_(totalCost), truckCost_(truckCost), 
                    droneCost_(droneCost) {};

    double Solution::getTotalCost() const {return totalCost_;} // Preserva custos fracionarios calculados pelo particionamento.
    double Solution::getTruckCost() const {return truckCost_;} // Preserva custos fracionarios calculados pelo particionamento.
    double Solution::getDroneCost() const {return droneCost_;} // Preserva custos fracionarios calculados pelo particionamento.

    vector<int> Solution::getTruckNodes() const
        {return truckNodes_;}
    vector<int> Solution::getDroneNodes() const
        {return droneNodes_;}

}
