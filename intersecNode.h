/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   intersecNode.h
 * Author: yuan4
 *
 * Created on January 23, 2020, 10:52 AM
 */

#ifndef INTERSECNODE_H
#define INTERSECNODE_H

#include "LatLon.h"
#include <vector>
#include "StreetsDatabaseAPI.h"

struct intersecNode{
    intersecNode(){
        pastIntersec.reserve(4);
    }
    LatLon position;
    double xPos, yPos;
    string name;
    //each intersection node contains a vector of street segment index that it connects
    std::vector <StreetSegmentIndex> segment;
    
    //number of streets it connects
    int numStreet;
    bool highlight = false;
    bool searchFound = false;
    double distance;
    double heuristics;
    std::vector <int> pastIntersec;
    int prevSeg;
    double walkingCost;
    int insertLimit;
    double pastTravelTime;
};

#endif /* INTERSECNODE_H */

