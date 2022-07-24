/*
 * 
 * This header contains auxiliary functions that are needed in milestone 1
 * 
 * */
#pragma once
#include "globalData.h"
#include "LatLon.h"
#include "m1.h"
#include "StreetsDatabaseAPI.h"

double segmentLength(const InfoStreetSegment & targetSegment, int streetSegmentID);
double calcLat(double originLat, LatLon loc);
double calcLon(double originLon, LatLon loc);
double findAngle(double p1x, double p1y, double p2x, double p2y);
double latToY (double lat);
double lonToX (double lon);
double xToLon(double x);
double yToLat(double y);
double findDistance(LatLon & start, LatLon & end);

