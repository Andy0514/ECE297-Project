
/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

#include "m3.h"
#include "StreetsDatabaseAPI.h"
#include "globalData.h"
#include "segmentNode.h"

#include <vector>
#include <string>
#include <queue>
using namespace std;


  //map that is sorted by overall heuristics (double) and includes intersection ID (int).
    //this is the "queue" that determines which node (intersection) to check next
struct node {
    node(int segID, double time, double h, int poppedIndex, int previousSeg){
        ID = segID;
        netTravelTime = time;
        popped = poppedIndex;
        this->heuristics = h;
        prevSeg = previousSeg;
    }
   
    node(int segID, double time, int poppedIndex){
        ID = segID;
        netTravelTime = time;
        popped=poppedIndex;
        heuristics = DBL_MAX;
    }
    
    int ID;
    double netTravelTime;
    int popped;
    double heuristics;
    int prevSeg = -1;
};


struct popped{
    popped(int currSegm, int lastPopped) {
        currSeg = currSegm;
        lastPoppedID = lastPopped;
    }
    int currSeg;
    int lastPoppedID;
};


vector<int> traceback(int endPoppedID,  vector <popped> poppedIntersections);


map<int, vector<int>> reverse_find_path(
		          const IntersectionIndex intersect_id_start, const IntersectionIndex intersect_id_end,
                   vector <int> ends, const double turn_penalty);
map<int, vector<int>> getWalkableIntersections(int start, const double turnPenalty, const double walkSpeed, const double walkingTimeLimit);
void recursiveHelper(int start, const double turnPenalty, const double walkSpeed, const double walkingTimeLimit, map<int, vector<int>> & intersec);
double lookAhead(int current, int end, int fromSeg, double turn_penalty);




struct compareNode{
    bool operator()(const node & lhs, const node & rhs){
        
        return (lhs.heuristics > rhs.heuristics);
    }
};




double lookAhead(int current, int end, int fromSeg, double turn_penalty){
    if(current == end)
        return 0;
    double smallestHeuristic = DBL_MAX;
    int chosenSeg = fromSeg;
    vector<int> & segments = intersectionGraph[current].segment;
    for (auto i = segments.begin(); i != segments.end(); i++){
        
        if (*i == fromSeg){
            continue;
        }
        segmentNode & currentSeg = segmentDatabase[*i];
        int destID;
        if (currentSeg.to == current){
            //heuristics += intersectionGraph[currentSeg.from].distance/3100;
            destID = currentSeg.from;
        } else {
           // heuristics += intersectionGraph[currentSeg.to].distance/3100;
            destID = currentSeg.to;
        }
        
        double heuristic = currentSeg.travelTime + intersectionGraph[destID].distance / 31;
        if (heuristic < smallestHeuristic){
            smallestHeuristic = heuristic;
            chosenSeg = *i;
        }
        
    }
    
    if (segmentDatabase[chosenSeg].streetID != segmentDatabase[fromSeg].streetID){
        return turn_penalty;
    } else {
        return 0;
    }
}


vector<int> traceback(int endPoppedID,  vector <popped> poppedIntersections){
    
    vector <int> segResult;
    
    int currentPopped = endPoppedID;
    while (currentPopped != -1 && currentPopped != 0){
        segResult.push_back(poppedIntersections[currentPopped].currSeg);
        currentPopped = poppedIntersections[currentPopped].lastPoppedID;
    }
    
    reverse(segResult.begin(), segResult.end());
    return segResult;
}



//compute travel time for driving
double compute_path_travel_time(const std::vector<StreetSegmentIndex>& path, 
                                const double turn_penalty){
    
    double currentTime = 0.0;
    for (auto it = path.begin(); it != path.end(); it++){

        currentTime += segmentDatabase[*it].travelTime;
        
        if ((it+1)!=path.end()) {
            StreetSegmentIndex nextSeg = *(it+1);
            if (segmentDatabase[nextSeg].streetID != segmentDatabase[*it].streetID){
                currentTime += turn_penalty;
            }
        }
    }
    return currentTime;
}





struct intersecData{
    double distance;
    double heuristics;
};

//find path for driving
vector<StreetSegmentIndex> find_path_between_intersections(
		          const IntersectionIndex intersect_id_start, 
                  const IntersectionIndex intersect_id_end,
                  const double turn_penalty){
    drivePath.clear();
    walkPath.clear();
    priority_queue <node, vector<node>, compareNode> q;
    vector <popped> poppedIntersections;
    
    //we need this in order to perform SMT with find path function (for M4))
    vector <intersecData> intersections;
    intersections.resize(intersectionGraph.size());
    
    //calculate the distance from end_id to each intersection
    LatLon endPt = intersectionGraph[intersect_id_end].position;
    
    for (int i = 0; i < intersectionGraph.size(); i++){
        intersections[i].distance = findDistance(intersectionGraph[i].position, endPt);
        intersections[i].heuristics = DBL_MAX;
    }
    node startNode(intersect_id_start, 0.0, 0);
    q.push(startNode);
    popped firstNode (-1, -1);
    poppedIntersections.push_back(firstNode);
    int poppedCounter = 1;
    while (q.size() != 0){

        //top is a REFERENCE to the first element in q
        node top = q.top();
        q.pop();
        int poppedID = top.popped;
        if (top.ID == intersect_id_end){
            
            return traceback(poppedID, poppedIntersections);
        }
        
        //get the connecting street segments
        vector <StreetSegmentIndex> & segments = intersectionGraph[top.ID].segment;

        //loop through street segments of the current node
        for (auto i = segments.begin(); i != segments.end(); i++){ 
            segmentNode & currentSeg = segmentDatabase[*i];
            if (currentSeg.oneway && currentSeg.to == top.ID) {
                 //one way street pointing the wrong way
                continue;
            }

            if (*i == top.prevSeg){
                continue; //this is the segment that we came from
            } 
           // if ((top.path.size() != 0) && *i == top.path[top.path.size()-1]) continue; //this is the segment that we came from
            double heuristics = currentSeg.travelTime + top.netTravelTime;
            int destID;
            if (currentSeg.to == top.ID){
                heuristics += intersections[currentSeg.from].distance/32;
                destID = currentSeg.from;
            } else {
               heuristics += intersections[currentSeg.to].distance/32;
                destID = currentSeg.to;
            }
            
            double turnPenalty = 0;
            //see if the street has changed
            if (top.prevSeg >= 0 && segmentDatabase[top.prevSeg].streetID !=currentSeg.streetID){
                turnPenalty = turn_penalty;
            }

            heuristics += turnPenalty;
            
            //update heuristics for this newly searched intersection
            if(intersections[destID].heuristics > (heuristics)){

                node newNode(destID, top.netTravelTime + currentSeg.travelTime+turnPenalty, heuristics, poppedCounter, *i);

                poppedCounter++;
                intersections[destID].heuristics = heuristics;
                q.push(newNode);

                popped newPoppedNode(*i, poppedID);
                poppedIntersections.push_back(newPoppedNode);
            }
        }
    }
    vector <int> empty = {};
    return empty;
}

// Returns the time required to "walk" along the path specified, in seconds.
// The path is given as a vector of street segment ids. The vector can be of
// size = 0, and in this case, it the function should return 0. The walking
// time is the sum of the length/<walking_speed> for each street segment, plus
// the given turn penalty, in seconds, per turn implied by the path. If there
// is no turn, then there is no penalty.  As mentioned above, going from Bloor
// Street West to Bloor street East is considered a turn
double compute_path_walking_time(const std::vector<StreetSegmentIndex>& path, 
                                 const double walking_speed, 
                                 const double turn_penalty){
    
    double currentTime = 0.0;
    for (auto it = path.begin(); it != path.end(); it++){
        currentTime += segmentDatabase[*it].length / walking_speed;
        
        if ((it+1)!=path.end()) {
            StreetSegmentIndex nextSeg = *(it+1);
            if (segmentDatabase[nextSeg].streetID != segmentDatabase[*it].streetID){
                currentTime += turn_penalty;
            }
        }
        
    }
    return currentTime;
}



//walkq consists of <heuristics, intersection node>
//put in multimap so that we get lowest heuristic easily
multimap <double, node> walkq;


//walkVisited consists of <distance to distination, intersection node>
//put in multimap so that we get lowest distance easily
multimap <double, node> walkVisited;


//recursively find walkable intersections within time limit
void recursiveHelper(int start, const double turnPenalty, const double walkSpeed, const double walkingTimeLimit, map<int, vector<int>> & intersec){
    if (intersectionGraph[start].walkingCost >= walkingTimeLimit) return;
    
    vector<int> segments = intersectionGraph[start].segment;
    vector<int> currentPath = intersec[start];
    for (auto i = segments.begin(); i != segments.end(); i++){
        
        
        segmentNode & currentSeg = segmentDatabase[*i];
        
        int destID;
        if (currentSeg.to == start){
            destID = currentSeg.from;
        } else {
            destID = currentSeg.to;
        }
        if (*i == intersectionGraph[start].prevSeg) continue;//this is the path that we came from
        double cost = (currentSeg.length/walkSpeed) + intersectionGraph[start].walkingCost;
        
        //add turn penalty
        if (intersectionGraph[start].prevSeg != -1 && segmentDatabase[intersectionGraph[start].prevSeg].streetID !=segmentDatabase[*i].streetID){
            cost += turnPenalty;
        }
        
        //if cost is smaller than the existing cost: either replace existing map node or insert a new map node
        if (cost < intersectionGraph[destID].walkingCost){
            
            auto it = intersec.find(destID);
            if (it != intersec.end()){
                intersec.erase(it);
            }
            
            vector <int> path = currentPath;
            path.push_back(*i);
            intersec.insert(make_pair(destID, path));
                
            //update intersectionGraph entries
            intersectionGraph[destID].walkingCost = cost;
            intersectionGraph[destID].prevSeg = *i;
            
            //recursively check the destination node
            recursiveHelper(destID, turnPenalty, walkSpeed, walkingTimeLimit, intersec);
        }
    }
}


// returns a map of walkable intersections within the time limit
map<int, vector<int>> getWalkableIntersections(int start, const double turnPenalty, const double walkSpeed, const double walkingTimeLimit){
    
    for (auto i = intersectionGraph.begin(); i != intersectionGraph.end(); i++){
        i->walkingCost = walkingTimeLimit;
        i->prevSeg = -1;
    }
    
    map <int, vector<int>> walkableIntersections;
    intersectionGraph[start].walkingCost = 0;
    vector<int> blank = {};
    walkableIntersections.insert(make_pair(start, blank));
    recursiveHelper(start, turnPenalty, walkSpeed, walkingTimeLimit, walkableIntersections);
    
    //clear prevSeg
    for (auto i = intersectionGraph.begin(); i != intersectionGraph.end(); i++){
        i->prevSeg = -1;
    }
    
    return walkableIntersections;
}



//find path by going from destination to walkable intersections
map<int, vector<int>> reverse_find_path(
		          const IntersectionIndex intersect_id_start, const IntersectionIndex intersect_id_end,
                   vector <int> ends, const double turn_penalty){
    
    map <int, vector<int>> result;
    vector <popped> poppedIntersections;
    drivePath.clear();
    walkPath.clear();
    priority_queue <node, vector<node>, compareNode> q;
    poppedIntersections.clear();
    
    //calculate the distance from end_id to each intersection
    LatLon endPt = intersectionGraph[intersect_id_end].position;
    for (auto i = intersectionGraph.begin(); i != intersectionGraph.end(); i++){
        i->distance = findDistance(i->position, endPt);
        i->heuristics = DBL_MAX;
        i->insertLimit = 3;
    }
    node startNode(intersect_id_start, 0.0, 0);
    q.push(startNode);
    popped firstNode (-1, -1);
    poppedIntersections.push_back(firstNode);
    int poppedCounter = 1;
    
    while (q.size() != 0){
        //top is a REFERENCE to the first element in q
        node top = q.top();
        q.pop();
        int poppedID = top.popped;
        //this intersection is walkable, so check it
        auto it = find(ends.begin(), ends.end(), top.ID);
        if (it != ends.end()){

            auto alreadyInserted = result.find(top.ID);
            if (alreadyInserted == result.end()){
                //this node has not been checked before
                result.insert(make_pair(top.ID, traceback(poppedID, poppedIntersections)));
            } else {
                //compare existing cost with new cost
                double existingCost = compute_path_travel_time(alreadyInserted->second, turn_penalty);
                double newCost = compute_path_travel_time(traceback(poppedID, poppedIntersections), turn_penalty);
                
                if (newCost < existingCost){
                    result.erase(alreadyInserted);
                    result.insert(make_pair(top.ID, traceback(poppedID, poppedIntersections)));
                }
            }
        }
        if ((ends.size() < 3 && result.size() == ends.size()) || (ends.size() >= 3 && ends.size() - result.size() <= 1)){   
            return result;
        }
        
        
        //get the connecting street segments
        vector <StreetSegmentIndex> & segments = intersectionGraph[top.ID].segment;
       
        //loop through street segments of the current node
        for (auto i = segments.begin(); i != segments.end(); i++){ 
            segmentNode & currentSeg = segmentDatabase[*i];
            if (currentSeg.oneway && currentSeg.from == top.ID) {
                 //one way street pointing the wrong way
                continue;
            }

            if (*i == top.prevSeg){
                continue; //this is the segment that we came from
            } 
            double heuristics = currentSeg.travelTime + top.netTravelTime;
            int destID;
            if (currentSeg.to == top.ID){
                heuristics += intersectionGraph[currentSeg.from].distance/32;
                destID = currentSeg.from;
            } else {
               heuristics += intersectionGraph[currentSeg.to].distance/32;
                destID = currentSeg.to;
            }
            
            double turnPenalty = 0;
            //see if the street has changed
            if (top.prevSeg >= 0 && segmentDatabase[top.prevSeg].streetID !=currentSeg.streetID){
                turnPenalty = turn_penalty;
            }
            heuristics += turnPenalty;
            
            heuristics += lookAhead(destID, intersect_id_end, *i, turn_penalty);

            
            //update heuristics for this newly searched intersection
            if(intersectionGraph[destID].heuristics > (heuristics)){
                node newNode(destID, top.netTravelTime + currentSeg.travelTime+turnPenalty,heuristics, poppedCounter, *i);
                poppedCounter++;
                intersectionGraph[destID].heuristics = heuristics;
                q.push(newNode);
                intersectionGraph[destID].pastTravelTime = top.netTravelTime + currentSeg.travelTime+turnPenalty;
                
                popped newPoppedNode(*i, poppedID);
                poppedIntersections.push_back(newPoppedNode);
            }
            else if (intersectionGraph[destID].pastTravelTime > (top.netTravelTime + currentSeg.travelTime - turn_penalty) && intersectionGraph[destID].insertLimit > 0){
                node newNode(destID, top.netTravelTime + currentSeg.travelTime+turnPenalty, heuristics, poppedCounter, *i);
                poppedCounter++;
                q.push(newNode);
                intersectionGraph[destID].insertLimit--;
                
                popped newPoppedNode(*i, poppedID);
                poppedIntersections.push_back(newPoppedNode);
            }
        }
    }
    return result;
}


//first is the walk vector, second is the drive vector
std::pair<std::vector<StreetSegmentIndex>, std::vector<StreetSegmentIndex>>  
        find_path_with_walk_to_pick_up(const IntersectionIndex start_intersection, 
                                               const IntersectionIndex end_intersection,
                                               const double turn_penalty,
                                               const double walking_speed, 
                                               const double walking_time_limit){
    
    //first, get a list of all intersections reachable by walking, along with the path to that intersection
    map <int, vector<int>> walkableIntersections = getWalkableIntersections(start_intersection, turn_penalty, walking_speed, walking_time_limit);
    
    //if the intersection is walkable: walk it
    auto it = walkableIntersections.find(end_intersection);
    if (it != walkableIntersections.end()) {
        vector <int> empty;
        return make_pair(it->second, empty);
    }
    
    //extra all intersections that can be found by walking
    vector <int> walkable;
    for (it = walkableIntersections.begin(); it != walkableIntersections.end(); it++){
        walkable.push_back(it->first);
    }
    //then, use Dijkstra to return a map of all reachable nodes among walkableIntersections and their paths, starting from the END
    map<int, vector<int>> drivable = reverse_find_path(end_intersection, start_intersection, walkable, turn_penalty);

    //lastly, calculate all costs for the returned driving path, and find the fastest
    //if drivable is empty, return both walk and drive vector as empty
    if (drivable.size() == 0) {
        vector<int> empty;
        return make_pair(empty, empty);
    }
    
    double currentFastestTime = DBL_MAX;
    int fastestIntersection;
    for (auto iter = drivable.begin(); iter != drivable.end(); iter++){
        double cost = compute_path_travel_time(iter->second, turn_penalty);
        if (cost < currentFastestTime){
            currentFastestTime = cost;
            fastestIntersection = iter->first;
        }
    }
    
    reverse(drivable[fastestIntersection].begin(), drivable[fastestIntersection].end());
    
    return make_pair(walkableIntersections[fastestIntersection], drivable[fastestIntersection]);
}

