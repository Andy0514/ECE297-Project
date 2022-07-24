/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */
#include "calculations.h"
#include "streetNode.h"
#include "intersecNode.h"
#include "segmentNode.h"
#include "calculations.h"
#include "globalData.h"
#include "StreetsDatabaseAPI.h"
#include "OSMDatabaseAPI.h"
#include "OSMWayNode.h"
#include "m1.h"
#include <math.h>       /* for atan */

#define PI 3.14159265



//this function calculates the length of a single street segment
//LOGIC: determine the number of curvature points in the street segment, then call the functistreon
//          'find_distance_between_two_points' between the start/end points of the street
//            segment and each curvature points. Add up the total curvature
//Implemented by Haolin Wang
double segmentLength(const InfoStreetSegment & targetSegment, int streetSegmentID){
     //get the begin and end intersection locations
    int startID = targetSegment.from, endID = targetSegment.to;
    LatLon startLocation = getIntersectionPosition(startID), endLocation = getIntersectionPosition(endID);
    int curvePoints = targetSegment.curvePointCount;
    double result = 0;
    
    for(int iter = 0; iter <= curvePoints; iter++){
       LatLon op1, op2;
       
       //gets the two operands for finding distance, using either start location or curve point location
       if (iter == 0 ){
           op1 = startLocation;
           segmentDatabase[streetSegmentID].curvePoints.push_back(op1);
       } else{
           op1 = getStreetSegmentCurvePoint(iter-1, streetSegmentID);
           
           //stores all the curve points in the segment node
           segmentDatabase[streetSegmentID].curvePoints.push_back(op1);
       }
       
       if (iter == curvePoints){
           op2 = endLocation;
           segmentDatabase[streetSegmentID].curvePoints.push_back(op2);
       } else{
           op2 = getStreetSegmentCurvePoint(iter, streetSegmentID);
           
       }
  
       //calculate the distance and add to result
       result += find_distance_between_two_points(make_pair(op1, op2));
       
       
    }
    return result;
}

//Implemented by An Yu
//calculate latitude distance between origin and a specific LatLon
double calcLat(double originLat, LatLon loc){

    double x1 = 0 ;
    double x2 = 0;
    double y1 = originLat * DEGREE_TO_RADIAN;
    double y2 = loc.lat() * DEGREE_TO_RADIAN;
    
    return EARTH_RADIUS_METERS * sqrt(pow(y2 - y1,2)+pow(x2 - x1,2));
}

//Implemented by An Yu
//calculate longitude distance between origin and a specific LatLon
double calcLon(double originLon, LatLon loc){
    double latAvg = loc.lat() * DEGREE_TO_RADIAN;  //must convert first to radian, then calculate using provided formula
    double x1 = originLon * cos(latAvg) * DEGREE_TO_RADIAN ;
    double x2 = loc.lon() * cos(latAvg) * DEGREE_TO_RADIAN ;
    double y1 = 0;
    double y2 = 0;
    
    
    return EARTH_RADIUS_METERS * sqrt(pow(y2 - y1,2)+pow(x2 - x1,2));
}


//find the angle between two points. If the segment loops back on itself, set the angle to 0
double findAngle(double p1x, double p1y, double p2x, double p2y){
    
    double param=(p2y-p1y)/(p2x-p1x);
    
    double angle = atan (param) * 180 / PI;
    
    if (isnan(angle) == true) angle = 0;
    return angle;
    
}


//conversion between latlon and xy
double latToY (double lat){
    return lat * DEGREE_TO_RADIAN;
}
double lonToX (double lon){
    double averagecityLat = (maxLat + minLat)/2 * DEGREE_TO_RADIAN;
    return lon * cos(averagecityLat) * DEGREE_TO_RADIAN ;
}

double xToLon(double x){
    double averagecityLat = (maxLat + minLat)/2 * DEGREE_TO_RADIAN;
    return x / (cos(averagecityLat) * DEGREE_TO_RADIAN);
}

double yToLat(double y){
    return y / DEGREE_TO_RADIAN;
}

double findDistance(LatLon & start, LatLon & end){
    
    double latAvg = (start.lat() + end.lat())/2 * DEGREE_TO_RADIAN;  //must convert first to radian
    double x1 = start.lon() * cos(latAvg) * DEGREE_TO_RADIAN ;
    double x2 = end.lon() * cos(latAvg) * DEGREE_TO_RADIAN ;
    double y1 = start.lat() * DEGREE_TO_RADIAN;
    double y2 = end.lat() * DEGREE_TO_RADIAN;
    return EARTH_RADIUS_METERS * (abs(x2 - x1) + abs(y2 - y1));
}