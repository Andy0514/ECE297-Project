/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   segmentNode.h
 * Author: wangh275
 *
 * Created on January 27, 2020, 10:55 AM
 */

#pragma once
#include <vector>
#include "LatLon.h"
using namespace std;

struct segmentNode{
    int streetID = -1;
    int from, to;
    double travelTime;
    double length;
    double speedLimit;
    vector <LatLon> curvePoints;
    bool oneway;
//    bool isHighway;
    vector <pair<double, double>> curvePointsXY;
    //record the segment's angle by looking at its two nodes
    double angle; 
    bool streetNameDrawn;
    
};