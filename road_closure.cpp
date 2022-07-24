#include <vector>
#include <iostream>
#include <string.h>
#include <curl/curl.h>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/foreach.hpp>
#include <string>
#include "globalData.h"
#include "closedRoad.h"
#include "road_closure.h"


using namespace std;
using boost::property_tree::ptree;
using boost::property_tree::read_json;


size_t write_road_data(void *buffer, size_t /*size*/, size_t nmemb, void *userp) {
    if (buffer && nmemb && userp) {
        MyCustomStruct *pMyStruct = (MyCustomStruct *)userp;

        // Writes to struct passed in from main
        if (pMyStruct->response == nullptr) {
            // Case when first time write_data() is invoked
            pMyStruct->response = new char[nmemb + 1];
            strncpy(pMyStruct->response, (char *)buffer, nmemb);
        }
        else {
            // Case when second or subsequent time write_data() is invoked
            char *oldResp = pMyStruct->response;

            pMyStruct->response = new char[pMyStruct->size + nmemb + 1];

            // Copy old data
            strncpy(pMyStruct->response, oldResp, pMyStruct->size);

            // Append new data
            strncpy(pMyStruct->response + pMyStruct->size, (char *)buffer, nmemb);

            delete []oldResp;
        }

        pMyStruct->size += nmemb;
        pMyStruct->response[pMyStruct->size] = '\0';
    }

    return nmemb;
}



void getRoadClosureInfo(ptree &ptRoot) {
//    string roadName;
//    string type;
//    double longitude = 0, latitude = 0;
      BOOST_FOREACH(ptree::value_type &featVal, ptRoot.get_child("Closure")) {
        // "features" maps to a JSON array, so each child should have no name
        if ( !featVal.first.empty() )
            throw "\"features\" child node has a name";
        closedRoad road;
        road.latitude=featVal.second.get<double>("latitude");
        road.longitude = featVal.second.get<double>("longitude");
        road.type = featVal.second.get<string>("type");
        road.road = featVal.second.get<string>("road");
        closureInfo.push_back(road);

    }

    return;
}

int load_road_closure(){

    CURL *curlHandle = curl_easy_init();
    if ( !curlHandle ) {
        cout << "ERROR: Unable to get easy handle" << endl;
        return 0;
    } else {
        char errbuf[CURL_ERROR_SIZE] = {0};
        MyCustomStruct myStruct;
        char targetURL[] = "http://app.toronto.ca/opendata/cart/road_restrictions.json";

        CURLcode res = curl_easy_setopt(curlHandle, CURLOPT_URL, targetURL);
        if (res == CURLE_OK)
            res = curl_easy_setopt(curlHandle, CURLOPT_ERRORBUFFER, errbuf);
        if (res == CURLE_OK)
            res = curl_easy_setopt(curlHandle, CURLOPT_WRITEFUNCTION, write_road_data);
        if (res == CURLE_OK)
            res = curl_easy_setopt(curlHandle, CURLOPT_WRITEDATA, &myStruct);

        myStruct.url = targetURL;

        if (res != CURLE_OK) {
            cout << "ERROR: Unable to set libcurl option" << endl;
            cout << curl_easy_strerror(res) << endl;
        } else {
            res = curl_easy_perform(curlHandle);
        }

        if (res == CURLE_OK) {
            // Create an empty proper tree
            ptree ptRoot;

            /* Store JSON data into a Property Tree
             *
             * read_json() expects the first parameter to be an istream object,
             * or derived from istream (e.g. ifstream, istringstream, etc.).
             * The second parameter is an empty property tree.
             *
             * If your JSON data is in C-string or C++ string object, you can
             * pass it to the constructor of an istringstream object.
             */
            istringstream issJsonData(myStruct.response);
            read_json(issJsonData, ptRoot);


            try {
                getRoadClosureInfo(ptRoot);
            } catch (const char *errMsg) {
                cout << "ERROR: Unable to fully parse the JSON data" << endl;
                cout << "Thrown message: " << errMsg << endl;
            }

        } else {
            cout << "ERROR: res == " << res << endl;
            cout << errbuf << endl;
        }

        if (myStruct.response)
            delete []myStruct.response;

        curl_easy_cleanup(curlHandle);
        curlHandle = nullptr;
    }


    return 0;
}

