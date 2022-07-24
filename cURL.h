/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   cURL.h
 * Author: wangh275
 *
 * Created on February 19, 2020, 5:43 PM
 */
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include "LatLon.h"
#ifndef CURL_H
#define CURL_H
using namespace std;
using boost::property_tree::ptree;
using boost::property_tree::read_json;

typedef struct webDataStruct {
    char *url = nullptr;
    unsigned int size = 0;
    char *response = nullptr;
} webDataStruct;

size_t write_data(void *buffer, size_t size, size_t nmemb, void *userp);

void curlInit();


void curlEnd();

string getWeatherIcon(ptree &ptRoot);

void getHighLowTemp(ptree & ptRoot, double & high, double & low);

string getWeatherInfo(LatLon position, double & tempHigh, double & tempLow);


double toCelsius(double f);
#endif /* CURL_H */

