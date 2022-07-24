/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   POINode.h
 * Author: huan1386
 *
 * Created on February 14, 2020, 10:00 AM
 */

#pragma once
#include <cstring>
using namespace std;

struct POINode{
    
    string POIType;
    string POIName;
    LatLon POIPosition;
    pair<double, double> POIPositionXY;
    OSMID POIOSMID;
    
    
};