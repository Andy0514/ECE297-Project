
/* 
 * File:   drawFunctions.h
 * Author: wangh275
 *
 * Created on February 18, 2020, 10:45 AM
 */


#ifndef DRAWFUNCTIONS_H
#define DRAWFUNCTIONS_H

#include "ezgl/application.hpp"
#include "ezgl/graphics.hpp"
#include "ezgl/camera.hpp"
#include "ezgl/rectangle.hpp"
#include "ezgl/point.hpp"
#include "segmentNode.h"
#include "featureNode.h"
#include "streetNode.h"
#include "closedRoad.h"

//this header file contains the declarations for functions that perform drawing actions on the main window

void drawOneWayStreetImage(ezgl::renderer * g, segmentNode & currentSeg);

ezgl::surface * choosePicture(double angle);

void writeStreetNames(ezgl:: renderer * g);

void drawStreetNamesOnChosenSegments(ezgl::renderer * g, map <string, segmentNode> writeToSegment);

void drawOSMWays(ezgl::renderer * g);

void drawFeature(ezgl::renderer * g);

void drawSubway(ezgl::renderer * g);

void colortranscode(string c, int & red, int & green, int & blue);

void drawStreets(ezgl::renderer * g, vector<streetNode>foundStreets, int previousHighlightedStreet);

void drawOutEntireStreet(ezgl::renderer * g, streetNode & str);

void drawSingleSegment(ezgl::renderer * g, segmentNode & seg, bool driving);

void drawSearchedSegment(ezgl::renderer * g, segmentNode & seg);

void drawWalkSegment(ezgl::renderer * g, segmentNode & seg);

void drawHighlightStreetInfo(ezgl::renderer * g, int previousHighlightedStreet, pair <double, double> labelLocation);

void drawWeatherInfo(ezgl::renderer * g, string currentWeather, int tempHigh, int tempLow);

void draw_hamburger_icon(ezgl::renderer * g, bool searchBarShown, bool searchFieldShown,bool currentlySearchActive, string searchText);

void draw_nightmode_icon(ezgl::renderer * g, bool currentlySearchActive);

void search_icon(ezgl::renderer *g);

void draw_png(ezgl::renderer *g);

void displayPOI(ezgl::renderer *g, bool currentlySearchActive);

void drawPOIIcon(ezgl::renderer *g);

void drawFoundIntersection(ezgl::renderer * g, int & previousHighlightedIntersection, vector <int> & previousFoundIntersection, int foundIntersectionIndex, int highlightStreet, bool & cameraMoved);

void drawFoundStreets(ezgl::renderer * g, vector<streetNode> & foundStreets, int foundStreetIndex, bool & cameraMoved, int previousHighlightedStreet);

void drawMapSelectionScreen(ezgl::renderer * g, vector <string> mapNames, double mouseX, double mouseY);

void drawIndividualFeature(ezgl::renderer * g, featureNode & it, int type);

void drawroadClosure(ezgl::renderer * g);

void writeFeatureName(ezgl::renderer * g);

void drawHomePin(ezgl::renderer * g, bool & startSetHome, bool & homeSet, pair <double, double> & homeLocation);

void drawHomePin_first_time(ezgl::renderer * g, bool & startSetHome, bool & homeSet, pair <double, double> & homeLocation);

void writeClickedPOI(ezgl::renderer * g);

void drawNextIcon(ezgl::renderer * g);

void drawNavigationPanelIcon(ezgl::renderer * g);


void drawNavigationPanel(ezgl::renderer * g, string & toText, string & fromText, int previousShownStep, int numWalkStreets, int numDriveStreets, int errorCode);

void drawSearchField(ezgl::renderer * g, double sy, bool active, string & searchText);

void drawNavigationIntersections(ezgl::renderer * g, int fromID, int toID);

void draw_arrow(double angle1, double angle2, double x, double y, ezgl::renderer *g);
string drawAllArrows(ezgl::renderer * g, int previousShownStep, bool cameraMoved);

void drawNavInstruction(ezgl::renderer * g, int previousShownStep, int numWalkStreets, int numDriveStreets);
string getDirection(int segID1, int segID2, double angle1, double angle2);
#endif /* DRAWFUNCTIONS_H */