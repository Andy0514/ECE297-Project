/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   featureNode.h
 * Author: huan1386
 *
 * Created on February 13, 2020, 3:32 PM
 */
#pragma once
#include <vector>
#include "ezgl/graphics.hpp"
#include "StreetsDatabaseAPI.h"

using namespace std;

struct featureNode{
    
    featureNode(): featureName("NO_NAME"){
        //reserve some amount of space in each vector to prevent pushback
        featurePointCount = 0;
        featurePoint.reserve(50);
    }
    //streetName contains the street's name, all lower cases and space removed
    string featureName;
    FeatureType featureType;
    bool closed;
    int featurePointCount;
    TypedOSMID featureOSMID;
    vector <LatLon> featurePoint;
    vector <pair<double, double>> featurePointXY;
    vector <ezgl::point2d> fillPolyPoints;
    
    //center
    pair <double, double> center;
    //feature area
    double area;
    
    //parameters
    double xmin, xmax, ymin, ymax;
};

