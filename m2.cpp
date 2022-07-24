#include "m2.h"
#include "m1.h"
#include "m3.h"
#include "m4.h"
#include "StreetsDatabaseAPI.h"
#include "OSMDatabaseAPI.h"
#include "LatLon.h"
#include "OSMWayNode.h"
#include "cURL.h"
#include "loadingScreen.h"
#include "streetNode.h"
#include "intersecNode.h"
#include "featureNode.h"
#include "POINode.h"
#include "segmentNode.h"
#include "calculations.h"
#include "globalData.h"
#include "drawFunctions.h"
#include "POIIcon.h"
#include "gtkDropDown.h"

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
#include <regex> 
#include <chrono>


#include "ezgl/application.hpp"
#include "ezgl/graphics.hpp"
#include "ezgl/camera.hpp"
#include "ezgl/rectangle.hpp"
#include "ezgl/point.hpp"


using namespace std;
void initial_setup(ezgl::application *application, bool new_window);
void act_on_mouse_press(ezgl::application *application, GdkEventButton *event, double x, double y);
void act_on_mouse_move(ezgl::application *application, GdkEventButton *event, double x, double y);
void act_on_key_press(ezgl::application *application, GdkEventKey *event, char *key_name);
void search_button(GtkWidget *widget, ezgl::application *application);
void displayPOI(ezgl::renderer *g);
void displayBusStop(ezgl::renderer *g);
void drawWeatherIcon(ezgl::renderer * g);
void draw_hamburger_icon(ezgl::renderer * g);
void draw_main_canvas(ezgl::renderer *g);
void changeMaps(GtkWidget *widget, ezgl::application *application);
void resetSearchButton(GtkWidget * /*widget*/, ezgl::application *application);
void clearDatabases();
void findClosestStreet(intersecNode & closestIntersection);
void advanceSearch(GtkWidget * widget, ezgl::application * application); 
void enterKey(GtkWidget * /*widget*/, ezgl::application *application);
void typingName(GtkEntry * entry, ezgl::application * user_data);
void showDropdownStreets(ezgl::renderer * g);
void resetHome(GtkWidget * /*widget*/, ezgl::application *application);
bool clickOnDropdown(ezgl::application * application, double sy);
void setCurrentHome(ezgl::application * application, double x, double y);
void cancelHighlight();
void showExtraButtons(ezgl::renderer * g, double initX, double initY);
void useHome();
void useRedPin();
void useBluePin();
void clearDefaultField();
void cancelNav();
vector<int> find_street_ids_with_misspelled_names(string street_prefix, int maxStreet);
vector <int> pickupLocations, dropoffLocations;

//global variables needed for drawing
int previousHighlightedIntersection = 0;
int savedIntersection = -1;
vector <int> previousFoundIntersection;
int previousHighlightedStreet = 0;
int previousShownStep = -1;
double prevX = 0, prevY = 0;
pair <double, double> labelLocation;
vector<int> drivePath;
vector <int> walkPath;
int numDriveStreets = -1;
int numWalkStreets = -1;
bool currentlySearchActive = false;
int errorCode = 0;

bool restaurant = 0;
bool subway = 0;
bool fastfood = 0;
bool home = 0;
bool bikeshare = 0;
bool traffic = 0;
bool homeSet=false;
bool startSetHome=false;
bool POIBarShown = 0;
bool cameraMoved = true;
bool night = false;
bool searchBarExpanded = 0;
bool firstSetHome;
int fromTo = 0; //0 is "from", 1 is "to";
int highlightedExtraButtons;
bool didYouMean=false;
bool showHelp = 0;
bool enableM4 = false;

double screenToWorldRatioX;
double screenToWorldRatioY;
string currentWeather = "";
string navInstruction = "";
string searchText = "Enter a location";
string toText = "What is the destination intersection?", fromText = "What is the starting intersection?";
int toID = -1, fromID = -1;
ezgl::rectangle bounds;
ezgl::rectangle screenBounds;
pair <double, double> homeLocation;
vector <streetNode> tempFoundStreets;
vector <int> tempFoundIntersections;
vector <streetNode> foundStreets;
vector <string> mapNames;
vector <POIIcon> displayedPOIIcon;
double tempHigh, tempLow;
int highlightedDropdown;
bool dropDownShown;
bool walk;
double walkingSpeed = -1.0, walkingTime = -1.0;
double mouseX, mouseY;
bool displaybox = false;
struct POIIcon clickedPOI;
bool navigationHidden = true;
bool needToSetPin = false;
bool showPath = false;

int foundStreetIndex = 0, foundIntersectionIndex = 0;
int searchMode = 0;
//0: no search being performed
//1: top bar search being performed
//2: "from" field search being performed
//3: "to" field search being performed

string mapName = "null";
bool mapLoaded = false;
const auto beginCount = chrono::steady_clock::now();
void draw_map(){
    ezgl::application::settings settings;
    settings.main_ui_resource = "libstreetmap/resources/main.ui";
    settings.window_identifier = "MainWindow";
    settings.canvas_identifier = "MainCanvas";

    ezgl::rectangle initial_world{{0, 0}, {500, 500}};
    ezgl::application application(settings);
    application.add_canvas("MainCanvas", draw_main_canvas, initial_world);
    application.run(initial_setup, act_on_mouse_press, act_on_mouse_move, act_on_key_press);
}
ezgl::renderer * a;


void draw_main_canvas(ezgl::renderer *g) {
    
    a = g;
    if (mapName == "/cad2/ece297s/public/maps/tehran_iran.streets.bin" || mapName == "/cad2/ece297s/public/maps/new-delhi_india.streets.bin")
        g->format_font("Free Mono Bold", ezgl::font_slant::normal, ezgl::font_weight::normal, 9);
    else
        g->format_font("Noto Sans CJK TC", ezgl::font_slant::normal, ezgl::font_weight::normal, 9);
    
    //get visible world bounds and screen pixel to world conversion ratio
    bounds = g->get_visible_world();
    screenBounds = g->world_to_screen(bounds);
    ezgl::rectangle testBox({bounds.left(), bounds.top()}, 0.00001, 0.00001);
    ezgl::rectangle screenBox = g->world_to_screen(testBox);
    screenToWorldRatioX = (screenBox.right() - screenBox.left()) / 0.00001;
    screenToWorldRatioY = (screenBox.bottom() - screenBox.top()) / 0.00001;
    
    //fill background color
    if(!night || mapLoaded == false)
        g->set_color(ezgl::LIGHT_BACKGROUND);
    else
        g->set_color(ezgl::DARK_BACKGROUND);
    
    g->fill_rectangle(bounds);

    
    
    //enter map loading page if no map is loaded
    if (!mapLoaded){
        mapNames = getMapNames();
        drawMapSelectionScreen(g, mapNames, prevX, prevY);
        return;
    }
    

    //if a map is loaded: draw aspects of the map in order of increasing precedence
    drawFeature(g);
    
    drawStreets(g, foundStreets, previousHighlightedStreet);
        
    if (streetDatabase[previousHighlightedStreet].highlight == false && foundStreets.size() == 0){
        g->set_color(141, 144, 146);
        writeStreetNames(g);
    }

    writeFeatureName(g);

    drawSubway(g);

    drawPOIIcon(g);
    
    displayPOI(g, currentlySearchActive);
    //previousHighlightedIntersection = 1132;
    //intersectionGraph[previousHighlightedIntersection].highlight = true;
    drawFoundIntersection(g, previousHighlightedIntersection, previousFoundIntersection, foundIntersectionIndex, previousHighlightedStreet, cameraMoved);

    if (enableM4){
        ezgl::surface *pinIcon = ezgl::renderer::load_png("libstreetmap/resources/pin.png");
        ezgl::surface *flagIcon = ezgl::renderer::load_png("libstreetmap/resources/redFlag.png");
        //draws the pickup and dropoff intersections
        for (auto i = pickupLocations.begin(); i != pickupLocations.end(); i++){
            g->draw_surface(pinIcon, {intersectionGraph[*i].xPos - 16/screenToWorldRatioX, 
                                intersectionGraph[*i].yPos - 32/screenToWorldRatioY});
        }
        for (auto i = dropoffLocations.begin(); i != dropoffLocations.end(); i++){
            g->draw_surface(flagIcon, {intersectionGraph[*i].xPos - 16/screenToWorldRatioX, 
                                intersectionGraph[*i].yPos - 32/screenToWorldRatioY});
        }
        g->free_surface(pinIcon);
        g->free_surface(flagIcon);

    }
    
    if (foundStreets.size() != 0){
        drawFoundStreets(g, foundStreets, foundStreetIndex, cameraMoved, previousHighlightedStreet);
    }

    if (showPath == true) {
        navInstruction = drawAllArrows(g, previousShownStep, cameraMoved);
        cameraMoved = true;
    }
    
    drawHighlightStreetInfo(g, previousHighlightedStreet, labelLocation);

    drawHomePin(g, startSetHome, homeSet, homeLocation);
    
    drawroadClosure(g);
    
    if (!navigationHidden || needToSetPin || showPath)
        drawNavigationIntersections(g, fromID, toID);

    writeClickedPOI(g);

    //draw persistent buttons and icons last to prevent being overwritten
    draw_hamburger_icon(g, POIBarShown, (searchMode == 1), currentlySearchActive, searchText);
    
    //draw the icons on the search bar
    if (POIBarShown){
        search_icon(g);
        draw_png(g);
    }
    
    if (foundStreets.size() == 1 || previousFoundIntersection.size() == 1 || 
           (foundStreets.size() == 0 && previousFoundIntersection.size() == 0) )
        draw_nightmode_icon(g, false);
    
    if (needToSetPin){
        g->set_coordinate_system(ezgl::SCREEN);
        ezgl::surface * pin;
        if (searchMode == 2){
            pin = ezgl::renderer::load_png("libstreetmap/resources/pin.png");
            g->draw_surface(pin, {mouseX - 15, mouseY-30});
        }
        else {
            pin = ezgl::renderer::load_png("libstreetmap/resources/redFlag.png");
            g->draw_surface(pin, {mouseX - 5, mouseY-30});
        }
        g->free_surface(pin);
    }
    
    //get weather information when loaded onto a new map
    if (currentWeather == "")
        currentWeather = getWeatherInfo(intersectionGraph[2].position, tempHigh, tempLow);    
    
    if (bounds.right() - bounds.left() > 0.0003)
        drawWeatherInfo(g, currentWeather, tempHigh, tempLow);
    
    if (currentlySearchActive && (foundStreets.size() > 1 || previousFoundIntersection.size() > 1)){
        drawNextIcon(g);
    }
    
     if (navigationHidden)
        drawNavigationPanelIcon(g);
     else 
        drawNavigationPanel(g, fromText, toText, previousShownStep,numWalkStreets, numDriveStreets, errorCode);

    if ((searchMode == 1 && dropDownShown) || (searchMode == 2) || (searchMode == 3))
        showDropdownStreets(g);
    
    //enter the help page if "showHelp" is true
    if (showHelp){
        g->set_coordinate_system(ezgl::SCREEN);
        g->set_color(160, 160, 160, 200);
        g->fill_rectangle({0, 0}, {1920, 1200});
        
        ezgl::surface * help = ezgl::renderer::load_png("libstreetmap/resources/helpPage.png");
        g->draw_surface(help, {250, 70});
        
        g->free_surface(help);
        g->set_coordinate_system(ezgl::WORLD);
    }
    
    auto now =  std::chrono::steady_clock::now();
    int milliSecond = chrono::duration_cast<chrono::milliseconds>(now - beginCount).count();
    //cout<<milliSecond<<endl;
}

ezgl::application * appli;
//processes mouse clicks on UI buttons
void act_on_mouse_press(ezgl::application *application, GdkEventButton *event, double x, double y){
    appli = application;
    if (event->button == 2 || event->button == 3) return;
    int sx=event->x, sy=event->y;
    ezgl::renderer * g = application->get_renderer();
    if (!mapLoaded && mapName == "null"){
        //on map selection screen: select the correct map based on click location
      
        int yIndex;
        if (x > 20 && x < 220 && y < 400 && y > 100)
            yIndex = (400-y) / 30;
        else if (x > 220 && x < 420 && y < 400 && y > 130)
            yIndex = ((400 - y) / 30) + 10;
        else return;
        
        mapName = mapNames[yIndex];
        mapName = "/cad2/ece297s/public/maps/" + mapName + ".streets.bin";
        load_map(mapName);
        mapLoaded = true;
        
        //set initial map coordinates
        ezgl::rectangle worldCoordinate({lonToX(minLon), latToY(minLat)},  {lonToX(maxLon), latToY(maxLat)});
        application->change_canvas_world_coordinates("MainCanvas", worldCoordinate);
        application->refresh_drawing();
        return;
    }
    
    //if the user is in the help screen: clicking anywhere cancels the help
    if (showHelp){
        showHelp = false;
        application->refresh_drawing();
        return;
    }
    if (sx > screenBounds.right() - 50 && sy > 10 && sy < 40){
        night = !night;
        application->refresh_drawing();
        return;
    }
    
    //place a red pin for navigation
    if (needToSetPin && sx > 160 && sx < screenBounds.right() - 45){
        int intersectionID = find_closest_intersection(LatLon(yToLat(y), xToLon(x)));
        intersectionGraph[previousHighlightedIntersection].highlight = false;
//        intersectionGraph[intersectionID].highlight = true;
//        previousHighlightedIntersection = intersectionID;
        
        if (searchMode == 2) {
            fromID = intersectionID;
            fromText = "Using pin: "+intersectionGraph[intersectionID].name;
            searchMode = 3;
        } else if (searchMode == 3) {
            toID = intersectionID;
            toText = "Using pin: "+intersectionGraph[intersectionID].name;
            searchMode = 0;
        }
        navigationHidden = false;
        needToSetPin = false;
        if (fromID != -1 && toID != -1) errorCode = 3;
        application->refresh_drawing();
        return;
    }
    
    
    //toggles the navigation display
    if (sx > screenBounds.right() - 45 && sy > 400 && sy < 450 && navigationHidden){
        navigationHidden = false;
        drawNavigationPanel(g, fromText, toText, previousShownStep, numWalkStreets, numDriveStreets, errorCode);
        showDropdownStreets(g);
        drawNavigationIntersections(g, fromID, toID);
        application->flush_drawing();
        return;
    } else if (!navigationHidden && (sx < screenBounds.right() - 600 || sy > screenBounds.top() - 300 || sy < screenBounds.top()-900)){
        navigationHidden = true;
        resetSearchButton(nullptr, application);
    }
    
    int entrySize = max(tempFoundStreets.size(), tempFoundIntersections.size());
    if (entrySize > 10) entrySize = 10;
    //clicked on a street or intersection search result in drop down menu
    if (dropDownShown == true) {
        if ((searchMode == 1 && sx > 200 && sx < screenBounds.right() - 200 && sy < (30 * entrySize + 40)) || 
        (searchMode == 2 && sx > screenBounds.right() - 580 && sx < screenBounds.right() && sy > screenBounds.top() - 820 && sy < (screenBounds.top() - 820) + 30*entrySize) || 
        (searchMode == 3 && sx > screenBounds.right() - 580 && sx < screenBounds.right() && sy > screenBounds.top() - 720 && sy < (screenBounds.top() - 720) + 30*entrySize))
        {
               bool clicked = clickOnDropdown(application, sy);
               if (clicked) return;
        }
    }
    
    //detects clicks on one of the extra buttons if in navigation mode
    if (searchMode == 2 && sx > screenBounds.right() - 600 && sy > (screenBounds.top() - 820) + 30*entrySize && sy < ((screenBounds.top() - 720) + 30*entrySize)){
        if (sx < screenBounds.right() - 433){
            useRedPin();
            application->refresh_drawing();
            return;
        } else if (sx < screenBounds.right() - 266){
            useBluePin();
        } else if (sx < screenBounds.right() - 99){
            useHome();
        }
    } else if (searchMode == 3 && sx > screenBounds.right() - 600 && sy > (screenBounds.top() - 720) + 30*entrySize && sy < ((screenBounds.top() - 620) + 30*entrySize)){
        if (sx < screenBounds.right() - 433){
            useRedPin();
            application->refresh_drawing();
            return;
        } else if (sx < screenBounds.right() - 266){
            useBluePin();
        } else if (sx < screenBounds.right() - 99){
            useHome();
        }
    }
    
    //detects clicks on search bars, which changes the search mode
    if (sy < 40 && sx > 45 && sx < 270) {
        searchMode = 1;
        clearDefaultField();
        errorCode = 0;
        typingName(nullptr, application);
        application->refresh_drawing();
        return;
    } else if (searchMode == 1 && (sy > 40 || sx > screenBounds.right() - 200)){
        searchMode = 0;
        errorCode = 0;
        if (!currentlySearchActive) resetSearchButton(nullptr, application);
    }
    if (!navigationHidden && sy < screenBounds.top() - 820 && sy > screenBounds.top() - 850 && sx > screenBounds.right() - 600) {
        drivePath.clear();
        walkPath.clear();
        searchMode = 2;
        clearDefaultField();
        typingName(nullptr, application);
        showPath = false;
        errorCode = 0;
        application->refresh_drawing();
        return;
    } else if (searchMode == 2 && !(sy < screenBounds.top() - 820 && sy > screenBounds.top() - 850 && sx > screenBounds.right() - 600)){
        searchMode = 0;
    }
    
    if (!navigationHidden && sy < screenBounds.top() - 720 && sy > screenBounds.top() - 750 && sx > screenBounds.right() - 600) {
        drivePath.clear();
        walkPath.clear();
        searchMode = 3;
        errorCode = 0;
        clearDefaultField();
        typingName(nullptr, application);
        showPath = false;
        application->refresh_drawing();
        return;
    } else if (searchMode == 3 && !(sy < screenBounds.top() - 720 && sy > screenBounds.top() - 750 && sx > screenBounds.right() - 600)){
        searchMode = 0;
    }
    
        typingName(nullptr, application);

    //detects click on one of the 4 options for navigation: search, cancel, help, and enable walking
    if (!navigationHidden && sx > screenBounds.right() -70){
        errorCode = 0;
        if (sy > screenBounds.top() -670 && sy < screenBounds.top() -630){
            
            if (fromID == -1 || toID == -1){
                errorCode = 4;
                application->refresh_drawing();
                return;
            } else if (!showPath && !walk){
                walkPath.clear();
                drivePath.clear();
                
                if (enableM4){
                    std::vector<DeliveryInfo> deliveries;
                    std::vector<IntersectionIndex> depots;
                    std::vector<CourierSubpath> result_path;
                    float turn_penalty;
                    float truck_capacity;

        deliveries = {DeliveryInfo(48420, 74209, 50.03543)};
        //deliveries = {DeliveryInfo(12329, 12927, 50.03543), DeliveryInfo(15162, 71331, 60.50912), DeliveryInfo(6051, 141242, 35.54058)};
        depots = { 33611};
        turn_penalty = 15.000000000;
        truck_capacity = 13465.874023438;
        
                    vector<CourierSubpath> temp = traveling_courier(deliveries, depots, turn_penalty, truck_capacity);
                    
                    for (auto i = temp.begin(); i != temp.end(); i++){
                        drivePath.insert(drivePath.end(), (i->subpath).begin(), (i->subpath).end());
                        cout<<i->start_intersection<<" "<<i->end_intersection<<endl;
                    }
                    for (auto i = deliveries.begin(); i != deliveries.end(); i++){
                        pickupLocations.push_back(i->pickUp);
                        dropoffLocations.push_back(i->dropOff);
                        
                    }
                } else {
                    drivePath = find_path_between_intersections(fromID, toID, 0.00000000000000000);
                }
            } else if (!showPath && walk){
                
                pair <vector<StreetSegmentIndex>, vector<StreetSegmentIndex>> walkToPickup = 
                       find_path_with_walk_to_pick_up(fromID, toID, 0, walkingSpeed,walkingTime);
                walkPath = walkToPickup.first;
                drivePath = walkToPickup.second;
            }
            
            cout<<compute_path_travel_time(drivePath, 0)<<endl;
            if (showPath){
                cameraMoved = false;
                previousShownStep++;
                if (previousShownStep > (numWalkStreets + numDriveStreets))
                    previousShownStep = 0;
                application->refresh_drawing();
                application->refresh_drawing();
                return;
            } else if (drivePath.size() == 0 && walkPath.size() == 0) {
                errorCode = 1;
                application->refresh_drawing();
                return;
            } else{
                cout<<"walk path size: "<<walkPath.size()<<endl;
                cout<<"drive path size: "<<drivePath.size()<<endl;
                if (walkPath.size() != 0)
                    numWalkStreets = 1;
                else
                    numWalkStreets = 0;
                if (drivePath.size() != 0)
                    numDriveStreets = 1;
                else 
                    numDriveStreets = 0;

                for (auto i = walkPath.begin(); walkPath.size() != 0 && (i+1) != walkPath.end(); i++) {
                    segmentNode & seg1 = segmentDatabase[*i], &seg2 = segmentDatabase[*(i+1)];
                    if (seg1.streetID != seg2.streetID) {
                        numWalkStreets++;
                    }
                }
                for (auto i = drivePath.begin(); drivePath.size() != 0 && (i+1) != drivePath.end(); i++) {
                    segmentNode & seg1 = segmentDatabase[*i], &seg2 = segmentDatabase[*(i+1)];
                    if (seg1.streetID != seg2.streetID) numDriveStreets++;
                }
                cout<<"numWalkStreets: "<<numWalkStreets<<endl;
                cout<<"numDriveStreets: "<<numDriveStreets<<endl;
                cameraMoved = false;
                showPath = true;
                previousShownStep = 0;
                application->refresh_drawing();
                application->refresh_drawing();
                return;
            }
        } else if (sy > screenBounds.top() - 610 && sy < screenBounds.top() - 570){
            if (!walk && !showPath){
                
                bool speedEntered = selectWalkingSpeed(application, walkingSpeed, walkingTime);
                if (speedEntered && (walkingSpeed == -1 || walkingTime == -1)){
                    walkingSpeed = walkingTime = -1;
                    
                    if (night) g->set_color(ezgl::DARK_TEXT);
                    else g->set_color(ezgl::BLACK);
                    errorCode = 2;
                    application->refresh_drawing();
                    return;
                }
                else if (speedEntered) {
                    walk = !walk;
                    if (fromID != -1 && toID != -1)
                        errorCode = 3;
                }
                
            } else if (walk && !showPath) {
                walk = !walk;
                walkingSpeed = walkingTime = -1;
            } else if (showPath){
                //this now acts as back button
                cameraMoved = false;
                previousShownStep--;
                if (previousShownStep == -1) previousShownStep = (numWalkStreets + numDriveStreets);
                application->refresh_drawing();
                application->refresh_drawing();
                return;
            }
        } else if (sy > screenBounds.top() - 400 && sy < screenBounds.top() - 360){
            if (showPath){
                cout<<"cancel"<<endl;
                cancelNav();
                dropDownShown = false;
            } else {
                navigationHidden = true;
            }
            application->refresh_drawing();
            return;
        } else if (sy > screenBounds.top() - 350){
            cout<<"help"<<endl;
            showHelp = true;
            application->refresh_drawing();
            return;
        }
        drawNavigationPanel(g, fromText, toText, previousShownStep, numWalkStreets, numDriveStreets, errorCode);
            showDropdownStreets(g);       
            application->flush_drawing();
            return;
    }
    
    //clicking on the magnifying glass icon cancels the search
    if ((searchMode == 1 && sx > screenBounds.right() - 230 && sx < screenBounds.right() - 200 && sy < 40)
            ||(searchMode == 0 && sx > 270 && sx < 300 && sy < 40) ){
        if (currentlySearchActive)
            resetSearchButton(nullptr, application);
        else
            search_button(nullptr, application);
        application->refresh_drawing();
        return;
    }
    
    dropDownShown = false;
    //detect if a POI icon is clicked
    for(auto it=displayedPOIIcon.begin(); it !=displayedPOIIcon.end(); it++){
        //boundary of this box
        if(x>= it->xmin && x<= it->xmax && y <= it->ymin && y>= it->ymax){ 
            intersectionGraph[previousHighlightedIntersection].highlight = false;
            streetDatabase[previousHighlightedStreet].highlight = false;
            displaybox=true;
            //stores in global data for draw function
            clickedPOI = *it;
            application->refresh_drawing();
            return;
        }
    }
    
    
    if(startSetHome==True && sx > 90){
        //detect second click to set home
        setCurrentHome(application, x, y);
    }
    else if (currentlySearchActive && sx>screenBounds.right() - 130 && sy < screenBounds.top() + 80 && 
            (foundStreets.size() > 1 || previousFoundIntersection.size() > 1)){
        //detect clicks on the "next" icon
        advanceSearch(nullptr, application);
        application->refresh_drawing();
        return;
        
    }
    else if (sx > 10 && sx < 45 && sy > 10 && sy < 40){
        //detect clicks on the hamburger icon
        POIBarShown = !POIBarShown;
    }
    else if (sx > 10 && sx < 160 && sy > 340 && sy < 440 && POIBarShown){
        //user clicks on home icon
        if(homeSet==false)
            startSetHome=true;
        else
            home=!home;
        application->refresh_drawing();
    }
    else if(sx>10 && sx<160 && sy>40 && sy<640 && POIBarShown){
        //toggle different POI display options
        if(sy<140)
            restaurant = !restaurant;
        else if(sy<240)
            fastfood = !fastfood;
        else if (sy<340)
            subway = !subway;
        else if (sy > 440 && sy< 540)
            bikeshare = !bikeshare;
        else
            traffic = !traffic;
    }
    else if (event->button == 1){
        //left click action: highlight intersections if 1 click, highlight nearest street if 2 clicks
        int intersectionID = find_closest_intersection(LatLon(yToLat(y), xToLon(x)));
        intersecNode & closestIntersection = intersectionGraph[intersectionID];
        displaybox = false;

        string message = "Intersection name: "+closestIntersection.name +" ID: "+to_string(intersectionID);
        application->update_message(message);

        if (prevX == x && prevY == y){
            resetSearchButton(nullptr, application);
            streetDatabase[previousHighlightedStreet].highlight = false;
            //it's the 2nd click on the same spot. Highlight the nearest street
            findClosestStreet(closestIntersection);
            
        } else{
            //it's the first click, so highlight intersection
            //first clear previous highlights
            intersectionGraph[previousHighlightedIntersection].highlight = false;
            streetDatabase[previousHighlightedStreet].highlight = false;
            
            double mouseDistance = sqrt(pow(closestIntersection.xPos - x, 2) + pow(closestIntersection.yPos - y, 2));
            if (mouseDistance < 0.000002)
                closestIntersection.highlight = true;
            previousHighlightedIntersection = intersectionID;
        }
            prevX = x, prevY = y;
    }
    
    //refresh
    application->refresh_drawing();
}

void cancelNav(){
    errorCode = 0;
    previousShownStep = -1;
    numWalkStreets = numDriveStreets = 0;
    showPath = false;
    fromID = toID = -1;
    toText = "What is the destination intersection?", fromText = "What is the starting intersection?";
    drivePath.clear();
    walkPath.clear();
    walk = false;
    dropDownShown = false;
}

void findClosestStreet(intersecNode & closestIntersection){
    //look at the nearest intersection, then look at all street segments pointing from that intersection.
    //then calculate the angle between the first curve point in the segment and the mouse, to the intersection
    //find the street segment that is closest to mouse direction
    double closestAngle = 99;
    int closestSegmentID = 0;
    for (auto i = closestIntersection.segment.begin(); i != closestIntersection.segment.end(); i++){
        LatLon firstCurvePoint = segmentDatabase[*i].curvePoints[1];

        double curvePointX = lonToX(firstCurvePoint.lon());
        double curvePointY = latToY(firstCurvePoint.lat());

        double angleOfStreet = atan((curvePointY - latToY(closestIntersection.position.lat())) / (curvePointX - lonToX(closestIntersection.position.lon())));
        double angleOfMouse = atan((prevY - latToY(closestIntersection.position.lat())) / (prevX - lonToX(closestIntersection.position.lon())));


        if (abs(angleOfStreet - angleOfMouse) < closestAngle) {
            closestAngle = abs(angleOfStreet - angleOfMouse);
            closestSegmentID = *i;
            labelLocation = make_pair(curvePointX, curvePointY);
        }
    }
    cout<<"closest Segment speed limit: "<<segmentDatabase[closestSegmentID].speedLimit<<" , ID = "<<closestSegmentID<<endl;
    
    int closestStreetID = segmentDatabase[closestSegmentID].streetID;
    if (streetDatabase[closestStreetID].streetName == "unknown" || streetDatabase[closestStreetID].streetName == "NO_NAME") return;
    streetDatabase[closestStreetID].highlight = true;
    previousHighlightedStreet = closestStreetID;
    
}


//this function provides mouse cursor data to loading screen (for hovering over map choices)
void act_on_mouse_move(ezgl::application *application, GdkEventButton * event, double x, double y){
    ezgl::renderer * g = application->get_renderer();
            
    if (mapName == "null" || mapLoaded == false){
        prevX = x, prevY = y;
        
        if (bounds.left() < 0 || bounds.right() > 500 || bounds.top() > 500 || bounds.bottom() < 0){
            ezgl::rectangle loadingScreen ({0, 0}, {500, 500});
            application->change_canvas_world_coordinates("MainCanvas", loadingScreen);
        }
        
        application->refresh_drawing();
    }   
    else if (needToSetPin){
        mouseX = event->x;
        mouseY = event->y;
        application->refresh_drawing();
    }
    
    //suggests name on search bar
    else if ((searchMode == 1 && dropDownShown) || (((searchMode == 2) || (searchMode == 3)) && !needToSetPin)){
        int entrySize = max(tempFoundStreets.size(), tempFoundIntersections.size());
        if (entrySize > 10) entrySize = 10;
        int sx = event->x, sy = event->y;
        if (searchMode == 1 && sx > 200 && sx < screenBounds.right() - 200 && sy < (30 * entrySize + 40)) {
            highlightedDropdown = (sy-40) / 30;
            application->refresh_drawing();
        } else if (searchMode == 2 && sx > screenBounds.right() - 580 && sy > screenBounds.top() - 820 && sy < (screenBounds.top() - 820) + 30*entrySize){
            highlightedDropdown = (sy-screenBounds.top() + 820) / 30;
            drawNavigationPanel(g, fromText, toText, previousShownStep, numWalkStreets, numDriveStreets, errorCode);
            showDropdownStreets(g);       
            application->flush_drawing();
        } else if (searchMode == 3 && sx > screenBounds.right() - 580 && sy > screenBounds.top() - 720 && sy < (screenBounds.top() - 720) + 30*entrySize){
            highlightedDropdown = (sy-screenBounds.top() + 720) / 30;
            drawNavigationPanel(g, fromText, toText, previousShownStep, numWalkStreets, numDriveStreets, errorCode);
            showDropdownStreets(g);       
            application->flush_drawing();
        } else if (searchMode == 2 && sx > screenBounds.right() - 580 && sy > (screenBounds.top()-820+30*entrySize) && sy < (screenBounds.top()-720+30*entrySize)){
            highlightedDropdown = -1;
            highlightedExtraButtons = (sx - (screenBounds.right() - 580))/(500/3) + 1;
            drawNavigationPanel(g, fromText, toText, previousShownStep, numWalkStreets, numDriveStreets, errorCode);
            showDropdownStreets(g);       
            application->flush_drawing();
        } else if (searchMode == 3 && sx > screenBounds.right() - 580 && sy > (screenBounds.top() - 720) + 30*entrySize && sy < (screenBounds.top() - 620) + 30*entrySize){
            highlightedDropdown = -1;
            highlightedExtraButtons = (sx - (screenBounds.right() - 580))/(500/3) + 1;
            drawNavigationPanel(g, fromText, toText, previousShownStep, numWalkStreets, numDriveStreets, errorCode);
            showDropdownStreets(g);       
            application->flush_drawing();
        }
    }
}



void initial_setup(ezgl::application *application, bool /*new_window*/){

    
    application->create_button("Reset Home", 6, resetHome);
    application->create_button("Change map", 7, changeMaps);
}


 void typingName(GtkEntry * /*entry*/, ezgl::application * application){
    didYouMean=false;
    tempFoundStreets.clear();
    tempFoundIntersections.clear();

    if (searchMode == 1)
        resetSearchButton(nullptr, application);

    dropDownShown = false;
    
    if (streetDatabase.size() == 0 || searchMode == 0) return; //map not loaded
    
    string textentry;
    if (searchMode == 1)
        textentry = searchText;
    else if (searchMode == 2)
        textentry = fromText;
    else
        textentry = toText;
    
    //remove space, prevent bugs
    textentry.erase(remove_if(textentry.begin(), textentry.end(), ::isspace), textentry.end()); 
    std::transform(textentry.begin(), textentry.end(), textentry.begin(),::tolower); 
    
    
    int dividePosition = textentry.find("+");
    if (dividePosition == 0 || dividePosition >= textentry.length()) dividePosition = textentry.find("&");
    if (dividePosition == 0 || dividePosition >= textentry.length()){
        //the user has only entered 1 street name. Search for that street.
        if (searchMode == 2 || searchMode == 3) return; //these modes only search for intersection
        vector <int> matchingStreetNames = find_street_ids_from_partial_street_name(textentry);
        
        //if first time not found: try misspelled names with Regex
        //try this only if user entered more than 5 characters
        if (matchingStreetNames.size() == 0 && textentry.size()>5){
            cout<<"no street found, start mispelled"<<endl;
            matchingStreetNames = find_street_ids_with_misspelled_names(textentry, 6);
            //try second time for found street
            if(matchingStreetNames.size() != 0){
                cout<<"second time works, start printing"<<endl;
                didYouMean=true;
                //found street(s) that begin with given street name prefix
                for (auto i = matchingStreetNames.begin(); i != matchingStreetNames.end(); i++){
                    streetDatabase[*i].searchFound = true;
                    tempFoundStreets.push_back(streetDatabase[*i]);
                }
                dropDownShown = true;
            }
        }else {
            didYouMean=false;
            //found street(s) that begin with given street name prefix
            for (auto i = matchingStreetNames.begin(); i != matchingStreetNames.end(); i++){
                streetDatabase[*i].searchFound = true;
                tempFoundStreets.push_back(streetDatabase[*i]);
            }
            dropDownShown = true;
        }
    } else {
        //the user has entered 2 street names. Search for all possible intersections, separated by "+" symbol
        vector <int> matchingFirstStreet = find_street_ids_from_partial_street_name(textentry.substr(0, dividePosition));
        vector <int> matchingSecondStreet = find_street_ids_from_partial_street_name(textentry.substr(dividePosition+1));
        
        //find common intersections
        for (auto first = matchingFirstStreet.begin(); first != matchingFirstStreet.end(); first++){
            for (auto second = matchingSecondStreet.begin(); second != matchingSecondStreet.end(); second++){
                vector <int> results = find_intersections_of_two_streets(make_pair(*first, *second));
                tempFoundIntersections.insert(tempFoundIntersections.end(), results.begin(), results.end());
            }
        }
        //if no intersection found, try misspelled
        if (tempFoundIntersections.size() == 0){
            cout<<"misspelled intersection"<<endl;
            // first one misspelled
            matchingFirstStreet = find_street_ids_with_misspelled_names(textentry.substr(0, dividePosition), 15);
            //find common intersections
            for (auto first = matchingFirstStreet.begin(); first != matchingFirstStreet.end(); first++){
                for (auto second = matchingSecondStreet.begin(); second != matchingSecondStreet.end(); second++){
                    vector <int> results = find_intersections_of_two_streets(make_pair(*first, *second));
                    tempFoundIntersections.insert(tempFoundIntersections.end(), results.begin(), results.end());
                }
            }
            
            //if not enough intersections, try second street misspelled
            if(tempFoundIntersections.size()==0){
                matchingSecondStreet = find_street_ids_with_misspelled_names(textentry.substr(dividePosition+1), 20);
                for (auto first = matchingFirstStreet.begin(); first != matchingFirstStreet.end(); first++){
                    for (auto second = matchingSecondStreet.begin(); second != matchingSecondStreet.end(); second++){
                        vector <int> results = find_intersections_of_two_streets(make_pair(*first, *second));
                        tempFoundIntersections.insert(tempFoundIntersections.end(), results.begin(), results.end());
                    }
                }
            }
            cout<<"tempFoundIntersections size is: "<<tempFoundIntersections.size()<<endl;
            //if found intersections with misspelled
            if (tempFoundIntersections.size()!= 0){
                cout<<"didyoumean==true in intersections"<<endl;
                didYouMean=true;
            }else{//not found with misspelled
                return;
            }
        }
        dropDownShown = true;
    }
    
    //update did you mean message
    if(didYouMean==true){
        string message = "No result found. Did you mean these?";
        application->update_message(message);
    }else{
        string message = " ";
        application->update_message(message);
    }
    
}

//this function displays the drop down list of streets as users type
void showDropdownStreets(ezgl::renderer * g){
    
    if (searchMode == 0) return; 
    if (tempFoundStreets.size() == 0 && tempFoundIntersections.size() == 0 && (searchMode == 1 || searchMode == 0)) return;
    if ((searchMode == 2 || searchMode == 3) && navigationHidden) return;
    g->set_font_size(12);
    g->set_coordinate_system(ezgl::SCREEN);
    g->set_horiz_text_just(ezgl::text_just::left);

    int entrySize = max(tempFoundStreets.size(), tempFoundIntersections.size());
    if (entrySize > 10) entrySize = 10;
    
    //set initial x y value with 3 modes
    double initX, initY, width;
    if (searchMode == 1){
        initX = 200, initY = 41, width = screenBounds.right() - 400;
    } else if (searchMode == 2){
        initX = screenBounds.right() - 580, initY = screenBounds.top() - 819, width = 500;
    } else {
        initX = screenBounds.right() - 580, initY = screenBounds.top() - 719, width = 500;
    }

    //draw each drop down street/intersection name
    for (int e = 0; e < entrySize; e++){
        if (!night)
            if (highlightedDropdown == e)
                g->set_color(230, 230, 230);
            else
                g->set_color(250, 250, 250);
        else
            if (highlightedDropdown == e)
                g->set_color(30, 30, 30);
            else
                g->set_color(60, 60, 60);
        
        g->fill_rectangle({initX, initY + 30*e}, {initX + width, initY + 30 * (e+1)});
        
        if (night)
            g->set_color(ezgl::DARK_TEXT);
        else
            g->set_color(ezgl::BLACK);
        

        if (tempFoundStreets.size() != 0){
                g->draw_text({initX + 20, initY + 30*(e) + 15}, tempFoundStreets[e].streetName);
        }else{
                g->draw_text({initX + 20, initY + 30*(e) + 15}, intersectionGraph[tempFoundIntersections[e]].name);
        }
    }
    if ((searchMode == 2 || searchMode == 3) && !needToSetPin)
        showExtraButtons(g, initX, initY + 30*entrySize);
    

}

//this function handles what happens when users click on a dropdown street - it should
//perform a search on that street
bool clickOnDropdown(ezgl::application * application, double sy){
    
//    if (sy < 40) return false;
    int entrySize = max(tempFoundStreets.size(), tempFoundIntersections.size());
    if (entrySize > 10) entrySize = 10;
    
    if (searchMode == 1){
        sy = sy - 40;
    }
    else if (searchMode == 2){
            sy -= screenBounds.top() - 820;
    }
    else if (searchMode == 3){
            sy -= screenBounds.top() - 720;
    }
    
    if (sy < 30 * entrySize) {
        foundStreets = tempFoundStreets;
        previousFoundIntersection = tempFoundIntersections;
        currentlySearchActive = true;
        cameraMoved = false;
        cancelHighlight();
        string * currentText;
        if (searchMode == 1) currentText = &searchText;
        else if (searchMode == 2) currentText = &fromText;
        else if (searchMode == 3) currentText = &toText;
        if (foundStreets.size() == 0){
            foundIntersectionIndex = sy / 30;
            int locatedIntersection = previousFoundIntersection[foundIntersectionIndex];
            
            if (searchMode == 2)
                fromID = locatedIntersection;
            else if (searchMode == 3)
                toID = locatedIntersection;
            else if (searchMode == 1)
                savedIntersection = locatedIntersection;
            
            *currentText = intersectionGraph[locatedIntersection].name;
            intersectionGraph[locatedIntersection].searchFound = true;
            foundIntersectionIndex = 0;
            previousFoundIntersection.clear();
            previousFoundIntersection.push_back(locatedIntersection);
            
        } else{
            foundStreetIndex = sy / 30;
            streetNode locatedStreet = foundStreets[foundStreetIndex];
            *currentText = locatedStreet.streetName;
            streetDatabase[locatedStreet.streetID].searchFound = true;
            foundStreetIndex = 0;
            foundStreets.clear();
            foundStreets.push_back(locatedStreet);
        }
        if (fromID != -1 && toID != -1){
            errorCode = 3;
        }
        searchMode = 0;
        application->refresh_drawing();
        application->refresh_drawing();
        return true;
    }
    return false;
}

void showExtraButtons(ezgl::renderer * g, double initX, double initY){
//this functions gives users the option of picking from home, previous found location, or manually
//click on map to set to/from points in navigation
    double width = 500/3;
    for (int i = 1; i <= 3; i++){
        if (highlightedExtraButtons == i)
            g->set_color(225, 225, 225);
        else
            g->set_color(240, 240, 240);
        
        g->fill_rectangle({initX + (i-1)*width, initY}, {initX + i*width, initY + 100});
    }

    ezgl::surface *redPin;
    if (searchMode == 2) 
        redPin = ezgl::renderer::load_png("libstreetmap/resources/nav/bigRedPin.png");
    else
        redPin = ezgl::renderer::load_png("libstreetmap/resources/nav/bigRedFlag.png");
    ezgl::surface *bluePin = ezgl::renderer::load_png("libstreetmap/resources/nav/bigBluePin.png");
    ezgl::surface * homePin = ezgl::renderer::load_png("libstreetmap/resources/nav/home.png");
    
    g->draw_surface(redPin,{initX + 60, initY + 10});
    g->draw_surface(bluePin,{initX + 230, initY + 10});
    g->draw_surface(homePin,{initX + 390, initY + 10});
    
    g->free_surface(redPin);
    g->free_surface(bluePin);
    g->free_surface(homePin);
    
    if (night)
            g->set_color(ezgl::DARK_TEXT);
    else
            g->set_color(ezgl::BLACK);
    g->draw_text({initX + 30, initY + 70}, "Pick an Intersection");
    g->draw_text({initX + 198, initY + 70}, "Found Intersection");
    g->draw_text({initX + 400, initY + 70}, "Home");
}


void useHome(){
    if (homeSet == false) {
        if (searchMode == 2)
            fromText = "Error: no home set";
        else if (searchMode == 3)
            toText = "Error: no home set";
        return;
    }
    double homeLon = xToLon(homeLocation.first);
    double homeLat = yToLat(homeLocation.second);
    LatLon homePos(homeLat, homeLon);
    int closestIntersectionToHome = find_closest_intersection(homePos);
    if (searchMode == 2){
        fromText = "Using home as starting location";
        fromID = closestIntersectionToHome;
    }
    else if (searchMode == 3){
        toText = "using home as destination";
        toID = closestIntersectionToHome;
    }
    
    cout<<"closest intersection to home: "<<intersectionGraph[closestIntersectionToHome].name<<endl;
}

void useRedPin(){
    needToSetPin = true;
    navigationHidden = true;
    dropDownShown = false;
}

void useBluePin(){
    cout<<"saved intersection:"<<savedIntersection<<endl;
    if (savedIntersection == -1){
        if (searchMode == 2)
            fromText = "Error: no previous search record";
        else if (searchMode == 3)
            toText = "Error: no previous search record";
        
        return;
    }
    
    if (searchMode == 2){
        fromText = "Using previous search result: " + intersectionGraph[savedIntersection].name;
        fromID = savedIntersection;
    }
    else if (searchMode == 3){
        toText =  "Using previous search result: " + intersectionGraph[savedIntersection].name;
        toID = savedIntersection;
    }   
}

void clearDefaultField(){
    string * text;
    if (searchMode == 1) text = &searchText;
    else if (searchMode == 2) text = &fromText;
    else if (searchMode == 3) text = &toText;
    else return;
    
    if ((*text).substr(0, 8) == "Enter a " || (*text).substr(0, 11) == "What is the" || 
            (*text).substr(0, 6) == "Error:" || (*text).substr(0, 5) == "Using"){
        *text = "";
    }
}
//this function turns search off when typing in the search box
void act_on_key_press(ezgl::application *application, GdkEventKey * /*event*/, char *key_name) {
    

    if (searchMode != 0 && strcmp(key_name, "Return")!=0 ) {
        //user has clicked on the search bar. begin registering typed text

        tempFoundIntersections.clear();
        foundStreets.clear();
        previousFoundIntersection.clear();
        tempFoundStreets.clear();
        if (currentlySearchActive && searchMode == 1)
            resetSearchButton(nullptr, application);
        
        string * currentText;
        if (searchMode == 1) currentText = &searchText;
        else if (searchMode == 2) currentText = &fromText;
        else if (searchMode == 3) currentText = &toText;
        else return;
        
        string newChar(key_name);
        if (newChar.size() == 1)
            *currentText += newChar;
        else if (newChar == "space")
            *currentText += " ";
        else if (newChar == "plus")
            *currentText += "+";
        else if (newChar == "ampersand")
            *currentText += "&";
        else if (newChar == "BackSpace" && (*currentText).size() > 0)
            (*currentText).pop_back();

        typingName(nullptr, application);
        
        if (searchMode == 2 || searchMode == 3) {
            ezgl::renderer * g = application->get_renderer();
            drawNavigationPanel(g, fromText, toText, previousShownStep, numWalkStreets, numDriveStreets, errorCode);
            showDropdownStreets(g);
            application->flush_drawing();
        } else
            application->refresh_drawing();
    }
    else if (!currentlySearchActive && strcmp(key_name, "Return") == 0){
        search_button(nullptr, application);
        application->refresh_drawing();
    }
}


//this function resets the search button from "cancel" state to "find" state
void resetSearchButton(GtkWidget * /*widget*/, ezgl::application *application){
    if (currentlySearchActive || dropDownShown) {
        
        currentlySearchActive = false;
        
        //reset all highlight states
        cancelHighlight();
        cancelNav();
        previousFoundIntersection.clear();
        foundStreets.clear();
        tempFoundStreets.clear();
        tempFoundIntersections.clear();
        dropDownShown = false;
        application->refresh_drawing();
    }
}

void cancelHighlight(){
        for (auto i = streetDatabase.begin(); i != streetDatabase.end(); i++){
            i->searchFound = false;
        }
        for (auto i = previousFoundIntersection.begin(); i != previousFoundIntersection.end(); i++){
            intersectionGraph[*i].searchFound = false;
        }
}


//this allows the camera to zoom into the next item found in search
void advanceSearch(GtkWidget * /*widget*/, ezgl::application * application){
    
    if (previousFoundIntersection.size() != 0){
        foundIntersectionIndex += 1;
        if (foundIntersectionIndex == previousFoundIntersection.size())
            foundIntersectionIndex = 0;
        
    } else {
        foundStreetIndex += 1;
        if (foundStreetIndex == foundStreets.size())
            foundStreetIndex = 0;
    }
    cameraMoved = false;
    application->refresh_drawing();
}


//controls search button action and performs the search
void search_button(GtkWidget * widget, ezgl::application *application){
    
    tempFoundStreets.clear();
    tempFoundIntersections.clear();
    foundStreets.clear();
    previousFoundIntersection.clear();
    
    if (currentlySearchActive){
        resetSearchButton(widget, application);
    } else{
        //allows user to find a street by partial street name, or find intersections of two streets by partial street names
        string query(searchText);
        
        int dividePosition = query.find("+");
        if (dividePosition == 0 || dividePosition >= query.length()) dividePosition = query.find("&");
        if (dividePosition == 0 || dividePosition >= query.length()){
            //the user has only entered 1 street name. Search for that street.
            vector <int> matchingStreetNames = find_street_ids_from_partial_street_name(query);
            if (matchingStreetNames.size() == 0){
                return;
            } else {
                //found street(s) that begin with given street name prefix
                for (auto i = matchingStreetNames.begin(); i != matchingStreetNames.end(); i++){
                    streetDatabase[*i].searchFound = true;
                    foundStreets.push_back(streetDatabase[*i]);
                }
                //move camera to search result
                if (foundStreets.size() == 0)
                    return;
                foundStreetIndex = 0;
                application->refresh_drawing();
            }
        } else {
            //the user has entered 2 street names. Search for all possible intersections, separated by "+" symbol
            vector <int> matchingFirstStreet = find_street_ids_from_partial_street_name(query.substr(0, dividePosition));
            vector <int> matchingSecondStreet = find_street_ids_from_partial_street_name(query.substr(dividePosition+1));
            
            //find common intersections
            for (auto first = matchingFirstStreet.begin(); first != matchingFirstStreet.end(); first++){
                for (auto second = matchingSecondStreet.begin(); second != matchingSecondStreet.end(); second++){
                    vector <int> results = find_intersections_of_two_streets(make_pair(*first, *second));
                    previousFoundIntersection.insert(previousFoundIntersection.end(), results.begin(), results.end());
                }
            }
            if (previousFoundIntersection.size() == 0) 
                return;
            foundIntersectionIndex = 0;
            application->refresh_drawing();
        }
        
        cameraMoved = false;
        //change button icon to "cancel"
        currentlySearchActive = true;
    }
    application->refresh_drawing();
}


//change to loading screen when button is clicked
void changeMaps(GtkWidget * /*widget*/, ezgl::application *application){
    if (!mapLoaded) return;
    
    close_map();
    mapName = "null";
    mapLoaded = false;
    clearDatabases();
    ezgl::rectangle loadingScreen ({0, 0}, {500, 500});
    application->change_canvas_world_coordinates("MainCanvas", loadingScreen);

    application->refresh_drawing();
}




//sets a home location for the current map
void setCurrentHome(ezgl::application * application, double x, double y){
        homeLocation.first=x;
        homeLocation.second=y;
        //write to file
        ofstream homefile;
        int namelen = mapName.length()-1;
        string r = mapName.substr(26, namelen);
        //eliminate punctuation
        for (int i = 0, len = r.size(); i < len; i++) { 
        // check whether parsing character is punctuation or not 
            if (ispunct(r[i])) { 
                r.erase(i--, 1); 
                len = r.size(); 
            } 
        }
        homefile.open ("libstreetmap/resources/home/" + r);
        homefile << x<<endl;
        homefile << y<<endl;
        homefile.close();
        homeSet=true;
        startSetHome=false;
        application->refresh_drawing();
        application->refresh_drawing();
}


//deletes the home location file, which allows user to reset their home
void resetHome(GtkWidget * /*widget*/, ezgl::application *application){
    //remove all files
    system("exec rm -r libstreetmap/resources/home/*");
    homeSet=false;
    application->refresh_drawing();
}


//clear all data in previous map
void clearDatabases(){
    cancelNav();
    OSMWayData.clear();
    OSMNodeData.clear();
    OSMRelationData.clear();
    OSMWayDatabase.clear();
    streetDatabase.clear();
    intersectionGraph.clear();
    featureDatabase.clear();
    specialWay.clear();
    segmentDatabase.clear();
    POIDatabase.clear();
    subwayLines.clear();
    subwayLineName.clear();
    subwayStationInfo.clear();
    sortedStreetName.clear();
    foundStreets.clear();
    previousFoundIntersection.clear();
    displayedPOIIcon.clear();
    tempFoundStreets.clear();
    tempFoundIntersections.clear();
    
    //reclaims memory
    OSMWayDatabase.shrink_to_fit();
    streetDatabase.shrink_to_fit();
    intersectionGraph.shrink_to_fit();
    featureDatabase.shrink_to_fit();
    specialWay.shrink_to_fit();
    segmentDatabase.shrink_to_fit();
    POIDatabase.shrink_to_fit();
    subwayLines.shrink_to_fit();
    subwayLineName.shrink_to_fit();
    foundStreets.shrink_to_fit();
    previousFoundIntersection.shrink_to_fit();
    displayedPOIIcon.shrink_to_fit();
    tempFoundStreets.shrink_to_fit();
    tempFoundIntersections.shrink_to_fit();
    
    currentlySearchActive = false;
    cameraMoved = true;
    showPath = false;
    homeSet = false;
    startSetHome = false;
    searchMode = 0;
    searchText = "";
    previousHighlightedStreet = 0;
    currentWeather = "";
    errorCode = 0;
}


vector<int> find_street_ids_with_misspelled_names(string street_prefix, int maxStreet){
    
    vector<int> street_id= {};
    if(street_prefix == "")
        return street_id;
    
    //parses the input street_prefix. Erase all white spaces using "erase", then convert
    //everything to lower case using "transform"
    street_prefix.erase(remove_if(street_prefix.begin(), street_prefix.end(), ::isspace), street_prefix.end()); 
    std::transform(street_prefix.begin(), street_prefix.end(), street_prefix.begin(),::tolower); 

    //get the first three characters
    string searchstr=street_prefix.substr(0,3);
    //erase whitespace: fix bugs for user entering space
    searchstr.erase(remove_if(searchstr.begin(), searchstr.end(), ::isspace), searchstr.end()); 
    std::transform(searchstr.begin(), searchstr.end(), searchstr.begin(),::tolower); 

    //manipulate the searchstr while returned street's number is less than what we desired
    for(int i=0; i<searchstr.size()-1 && street_id.size()<maxStreet; i++){
        for(int j=0; j<searchstr.size() && street_id.size()<maxStreet; j++){
            string temp = searchstr;
            temp[0]=searchstr[i];
            searchstr[i]=searchstr[j];
            searchstr[j]=temp[0];
            //find if is a substring
            for (auto it = sortedStreetName[(int)street_prefix[0]].begin(); it != sortedStreetName[(int)street_prefix[0]].end() && street_id.size()<maxStreet; it++){

                string name = it->first;
                int streetPrefixLength = street_prefix.length();

                //if user has entered more than 4 characters
                if(streetPrefixLength>4){
                    //match string prefix
                    if(name.find(searchstr)!=std::string::npos){
                        cout<<"found street with misspelled name"<<endl;
                        street_id.push_back(it->second);

                    } 
                }
                
                //eliminate duplicated vectors
                sort(street_id.begin(), street_id.end());
                street_id.erase(unique(street_id.begin(),street_id.end()), street_id.end());
            }
        }
        
        
    }
    return street_id;
}
