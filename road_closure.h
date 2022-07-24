/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   road_closure.h
 * Author: yuan4
 *
 * Created on February 22, 2020, 7:47 PM
 */
#ifndef ROAD_CLOSURE_H
#define ROAD_CLOSURE_H
#include <curl/curl.h>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/foreach.hpp>


int load_road_closure();
size_t write_road_data(void *buffer, size_t size, size_t nmemb, void *userp);
void getRoadClosureInfo(boost::property_tree::ptree &ptRoot);

typedef struct MyCustomStruct {
    char *url = nullptr;
    unsigned int size = 0;
    char *response = nullptr;
} MyCustomStruct;


#endif /* ROAD_CLOSURE_H */

