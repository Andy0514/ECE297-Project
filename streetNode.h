/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   streetNode.h
 * Author: wangh275
 *
 * Created on January 25, 2020, 5:32 PM
 */
#pragma once
#include <vector>
#include <string>

struct streetNode{
    
    streetNode(): streetName("NO_NAME"){
        //reserve some amount of space in each vector to prevent pushback
        intersections.reserve(20);
        streetSegments.reserve(20);
        highway = 0;
    }
    //streetName contains the street's name, all lower cases and space removed
    std::string streetName;
    bool highway;
    bool primary;
    bool tertiary;
    vector<int> streetSegments;
    vector<int> intersections;
    bool highlight = false;
    bool searchFound = false;
    int streetID;
};