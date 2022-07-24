/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

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


//functions that load parts of database using multiple threads
void loadWay();
void loadNode();

//this function loads the two data structures
bool load_dotBin_files(std::string mapPath){
    
    int index = mapPath.find(".");
    
    //load the two databases. Return false if load failed, or true if it passed.
    return loadStreetsDatabaseBIN(mapPath) && loadOSMDatabaseBIN(mapPath.substr(0, index) + ".osm.bin"); //loading the two map databases
    
}

double maxLat, maxLon, minLat, minLon;
void loadIntersectionGraph(int numIntersections){
    
    maxLat = -90, maxLon = -180, minLat = 90, minLon = 180;
    //load intersectionGraph
    for(int intersec=0; intersec<numIntersections; intersec++){
        //create new node, set num of street it connects, set the segment vector of this node with segment id
        struct intersecNode node;
        LatLon intersectionPosition = getIntersectionPosition(intersec);
        
        //update the largest and smallest latitude and longitudes found on the map
        if (intersectionPosition.lat() > maxLat) maxLat = intersectionPosition.lat();
        else if (intersectionPosition.lat() < minLat) minLat = intersectionPosition.lat();
        if (intersectionPosition.lon() > maxLon) maxLon = intersectionPosition.lon();
        else if (intersectionPosition.lon() < minLon) minLon = intersectionPosition.lon();
        
        
        node.numStreet=getIntersectionStreetSegmentCount(intersec);
        node.position = intersectionPosition;
        node.name = getIntersectionName(intersec);
        
        for(int i=0; i<getIntersectionStreetSegmentCount(intersec);i++){
            int ss_id=getIntersectionStreetSegment(intersec, i);
            node.segment.push_back(ss_id);
        }

        //push this node into the streetgraph class
        intersectionGraph.push_back(node);
    }
    
    //for each intersection, calculate its X-Y position
    for (auto i = intersectionGraph.begin(); i != intersectionGraph.end(); i++){
        i->xPos = lonToX(i->position.lon());
        i->yPos = latToY(i->position.lat());
    }
}

void loadFeatureDatabases(int numberOfFeatures){
    for(int i = 0; i < numberOfFeatures; i++){
        struct featureNode feature;
        feature.featureName = getFeatureName(i);
        feature.featurePointCount = getFeaturePointCount(i);
        feature.featureOSMID = getFeatureOSMID(i);
        feature.featureType = getFeatureType(i);
        double xmin=std::numeric_limits<double>::infinity();
        double xmax=(-1)*std::numeric_limits<double>::infinity();
        double ymin=std::numeric_limits<double>::infinity();
        double ymax=(-1)*std::numeric_limits<double>::infinity();
        
        for(int j = 0; j < getFeaturePointCount(i); j++){
            LatLon fp = getFeaturePoint(j, i);
            feature.featurePoint.push_back(fp);
            
            double xPos = lonToX(fp.lon());
            double yPos = latToY(fp.lat());
            
            //set xmax and min
            if(xPos>xmax){
                xmax=xPos;
            }else if(xPos<xmin){
                xmin=xPos;
            }
            if(yPos>ymax){
                ymax=yPos;
            }else if(yPos<ymin){
                ymin=yPos;
            }

            feature.featurePointXY.push_back(make_pair(xPos, yPos));
        }
        //set parameters
        feature.xmin=xmin;
        feature.xmax=xmax;
        feature.ymin=ymin;
        feature.ymax=ymax;
        //find center of the feature
        double width=xmax-xmin;
        double height = ymax-ymin;
        
        //detect if the feature is too long
        if(width/height <0.05 || width/height>20){
            feature.center = make_pair(0,0);
        }
        else{
            feature.center= make_pair(xmin+width/2,ymin+height/2);
        }
        
        //calculate feature area
        feature.area=find_feature_area(i);
        
        feature.closed = (feature.featurePoint[0].lat() == feature.featurePoint[feature.featurePointCount-1].lat() && feature.featurePoint[0].lon() == feature.featurePoint[feature.featurePointCount-1].lon()) && feature.featurePointCount>2;
        
        if(feature.featureType == Island && getFeaturePointCount(i) > 200000){
            for (int pt = 0; pt < feature.featurePointCount && feature.closed; pt++){
                pair<double, double> Pt = (feature.featurePointXY)[pt];
                ezgl::point2d point(Pt.first, Pt.second);
                if(pt == 0 || pt == feature.featurePointCount-1)
                    feature.fillPolyPoints.push_back(point);
                else if(pt % 1 == 0)
                    feature.fillPolyPoints.push_back(point);
            }
        }
        else{
            for (int pt = 0; pt < feature.featurePointCount && feature.closed; pt++){
                pair<double, double> Pt = (feature.featurePointXY)[pt];
                ezgl::point2d point(Pt.first, Pt.second);
                feature.fillPolyPoints.push_back(point);
            }
        }
        
        featureDatabase.resize(6);
        if((feature.featureType == Lake || feature.featureType == River || feature.featureType == Stream) && feature.area > 100000000)
            featureDatabase[0].push_back(feature);
        else if(feature.featureType == Island)
            featureDatabase[1].push_back(feature);
        else if(feature.featureType == Park || feature.featureType == Greenspace || feature.featureType == Golfcourse)
            featureDatabase[2].push_back(feature);
        else if(feature.featureType == Beach)
            featureDatabase[3].push_back(feature);
        else if(feature.featureType == Lake || feature.featureType == River || feature.featureType == Stream)
            featureDatabase[4].push_back(feature);
        else if(feature.featureType == Building)
            featureDatabase[5].push_back(feature);
    }
}

void loadPOIDatabases(int numberOfPOIs){
    for(int j = 0; j < numberOfPOIs; j++){
        struct POINode POI;
    
        POI.POIOSMID = getPointOfInterestOSMNodeID(j);
        const OSMNode & Node = *(OSMNodeData[POI.POIOSMID]);
        
        for(int i = 0; i < getTagCount(&Node); i++){
              pair<string, string> tag = getTagPair(&Node, i);

              if (tag.first == "amenity"){
                  if (tag.second == "cafe" || tag.second == "restaurant" || tag.second == "pub" || tag.second=="bar" ||
                      tag.second == "fast_food" || tag.second == "food_court"  || tag.second == "college" || tag.second == "library" || tag.second == "university" || 
                      tag.second == "bank" || tag.second == "hospital" || tag.second == "clinic" || tag.second == "doctors" || tag.second == "pharmacy" || tag.second == "bicycle_rental"){
                      
                      POI.POIType = getPointOfInterestType(j);
                      POI.POIName = getPointOfInterestName(j);
                      POI.POIPosition = getPointOfInterestPosition(j);
                      double xPos = lonToX(POI.POIPosition.lon());
                      double yPos = latToY(POI.POIPosition.lat());
                      POI.POIPositionXY = make_pair(xPos, yPos);
                      POIDatabase.push_back(POI);
                      break;
                  }
              }
        }
        
    }
}


//this function loads two databases related to streets (streetDatabase and segmentDatabase)
//and a tempArrayOfStreetNames vector, which is used to load future databases.
//They are grouped together because they can be performed in one loop
void loadStreetDatabases(int numberOfSegments, int numberOfStreets, vector <pair<string, int>> & tempArrayOfStreetNames){
    
    for(int iter = 0; iter < numberOfSegments; iter++){
        //street data base
        //get information on this iteration's street segmentf
        const InfoStreetSegment currentSegment = getInfoStreetSegment(iter);
        int streetID = currentSegment.streetID;
        string streetName = getStreetName(streetID);
        streetNode * currentNode;
        if (streetName == "<unknown>"){
            streetDatabase.resize(streetDatabase.size() + 1);
            currentNode = &(streetDatabase[streetDatabase.size()-1]);
        } else{
            currentNode = &(streetDatabase[streetID]);
        }
        const OSMWay currentWay = OSMWayData[currentSegment.wayOSMID];
        for(int i = 0; i < getTagCount(&currentWay); i++){
            pair<std::string, std::string> tag = getTagPair(&currentWay, i);
            if(tag.first == "highway" && (tag.second == "motorway" || tag.second == "motorway_link" || tag.second == "trunk" || tag.second == "trunk_link")){
                currentNode->highway = 1;
            }
            else if(tag.first == "highway" && (tag.second == "primary" || tag.second == "secondary" )){
                currentNode->primary = 1;
            }
            else if(tag.first == "highway" && (tag.second == "tertiary" || tag.second == "tertiary_link"|| tag.second == "primary_link" || tag.second == "secondary_link")){
                currentNode->tertiary = 1;
            }
        }
        
        //if this street has not been initialized (default name: "NO_NAME"):
        if (currentNode->streetName == "NO_NAME") {
            
            //initialize name for this street
            currentNode->streetName = streetName;
            
            //parses the street name and adds an entry for it into tempArrayOfStreetNames
            streetName.erase(std::remove_if(streetName.begin(), streetName.end(), ::isspace), streetName.end()); //erase all white spaces in the name
            std::transform(streetName.begin(), streetName.end(), streetName.begin(),::tolower); //transform street name to lower case
            tempArrayOfStreetNames.push_back(make_pair(streetName, streetID));
 
        }
        //continue loading street database
        //add information on the street's street segment and intersections
        currentNode->streetSegments.push_back(iter);
        currentNode->intersections.push_back(currentSegment.to);
        currentNode->intersections.push_back(currentSegment.from);
        currentNode->streetID = iter;
        /***********************************************************************************************/
        //loads segmentDatabase for the current street segment, which contains length, location, and speed information
        segmentNode * currentSeg = &(segmentDatabase[iter]);
        double length = segmentLength(currentSegment, iter);
        currentSeg->length = length;
        currentSeg->speedLimit = currentSegment.speedLimit;
        currentSeg->streetID = currentSegment.streetID;
        currentSeg->oneway = currentSegment.oneWay;
        currentSeg->to = currentSegment.to, currentSeg->from = currentSegment.from;
        currentSeg->travelTime = length * 3.6 / currentSegment.speedLimit;
        
        //calculate angle
        double p1y= latToY (getIntersectionPosition(currentSegment.from).lat());
        double p1x= lonToX (getIntersectionPosition(currentSegment.from).lon());
        double p2y= latToY (getIntersectionPosition(currentSegment.to).lat());
        double p2x= lonToX (getIntersectionPosition(currentSegment.to).lon());

        currentSeg->angle= findAngle(p1x, p1y, p2x, p2y);
        
        for (int i = 0; i < currentSegment.curvePointCount + 2; i++){
            currentSeg->curvePointsXY.push_back(make_pair(lonToX(currentSeg->curvePoints[i].lon()), latToY(currentSeg->curvePoints[i].lat())));
        }  
    }

    //sorts the intersection and street segment information for each node in streetDatabase and erases duplicates
    //"unique" erases identical consecutive elements
    for(int i = 0; i < numberOfStreets; i++){
        sort(streetDatabase[i].intersections.begin(), streetDatabase[i].intersections.end());
        streetDatabase[i].intersections.erase(unique(streetDatabase[i].intersections.begin(), streetDatabase[i].intersections.end()), streetDatabase[i].intersections.end()); 
        
        sort(streetDatabase[i].streetSegments.begin(), streetDatabase[i].streetSegments.end());
        streetDatabase[i].streetSegments.erase(unique(streetDatabase[i].streetSegments.begin(), streetDatabase[i].streetSegments.end()), streetDatabase[i].streetSegments.end()); 
    }
}

//loads sortedStreetName database, which allows for quickly finding street names by the first character
void loadSortedStreetName (vector <pair<std::string, int>> & tempArrayOfStreetNames){

    for (auto it = tempArrayOfStreetNames.begin(); it != tempArrayOfStreetNames.end(); it++){
        std::string name = it->first;
        int index = name[0];
        sortedStreetName[index].push_back(*it);
    }
    
    //sort each vector entry of sortedStreetName for fast accessing
    for (auto it = sortedStreetName.begin(); it != sortedStreetName.end(); it++){
        sort((*it).second.begin(), (*it).second.end());
    }
}
void loadWay(){
    int numberOfWays = getNumberOfWays();
    for (int idx = 0; idx < numberOfWays; idx++){
        const OSMWay* currentWay = getWayByIndex(idx);
        OSMID currentWayID = currentWay->id();
        OSMWayData.insert(unordered_map<OSMID, const OSMWay>::value_type(currentWayID, *currentWay));
    }
}

void loadNode(){
    //loads OSMNodeData that helps find OSM Node with its OSM ID rather than 
    //using index
    int numberOfNodes = getNumberOfNodes();
    for (int idx = 0; idx < numberOfNodes; idx++){
        const OSMNode* currentNode = getNodeByIndex(idx);
        OSMID currentNodeID = currentNode->id();
        OSMNodeData.insert(unordered_map<OSMID, const OSMNode *>::value_type(currentNodeID, currentNode));
    }
}

//loads OSM databases that are used to index OSMWay and OSMNode by its unique ID
void loadOSMDatabases(){
    thread n(loadNode);
    thread way(loadWay);
    
    int numberOfRelations = getNumberOfRelations();
    for (int idx = 0; idx < numberOfRelations; idx++){
        const OSMRelation* currentRelation = getRelationByIndex(idx);
        for(int i = 0; i < getTagCount(currentRelation); i++){
            pair<std::string, std::string> tag = getTagPair(currentRelation, i);
            if(tag.first == "route" && tag.second == "subway"){
                subwayLines.push_back(*currentRelation);
                continue;
            }
        }
    }
    n.join();
    vector<TypedOSMID> subwayStation;
    //load all subway lines and stations
    for (int idx = 0; idx < subwayLines.size(); idx++){
        LatLon startpos, endpos, pos;
        subwayStation = getRelationMembers(&subwayLines[idx]);
        for (int j = 0; j < subwayStation.size(); j++){
            if(subwayStation[j].type() == TypedOSMID::Node){
                OSMNode node = *(OSMNodeData[subwayStation[j]]);
                if(node.id() == subwayStation[j]){
                    pos = getNodeCoords(&node);
                    for (int k = 0; k < getTagCount(&node); k++){
                        pair<std::string, std::string> tag = getTagPair(&node, k);
                        if(tag.first == "name")
                            subwayStationInfo.insert(make_pair(tag.second, pos));
                        break;
                    }
                }
            }
        }
    }
    way.join();
}
