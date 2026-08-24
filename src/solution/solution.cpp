#include "solution.hpp"

namespace tspd::solution{
    Solution::Solution(vector<int> truckNodes, 
                    vector<int> droneNodes,
                    int totalCost, int truckCost, int droneCost) : 
                    truckNodes_(truckNodes), droneNodes_(droneNodes),
                    totalCost_(totalCost), truckCost_(truckCost), 
                    droneCost_(droneCost) {};

    int Solution::getTotalCost() const {return totalCost_;}
    int Solution::getTruckCost() const {return truckCost_;}
    int Solution::getDroneCost() const {return droneCost_;}

    vector<int> Solution::getTruckNodes() const
        {return truckNodes_;}
    vector<int> Solution::getDroneNodes() const
        {return droneNodes_;}

}