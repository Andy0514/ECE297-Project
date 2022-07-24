#include <iostream>
#include <string.h>
#include <curl/curl.h>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/foreach.hpp>
#include <string>
#include <cstring>
#include "cURL.h"
#include "LatLon.h"
using namespace std;
using boost::property_tree::ptree;
using boost::property_tree::read_json;



void curlInit(){
    curl_global_init(CURL_GLOBAL_ALL);
}

void curlEnd(){
    curl_global_cleanup();
}

/* buffer holds the data received from curl_easy_perform()
 * size is always 1
 * nmemb is the number of bytes in buffer
 * userp is a pointer to user data (i.e. myStruct from main)
 *
 * Should return same value as nmemb, else it will signal an error to libcurl
 * and curl_easy_perform() will return an error (CURLE_WRITE_ERROR). This is
 * useful if you want to signal an error has occured during processing.
 */
size_t write_data(void *buffer, size_t /*size*/, size_t nmemb, void *userp) {
    if (buffer && nmemb && userp) {
        webDataStruct *pMyStruct = (webDataStruct *)userp;

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

/* Boost uses the following JSON to property tree mapping rules:
 *   1) JSON objects are mapped to nodes. Each property is a child node.
 *   2) JSON arrays are mapped to nodes.
 *      Each element is a child node with an empty name. If a node has both
 *      named and unnamed child nodes, it cannot be mapped to a JSON representation.
 *   3) JSON values are mapped to nodes containing the value.
 *      However, all type information is lost; numbers, as well as the literals
 *      "null", "true" and "false" are simply mapped to their string form.
 *   4) Property tree nodes containing both child nodes and data cannot be mapped.
 */
string getWeatherIcon(ptree &ptRoot) {
    BOOST_FOREACH(ptree::value_type &featVal, ptRoot.get_child("daily").get_child("data")) {
        // "features" maps to a JSON array, so each child should have no name

        string iconName = featVal.second.get<string>("icon");
        return iconName;
    }
    return "";
}

//returns the high and low temperature for the next day
void getHighLowTemp(ptree & ptRoot, double & high, double & low){
        BOOST_FOREACH(ptree::value_type &featVal, ptRoot.get_child("daily").get_child("data")) {
        // "features" maps to a JSON array, so each child should have no name

        high = toCelsius(stod(featVal.second.get<string>("temperatureHigh")));
        low = toCelsius(stod(featVal.second.get<string>("temperatureLow")));
    }
}


double toCelsius(double f){
    return (f-32)/1.8;
}


string getWeatherInfo(LatLon position, double & tempHigh, double & tempLow){
    
    string weatherToday;

    CURL *curlHandle = curl_easy_init();
    if ( !curlHandle ) {
        cout << "ERROR: Unable to get easy handle" << endl;
        return 0;
    } else {
        char errbuf[CURL_ERROR_SIZE] = {0};
        webDataStruct data;

        //format URL
        string targetURL = "https://api.darksky.net/forecast/5f2d2bfde8f622b6062cda7bcc33032e/";
        targetURL += to_string(position.lat()) + "," + to_string(position.lon()) + "?exclude=[currently,minutely,hourly,alerts,flags]"; 

        //convert to C string
        char * URL = new char [targetURL.length()+1];
        strcpy(URL, targetURL.c_str());

        CURLcode res = curl_easy_setopt(curlHandle, CURLOPT_URL, URL);
        if (res == CURLE_OK)
            res = curl_easy_setopt(curlHandle, CURLOPT_ERRORBUFFER, errbuf);
        if (res == CURLE_OK)
            res = curl_easy_setopt(curlHandle, CURLOPT_WRITEFUNCTION, write_data);
        if (res == CURLE_OK)
            res = curl_easy_setopt(curlHandle, CURLOPT_WRITEDATA, &data);

        data.url = URL;
        
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
            istringstream issJsonData(data.response);
            read_json(issJsonData, ptRoot);

            // Parsing and printing the data
            try {
                weatherToday = getWeatherIcon(ptRoot);
                getHighLowTemp(ptRoot, tempHigh, tempLow);
            } catch (const char *errMsg) {
                cout << "ERROR: Unable to fully parse the TTC JSON data" << endl;
                cout << "Thrown message: " << errMsg << endl;
            }

        } else {
            cout << "ERROR: res == " << res << endl;
            cout << errbuf << endl;
        }

        if (data.response)
            delete []data.response;
            
        curl_easy_cleanup(curlHandle);
        curlHandle = nullptr;
        delete [] URL;
    }
    
    
    return weatherToday;
}
