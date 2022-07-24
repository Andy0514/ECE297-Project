/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

#include "drawFunctions.h"
#include "m2.h"
#include "m1.h"
#include "StreetsDatabaseAPI.h"
#include "OSMDatabaseAPI.h"
#include "LatLon.h"
#include "OSMWayNode.h"

#include "POIIcon.h"

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
#include <stdlib.h>
#include <iomanip>
#include <sstream>

#include "streetNode.h"
#include "intersecNode.h"
#include "featureNode.h"
#include "POINode.h"
#include "segmentNode.h"
#include "calculations.h"
#include "globalData.h"
#include "closedRoad.h"

#include "ezgl/application.hpp"
#include "ezgl/graphics.hpp"
#include "ezgl/camera.hpp"
#include "ezgl/rectangle.hpp"
#include "ezgl/point.hpp"

using namespace std;


void drawMapSelectionScreen(ezgl::renderer * g, vector <string> mapNames, double mouseX, double mouseY){
    
    //draw background image
    ezgl::surface * background = ezgl::renderer::load_png("libstreetmap/resources/background.png");
    ezgl::surface * logo = ezgl::renderer::load_png("libstreetmap/resources/logo.png");
    g->draw_surface(background, {-500, 600});
    g->draw_surface(logo, {155, 580-(screenBounds.right()-screenBounds.left()-800)/20});
    ezgl::renderer::free_surface(background);
    ezgl::renderer::free_surface(logo);
    
    g->set_horiz_text_just(ezgl::text_just::left);
    g->set_font_size(25);
    g->set_line_width(0);
    g->set_color(ezgl::BLACK);
    g->draw_text({30, 420}, "Pick your map");
    
    //draw boxes containing maps
    g->set_font_size(15);
    double yPosition, xPosition;
    xPosition = 20;
    int width = 200, height = 30;
    for (int i = 0; i < mapNames.size(); i++){

        if (i < 10)
            yPosition = 400 - (height * i);
        else {
            yPosition = 400 - height * (i-10);
            xPosition = 220;
        }
        
        if (mouseX > xPosition && mouseX < (xPosition + width) && mouseY < yPosition && mouseY > (yPosition - height)){
            //mouse hover overs an selection
            g->set_color(37, 148, 9, 100);
            g->fill_rectangle({xPosition, yPosition}, {xPosition + width, yPosition - height});
        }
        g->set_color(ezgl::BLACK);
        g->draw_rectangle({xPosition, yPosition}, {xPosition + width, yPosition - height});
        g->draw_text({xPosition + 20, yPosition-18}, mapNames[i], 350, 30);
    }
}
  

//write down street names
void writeStreetNames(ezgl::renderer * g){

    map <string, segmentNode> writeToSegment;

    //gets one street segment for each street on-screen
    for (auto seg = segmentDatabase.begin(); seg != segmentDatabase.end(); seg++){
        
        //don't record segments that correspond to <unknown> street, or are too short
        if (streetDatabase[seg->streetID].streetName == "<unknown>" || streetDatabase[seg->streetID].streetName == "NO_NAME" || seg->length < 50) continue;
        
        streetNode temp = streetDatabase[seg->streetID];
        if (temp.highway == 1 && (temp.streetName.size() > 15 || temp.streetName == "<unknown>")) continue;
        
        double x1 = seg->curvePointsXY[0].first, y1 = seg->curvePointsXY[0].second;
        double x2 = seg->curvePointsXY[seg->curvePointsXY.size()-1].first, y2 = seg->curvePointsXY[seg->curvePointsXY.size()-1].second;
        
        //if either end of the street segment is not in the map, don't draw its name
        bool firstPointOut = (x1 < bounds.left() || x1 > bounds.right() || y1 < bounds.bottom() || y1 > bounds.top());
        bool secondPointOut = (x2 < bounds.left() || x2 > bounds.right() || y2 < bounds.bottom() || y2 > bounds.top());
        if (firstPointOut || secondPointOut) continue;
        
        //if the street is curved or too short and the view area is large, don't draw its name
        if (bounds.right() - bounds.left() > 0.00003){
            if (seg->length < 100) continue;
            
            int curveNum = seg->curvePoints.size();
            if (curveNum > 3){
                double xMid = seg->curvePointsXY[curveNum/2].first, yMid = seg->curvePointsXY[curveNum/2].second;
                double angle1 = atan((yMid - y1)/(xMid - x1));
                double angle2 = atan((y2 - yMid) / (x2 - xMid));
                if (abs(angle2 - angle1) > 0.2) continue;   //the street segment has curve
            }
        }

        writeToSegment.insert(make_pair(streetDatabase[seg->streetID].streetName, *seg));
        seg->streetNameDrawn = true;
    }
    drawStreetNamesOnChosenSegments(g, writeToSegment);
}

//draws names on all the street segments within writeToSegment
void drawStreetNamesOnChosenSegments(ezgl::renderer * g, map <string, segmentNode> writeToSegment){
    
    //draw street names on the chosen street segments
    for (auto segIter = writeToSegment.begin(); segIter != writeToSegment.end(); segIter++){
        
        segmentNode & seg = segIter->second;
        streetNode currentStreet = streetDatabase[seg.streetID];
        string streetName = currentStreet.streetName;
        
        //sets the font size
        if (bounds.right() - bounds.left() > 0.0003){
            g->set_font_size(12);
        } else{
            if (currentStreet.highway == true || currentStreet.streetSegments.size() > 30)
                g->set_font_size(14);
            else
                g->set_font_size(11);
        }
          
        //sets the horizontal bounds for text, based on the street's segment count
        double horizontalBound;
        if (currentStreet.highway == true || currentStreet.streetSegments.size() > 200)
            horizontalBound = 0.0003;
        else if (currentStreet.streetSegments.size() > 100)
            horizontalBound = 0.00008;
        else if (currentStreet.streetSegments.size() > 40)
            horizontalBound = 0.00005;
        else if (currentStreet.streetSegments.size() > 20)
            horizontalBound = 0.00002;
        else
            horizontalBound = 0.00001;
        
        double avgX, avgY;
        if (bounds.right() - bounds.left() < 0.00003) {
            if (seg.curvePointsXY.size() <= 3){
                //no curve point on the street, center the street name at the middle of the segment
                double x1 = seg.curvePointsXY[0].first;
                double y1 = seg.curvePointsXY[0].second;
                double x2 = seg.curvePointsXY[seg.curvePointsXY.size()-1].first;
                double y2 = seg.curvePointsXY[seg.curvePointsXY.size()-1].second;
                avgX = (x2 + x1) / 2;
                avgY = (y2 + y1) / 2;
            } else{
                //center the street name on a middle curve point
                int midCurvePt = seg.curvePoints.size() / 2;
                avgX = seg.curvePointsXY[midCurvePt].first;
                avgY = seg.curvePointsXY[midCurvePt].second;
            }
        }
        else{
            double x1 = seg.curvePointsXY[0].first;
            double y1 = seg.curvePointsXY[0].second;
            double x2 = seg.curvePointsXY[seg.curvePointsXY.size()-1].first;
            double y2 = seg.curvePointsXY[seg.curvePointsXY.size()-1].second;
            avgX = (x2 + x1) / 2;
            avgY = (y2 + y1) / 2; 
        }
        
        //rotate text to align with the street direction
        g->set_text_rotation(seg.angle);
        g->draw_text({avgX, avgY}, streetName, horizontalBound, DBL_MAX);    
    } 
    g->set_text_rotation(0);
}


//draw features based on the current scale
void drawFeature(ezgl::renderer * g){
    //draw features
    g->set_line_width(2);
    g->set_font_size(8);
    g->set_color(ezgl::BLACK);
    
    
    //bounds<0.0003: draw everything
    if (bounds.right() - bounds.left() < 0.0003){
        for(int type = 0; type < 6; type ++){
            for(auto iter = featureDatabase[type].begin(); iter != featureDatabase[type].end(); iter++){
                if (iter->xmax > bounds.left() && iter->xmin < bounds.right() && iter->ymax > bounds.bottom() && iter->ymin < bounds.top()){
                    drawIndividualFeature(g, *iter, type);
                }
            }
        }
    }
    //0.0003<bounds<0.001:draw area >1000
    else if (bounds.right() - bounds.left() < 0.0008 && bounds.right() - bounds.left() > 0.0003){
        for(int type = 0; type < 5; type++){
            for(auto iter = featureDatabase[type].begin(); iter != featureDatabase[type].end(); iter++){
                if (iter->xmax > bounds.left() && iter->xmin < bounds.right() && iter->ymax > bounds.bottom() && iter->ymin < bounds.top() && iter->area > 2000)
                    drawIndividualFeature(g, *iter, type);
            }
        }
    }
    else if (bounds.right() - bounds.left() < 0.0015 && bounds.right() - bounds.left() > 0.0008){
        for(int type = 0; type < 5; type++){
            for(auto iter = featureDatabase[type].begin(); iter != featureDatabase[type].end(); iter++){
                if (iter->xmax > bounds.left() && iter->xmin < bounds.right() && iter->ymax > bounds.bottom() && iter->ymin < bounds.top() && iter->area > 10000)
                    drawIndividualFeature(g, *iter, type);
            }
        }
    }
    //0.0015<bounds<0.003: only draw features with areas >10000
    else if (bounds.right() - bounds.left() < 0.003 && bounds.right() - bounds.left() > 0.0015){
        for(int type = 0; type < 5; type++){
            for(auto iter = featureDatabase[type].begin(); iter != featureDatabase[type].end(); iter++){
                if (iter->xmax > bounds.left() && iter->xmin < bounds.right() && iter->ymax > bounds.bottom() && iter->ymin < bounds.top() && iter->area > 20000){
                    drawIndividualFeature(g, *iter, type);
                }
            } 
        }
    }
    else if (bounds.right() - bounds.left() < 0.005 && bounds.right() - bounds.left() > 0.003){
        for(int type = 0; type < 5; type++){
            for(auto iter = featureDatabase[type].begin(); iter != featureDatabase[type].end(); iter++){
                if (iter->xmax > bounds.left() && iter->xmin < bounds.right() && iter->ymax > bounds.bottom() && iter->ymin < bounds.top() && iter->area > 50000)
                    drawIndividualFeature(g, *iter, type);
                    
            }
        }
    }
    else if (bounds.right() - bounds.left() < 0.008 && bounds.right() - bounds.left() > 0.005){
        for(int type = 0; type < 5; type++){
            for(auto iter = featureDatabase[type].begin(); iter != featureDatabase[type].end(); iter++){
                if (iter->xmax > bounds.left() && iter->xmin < bounds.right() && iter->ymax > bounds.bottom() && iter->ymin < bounds.top() && iter->area > 80000)
                    drawIndividualFeature(g, *iter, type);
                    
            }
        }
    }
    else if (bounds.right() - bounds.left() < 0.01 && bounds.right() - bounds.left() > 0.008){
        for(int type = 0; type < 5; type++){
            for(auto iter = featureDatabase[type].begin(); iter != featureDatabase[type].end(); iter++){
                if (iter->xmax > bounds.left() && iter->xmin < bounds.right() && iter->ymax > bounds.bottom() && iter->ymin < bounds.top() && iter->area > 100000)
                    drawIndividualFeature(g, *iter, type);
            }
        }
    }
    else if (bounds.right() - bounds.left() < 0.02 && bounds.right() - bounds.left() > 0.01){
        for(int type = 0; type < 5; type++){
            for(auto iter = featureDatabase[type].begin(); iter != featureDatabase[type].end(); iter++){
                if (iter->xmax > bounds.left() && iter->xmin < bounds.right() && iter->ymax > bounds.bottom() && iter->ymin < bounds.top() && iter->area > 140000)
                    drawIndividualFeature(g, *iter, type);
            }
        }
    }
    else if (bounds.right() - bounds.left() < 0.04 && bounds.right() - bounds.left() > 0.02){
        for(int type = 0; type < 5; type++){
            for(auto iter = featureDatabase[type].begin(); iter != featureDatabase[type].end(); iter++){
                if (iter->xmax > bounds.left() && iter->xmin < bounds.right() && iter->ymax > bounds.bottom() && iter->ymin < bounds.top() && iter->area > 140000)
                    drawIndividualFeature(g, *iter, type);
            }
        }
    }
    else if (bounds.right() - bounds.left() > 0.04){
        for(int type = 0; type < 5; type++){
            for(auto iter = featureDatabase[type].begin(); iter != featureDatabase[type].end(); iter++){
                if (iter->xmax > bounds.left() && iter->xmin < bounds.right() && iter->ymax > bounds.bottom() && iter->ymin < bounds.top() && iter->area > 100000)
                    drawIndividualFeature(g, *iter, type);
            }
        }
    }
}

//helper function that draws one feature
void drawIndividualFeature(ezgl::renderer * g, featureNode & it, int type){
    if(type == 0 && night == 0){ //large water bodies
        if(it.featureType == Lake || it.featureType == River || it.featureType == Stream)
            g->set_color(ezgl::LIGHT_WATER);
        else
            g->set_color(ezgl::LIGHT_PARK);
    }
    else if(type == 0 && night == 1){
        if(it.featureType == Lake || it.featureType == River || it.featureType == Stream)
            g->set_color(ezgl::DARK_WATER);
        else
            g->set_color(ezgl::DARK_PARK);
    }
    else if(type == 1 && night == 0) //island
        g->set_color(ezgl::LIGHT_ISLAND);
    else if(type == 1 && night == 1)
        g->set_color(ezgl::DARK_ISLAND);
    else if(type == 2 && night == 0) //small water bodies
        g->set_color(ezgl::LIGHT_PARK);
    else if(type == 2 && night == 1)
        g->set_color(ezgl::DARK_PARK);
    else if(type == 3 && night == 0)
        g->set_color(ezgl::LIGHT_BEACH);
    else if(type == 3 && night == 1)
        g->set_color(ezgl::DARK_BEACH);
    else if(type == 4 && night == 0)
        g->set_color(ezgl::LIGHT_WATER);
    else if(type == 4 && night == 1)
        g->set_color(ezgl::DARK_WATER);
    else if(type == 5 && bounds.right() - bounds.left() < 0.00015 && night == 0)
        g->set_color(ezgl::LIGHT_BUILDING);
    else if(type == 5 && bounds.right() - bounds.left() < 0.00015 && night == 1)
        g->set_color(ezgl::DARK_BUILDING);
    else
        return;
    
    if (it.closed){
        g->fill_poly(it.fillPolyPoints);
    }else{
        //open polygon
        for(int fp = 0; fp < it.featurePointCount - 1; fp++){
            double x1 = it.featurePointXY[fp].first;
            double y1 = it.featurePointXY[fp].second;
            double x2 = it.featurePointXY[fp+1].first;
            double y2 = it.featurePointXY[fp+1].second;
            g->draw_line({x1, y1}, {x2, y2});
        }
    }
}


//draws POI name upon clicking
void writeClickedPOI(ezgl::renderer * g){
    bounds = g->get_visible_world();
        if(displaybox==true){
            if(bounds.right()-bounds.left()<0.0005){
                double textX= clickedPOI.x;
                double textY= clickedPOI.y - (14/screenToWorldRatioY);

                g->set_coordinate_system(ezgl::WORLD);
                g->set_font_size(12);
                if(!night)
                    g->set_color(ezgl::BLACK);
                else
                    g->set_color(ezgl::DARK_TEXT);
                g->set_horiz_text_just(ezgl::text_just::center);
                g->draw_text({textX,textY}, clickedPOI.name);
            }
        } 
}



//write out parks' names
void writeFeatureName(ezgl::renderer * g){
    g->set_font_size(8);
    g->set_color(ezgl::BLACK);
    g->set_coordinate_system(ezgl::WORLD);
    //write feature name only if bounds<0.0003
    if (bounds.right() - bounds.left() < 0.0003){
        ezgl::surface *park = ezgl::renderer::load_png("libstreetmap/resources/POIIcons/park.png");
        for(auto iter = featureDatabase[2].begin(); iter!= featureDatabase[2].end(); iter++){

            featureNode feat = *iter;
            if(feat.xmax > bounds.left() && feat.xmin < bounds.right() && feat.ymax > bounds.bottom() && feat.ymin < bounds.top()){
                if(feat.featureName != "<noname>"){
                    if(!night)
                        g->set_color(51, 81, 130);
                    else
                        g->set_color(ezgl::DARK_TEXT);
                    g->draw_text({(feat.center).first, (feat.center).second - 5/screenToWorldRatioY}, feat.featureName, 0.00002, DBL_MAX);
                    g->draw_surface(park, {(feat.center).first - 9/screenToWorldRatioX, ((feat.center).second)});
                }
            }
        }
        ezgl::renderer::free_surface(park);
    }
}

//draw subway lines and station based on button clicked
void drawSubway(ezgl::renderer * g){
    if(subway){
        ezgl::line_dash dash = ezgl::line_dash::asymmetric_5_3;
        g->set_line_dash(dash);
        g->set_color(ezgl::BLUE);
        g->set_line_width(2);
        int red = 0, green = 0, blue = 0;
        for(auto it = subwayLines.begin(); it != subwayLines.end(); it++){
            const OSMRelation r = *it;
            vector<TypedOSMID> OSMMembers = getRelationMembers(&r);
            vector<OSMWay> subwayOSMWay;
            for(int i = 0; i < getTagCount(&r); i++){
                pair<string, string> tag = getTagPair(&r, i);
                if(tag.first == "colour"){
                    colortranscode(tag.second, red, green, blue);
                    break;
                }
            }
            g->set_color(red, green, blue);
            for(int i = 0; i < OSMMembers.size(); i++){
                if(OSMMembers[i].type() == TypedOSMID::Way)
                    subwayOSMWay.push_back(OSMWayData[OSMMembers[i]]);
            }
            //draw subway lines
            for (auto secondIt = subwayOSMWay.begin(); secondIt != subwayOSMWay.end(); secondIt++){
                const OSMWay & way = *secondIt;
                const std::vector<OSMID> members = getWayMembers(&way);
                for(auto memberIt = members.begin(); memberIt != members.end()-1; memberIt++){ //iterate through all nodes and add up all distances
                    double x1, y1, x2, y2;
                    const OSMNode first = *(OSMNodeData[*memberIt]); 
                    const OSMNode second = *(OSMNodeData[*(memberIt+1)]);
                    LatLon one = getNodeCoords(&first);
                    LatLon two = getNodeCoords(&second);
                    x1 = lonToX(one.lon());
                    y1 = latToY(one.lat());
                    x2 = lonToX(two.lon());
                    y2 = latToY(two.lat());
                    g->draw_line({x1, y1}, {x2, y2});
                }
            }
        }
        //draw and print subway stations
        for(auto stationIt = subwayStationInfo.begin(); stationIt != subwayStationInfo.end(); stationIt++){
            if(bounds.right() - bounds.left() < 0.0015){
                bounds = g->get_visible_world();
                g->set_font_size(10);
                LatLon point = (*stationIt).second;
                ezgl::point2d p1(lonToX(point.lon()), latToY(point.lat()));
                double elementSize = pow((bounds.right() - bounds.left()), 0.75) / 6000;
                g->fill_arc(p1, elementSize*3, 0, 360);
                ezgl::point2d p2(lonToX(point.lon()), latToY(point.lat())-(20/screenToWorldRatioY));
                if(!night)
                    g->set_color(ezgl::BLACK);
                else
                    g->set_color(ezgl::DARK_TEXT);
                g->draw_text(p2, (*stationIt).first);
            }
        }
    }
}

//transcoding HEX color code from OSM tags to RGB
void colortranscode(string c, int & red, int & green, int & blue){
    if(c == "black")
        c = "000000";
    else if(c == "gray")
        c = "808080";
    else if(c == "maroon")
        c = "800000";
    else if(c == "olive")
        c = "808000";
    else if(c == "green")
        c = "008000";
    else if(c == "teal")
        c = "008080";
    else if(c == "navy")
        c = "000080";
    else if(c == "purple")
        c = "800080";
    else if(c == "white")
        c = "FFFFFF";
    else if(c == "silver")
        c = "C0C0C0";
    else if(c == "red")
        c = "FF0000";
    else if(c == "yellow")
        c = "FFFF00";
    else if(c == "lime")
        c = "00FF00";
    else if(c == "aqua" || c == "cyan")
        c = "00FFFF";
    else if(c == "blue")
        c = "0000FF";
    else if(c == "purple")
        c = "FF00FF";
    else if(c == "orange")
        c = "FF5E00";
    else if(c == "pink")
        c = "F801AA";
    int hex = 0;
    if(c.at(0) == '#'){
        c = c.substr(1, 6);
    }
    hex = stoi(c, 0, 16);
    red = ((hex >> 16) & 0xFF);
    green = ((hex >> 8) & 0xFF);
    blue = ((hex) & 0xFF);
}

//draw all streets with navigation
void drawStreets(ezgl::renderer * g, vector<streetNode> foundStreets, int previousHighlightedStreet){
    
    
    ezgl::line_dash dash = ezgl::line_dash::none;
    g->set_line_dash(dash);
    //draw streets
    for (auto str = streetDatabase.begin(); str != streetDatabase.end(); str++){
        drawOutEntireStreet(g, *str);
    }
    
    //draw highlighted streets
    if (streetDatabase[previousHighlightedStreet].highlight == true && streetDatabase[previousHighlightedStreet].streetName != "<unknown>"){
        drawOutEntireStreet(g, streetDatabase[previousHighlightedStreet]);
    }
    
    //draw found streets
    for (auto str = foundStreets.begin(); str != foundStreets.end(); str++){
         drawOutEntireStreet(g, *str);
    }
    
    for (auto it = drivePath.begin(); it != drivePath.end(); it++){
        drawSingleSegment(g, segmentDatabase[*it], 1);
    }
    
    for (auto it = walkPath.begin(); it != walkPath.end(); it++){
        drawSingleSegment(g, segmentDatabase[*it], 0);
    }
    
    
}

void drawSearchedSegment(ezgl::renderer * g, segmentNode & seg){
    g->set_line_width (5);
    g->set_color(50, 168, 82);
    int numCurvePoints = seg.curvePoints.size();
        
    //draw out all the lines that make up the street segment
    for (int curvePt = 0; curvePt < numCurvePoints - 1; curvePt++){
        double x1 = (seg.curvePointsXY)[curvePt].first;
        double y1 = (seg.curvePointsXY)[curvePt].second;
        double x2 = (seg.curvePointsXY)[curvePt + 1].first;
        double y2 = (seg.curvePointsXY)[curvePt + 1].second;
        g->draw_line({x1, y1}, {x2, y2});
    }
    
}
void drawWalkSegment(ezgl::renderer * g, segmentNode & seg){
    g->set_line_width (7);
    g->set_color(0, 0, 0);
    int numCurvePoints = seg.curvePoints.size();
        
    //draw out all the lines that make up the street segment
    for (int curvePt = 0; curvePt < numCurvePoints - 1; curvePt++){
        double x1 = (seg.curvePointsXY)[curvePt].first;
        double y1 = (seg.curvePointsXY)[curvePt].second;
        double x2 = (seg.curvePointsXY)[curvePt + 1].first;
        double y2 = (seg.curvePointsXY)[curvePt + 1].second;
        g->draw_line({x1, y1}, {x2, y2});
    }
    
}




void drawSingleSegment(ezgl::renderer * g, segmentNode & seg, bool driving){
    g->set_coordinate_system(ezgl::WORLD);
    g->set_line_width (5);
    
    if (driving)
        g->set_color(87, 178, 247);
    else
        g->set_color(23, 209, 73);
    int numCurvePoints = seg.curvePoints.size();
        
    //draw out all the lines that make up the street segment
    for (int curvePt = 0; curvePt < numCurvePoints - 1; curvePt++){
        double x1 = (seg.curvePointsXY)[curvePt].first;
        double y1 = (seg.curvePointsXY)[curvePt].second;
        double x2 = (seg.curvePointsXY)[curvePt + 1].first;
        double y2 = (seg.curvePointsXY)[curvePt + 1].second;
        g->draw_line({x1, y1}, {x2, y2});
    }
    
}
//helper function that draw one complete street
void drawOutEntireStreet(ezgl::renderer * g, streetNode & str){
    int segCount = str.intersections.size();
    string streetName = str.streetName;
    bool highway = str.highway, primary = str.primary, tertiary = str.tertiary;

    //don't display small streets if the view is zoomed out
    if (bounds.right() - bounds.left() > 0.01 && !highway && segCount < 200 && !str.highlight && !str.searchFound)
        return;
    else if (bounds.right() - bounds.left() > 0.0055 && (!primary || (primary && segCount < 40)) && !highway && !str.highlight && !str.searchFound)
        return;
    else if (bounds.right() - bounds.left() > 0.0025 && !primary && !highway && !str.highlight && !str.searchFound)
        return;
    else if(bounds.right() - bounds.left() > 0.001 && !primary && !tertiary && !highway && !str.searchFound && !str.highlight)
        return;
    else if (bounds.right() - bounds.left() > 0.0003 && (streetName == "<unknown>" || streetName=="NO_NAME") && !highway && !primary && !tertiary)
        return;
    
    //set colors and line widths for based on type of street
    
    if (str.highlight == true && streetName != "<unknown>"){
        g->set_line_width (5);
        g->set_color(94, 168, 50);
    }else if (str.searchFound == true && streetName != "<unknown>"){
        g->set_line_width(5);
        g->set_color(227, 147, 124);
    } else if (highway && !night){
        g->set_line_width(4);
        g->set_color(248, 216, 126);
    } else if (highway && night){
        g->set_line_width(4);
        g->set_color(ezgl::DARK_HIGHWAY);
    } else if ((primary || tertiary) && !night) {
        g->set_line_width(3);
        g->set_color(ezgl::WHITE);
    } else if ((primary || tertiary) && night){
        g->set_line_width(4);
        g->set_color(ezgl::DARK_HIGHWAY);
    } else if (streetName != "<unknown>" && !night){
        g->set_line_width(2);
        g->set_color(ezgl::WHITE);
    } else if (streetName != "<unknown>" && night){
        g->set_line_width(2);
        g->set_color(ezgl::DARK_ROAD);
    } else if (!night){
        g->set_line_width(0);
        g->set_color(200,200,200);
    } else{
        g->set_line_width(0);
        g->set_color(ezgl::DARK_ROAD);
    }

    for (auto seg = str.streetSegments.begin(); seg != str.streetSegments.end(); seg++){

        segmentNode & currentSeg = segmentDatabase[*seg];
        int numCurvePoints = currentSeg.curvePoints.size();
        
        //draw out all the lines that make up the street segment
        for (int curvePt = 0; curvePt < numCurvePoints - 1; curvePt++){
            double x1 = (currentSeg.curvePointsXY)[curvePt].first;
            double y1 = (currentSeg.curvePointsXY)[curvePt].second;
            double x2 = (currentSeg.curvePointsXY)[curvePt + 1].first;
            double y2 = (currentSeg.curvePointsXY)[curvePt + 1].second;
            g->draw_line({x1, y1}, {x2, y2});
        }
        
        //draw the one way street symbol on top of the street, if name is not written on that segment
        if (currentSeg.oneway && (bounds.right() - bounds.left() < 0.00015)){
            
            if (currentSeg.streetNameDrawn == true){
                currentSeg.streetNameDrawn = false;
                continue;
            } else if (currentSeg.curvePointsXY[0].first> bounds.left() && currentSeg.curvePointsXY[0].first < bounds.right()
                    && currentSeg.curvePointsXY[0].second > bounds.bottom() && currentSeg.curvePointsXY[0].second < bounds.top()){
                //this street is in view
               drawOneWayStreetImage(g, currentSeg);
            }
        }
    }
}

   
//draw one way symbol (arrow) at the midpoint of the street, pointing in the correct direction
void drawOneWayStreetImage(ezgl::renderer * g, segmentNode & currentSeg){

    int numCurvePoints = currentSeg.curvePoints.size();

    if (numCurvePoints == 2){
        double x1 = lonToX(currentSeg.curvePoints[0].lon());
        double y1 = latToY(currentSeg.curvePoints[0].lat());
        double x2 = lonToX(currentSeg.curvePoints[1].lon());
        double y2 = latToY(currentSeg.curvePoints[1].lat());

        double avgX = (x2 + x1) / 2;
        double avgY = (y2 + y1) / 2;
        double streetAngle = atan((y2-y1)/(x2-x1));
        
        
        //modify streetAngle so it is between 0 and 360 degrees
        if (((y2-y1 > 0) && (x2-x1 < 0)) || (y2-y1 < 0 && x2-x1 < 0)) streetAngle += M_PI;
        if ((y2-y1<0) && (x2-x1>0)) streetAngle += 2*M_PI;
        streetAngle = streetAngle / DEGREE_TO_RADIAN;
        if (isnan(streetAngle) == true) streetAngle = 90;
        
        ezgl::surface *oneWayPic = choosePicture(streetAngle);
        g->draw_surface(oneWayPic, {avgX - (5/screenToWorldRatioX), avgY - (5/screenToWorldRatioY)});
        ezgl::renderer::free_surface(oneWayPic);
    } else {
        int midCurvePt = numCurvePoints / 2;

        double x1 = lonToX(currentSeg.curvePoints[midCurvePt].lon());
        double x2 = lonToX(currentSeg.curvePoints[midCurvePt + 1].lon());
        double y1 = latToY(currentSeg.curvePoints[midCurvePt].lat());
        double y2 = latToY(currentSeg.curvePoints[midCurvePt + 1].lat());
        double streetAngle = atan((y2 - y1)/(x2 - x1));
        
        double avgX = (x2 + x1) / 2;
        double avgY = (y2 + y1) / 2;
        
        //modify streetAngle so it is between 0 and 360 degrees
        if (((y2-y1 > 0) && (x2-x1 < 0)) || (y2-y1 < 0 && x2-x1 < 0)) streetAngle += M_PI;
        if ((y2-y1<0) && (x2-x1>0)) streetAngle += 2*M_PI;
        streetAngle = streetAngle / DEGREE_TO_RADIAN;
        if (isnan(streetAngle) == true) streetAngle = 90;
        
        ezgl::surface *oneWayPic = choosePicture(streetAngle);
        g->draw_surface(oneWayPic, {avgX - (5/screenToWorldRatioX), avgY - (5/screenToWorldRatioY)});
        ezgl::renderer::free_surface(oneWayPic);
    }
    
}

//loading arrow images in terms of street angles
ezgl::surface * choosePicture(double angle){
    
    int arrowIcon = (int)(angle+5)/10;
    if (arrowIcon >= 36 || arrowIcon < 0) arrowIcon = 0;
    string imagePath = "libstreetmap/resources/arrows/image("+to_string(arrowIcon)+").png";
    return ezgl::renderer::load_png(imagePath.c_str());
}


//print highlighted street name and number of intersections
void drawHighlightStreetInfo(ezgl::renderer * g, int previousHighlightedStreet, pair <double, double> labelLocation){
    if (streetDatabase[previousHighlightedStreet].highlight == true){
        
        int intersections = streetDatabase[previousHighlightedStreet].intersections.size();
        string streetName = streetDatabase[previousHighlightedStreet].streetName;


        if(!night)
            g->set_color(ezgl::BLACK);
        else
            g->set_color(ezgl::DARK_TEXT);
         
        g->set_font_size(14);
        g->draw_text({labelLocation.first + 20/screenToWorldRatioX, labelLocation.second - 40/screenToWorldRatioY}, streetName);
        g->draw_text({labelLocation.first + 20/screenToWorldRatioX, labelLocation.second - 20/screenToWorldRatioY}, to_string(intersections) + " intersections");
    }
}


//display weather icon based on libcurl info
void drawWeatherInfo(ezgl::renderer * g, string currentWeather, int tempHigh, int tempLow){
    //choose the correct weather icon based on "currentWeather" string, and draw it
    g->set_coordinate_system(ezgl::SCREEN);
    ezgl::surface * weatherIcon;
    if (currentWeather == "clear-day" || currentWeather == "clear-night" || currentWeather == "wind"){
        weatherIcon = ezgl::renderer::load_png("libstreetmap/resources/weather/sun.png");
    } else if (currentWeather == "rain"){
        weatherIcon = ezgl::renderer::load_png("libstreetmap/resources/weather/rain.png");
    }else if (currentWeather == "snow" || currentWeather == "sleet"){
        weatherIcon = ezgl::renderer::load_png("libstreetmap/resources/weather/snowing.png");
    } else if (currentWeather == "partly-cloudy-day" || currentWeather == "partly-cloudy-night"){
        weatherIcon = ezgl::renderer::load_png("libstreetmap/resources/weather/cloudy.png");
    } else if (currentWeather == "cloudy"){
        weatherIcon = ezgl::renderer::load_png("libstreetmap/resources/weather/overcast.png");
    } else {
        weatherIcon = ezgl::renderer::load_png("libstreetmap/resources/weather/fog.png");
    }

    g->draw_surface(weatherIcon,{screenBounds.right()-80 , screenBounds.top()-120});
    ezgl::renderer::free_surface(weatherIcon);
        
    //draw temperature information
    g->set_color(255, 157, 0);
    g->set_font_size(20);
    g->draw_text({screenBounds.right() - 80, screenBounds.top() - 40}, (to_string(tempHigh) + "ºc").c_str());
    if (!night)
        g->set_color(31, 102, 255);
    else
        g->set_color(ezgl::DARK_TEXT);
    g->draw_text({screenBounds.right() - 50, screenBounds.top() - 20}, (to_string(tempLow) + "ºc").c_str());
    
    g->set_coordinate_system(ezgl::WORLD);
}

//draw the advanced search option button
void draw_hamburger_icon(ezgl::renderer * g, bool POIBarShown, bool searchFieldShown, bool currentlySearchActive, string searchText){
    ezgl::surface * hamburger;
    if (night && POIBarShown){
        hamburger = ezgl::renderer::load_png("libstreetmap/resources/hamburger/hamburger_dark_click.png");
    } else if (night && !POIBarShown){
        hamburger = ezgl::renderer::load_png("libstreetmap/resources/hamburger/hamburger_dark_unclick.png");
    } else if (POIBarShown){
        hamburger = ezgl::renderer::load_png("libstreetmap/resources/hamburger/hamburger_light_click.png");
    } else {
        hamburger = ezgl::renderer::load_png("libstreetmap/resources/hamburger/hamburger_light_unclick.png");
    }
    
    g->set_coordinate_system(ezgl::SCREEN);
    g->draw_surface(hamburger, {10, 10});
    ezgl::renderer::free_surface(hamburger);
    
    double rightBound;
    if (searchFieldShown) rightBound = screenBounds.right() - 200;
    else rightBound = 300;
    
    if (!night)
        g->set_color(236, 236, 236);
    else
        g->set_color(67, 69, 71);
    g->fill_rectangle({45, 10}, {rightBound, 40});
    
    if (searchFieldShown) g->set_color(45, 113, 108);
    else g->set_color(ezgl::BLACK);
    g->set_line_width(0);
    g->draw_rectangle({10, 10}, {rightBound, 40});
    ezgl::surface * findIcon;
    if (!currentlySearchActive)
        findIcon = ezgl::renderer::load_png("libstreetmap/resources/findicon.png");
    else 
        findIcon = ezgl::renderer::load_png("libstreetmap/resources/cancelicon.png");
    g->draw_surface(findIcon, {rightBound - 30, 14});
    g->free_surface(findIcon);
    
    g->set_font_size(15);
    g->set_horiz_text_just(ezgl::text_just::left);
    string displayedText = searchText;
    if (!searchFieldShown && searchText.size() > 20){
        displayedText.resize(20);
        displayedText += "...";
    }
    if (night) g->set_color(ezgl::DARK_TEXT);
    else g->set_color(ezgl::BLACK);
    g->draw_text({60, 25}, displayedText);

}

void draw_nightmode_icon(ezgl::renderer * g, bool currentlySearchActive){
    if (currentlySearchActive) return;
    g->set_coordinate_system(ezgl::SCREEN);
    ezgl::surface * moonIcon;
    if(night)
        moonIcon = ezgl::renderer::load_png("libstreetmap/resources/night.png");
    else
        moonIcon = ezgl::renderer::load_png("libstreetmap/resources/day.png");

    g->draw_surface(moonIcon, {screenBounds.right() - 50, 10});
    ezgl::renderer::free_surface(moonIcon);
}

void drawNextIcon(ezgl::renderer * g){
    
    ezgl::surface * nextButton = ezgl::renderer::load_png("libstreetmap/resources/nextButton.png");
    
    g->set_coordinate_system(ezgl::SCREEN);
    g->draw_surface(nextButton, {screenBounds.right() - 100, 10});
    
    ezgl::renderer::free_surface(nextButton);
    
}

//print all advanced search options
void search_icon(ezgl::renderer *g){
    g->set_coordinate_system(ezgl::SCREEN);
   
    //set fill colors for the rectangle, based on whether the button is "clicked" or not
    if (restaurant == true){
        if(!night)
            g->set_color(210, 210, 210);
        else
            g->set_color(0, 0, 0);
    }
    else{
        if(!night)
            g->set_color(255, 255, 255);
        else
            g->set_color(67, 69, 71);
    }
    
    g->fill_rectangle({10,40}, {150, 140});

    if (fastfood == true){
        if(!night)
            g->set_color(210, 210, 210);
        else
            g->set_color(0, 0, 0);
    }
    else{
        if(!night)
            g->set_color(255, 255, 255);
        else
            g->set_color(67, 69, 71);
    }
    
    g->fill_rectangle({10, 140}, {150, 240});

    if (subway == true){
        if(!night)
            g->set_color(210, 210, 210);
        else
            g->set_color(0, 0, 0);
    }
    else{
        if(!night)
            g->set_color(255, 255, 255);
        else
            g->set_color(67, 69, 71);
    }
    
    g->fill_rectangle({10, 240}, {150, 340});

    if (home == true){
        if(!night)
            g->set_color(210, 210, 210);
        else
            g->set_color(0, 0, 0);
    }
    else{
        if(!night)
            g->set_color(255, 255, 255);
        else
            g->set_color(67, 69, 71);
    }
    g->fill_rectangle({10, 340}, {150, 440});
    
    if (bikeshare == true){
        if(!night)
            g->set_color(210, 210, 210);
        else
            g->set_color(0, 0, 0);
    }
    else{
        if(!night)
            g->set_color(255, 255, 255);
        else
            g->set_color(67, 69, 71);
    }
    g->fill_rectangle({10, 440}, {150, 540});
    
    if (traffic == true){
        if(!night)
            g->set_color(210, 210, 210);
        else
            g->set_color(0, 0, 0);
    }
    else{
        if(!night)
            g->set_color(255, 255, 255);
        else
            g->set_color(67, 69, 71);
    }
    g->fill_rectangle({10, 540}, {150, 640});
   

    //drawing the outline of the boxes
    g->set_color(0, 0, 0);
    g->set_line_width(0);
    g->set_line_dash(ezgl::line_dash::none);
    g->draw_rectangle({10, 40}, {150, 140});
    g->draw_rectangle({10, 140}, {150, 240});
    g->draw_rectangle({10, 240}, {150, 340});
    g->draw_rectangle({10, 340}, {150, 440});
    g->draw_rectangle({10, 440}, {150, 540});
    g->draw_rectangle({10, 540}, {150, 640});
    
    //write icon labels
    if(!night)
        g->set_color(ezgl::BLACK);
    else
        g->set_color(ezgl::DARK_TEXT);
    g->set_font_size(12);
    g->set_horiz_text_just(ezgl::text_just::left);
    g->draw_text({85, 90}, "Eateries");
    g->draw_text({85, 190}, "Fast Food");
    g->draw_text({85, 290}, "Subway");
    g->draw_text({85, 390}, "Home");
    g->draw_text({85, 490}, "Bikeshare");
    g->draw_text({85, 590}, "Road");
    g->draw_text({85, 605}, "Condition");
   
    g->set_coordinate_system(ezgl::WORLD);
}
   
//draw all advanced search icons
void draw_png(ezgl::renderer *g) {
    //set coordinate system to screen
    g->set_coordinate_system(ezgl::SCREEN);
    
    //drawing the 4 pngs
    ezgl::surface *png_surface = ezgl::renderer::load_png("libstreetmap/resources/searchIcon/food.png");
    g->draw_surface(png_surface,{20, 70});

    png_surface = ezgl::renderer::load_png("libstreetmap/resources/searchIcon/fastfood.png");
    g->draw_surface(png_surface,{20, 170});
    
    png_surface = ezgl::renderer::load_png("libstreetmap/resources/searchIcon/subway.png");
    g->draw_surface(png_surface,{20, 270});
    
    png_surface = ezgl::renderer::load_png("libstreetmap/resources/searchIcon/home.png");
    g->draw_surface(png_surface,{20, 370});
    
    png_surface = ezgl::renderer::load_png("libstreetmap/resources/searchIcon/bikeshare.png");
    g->draw_surface(png_surface,{20, 470});
    
    png_surface = ezgl::renderer::load_png("libstreetmap/resources/searchIcon/traffic.png");
    g->draw_surface(png_surface,{20, 570});
    
    ezgl::renderer::free_surface(png_surface);
    
    //set the coordinate system back to world
    g->set_coordinate_system(ezgl::WORLD);
    
}

//write POI names
void displayPOI(ezgl::renderer *g, bool currentlySearchActive){
    
    int numPOI = 0;
    int maxNumPOI = 0;
    double width = bounds.right() - bounds.left();
    if (width > 0.00035) return;
    //limit number of POI shown on the screen
    if (width > 0.00025){
        maxNumPOI = 8;
    } else if(width > 0.0001){
        maxNumPOI = 12;
    } else{
        maxNumPOI = 16;
    }

    g->set_font_size(8);
    
    for(auto it = POIDatabase.begin(); it != POIDatabase.end() && !currentlySearchActive; it++){
        pair <double, double> & Pt = it->POIPositionXY;
        //POI is not in visible bound: skip it
        if (Pt.first < bounds.left() || Pt.first > bounds.right() || Pt.second < bounds.bottom() || Pt.second > bounds.top()){
            continue;
        }
        
        const OSMNode Node = *(OSMNodeData[it->POIOSMID]);
        //get tag
        for(int i = 0; i < getTagCount(&Node); i++){
            pair<string, string> tag = getTagPair(&Node, i);
          
            ezgl::point2d point(Pt.first, Pt.second + 20/screenToWorldRatioY);
            
            if (tag.first == "amenity"){

                if ((tag.second == "cafe" || tag.second == "restaurant" || tag.second == "pub" || tag.second=="bar") && restaurant == true){
                    if(!night)
                        g->set_color(51, 81, 130);
                    else
                        g->set_color(ezgl::DARK_TEXT);
                    g->draw_text(point, it->POIName,  0.0000015, DBL_MAX);
                    numPOI++;
                }
                
                if ((tag.second == "fast_food" || tag.second == "food_court") && fastfood == true){
                    if(!night)
                        g->set_color(51, 81, 130);
                    else
                        g->set_color(ezgl::DARK_TEXT);
                    g->draw_text(point, it->POIName,  0.0000015, DBL_MAX);
                    numPOI++;
                }
                
                if (tag.second == "college" || tag.second == "library" || tag.second == "university" || tag.second == "bank" || 
                        tag.second == "hospital" || tag.second == "clinic" || tag.second == "doctors" || tag.second == "pharmacy"){
                    if(!night)
                        g->set_color(51, 81, 130);
                    else
                        g->set_color(ezgl::DARK_TEXT);
                    g->draw_text(point, it->POIName,  0.000002, DBL_MAX);
                    numPOI++;
                }   
            }
            
            if (numPOI > maxNumPOI) break;
        }
        //constrain the number of POI displayed to avoid overcrowded screen; only keep going if under maxpoi
    }
}

//draw all POI icons at their locations
void drawPOIIcon(ezgl::renderer *g){
    
    //clears POIIcon vector
    displayedPOIIcon.clear();
    
    int numIcon = 0;
    int maxNumIcon = 40;
    double width = bounds.right() - bounds.left();
    if (width > 0.0005) return;
 
    //initialize different sized icons according to current view area
    ezgl::surface *pubIcon;
    ezgl::surface *bookIcon;
    ezgl::surface *cafeIcon;
    ezgl::surface *restaurantIcon;
    ezgl::surface *burgerIcon;
    ezgl::surface *bikeShareIcon;
    ezgl::surface *bankIcon;
    ezgl::surface *hospitalIcon;
    
    string suffix;
    if (width > 0.00035){
        maxNumIcon = 80;
        suffix = "_sm.png\0";
    } else if (width > 0.0001){
        maxNumIcon = 60;
        suffix = "_m.png\0";
    }else {
        suffix = ".png\0";
    }
    
    pubIcon = ezgl::renderer::load_png(("libstreetmap/resources/POIIcons/beer" + suffix).c_str());
    bookIcon = ezgl::renderer::load_png(("libstreetmap/resources/POIIcons/book"+suffix).c_str());
    cafeIcon = ezgl::renderer::load_png(("libstreetmap/resources/POIIcons/cafe"+suffix).c_str());
    restaurantIcon = ezgl::renderer::load_png(("libstreetmap/resources/POIIcons/restaurant"+suffix).c_str());
    burgerIcon = ezgl::renderer::load_png(("libstreetmap/resources/POIIcons/burger"+suffix).c_str());
    bankIcon = ezgl::renderer::load_png(("libstreetmap/resources/POIIcons/bank"+suffix).c_str());
    hospitalIcon = ezgl::renderer::load_png(("libstreetmap/resources/POIIcons/hospital"+suffix).c_str());
    bikeShareIcon = ezgl::renderer::load_png(("libstreetmap/resources/POIIcons/bikeshare"+suffix).c_str());

    for(auto it = POIDatabase.begin(); it != POIDatabase.end(); it++){
        pair <double, double>  & Pt = it->POIPositionXY;
        //POI is not in visible bound: skip it
        if (Pt.first < bounds.left() || Pt.first > bounds.right() || Pt.second < bounds.bottom() || Pt.second > bounds.top()){
            continue;
        }
        
        const OSMNode Node = *(OSMNodeData[it->POIOSMID]);
        //get tag
        for(int i = 0; i < getTagCount(&Node); i++){
            pair<string, string> tag = getTagPair(&Node, i);
            if(tag.first == "amenity" && numIcon <= maxNumIcon){
                ezgl::point2d point(Pt.first, Pt.second);
                
                struct POIIcon icon;
                icon.name = it->POIName;
                icon.xmin = Pt.first - (8/screenToWorldRatioX);
                icon.xmax =  Pt.first + (8/screenToWorldRatioX);
                icon.ymin= Pt.second - (8/screenToWorldRatioY);
                icon.ymax = Pt.second + (8/screenToWorldRatioY);
                icon.x = Pt.first, icon.y = Pt.second;
                
                if (tag.second == "cafe" && restaurant == true){
                    g->draw_surface(cafeIcon, {Pt.first - (8/screenToWorldRatioX), Pt.second - (8/screenToWorldRatioY)});
                    numIcon++;
                    displayedPOIIcon.push_back(icon);
                    cout<<icon.name<<endl;
                } else if ((tag.second == "bar" || tag.second == "pub") && restaurant == true){
                    g->draw_surface(pubIcon, {Pt.first - (8/screenToWorldRatioX), Pt.second - (8/screenToWorldRatioY)});
                    numIcon++;
                    displayedPOIIcon.push_back(icon);
                    cout<<icon.name<<endl;
                } else if ((tag.second == "fast_food" || tag.second == "food_court") && fastfood == true) {
                    g->draw_surface(burgerIcon, {Pt.first - (8/screenToWorldRatioX), Pt.second - (8/screenToWorldRatioY)});
                    numIcon++;
                    displayedPOIIcon.push_back(icon);  
                    cout<<icon.name<<endl;                  
                } else if (tag.second == "restaurant" && restaurant == true) {
                    g->draw_surface(restaurantIcon, {Pt.first - (8/screenToWorldRatioX), Pt.second - (8/screenToWorldRatioY)});
                    numIcon++;
                    displayedPOIIcon.push_back(icon);  
                    cout<<icon.name<<endl;                 
                } else if ((tag.second == "college" || tag.second == "library" || tag.second == "university") && (restaurant == false && fastfood == false)){
                    g->draw_surface(bookIcon, {Pt.first - (8/screenToWorldRatioX), Pt.second - (8/screenToWorldRatioY)});
                    numIcon++;
                    displayedPOIIcon.push_back(icon);  
                } else if (tag.second == "bank" && (restaurant == false && fastfood == false)){
                    g->draw_surface(bankIcon, {Pt.first - (8/screenToWorldRatioX), Pt.second - (8/screenToWorldRatioY)});
                    numIcon++;
                    displayedPOIIcon.push_back(icon);  
                } else if ((tag.second == "hospital" || tag.second == "clinic" || tag.second == "doctors" || tag.second == "pharmacy") && (restaurant == false && fastfood == false)){
                    g->draw_surface(hospitalIcon, {Pt.first - (8/screenToWorldRatioX), Pt.second - (8/screenToWorldRatioY)});
                    numIcon++;
                    displayedPOIIcon.push_back(icon);  
                }
            } 
            if (tag.first == "amenity" && tag.second == "bicycle_rental" && bikeshare == true){
                g->draw_surface(bikeShareIcon, {Pt.first - (8/screenToWorldRatioX), Pt.second - (8/screenToWorldRatioY)});
            }
        }
    }
    //free all icons
    ezgl::renderer::free_surface(pubIcon);
    ezgl::renderer::free_surface(bookIcon);
    ezgl::renderer::free_surface(cafeIcon);
    ezgl::renderer::free_surface(restaurantIcon);
    ezgl::renderer::free_surface(burgerIcon);
    ezgl::renderer::free_surface(bikeShareIcon);
    ezgl::renderer::free_surface(bankIcon);
    ezgl::renderer::free_surface(hospitalIcon);
}


//draws all intersections that are either clicked on or found through search
void drawFoundIntersection(ezgl::renderer * g, int & previousHighlightedIntersection, vector <int> & previousFoundIntersection, int foundIntersectionIndex, int highlightedStreet, bool & cameraMoved){

    if (intersectionGraph[previousHighlightedIntersection].highlight == true && streetDatabase[highlightedStreet].highlight == false){
        ezgl::surface *pinIcon = ezgl::renderer::load_png("libstreetmap/resources/pin.png");
        
        g->draw_surface(pinIcon, {intersectionGraph[previousHighlightedIntersection].xPos - 16/screenToWorldRatioX, 
                                intersectionGraph[previousHighlightedIntersection].yPos - 32/screenToWorldRatioY});
        ezgl::renderer::free_surface(pinIcon);
        if (bounds.right() - bounds.left() < 0.001){
            if(!night)
                g->set_color(ezgl::BLACK);
            else
                g->set_color(ezgl::DARK_TEXT);
            g->set_font_size(12);
            g->draw_text({intersectionGraph[previousHighlightedIntersection].xPos, intersectionGraph[previousHighlightedIntersection].yPos + 12/screenToWorldRatioY}, 
                    intersectionGraph[previousHighlightedIntersection].name);
        }
//        cout<<"intersection name: "<<previousHighlightedIntersection<<endl;
    }
    
    //draw pin on the found intersection, and write its name
    ezgl::surface *pinIconb = ezgl::renderer::load_png("libstreetmap/resources/bluepin.png");
    for (auto i = previousFoundIntersection.begin(); i != previousFoundIntersection.end(); i++){
        double elementSize = pow((bounds.right() - bounds.left()), 0.75) / 6000;
        g->set_color(227, 147, 124);
        g->fill_arc({intersectionGraph[*i].xPos, intersectionGraph[*i].yPos}, elementSize*3, 0 , 360);
        
        g->draw_surface(pinIconb, {intersectionGraph[*i].xPos - 16/screenToWorldRatioX, intersectionGraph[*i].yPos - 32/screenToWorldRatioY});

        if (bounds.right() - bounds.left() < 0.004){
            if(!night)
                g->set_color(ezgl::BLACK);
            else
                g->set_color(ezgl::DARK_TEXT);
            g->set_font_size(12);
            g->draw_text({intersectionGraph[*i].xPos, intersectionGraph[*i].yPos + 12/screenToWorldRatioY}, intersectionGraph[*i].name);
        }
    }
    ezgl::renderer::free_surface(pinIconb);
    
    //move the camera to be over one of the results
    if (!cameraMoved && !showPath && previousFoundIntersection.size() != 0){
        intersecNode foundIntersection = intersectionGraph[previousFoundIntersection[foundIntersectionIndex]];
        double screenHeight = bounds.bottom() - bounds.top(), screenWidth = bounds.right() - bounds.left();
        ezgl::rectangle newWorld({foundIntersection.xPos - 0.00005, foundIntersection.yPos - 0.00005*screenHeight / screenWidth}, 
                                {foundIntersection.xPos + 0.00005, foundIntersection.yPos + 0.00005*screenHeight / screenWidth});
        g->set_visible_world(newWorld);
        cameraMoved = true;
    }

}


void drawNavigationIntersections(ezgl::renderer * g, int fromID, int toID){
    g->set_coordinate_system(ezgl::WORLD);
    if(!night) g->set_color(ezgl::BLACK);
    else g->set_color(ezgl::DARK_TEXT);
    g->set_font_size(159);
    
    if (fromID != -1) {
        ezgl::surface *pinIcon = ezgl::renderer::load_png("libstreetmap/resources/pin.png");
        g->draw_surface(pinIcon, {intersectionGraph[fromID].xPos - 16/screenToWorldRatioX, 
                                intersectionGraph[fromID].yPos - 32/screenToWorldRatioY});
        g->free_surface(pinIcon);
    }
    
    if (toID != -1) {
        ezgl::surface *flagIcon = ezgl::renderer::load_png("libstreetmap/resources/redFlag.png");
        g->draw_surface(flagIcon, {intersectionGraph[toID].xPos - 4/screenToWorldRatioX, 
                                intersectionGraph[toID].yPos - 32/screenToWorldRatioY});
        g->free_surface(flagIcon);
    }
    
    //draw the car pickup location
    if (walkPath.size() !=0 && drivePath.size() != 0){
        
        segmentNode & seg1 = segmentDatabase[*(walkPath.end()-1)];
        segmentNode & seg2 = segmentDatabase[*(drivePath.begin())];
        pair <double, double> intersec;
        if (seg2.curvePointsXY[0] == seg1.curvePointsXY[0] || seg2.curvePointsXY[0] == seg1.curvePointsXY[seg1.curvePointsXY.size() -1]){
            intersec = seg2.curvePointsXY[0];
        } else {
            intersec = seg2.curvePointsXY[seg2.curvePointsXY.size()-1];
        }
        
        ezgl::surface * carPin = ezgl::renderer::load_png("libstreetmap/resources/carPin.png");
        g->draw_surface(carPin, {intersec.first - 20/screenToWorldRatioX, intersec.second - 30/screenToWorldRatioY});
        g->free_surface(carPin);
        
        
    }
    

    
}
//move camera over found street, and write its name
void drawFoundStreets(ezgl::renderer * g, vector<streetNode> & foundStreets, int foundStreetIndex, bool & cameraMoved, int previousHighlightedStreet){
    
    double screenHeight = bounds.bottom() - bounds.top(), screenWidth = bounds.right() - bounds.left();
    segmentNode currentSeg = segmentDatabase[foundStreets[foundStreetIndex].streetSegments[0]];
    
     int curvePointCount = currentSeg.curvePointsXY.size();
     double xPos = (currentSeg.curvePointsXY)[curvePointCount/2].first;
     double yPos = (currentSeg.curvePointsXY)[curvePointCount/2].second;
     
    if (!cameraMoved && !showPath) {
        //set the camera 
        ezgl::rectangle newWorld({xPos - 0.0003, yPos - 0.0003*screenHeight / screenWidth}, {xPos + 0.0003, yPos + 0.0003*screenHeight / screenWidth});
        g->set_visible_world(newWorld);
        cameraMoved = true;
    }
    
    if (screenWidth < 0.003 && streetDatabase[previousHighlightedStreet].highlight == false){
        if(!night)
            g->set_color(ezgl::BLACK);
        else
            g->set_color(ezgl::DARK_TEXT);
        
        g->set_font_size(12);
        g->draw_text({xPos, yPos}, foundStreets[foundStreetIndex].streetName);
    }

}

//draw all road closures according to libcurl
void drawroadClosure(ezgl::renderer * g){
    //only if bounds<0.001 then draw road closure
    g->set_coordinate_system(ezgl::WORLD);
    
    if (traffic == 0 || bounds.right() - bounds.left() > 0.003) return;
    
    
    ezgl::surface *closeIcon;
    ezgl::surface *constructionIcon;
    if (bounds.right() - bounds.left() > 0.0001){
        closeIcon = ezgl::renderer::load_png("libstreetmap/resources/POIIcons/close_sm.png");
        constructionIcon = ezgl::renderer::load_png("libstreetmap/resources/POIIcons/construction_sm.png");
    } else{
        closeIcon = ezgl::renderer::load_png("libstreetmap/resources/POIIcons/close.png");
        constructionIcon = ezgl::renderer::load_png("libstreetmap/resources/POIIcons/construction.png");
    }

    g->set_font_size(10);
    g->set_color(ezgl::BLACK);
        
    for(auto it = closureInfo.begin(); it != closureInfo.end(); it++){
        double y = latToY (it->latitude);
        double x = lonToX (it-> longitude);

        //draw icon
        if((it->type) == "CONSTRUCTION"){
            g->draw_surface(constructionIcon, {x, y - (8/screenToWorldRatioY)});
        }else{
            g->draw_surface(closeIcon, {x, y - (8/screenToWorldRatioY)});
        }
        
        //write text
        if (bounds.right() - bounds.left() < 0.001){
            if((it->type) == "CONSTRUCTION"){
                g->set_font_size(10);
                g->set_color(51, 81, 130);
                g->set_text_rotation(0);
                g->draw_text({x+(8/screenToWorldRatioX),y +(10/screenToWorldRatioY)},"construction");
            }else{
                g->set_font_size(10);
                g->set_color(51, 81, 130);
                g->set_text_rotation(0);
                g->draw_text({x+(8/screenToWorldRatioX),y +(10/screenToWorldRatioY)},"closure");
            }
            
        }
    }
    
    ezgl::renderer::free_surface(closeIcon);
    ezgl::renderer::free_surface(constructionIcon);
}


void drawHomePin(ezgl::renderer * g, bool & startSetHome, bool & homeSet, pair <double, double> & homeLocation){
    //got to drawHomePin_first_time if first time setting home
    if(firstSetHome==true){
        drawHomePin_first_time(g, startSetHome, homeSet, homeLocation);
    }
    
    //determine homeset value
    string fileName;
    if(homeSet==false){
        //get file Location
        int nameLen = mapName.length()-1;
        fileName = mapName.substr(26, nameLen);
        //eliminate punctuation
        for (int i = 0, len = fileName.size(); i < len; i++) { 
        // check whether parsing character is punctuation or not 
            if (ispunct(fileName[i])) {
                fileName.erase(i--, 1); 
                len = fileName.size(); 
            } 
        }

       string fileLoc="libstreetmap/resources/home/" + fileName;
        //detect if file for the home location exist
        if(FILE *thisFile=fopen(fileLoc.c_str(),"r")){
            homeSet=true;
            fclose(thisFile);
        }else{
            homeSet=false;
        }
    }
        
        
    //if startSetHome is true, then display
    if(startSetHome==True){
        g->set_coordinate_system(ezgl::SCREEN);
        g->set_font_size(14);
        g->set_color(51, 153, 255);
        g->set_horiz_text_just(ezgl::text_just::center);
        g->draw_text({230, 420}, "Click your home");
        g->draw_text({230, 450}, "on the map");
        g->set_coordinate_system(ezgl::WORLD); 
        //mark this is the first time to set home
        firstSetHome=true;
    }
    
    //if not startsethome, then home is set
    //get home position
    string fileLoc="libstreetmap/resources/home/" + fileName;
    ifstream myfile (fileLoc);
    string xstr, ystr;
    double yloc, xloc;
    if (myfile.is_open()){
        getline (myfile,xstr);
        getline (myfile,ystr);
        myfile.close();
    }
    //conversion
    xloc=atof(xstr.c_str());
    yloc=atof(ystr.c_str());
    if(xloc!=0 && yloc !=0){
        homeLocation.first=xloc;
        homeLocation.second=yloc;
    }
    
      //draws home if home is already set
    if(homeSet==true){
        //draw home on the screen
        ezgl::surface *png_home = ezgl::renderer::load_png("libstreetmap/resources/homepin.png");
        g->draw_surface(png_home, {homeLocation.first- (12.5/screenToWorldRatioX), homeLocation.second  - (12.5/screenToWorldRatioY)});
        ezgl::renderer::free_surface(png_home);
    }
    //if home clicked, zoom in
    if(home==1){
        double screenHeight = bounds.bottom() - bounds.top(), screenWidth = bounds.right() - bounds.left();
        ezgl::rectangle newWorld({homeLocation.first - 0.00005, homeLocation.second - 0.00005*screenHeight / screenWidth}, {homeLocation.first + 0.00005, homeLocation.second + 0.00005*screenHeight / screenWidth});
        g->set_visible_world(newWorld);
        home=0;
    }
}

//draws home Pin: this is for first time setting home
void drawHomePin_first_time(ezgl::renderer * g, bool & startSetHome, bool & homeSet, pair <double, double> & homeLocation){
    if(startSetHome==True){
        g->set_coordinate_system(ezgl::SCREEN);
        g->set_font_size(14);
        g->set_color(51, 153, 255);
        g->set_horiz_text_just(ezgl::text_just::center);
        g->draw_text({230, 420}, "Click your home");
        g->draw_text({230, 450}, "on the map");
        g->set_coordinate_system(ezgl::WORLD);   
    }
    
    if(homeSet==true){
        ezgl::surface *png_home = ezgl::renderer::load_png("libstreetmap/resources/homepin.png");
        g->draw_surface(png_home, {homeLocation.first - (12.5/screenToWorldRatioX), homeLocation.second - (12.5/screenToWorldRatioY)});
        ezgl::renderer::free_surface(png_home);
    }
    
    //if home clicked, zoom in
    if(home==1){
        double screenHeight = bounds.bottom() - bounds.top(), screenWidth = bounds.right() - bounds.left();
        ezgl::rectangle newWorld({homeLocation.first - 0.00005, homeLocation.second - 0.00005*screenHeight / screenWidth}, {homeLocation.first + 0.00005, homeLocation.second + 0.00005*screenHeight / screenWidth});
        g->set_visible_world(newWorld);
        home=0;
    }
}


void drawNavigationPanelIcon(ezgl::renderer * g){
    g->set_coordinate_system(ezgl::SCREEN);
    ezgl::surface * navIcon = ezgl::renderer::load_png("libstreetmap/resources/directions.png");
    g->draw_surface(navIcon, {screenBounds.right() - 45, 400});
    g->free_surface(navIcon);
}


void drawNavigationPanel(ezgl::renderer * g, string & fromText, string & toText, int previousShownStep, int numWalkStreets, int numDriveStreets, int errorCode){
    g->set_coordinate_system(ezgl::SCREEN);
    
    if (night) 
        g->set_color(ezgl::DARK_BACKGROUND);
    else
        g->set_color(ezgl::WHITE);
    
    g->fill_rectangle({screenBounds.right() - 600, screenBounds.top() - 300}, {screenBounds.right(), screenBounds.top() - 900});
    g->set_color(ezgl::BLACK);
    g->draw_rectangle({screenBounds.right() - 600, screenBounds.top() - 300}, {screenBounds.right(), screenBounds.top() - 900});
    
    drawSearchField(g, screenBounds.top() - 850, searchMode == 2, fromText);
    drawSearchField(g, screenBounds.top() - 750, searchMode == 3, toText);
    g->draw_text({screenBounds.right() - 580, screenBounds.top() - 870}, "From");
    g->draw_text({screenBounds.right() - 580, screenBounds.top() - 770}, "To");
    
    g->set_color(ezgl::LIGHT_WATER);
    g->fill_rectangle({screenBounds.right() - 70, screenBounds.top() - 630}, {screenBounds.right(), screenBounds.top() - 670});
    g->set_color(ezgl::BLACK);
    if (!showPath)
        g->draw_text({screenBounds.right() - 62, screenBounds.top() - 650}, "ROUTE!");
    else
        g->draw_text({screenBounds.right() - 62, screenBounds.top() - 650}, "Next >");
    
    
    if ((!walk) || showPath)
        g->set_color(23, 209, 73);
    else if (!showPath)
        g->set_color(12, 171, 54);
    g->fill_rectangle({screenBounds.right() - 70, screenBounds.top() - 570}, {screenBounds.right(), screenBounds.top() - 610});
    g->set_color(ezgl::BLACK);
    if (!showPath && !walk) {
        g->draw_text({screenBounds.right() - 60, screenBounds.top() - 590}, "WALK");
    } else if (!showPath && walk){
        g->draw_text({screenBounds.right() - 60, screenBounds.top() - 590}, "walking");
    } else {
        g->draw_text({screenBounds.right() - 60, screenBounds.top() - 590}, "< Back");
    }
    
    g->set_color(ezgl::CORAL);
    g->fill_rectangle({screenBounds.right() - 70, screenBounds.top() - 400}, {screenBounds.right(), screenBounds.top() - 360});
    g->set_color(ezgl::BLACK);
    if (showPath)
        g->draw_text({screenBounds.right() - 64, screenBounds.top() - 380}, "CANCEL");
    else 
        g->draw_text({screenBounds.right() - 64, screenBounds.top() - 380}, "CLOSE");
    
    ezgl::surface * help = ezgl::renderer::load_png("libstreetmap/resources/help.png");
    g->draw_surface(help, {screenBounds.right() - 50, screenBounds.top() - 350});
    ezgl::renderer::free_surface(help);
    
    
    //draw the current transportation mode (walk / drive)
    g->set_font_size(28);
    if (showPath && walkPath.size() != 0 && previousShownStep <= numWalkStreets){
        g->set_color(12, 171, 54);
        g->draw_text({screenBounds.right() - 500, screenBounds.top() - 680}, "Walking:");
        g->draw_line({screenBounds.right() - 500, screenBounds.top() - 650}, {screenBounds.right() - 300, screenBounds.top() - 650});
    } else if (showPath){
        g->set_color(87, 178, 247);
        g->draw_text({screenBounds.right() - 500, screenBounds.top() - 680}, "Driving:");
        g->draw_line({screenBounds.right() - 500, screenBounds.top() - 650}, {screenBounds.right() - 300, screenBounds.top() - 650});
    }
    
    
    int totalSteps = numWalkStreets + numDriveStreets;
    if (previousShownStep != -1 && totalSteps >0){
        //draw status bar
        g->set_color(ezgl::BLACK);
        g->draw_rectangle({screenBounds.right() - 500, screenBounds.top() - 350}, {screenBounds.right() - 100, screenBounds.top() - 360});
        
        double width = 400.0 / totalSteps;
        if (previousShownStep < numWalkStreets){
            g->set_color(23, 209, 73);
            g->fill_rectangle({screenBounds.right() - 500, screenBounds.top() - 350}, {screenBounds.right() - 500 + (previousShownStep)*width, screenBounds.top() - 360});
        } else {
            g->set_color(23, 209, 73);
            g->fill_rectangle({screenBounds.right() - 500, screenBounds.top() - 350}, {screenBounds.right() - 500 + (numWalkStreets)*width, screenBounds.top() - 360});
            g->set_color(54, 168, 255);
            g->fill_rectangle({screenBounds.right() - 500+(numWalkStreets)*width, screenBounds.top() - 350}, {screenBounds.right() - 500 + (previousShownStep)*width, screenBounds.top() - 360});
        }

        ezgl::surface * pin;
        if (previousShownStep < numWalkStreets || (previousShownStep == numWalkStreets && drivePath.size() == 0))
            pin= ezgl::renderer::load_png("libstreetmap/resources/walkPin.png");
        else
            pin = ezgl::renderer::load_png("libstreetmap/resources/carPin.png");
        g->draw_surface(pin, {screenBounds.right() - 520 + (previousShownStep)*width, screenBounds.top() - 400});
        g->free_surface(pin);
    }
    
    //draw errors. Error code == 0 means no error, error code == 1 means path not found, error 2 means walking parameter error
    g->set_color(ezgl::RED);
    g->set_font_size(20);
    if (errorCode == 1){
        g->draw_text({screenBounds.right() -260, screenBounds.top() - 590}, "Error: no path found");
    } else if (errorCode == 2){
        g->draw_text({screenBounds.right() -440, screenBounds.top() - 590}, "Error: walking speed / time invalid");
    } else if (errorCode == 3){
        if (!walk)
            g->set_color(87, 178, 247);
        else
            g->set_color(12, 171, 54);
        g->draw_text({screenBounds.right() - 260, screenBounds.top() - 650}, "Press Route -->");
    } else if (errorCode == 4){
        g->draw_text({screenBounds.right() - 380, screenBounds.top() - 590}, "Error: unknown intersection(s)");
    }
    
    //draw walking speed and walking time
    if (walk){
        if (night) g->set_color(ezgl::DARK_TEXT);
        else g->set_color(ezgl::BLACK);
        g->set_font_size(13);
        
        std::stringstream stream1, stream2;
        stream1<<std::fixed<<std::setprecision(1)<<walkingSpeed;
        stream2<<std::fixed<<std::setprecision(1)<<walkingTime;
        
        string walkSpeedStr = "Walking speed: "+stream1.str()+"m/s";
        string walkTimeStr = "Walking time: "+stream2.str() + "s";
        g->draw_text({screenBounds.right() - 160, screenBounds.top() - 550}, walkSpeedStr);
        g->draw_text({screenBounds.right() - 160, screenBounds.top() - 520}, walkTimeStr);
    }
    
    //draw navigation instruction
    if (showPath) drawNavInstruction(g, previousShownStep, numWalkStreets, numDriveStreets);
}

void drawSearchField(ezgl::renderer * g, double sy, bool active, string & searchText){
    
    
    if (!night && !active)
        g->set_color(ezgl::WHITE);
    else if (!night)
        g->set_color(236, 236, 236);
    else if (night && !active)
        g->set_color(67, 69, 71);
    else
        g->set_color(30, 30, 30);
    g->fill_rectangle({screenBounds.right() - 580, sy}, {screenBounds.right() - 50, sy + 30});
    
    if (active) g->set_color(45, 113, 108);
    else g->set_color(ezgl::BLACK);
    g->set_line_width(0);
    g->draw_rectangle({screenBounds.right() - 580, sy}, {screenBounds.right() - 50, sy + 30});

    g->set_font_size(15);
    g->set_horiz_text_just(ezgl::text_just::left);
    string displayedText = searchText;
    if (searchText.size() >= 60){
        displayedText.resize(60);
        searchText.resize(60);
        displayedText += "...";
    }
    if (night) g->set_color(ezgl::DARK_TEXT);
    else g->set_color(ezgl::BLACK);
    g->draw_text({screenBounds.right() - 560, sy + 14}, displayedText);
}

//angle 1: turns off from this street
//angle 2: turn onto this street
//x, y: the intersection of interest
void draw_arrow(double angle1, double angle2, double x, double y, ezgl::renderer *g){
    double w = pow((bounds.right() - bounds.left()), 0.75) / 5000;
    double x1 = x - 3*w*cos(angle1);
    double y1 = y - 3*w*sin(angle1);
    double alpha = angle1 + M_PI/2;
    
    double x2 = x1 - w*cos(alpha);
    double y2 = y1 - w*sin(alpha);
    double x3 = x1 + w*cos(alpha);
    double y3 = y1 + w*sin(alpha);
    
    double phi = (angle1 + angle2)/2;
    double x4 = x - w*cos(phi);
    double y4 = y - w*sin(phi);
    double x5 = x + w*cos(phi);
    double y5 = y + w*sin(phi);
    
    double x6 = x - 3*w*cos(angle2);
    double y6 = y - 3*w*sin(angle2);
    double psi = angle2 + M_PI/2;
    double x7 = x6 - w*cos(psi);
    double y7 = y6 - w*sin(psi);
    double x8 = x6 + w*cos(psi);
    double y8 = y6 + w*sin(psi);
 //arrowhead
    double x9 = x6 - 2*w*cos(psi);
    double y9 = y6 - 2*w*sin(psi);
    double x10 = x6 + 2*w*cos(psi);
    double y10 = y6 + 2*w*sin(psi);
    double x11 = x - 5*w*cos(angle2);
    double y11 = y - 5*w*sin(angle2);

    g->fill_poly({{x2, y2}, {x4, y4}, {x8, y8}, {x9, y9}, {x11, y11}, {x10, y10}, {x7, y7}, {x5, y5}, {x3, y3}});
    g->fill_poly({{x2, y2}, {x5, y5}, {x8, y8}, {x9, y9}, {x11, y11}, {x10, y10}, {x7, y7}, {x4, y4}, {x3, y3}});
   
   
}

string getDirection(int segID1, int segID2, double angle1, double angle2){
    streetNode & str1 = streetDatabase[segmentDatabase[segID1].streetID];
    streetNode & str2 = streetDatabase[segmentDatabase[segID2].streetID];
    string name1 = str1.streetName;
    string name2 = str2.streetName;
    
    if (name1 == "<unknown>" || name1 == "NO_NAME"){
        name1 = "unnamed segment";
    }
    if (name2 == "<unknown>" || name2 == "NO_NAME"){
        name2 = "unnamed segment";
    }
    
    //normalize all angles to between 0 and 2*pi
    while (abs(angle2 - angle1) > M_PI){
        if (angle2 > angle1) angle2 -= 2*M_PI;
        else angle1 -= 2*M_PI;
    }
    
    string direction;
    if (abs(angle2 - angle1) < 0.2){
        direction = "Continue";
    } else if (angle2 < angle1) direction = "Make a right turn";
    else direction = "Make a left turn";
    return direction+" from "+name1+" to "+name2;
}

string drawAllArrows(ezgl::renderer * g, int index, bool cameraMoved){
    string result;
    g->set_coordinate_system(ezgl::WORLD);
    int counter = 1;
    
    //merge the paths to form a single vector
    vector<int> path = walkPath;
    path.insert(path.end(), drivePath.begin(), drivePath.end());
    
    //find out where the pickup happens or destination happens
    int lastWalk = -1, lastDrive = -1;
    if (walkPath.size() > 0)
        lastWalk = *(walkPath.end()-1);
    if (drivePath.size() > 0)
        lastDrive = *(drivePath.end()-1);
    
    if (path.size() <=1) return result;
    
    
    if (index == 0){
        //bring camera to starting location
        pair<double, double> intersec;
        segmentNode & seg1 = segmentDatabase[*(path.begin())], & segNext = segmentDatabase[*(path.begin()+1)];
        if (segNext.curvePointsXY[0] == seg1.curvePointsXY[0] || segNext.curvePointsXY[segNext.curvePointsXY.size()-1] == seg1.curvePointsXY[0]){
            intersec = seg1.curvePointsXY[seg1.curvePointsXY.size()-1];
        } else {
            intersec = seg1.curvePointsXY[0];
        }
        if (!cameraMoved){
            double screenHeight = bounds.bottom() - bounds.top(), screenWidth = bounds.right() - bounds.left();
            ezgl::rectangle newWorld({intersec.first - 0.00003, intersec.second - 0.00003*screenHeight / screenWidth}, {intersec.first + 0.00003, intersec.second + 0.00003*screenHeight / screenWidth});
            g->set_visible_world(newWorld);
        }
    }
    
    
    for (auto it = path.begin(); it != path.end(); it++){
        segmentNode & seg1 = segmentDatabase[*it];
        segmentNode seg2;
        if (it+1 != path.end()) seg2 = segmentDatabase[*(it+1)];
        
        if (seg1.streetID == seg2.streetID && (*it) != lastWalk && (*it) != lastDrive && counter != 0) continue;
        pair<double, double> cp1, cp2, intersec;
        double angle1 = 0, angle2 = 0, angle3 = 0;

        if (*it != lastWalk && *it != lastDrive){
            //not a pickup intersection or destination
            if (seg1.curvePointsXY[0] == seg2.curvePointsXY[0]){
                intersec = seg1.curvePointsXY[0];
                cp1 = seg1.curvePointsXY[1];
                cp2 = seg2.curvePointsXY[1];
            } else if (seg1.curvePointsXY[0] == seg2.curvePointsXY[seg2.curvePointsXY.size()-1]){
                intersec = seg1.curvePointsXY[0];
                cp1 = seg1.curvePointsXY[1];
                cp2 = seg2.curvePointsXY[seg2.curvePointsXY.size()-2];
            } else if (seg1.curvePointsXY[seg1.curvePointsXY.size() -1] == seg2.curvePointsXY[0]){
                intersec = seg2.curvePointsXY[0];
                cp1 = seg1.curvePointsXY[seg1.curvePointsXY.size()-2];
                cp2= seg2.curvePointsXY[1];
            } else {
                intersec = seg1.curvePointsXY[seg1.curvePointsXY.size()-1];
                cp1 = seg1.curvePointsXY[seg1.curvePointsXY.size()-2];
                cp2 = seg2.curvePointsXY[seg2.curvePointsXY.size()-2];
            }
            angle1 = atan((intersec.second - cp1.second)/(intersec.first - cp1.first));
            angle2 = atan((intersec.second - cp2.second) / (intersec.first - cp2.first));
            angle3 = atan((cp2.second - intersec.second) / (cp2.first - intersec.first));
            if ((intersec.first - cp1.first)<0) angle1 += M_PI;
            if ((intersec.first - cp2.first)<0) angle2 += M_PI;
            if (cp2.first - intersec.first < 0)angle3 += M_PI;
            
            if (counter != 0 && index == counter){
                g->set_color(255, 134, 54);
            } else {
                g->set_color(250, 191, 152);
            }
            draw_arrow(angle1, angle2, intersec.first, intersec.second, g);
        } else if ((*it) == *(path.end()-1)){
            segmentNode & segLast = segmentDatabase[*(it-1)];
            if (segLast.curvePointsXY[0] == seg1.curvePointsXY[0] || segLast.curvePointsXY[segLast.curvePointsXY.size() -1] == seg1.curvePointsXY[0]){
                intersec = seg1.curvePointsXY[seg1.curvePointsXY.size()-1];
            } else {
                intersec = seg1.curvePointsXY[0];
            }
        } else if ((*it) == lastWalk && drivePath.size() > 0){
            //DETERMINE CAR PICKUP INTERSECTION
            segmentNode & segNext = segmentDatabase[*(it+1)];
            if (segNext.curvePointsXY[0] == seg1.curvePointsXY[0] || segNext.curvePointsXY[0] == seg1.curvePointsXY[seg1.curvePointsXY.size() -1]){
                intersec = segNext.curvePointsXY[0];
            } else {
                intersec = segNext.curvePointsXY[segNext.curvePointsXY.size()-1];
            }
        }
        
        //cout<<index<<" "<<counter<<endl;
        //cout<<"last walk: "<<lastWalk<<" *it: "<<*it<<endl;
        if (index == counter && (*it != lastWalk && lastWalk != drivePath[0]) && *it != lastDrive && counter != 0){
            result = getDirection(*it, *(it+1), angle1, angle3);
        }
        if (!cameraMoved && index == counter){
            double screenHeight = bounds.bottom() - bounds.top(), screenWidth = bounds.right() - bounds.left();
            ezgl::rectangle newWorld({intersec.first - 0.00003, intersec.second - 0.00003*screenHeight / screenWidth}, {intersec.first + 0.00003, intersec.second + 0.00003*screenHeight / screenWidth});
            g->set_visible_world(newWorld);
        }
        counter++;
    } 
    return result;

}




void drawNavInstruction(ezgl::renderer * g, int previousShownStep, int numWalkStreets, int numDriveStreets){
    string instruction = navInstruction;
    if (night) g->set_color(ezgl::DARK_TEXT);
    else g->set_color(ezgl::BLACK);
    g->set_coordinate_system(ezgl::SCREEN);
    g->set_font_size(15);
    if (previousShownStep == 0){
        g->draw_text({screenBounds.right() - 500, screenBounds.top() - 630}, "Start");
    }else if (previousShownStep == numDriveStreets + numWalkStreets){
        g->draw_text({screenBounds.right() - 500, screenBounds.top() - 630}, "Head straight to destination");
    } else if (previousShownStep == numWalkStreets){
        g->draw_text({screenBounds.right() - 500, screenBounds.top() - 630}, "Pickup by car ");
    } else {

        int index1 = instruction.find(" from "), index2;
        string str1 = instruction.substr(0, index1);
        string str2 = instruction.substr(index1);
        index2 = str2.find(" to ");
        string str3 = str2.substr(0, index2);
        string str4 = str2.substr(index2);     

        if (str1 == "Continue"){
            g->set_color(29, 164, 220);
            g->set_font_size(18);
            g->draw_text({screenBounds.right() - 500, screenBounds.top() - 630}, str1);

        } else {
            g->draw_text({screenBounds.right() - 500, screenBounds.top() - 630}, str1.substr(0, 7));
            g->set_color(29, 164, 220);
            g->set_font_size(18);
            g->draw_text({screenBounds.right() - 440, screenBounds.top() - 630}, str1.substr(7, str1.length()-7));
        } 

        if (night) g->set_color(ezgl::DARK_TEXT);
        else g->set_color(ezgl::BLACK);
        g->set_font_size(15);
        g->draw_text({screenBounds.right() - 500, screenBounds.top() - 590}, str3.substr(0, 5));
        g->set_color(ezgl::DARK_WATER);
        g->draw_text({screenBounds.right() - 458, screenBounds.top() - 589}, str3.substr(5, str3.length()-5));

        if (night) g->set_color(ezgl::DARK_TEXT);
        else g->set_color(ezgl::BLACK);
        g->set_font_size(15);
        g->draw_text({screenBounds.right() - 500, screenBounds.top() - 550}, str4.substr(0, 3));
        g->set_color(29, 164, 220);
        g->set_font_size(20);
        g->draw_text({screenBounds.right() - 478, screenBounds.top() - 550}, str4.substr(3, str4.length()-3));

    }
    g->set_coordinate_system(ezgl::WORLD);
}