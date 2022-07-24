
#ifndef GLOBALDATA_H
#define GLOBALDATA_H

#include "m2.h"
#include "m1.h"
#include "m4.h"
#include "StreetsDatabaseAPI.h"
#include "OSMDatabaseAPI.h"
#include "LatLon.h"
#include "OSMWayNode.h"
#include "ezgl/rectangle.hpp"
#include "ezgl/application.hpp"

#include <string>
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
#include <stdlib.h>


#include "streetNode.h"
#include "intersecNode.h"
#include "segmentNode.h"
#include "calculations.h"
#include "featureNode.h"
#include "POINode.h"
#include "closedRoad.h"
#include "POIIcon.h"



/******************Data Structure Declaration******************/

//the following data structures allow accessing an OSM way using its OSMID, rather than index
extern unordered_map <OSMID, const OSMWay> OSMWayData; 
extern unordered_map <OSMID, const OSMNode*> OSMNodeData;
extern unordered_map <OSMID, const OSMRelation> OSMRelationData;

//streetDatabase data structure stores information on the street segments and intersections on a street. index by Street ID
extern vector<streetNode> streetDatabase;

//intersectionGraph stores information on each intersection - number of segments and the indices of these segments. Index by intersection ID
extern vector<intersecNode> intersectionGraph; 
extern vector<vector<featureNode>> featureDatabase;
extern vector <OSMWay> specialWay;


//segmentDatabase stores distance and speed limit information for each street segment, indexed by its segment ID
extern vector <segmentNode> segmentDatabase; 
extern vector <POINode> POIDatabase;
extern vector <OSMWay> OSMWayDatabase;


//this data structure arranges street names into bins based on the first character.
//The actual data are sorted for speedy access in the function "find_street_ids_from_partial_street_name"
extern unordered_map<int, vector<pair<string, int>>> sortedStreetName;

//databases required for displaying subway information
extern vector <OSMRelation> subwayLines;
extern vector <pair<string, LatLon>> subwayLineName;
extern map <string, LatLon> subwayStationInfo;


//stores the list of icons currently displayed
extern vector <POIIcon> displayedPOIIcon;

//stores currently clicked icon
extern struct POIIcon clickedPOI;

extern vector<closedRoad> closureInfo;

extern vector<int> drivePath;
extern vector <int> walkPath;
extern vector <CourierSubpath> courierPath;


extern vector <int>walkSearchedSegments;
extern ezgl::renderer * a;
extern ezgl::application* appli;

extern double walkingSpeed, walkingTime;

/******************End Data Structure Declaration******************/


/**************GLOBAL VARIABLES DECLARATION*********************/
//controls the screen characteristics
extern double maxLat, maxLon, minLat, minLon;
extern ezgl::rectangle bounds, screenBounds;
extern double screenToWorldRatioX;
extern double screenToWorldRatioY;


//variables for loading screen
extern string mapName;
extern bool mapLoaded;


//boolean that controls what to display
extern bool restaurant;
extern bool fastfood;
extern bool subway;
extern bool bikeshare;
extern bool home;
extern bool traffic;
extern bool displaybox;
extern bool night;
extern bool firstSetHome;
extern bool walk;
extern bool showPath;
extern bool showHelp;
extern bool didYouMean;


extern int searchMode;

extern string navInstruction;
/**************END GLOBAL VARIABLES DECLARATION*********************/
//We also build fast-lookups from: intersection id to DeliveryInfo index for both
        //pickUp and dropOff intersections
   extern     std::multimap<IntersectionIndex,size_t> intersections_to_pick_up; //Intersection_ID -> deliveries index
   extern     std::multimap<IntersectionIndex,size_t> intersections_to_drop_off; //Intersection_ID -> deliveries index

#endif /* GLOBALDATA_H */

