/* 
 * Copyright 2020 University of Toronto
 *
 * Permission is hereby granted, to use this software and associated 
 * documentation files (the "Software") in course work at the University 
 * of Toronto, or for personal use. Other uses are prohibited, in 
 * particular the distribution of the Software either publicly or to third 
 * parties.
 *
 * The above copyright notice and this permission notice shall be included in 
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR 
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, 
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE 
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER 
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include "m1.h"
#include "StreetsDatabaseAPI.h"
#include "OSMDatabaseAPI.h"

#include "OSMWayNode.h"
#include <string>
#include "LatLon.h"
#include <cmath>
#include <vector>
#include <set>
#include <algorithm>
#include <bits/stdc++.h>
#include <unordered_map>
#include <map>
#include <algorithm>
#include <limits>
#include <thread>

#include "loadMapDataStructure.h"
#include "calculations.h"
#include "road_closure.h"

#include "streetNode.h"
#include "intersecNode.h"
#include "featureNode.h"
#include "POINode.h"
#include "segmentNode.h"
#include "globalData.h"



using namespace std;


/******************Data Structure Declaration******************/

    //the following data structures allow accessing an OSM way using its OSMID, rather than index
 unordered_map <OSMID, const OSMWay> OSMWayData; 
 unordered_map <OSMID, const OSMNode *> OSMNodeData; 
 unordered_map <OSMID, const OSMRelation> OSMRelationData;

    //streetDatabase data structure stores information on the street segments and intersections on a street. index by Street ID
 vector<streetNode> streetDatabase;
 vector<vector<featureNode>> featureDatabase;
 vector<POINode> POIDatabase;
 vector<OSMWay> specialWay;
 vector<OSMRelation> subwayLines;
 vector<TypedOSMID> subwayStation;
 vector<pair<string, LatLon>> subwayLineName;
 map<string, LatLon> subwayStationInfo;
    //intersectionGraph stores information on each intersection - number of segments and the indices of these segments. Index by intersection ID
 vector<intersecNode> intersectionGraph; 

    //segmentDatabase stores distance and speed limit information for each street segment, indexed by its segment ID
 vector<segmentNode> segmentDatabase; 
 vector<OSMWay> OSMWayDatabase;
     //this data structure arranges street names into bins based on the first character.
     //The actual data are sorted for speedy access in the function "find_street_ids_from_partial_street_name"
 unordered_map<int, vector<pair<string, int>>> sortedStreetName;
 
 //Toronto road closure
 vector<closedRoad> closureInfo;
 

/******************End Data Structure Declaration******************/
void clearDatabases();

/******************load_map******************/
    //loads map data from the two given API's, and initializes the databases
bool load_map(string mapPath) {
    

    //attempts to load
    bool loadSuccessful = load_dotBin_files(mapPath);
    if (!loadSuccessful) return false;
    

    int numberOfStreets = getNumStreets();
    int numberOfIntersections = getNumIntersections();
    int numberOfSegments = getNumStreetSegments();
    int numberOfFeatures = getNumFeatures();
    int numberOfPOIs = getNumPointsOfInterest();
    
    
    
    //provides initial size allocation for databases to prevent memory copying
    segmentDatabase.resize(numberOfSegments);
    streetDatabase.resize(numberOfStreets + 10000);
    intersectionGraph.reserve(numberOfIntersections);
    OSMNodeData.reserve(getNumberOfNodes());
    OSMWayData.reserve(getNumberOfWays());

    
    //loads the OSMWayData database, which supports finding an OSM Way from its 
    //OSMID, rather than using the index
    loadOSMDatabases();

    //loads data structure that contains intersections
    loadIntersectionGraph(numberOfIntersections);

    //stores all the street names and their indices, useful for loading sortedStreetName structure later
    vector <pair<string, int>> tempArrayOfStreetNames;

     //load streetDatabase, segmentDatabase, and above "tempArrayOfStreetNames"
    loadStreetDatabases(numberOfSegments, numberOfStreets, tempArrayOfStreetNames);
   

    //get road closure info for Toronto
    load_road_closure();
    loadSortedStreetName(tempArrayOfStreetNames);
    tempArrayOfStreetNames.clear();
    tempArrayOfStreetNames.shrink_to_fit();
    
    thread featureData(loadFeatureDatabases, numberOfFeatures);
    thread POIData(loadPOIDatabases, numberOfPOIs);
    //wait for all the threads to merge back before returning
    featureData.join();
    POIData.join();

    
    return true;
}
/******************end load map******************/


void close_map() {
    //Clean-up your map related data structures here
    if (streetDatabase.size() != 0){
        clearDatabases();
    }
    thread t1(closeOSMDatabase);
    thread t2(closeStreetDatabase);
    t1.join();
    t2.join();
    
}

//Implemented by Zichun Huang & Haolin Wang
//calculate the distance between two points using provided formula
double find_distance_between_two_points(std::pair<LatLon, LatLon> points){
    double latAvg = (points.first.lat() + points.second.lat())/2 * DEGREE_TO_RADIAN;  //must convert first to radian
    double x1 = points.first.lon() * cos(latAvg) * DEGREE_TO_RADIAN ;
    double x2 = points.second.lon() * cos(latAvg) * DEGREE_TO_RADIAN ;
    double y1 = points.first.lat() * DEGREE_TO_RADIAN;
    double y2 = points.second.lat() * DEGREE_TO_RADIAN;
    
    return EARTH_RADIUS_METERS * sqrt(pow(y2 - y1,2)+pow(x2 - x1,2)); //formula provided by handout
}

//Implemented by Zichun Huang
//Find closest intersection by traversing through all intersections and compare 
//their distances
int find_closest_intersection(LatLon my_position){
    int temp, index = 0;
    int numIntersection = getNumIntersections(); //find total number of intersections
    int closestDistance = find_distance_between_two_points(make_pair(getIntersectionPosition(0), my_position)); 
    for(int i = 1; i < numIntersection; i++){ //traverse through all intersections by index
        temp = find_distance_between_two_points(make_pair(getIntersectionPosition(i), my_position)); //find distance between given point and the intersection
        if(temp < closestDistance){
            closestDistance = temp;
            index = i; //keep the index unless closer distance found
        }
    }
    return index;
}

/******************Functions that use segmentDatabase******************/

//Implemented by Haolin Wang
//Due to time constraint, all finding and calculations are done in load_map and
//this function returns the length stored in segmentDatabase
double find_street_segment_length(int street_segment_id){
    return segmentDatabase[street_segment_id].length;
}

//Implemented by Haolin Wang
//Returns the travel time to drive a street segment in seconds by directly accessing segmentDatabase
//(time = distance/speed_limit) * 3.6 (time unit conversion)
double find_street_segment_travel_time(int street_segment_id){
    return (segmentDatabase[street_segment_id].length*3.6)/(segmentDatabase[street_segment_id].speedLimit);
}

/******************end functions that use segmentDatabase******************/




/******************Functions that use intersectionGraph******************/

//Implemented by An Yu
//Returns the street segments for the given intersection 
vector<int> find_street_segments_of_intersection(int intersection_id){
    //uses data structure intersectionGraph
    return intersectionGraph[intersection_id].segment;
}

//Implemented by An Yu
//Find the name of the street by intersection id
vector<string> find_street_names_of_intersection(int intersection_id){
    //vector containing names of the street
    vector<string> result;
    //get all adjacent IDs
    vector<int> segment_ids=find_street_segments_of_intersection(intersection_id);
    //loop through to find all the names, and push them in to the result vector
    for(int i=0; i<segment_ids.size(); i++){
        StreetIndex strid=getInfoStreetSegment(segment_ids[i]).streetID;
        string name=getStreetName(strid);
        result.push_back(name);
    }
    return result;
}

//Implemented by Zichun Huang
//Check if two intersections are directly connected
bool are_directly_connected(pair<int, int> intersection_ids){
    if(intersection_ids.first == intersection_ids.second) //if two ids are the same, they are connected
        return true;
    vector<StreetSegmentIndex> street_around = intersectionGraph[intersection_ids.first].segment; 
    
    for(int i = 0; i < street_around.size(); i++){ // if two intersections are within a pair, they are connected
        InfoStreetSegment current = getInfoStreetSegment(street_around[i]);
        if(current.to == intersection_ids.second)
            return true;
        else if(current.from == intersection_ids.second && current.oneWay == 0)
            return true;
        else 
            continue;
    }
    return false;
}

// Implemented by Zichun Huang
// Find adjacent intersections but not including street direction that cannot be
// passed (one-way)
vector<int> find_adjacent_intersections(int intersection_id){
    bool repeat = 0;
    vector<StreetSegmentIndex> adjacent_street_id = intersectionGraph[intersection_id].segment; //find connected street segments
    vector<StreetSegmentIndex> adjacent_intersec_id = {};
    for(int i = 0; i < adjacent_street_id.size(); i++){
        repeat = 0;
        InfoStreetSegment current = getInfoStreetSegment(adjacent_street_id[i]);
        if(current.from == intersection_id) //if segment 'from' is current, then it is a valid adjacent intersection
            adjacent_intersec_id.push_back(current.to);
        else if(current.to == intersection_id && current.oneWay == 0){//if segment 'to' is current, and it is not one-way, then it is a valid adjacent intersection
            if(adjacent_intersec_id.size() != 0){
                for(int j = 0; j < adjacent_intersec_id.size(); j++){
                    if(current.from == adjacent_intersec_id[j]){
                        repeat = 1;
                        break;
                    }
                }
            }
            if(!repeat)
                adjacent_intersec_id.push_back(current.from);
        }
    }
    
    //sorts all intersections and erases repeating entries using "unique"
    sort(adjacent_intersec_id.begin(), adjacent_intersec_id.end());
    adjacent_intersec_id.erase(unique(adjacent_intersec_id.begin(), adjacent_intersec_id.end()), adjacent_intersec_id.end()); 
    
    return adjacent_intersec_id;
}
/******************End functions that use intersectionGraph******************/






/******************Functions that use streetDatabase******************/
//implemented by Haolin Wang
vector<int> find_street_segments_of_street(int street_id){
    //directly access streetDatabase for readily available data
    return streetDatabase[street_id].streetSegments;
    
}

//implemented by Haolin Wang
vector<int> find_intersections_of_street(int street_id){
    //direct accessing
    return streetDatabase[street_id].intersections;
    
}
//Implemented by Haolin Wang
vector<int> find_intersections_of_two_streets(pair<int, int> street_ids){
    vector<int> result;
    result.reserve(4);
    
    //the intersections are already in sorted order
    vector<int> firstStreetIntersection = find_intersections_of_street(street_ids.first);
    vector<int> secondStreetIntersection = find_intersections_of_street(street_ids.second);

    //Compare the two vectors element-by-element. If one element is smaller than the other then increment the smaller one.
    auto iter1 = firstStreetIntersection.begin(), iter2 = secondStreetIntersection.begin();
    while (iter1 != firstStreetIntersection.end() && iter2 != secondStreetIntersection.end()){
        if (*iter1 == *iter2) {
            result.push_back(*iter1);
            iter1++;
            iter2++;
        } else if (*iter1 > *iter2){
            iter2++;
        } else{
            iter1++;
        }
    }
    return result;
}
/******************End functions that use streetDatabase******************/






/******************Functions that use sortedStreetNames******************/
//Implemented by Haolin Wang & Zichun Huang
vector<int> find_street_ids_from_partial_street_name(string street_prefix){
    
    vector<int> street_id= {};
    if(street_prefix == "")
        return street_id;
    
    //parses the input street_prefix. Erase all white spaces using "erase", then convert
    //everything to lower case using "transform"
    street_prefix.erase(remove_if(street_prefix.begin(), street_prefix.end(), ::isspace), street_prefix.end()); 
    std::transform(street_prefix.begin(), street_prefix.end(), street_prefix.begin(),::tolower); 

    //identify the "bin" to search in by indexing into sortedStreetName using street_prefix's first character
    //then search for the street name within that bin
    for (auto it = sortedStreetName[(int)street_prefix[0]].begin(); it != sortedStreetName[(int)street_prefix[0]].end(); it++){
        
        string name = it->first;
        int streetPrefixLength = street_prefix.length();
        int nameLength = name.length();
        
        //compare 'street_prefix' with 'name' letter by letter: if a letter does not match then branch immediately
        int cont = 0;
        int i = 1;
        for (; i < nameLength && i < streetPrefixLength; i++){
            char prefixLetter = street_prefix[i];
            if (name[i] < prefixLetter) {
                cont = 1;
                break;
            }
           else if (name[i] > prefixLetter) return street_id;
        }
       
        if (cont == 1) continue;
        else if (i == streetPrefixLength) {
            //this means that there's a match
            street_id.push_back(it->second);
        }
        else continue;
    }
    return street_id;
}
/******************End functions that use sortedStreetNames******************/


//Implemented by An Yu
//Returns the area of the given closed feature in square meters
//Assume a non self-intersecting polygon (i.e. no holes)
//Return 0 if this feature is not a closed polygon.
double find_feature_area(int feature_id){
    int numPoint = getFeaturePointCount(feature_id);
    LatLon start = getFeaturePoint(0,feature_id);
    LatLon end = getFeaturePoint(numPoint-1,feature_id);
    //not closed polygon
    if(start.lat() != end.lat() || start.lon() != end.lon()){
        return 0;
    }
    //set coordinate system
    //get 10 points, find the left most Latitude, and lowest Longitude; this would give an approximation
    double latmin = std::numeric_limits<double>::infinity();
//    double latmax = (-1)*std::numeric_limits<double>::infinity();
    double latmax = 0;
    double lonmin = std::numeric_limits<double>::infinity();
//    double lonmax = (-1)*std::numeric_limits<double>::infinity();
    double lonmax = 0;
    
   //use for lop to find max min of lat and lon
    for(int i = 0;i < 9;i++){
        int index = numPoint/10*i;
        LatLon latlon = getFeaturePoint(index, feature_id);
        if(latlon.lat() < latmin){
            latmin = latlon.lat();
        }
        if(latlon.lat() > latmax){
            latmin = latlon.lat();
        }
        if(latlon.lon() < lonmin){
            lonmin = latlon.lon();
        }
        if(latlon.lon() > lonmax){
            lonmax = latlon.lon();
        }
        
    }
    //find approximate size of the feature
    double horizontal=latmax-latmin;
    double vertical=lonmax-lonmin;
    //set origin
    double originLat=latmin-horizontal/10;
    double originLon=lonmin-vertical/10;
    
    //calculate area using shoelace formula:
    double area=0.0;
    int j=numPoint-1;
    for(int i=0; i<numPoint; i++){
        double xj=calcLat(originLat, getFeaturePoint(j,feature_id));
        double xi=calcLat(originLat, getFeaturePoint(i,feature_id));
        double yj=calcLon(originLon, getFeaturePoint(j,feature_id));
        double yi=calcLon(originLon, getFeaturePoint(i,feature_id));
        area+=(xj+xi)*(yj-yi);
        j=i;
    }
    return abs(area/2.0);
}




/******************Functions that use OSMWayData, OSMNodeData******************/
//Implemented by Zichun Huang
//Find length of an OSM way by adding all distance between its nodes
double find_way_length(OSMID way_id){
    double result = 0;
    const OSMWay way = OSMWayData[way_id];
    const std::vector<OSMID> members = getWayMembers(&way); //retrieve all nodes in the way
    for(auto it = members.begin(); it != members.end()-1; it++){ //iterate through all nodes and add up all distances
        const OSMNode first = *(OSMNodeData[*it]); 
        const OSMNode second = *(OSMNodeData[*(it+1)]);
        LatLon one = getNodeCoords(&first);
        LatLon two = getNodeCoords(&second);
        result += find_distance_between_two_points(make_pair(one, two)); //add up distances between two OSM nodes
    }
    return result;
}
/******************End Functions that use OSMWayData, OSMNodeData******************/

