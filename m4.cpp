#include "globalData.h"
#include "m1.h"
#include "m2.h"
#include "m4.h"
#include "m3.h"
#include "courierVerify.h"
#include <thread>
#include <unordered_map>
#include <future>

#include <chrono> 
#include <fstream> 
//std::ofstream perf("test_speed.csv"); 
////profile_csv_file must already be open (opened in load map)
//void profile_csv(std::chrono::time_point<std::chrono::high_resolution_clock startTime,
//                     std::string func_name) { 
////#ifdef PROFILING
//  auto const endTime = std::chrono::high_resolution_clock::now();
//     auto const elapsedTime = std::chrono::duration_cast<std::chrono::duration<double>>
//                             (endTime-startTime);
//     profile_csv_file << func_name << "," << elapsedTime.count() << std::endline;
////#endif 
//}



map <int, vector<int>> reservedPickup, reservedDropoff;

using namespace std;
using ece297::courier_path_is_legal_with_capacity;
void getCourierPathThreadHelper(const vector <DeliveryInfo> & deliveries, const vector <int> & depots,  map <double, int> depotList,
       const float turn_penalty, const float truck_capacity, map <int, vector<int>> pickup, map <int, vector<int>> dropoff, vector<CourierSubpath>& returnedResult);
bool pickupDropoff(CourierSubpath & nextPath, const vector <DeliveryInfo> & deliveries, map <int, vector<int>> & pickup, 
        map <int, vector<int>> & dropoff, double &remainingRoom, vector<int>&truckContents, int currentIntersection);
void getCourierPath(int startingDepot, const vector <DeliveryInfo> & deliveries, const vector <int> & depots, 
                    const float turn_penalty, const float truck_capacity, map <int, vector<int>> pickup, map <int, vector<int>> dropoff,
                     vector <CourierSubpath> & result, int randomFactor);
void calcTravelTime(vector<int> startingIntersec, const vector <DeliveryInfo> & deliveries, double turn_penalty, vector<int> depots);
double calculatePathTime(vector <CourierSubpath> path, double turn_penalty);
void localPerturbation(vector<CourierSubpath> & perturbationResult, const vector <DeliveryInfo> & deliveries, const vector <int> & depots, 
                                                            const float turn_penalty, const float truck_capacity);
void twoEdgeOperations(int length1, int length2, int length3, vector<CourierSubpath> & perturbationResult, const vector <DeliveryInfo> & deliveries, const vector <int> & depots, 
                                                            const float turn_penalty, const float truck_capacity);
vector<CourierSubpath> longPathBreakDown(vector<CourierSubpath> & initialPath, const vector <DeliveryInfo> & deliveries, const vector <int> & depots, 
                                                            const float turn_penalty, const float truck_capacity, map <int, vector<int>> pickup, map <int, vector<int>> dropoff);
void fitNewPath(const vector <DeliveryInfo> & deliveries, const vector <int> & depots, 
                    const float turn_penalty, const float truck_capacity, vector <CourierSubpath> & result);
unordered_map <int, double> find_distance_between_points(
		          const IntersectionIndex intersect_id_start, //input every current pickup or dropoff
                  const vector <DeliveryInfo> & deliveries,
                  vector<int> depots,
                  const double turn_penalty, unordered_map<int, vector<int>> & paths);
void twoOpt(vector<CourierSubpath> & result, const vector <DeliveryInfo> & deliveries, const vector <int> & depots, 
                                                            const float turn_penalty, const float truck_capacity, int cutoutSize);
vector<CourierSubpath>::iterator search(vector<CourierSubpath>::iterator start, vector<CourierSubpath>::iterator end, CourierSubpath target);
void twoOptShift(vector<CourierSubpath> & result, const vector <DeliveryInfo> & deliveries, const vector <int> & depots, 
                                                   const float turn_penalty, const float truck_capacity, int firstSegSize);

std::multimap<IntersectionIndex,size_t> intersections_to_pick_up; //Intersection_ID -> deliveries index
        std::multimap<IntersectionIndex,size_t> intersections_to_drop_off; //Intersection_ID -> deliveries index
void twoOptSwap(vector<CourierSubpath> & result, const vector <DeliveryInfo> & deliveries, const vector <int> & depots, 
                                                            const float turn_penalty, const float truck_capacity, int cutoutSize);
void twoOptRetry(vector<CourierSubpath> & result, const vector <DeliveryInfo> & deliveries, const vector <int> & depots, 
                                                            const float turn_penalty, const float truck_capacity, int cutoutSize);
void twoEdgeOperationsWholePath(int length, vector<CourierSubpath> & perturbationResult, const vector <DeliveryInfo> & deliveries, const vector <int> & depots, 
                                                            const float turn_penalty, const float truck_capacity);
void doTwoEdgeOperationsWholePath(vector<CourierSubpath> & perturbationResult, const vector <DeliveryInfo> & deliveries, const vector <int> & depots, 
                                                            const float turn_penalty, const float truck_capacity);


unordered_map<int, map<double,int>> interToDepots;//<intersection, map<distance, depot number>
unordered_map<int, unordered_map<int, double>> travelTime;
unordered_map<int, unordered_map<int, vector<int>>> travelPath;
bool validPickupDropoff;
int iterations;
double globalBestTime;
auto start = chrono::steady_clock::now();
vector <CourierSubpath> traveling_courier(const vector <DeliveryInfo> & deliveries, const vector <int> & depots, 
                                                            const float turn_penalty, const float truck_capacity){
    start = chrono::steady_clock::now();
    intersections_to_pick_up.clear();
    intersections_to_drop_off.clear();
    validPickupDropoff=true;
    srand(1);
    globalBestTime = DBL_MAX;
    //cout<<deliveries[0].pickUp<<endl;
    
    interToDepots.clear();
    
    //check if delivery or depot is empty
    if(deliveries.size()==0 || depots.size()==0){
        vector<CourierSubpath> empty={};
        return empty;
    }
    

    
    
    //first determine a suitable depot to start
    //then determine the closest pickup location. At any time, if the pickup weight > capacity, return empty
    //after at the pickup location, determine the distance to other pickup locations that have collective weight < truck capacity,
    //and required dropoff location. Go to the shortest one.
    //<--repeat until no more pickup and dropoff

    //Load the look-ups (for checking path validity)
        for(size_t delivery_idx = 0; delivery_idx < deliveries.size(); ++delivery_idx) {
            IntersectionIndex pick_up_intersection = deliveries[delivery_idx].pickUp;
            IntersectionIndex drop_off_intersection = deliveries[delivery_idx].dropOff;

            intersections_to_pick_up.insert(std::make_pair(pick_up_intersection, delivery_idx));
            intersections_to_drop_off.insert(std::make_pair(drop_off_intersection, delivery_idx));
        }
    
    //store all the pickup and delivery locations in maps <intersectionIndex of pickup/dropoff, vector <int> indexOfPickup>
    map <int, vector<int>> pickup, dropoff;
    vector<int> needToCalculateTravelTime;
    int idx = 0;
    for (auto loc = deliveries.begin(); loc != deliveries.end(); loc++){
        
        //check if the delivery weight is larger than truck capacity, therefore impossible to make the delivery
        if(loc->itemWeight > truck_capacity){
            vector<CourierSubpath> empty={};
            return empty;
            
        }
        
        
        
        auto existingPickup = pickup.find(loc->pickUp);
        if (existingPickup == pickup.end()){
            //this intersection currently doesn't hold a second pickup
            vector <int> temp = {idx};
            pickup.insert(make_pair(loc->pickUp, temp));
            needToCalculateTravelTime.push_back(loc->pickUp);
            //travelTime.insert(make_pair(loc->pickUp, find_distance_between_points(loc->pickUp, deliveries, turn_penalty)));
        } else {
            //this intersection holds a subsequent pickup
            pickup[loc->pickUp].push_back(idx);
        }
        
        auto existingDropoff = dropoff.find(loc->dropOff);
        if (existingDropoff == dropoff.end()){
            //this intersection currently doesn't contain any other dropoff
            vector <int> temp = {idx};
            dropoff.insert(make_pair(loc->dropOff, temp));
            
            if (find(needToCalculateTravelTime.begin(), needToCalculateTravelTime.end(), loc->dropOff) == needToCalculateTravelTime.end()){
                needToCalculateTravelTime.push_back(loc->dropOff);
            }
        } else {
            dropoff[loc->dropOff].push_back(idx);
        }
        idx++;
    }
    
    
    //uses threads to calculate travel time
    vector<int> bin0, bin1, bin2, bin3, bin4, bin5, bin6, bin7;
    for (auto i = needToCalculateTravelTime.begin(); i != needToCalculateTravelTime.end(); i++){
        if (bin0.size() == bin1.size() && bin2.size() == bin3.size() && bin1.size() == bin2.size()){
            bin0.push_back(*i);
        } else if (bin0.size() > bin1.size()){
            bin1.push_back(*i);
        }else if (bin1.size() > bin2.size()){
            bin2.push_back(*i);
        } else if (bin2.size() > bin3.size()){
            bin3.push_back(*i);
        }
    }
    thread time0(calcTravelTime, bin0, deliveries, turn_penalty, depots);
    thread time1(calcTravelTime, bin1, deliveries, turn_penalty, depots);
    thread time2(calcTravelTime, bin2, deliveries, turn_penalty, depots);
    thread time3(calcTravelTime, bin3, deliveries, turn_penalty, depots);
    
    time0.join();
    time1.join();
    time2.join();
    time3.join();
    
    //some pickups or drop offs can't be made
    //flag was set in calcTravelTime -> find_distance_between_points
    if(validPickupDropoff==false){
        vector<CourierSubpath> empty={};
        return empty;
    }
    
    //saves the full pickup and dropoff info
    reservedPickup = pickup;
    reservedDropoff = dropoff;
    
    //suitable depot: choose one that is at a minimum distance to a pickup-only intersection. Also record that closest pickup location
    map <double, int> depotList;
    for (auto loc = deliveries.begin(); loc != deliveries.end(); loc++){
        for (auto dep = interToDepots[loc->pickUp].begin(); dep != interToDepots[loc->pickUp].end(); dep++){
            depotList.insert(*dep);
        }
    }
//    }
//    for (auto depot = depots.begin(); depot != depots.end(); depot++){
//        double minDist = DBL_MAX;
//        int i = 0;
//        for (auto loc = deliveries.begin(); loc != deliveries.end(); loc++){
//            if (loc->itemWeight > truck_capacity){
//                //this item can't be put in the truck, hence delivery impossible
//                vector<CourierSubpath> empty;
//                return empty;
//            }
//            
//            //if this point is both drop off and pickup point, don't consider
//            if (dropoff.find(loc->pickUp) != dropoff.end()) continue;
//            
//            if (interToDepots[loc->pickUp].find(i) 
//            double currentDistance = findDistance(intersectionGraph[*depot].position, intersectionGraph[loc->pickUp].position);
//            if (currentDistance < minDist){
//                minDist = currentDistance;
//            }
//            i++;
//        }
//        depotList.insert(make_pair(minDist, *depot));
//    }

     
    iterations = 0;
    vector<CourierSubpath> result, result0, result1, result2, result3;
    thread th0(getCourierPathThreadHelper, ref(deliveries), ref(depots), depotList, turn_penalty, truck_capacity, pickup, dropoff, ref(result0));
    thread th1(getCourierPathThreadHelper, ref(deliveries), ref(depots), depotList, turn_penalty, truck_capacity, pickup, dropoff, ref(result1));
    thread th2(getCourierPathThreadHelper, ref(deliveries), ref(depots), depotList, turn_penalty, truck_capacity, pickup, dropoff, ref(result2));
    thread th3(getCourierPathThreadHelper, ref(deliveries), ref(depots), depotList, turn_penalty, truck_capacity, pickup, dropoff, ref(result3));
    
    th0.join();
    th1.join();
    th2.join();
    th3.join();
    double t0 = calculatePathTime(result0, turn_penalty);
    double t1 = calculatePathTime(result1, turn_penalty);
    double t2 = calculatePathTime(result2, turn_penalty);
    double t3 = calculatePathTime(result3, turn_penalty);
    
    cout<<"times from threads are: "<<t0<<" "<<t1<<" "<<t2<<" "<<t3<<endl;
    if (t0 <= t1 && t0 <= t2 && t0 <= t3){
        result = result0;
    } else if (t1 <= t0 && t1 <= t2 && t1 <= t3){
        result = result1;
    } else if (t2 <= t0 && t2 <= t1 && t2 <= t3){
        result = result2;
    } else {
        result = result3;
    }

    //make a copy of the result
    vector<CourierSubpath> perturbationResult=result;
    //vector<CourierSubpath> perturbationResult=longPathBreakDown(result, deliveries, depots, turn_penalty, truck_capacity, pickup, dropoff);
    auto const startTime = std::chrono::high_resolution_clock::now();
    
    int resultSize=perturbationResult.size();
    int range=resultSize-3;
    for(int r=2;r<range/5;r++){

        twoEdgeOperations(r,r,r,perturbationResult, deliveries, depots, turn_penalty, truck_capacity);
        
    }
    doTwoEdgeOperationsWholePath(perturbationResult,deliveries, depots, turn_penalty, truck_capacity);
    localPerturbation(perturbationResult, deliveries, depots, turn_penalty, truck_capacity);
    auto const endTime = std::chrono::high_resolution_clock::now();
    auto const elapsedTime = std::chrono::duration_cast<std::chrono::duration<double>>
                             (endTime-startTime);
   

    result=perturbationResult;  
//    for (int i = 1; i < 10; i++){
//       // cout<<"_____div: "<<i<<" _____"<<endl;
//        twoOptSwap(result, deliveries, depots, turn_penalty, truck_capacity, i);
//    }
//    for (int i = 1; i < 3; i++){
//       // cout<<"_____div: "<<i<<" _____"<<endl;
//        twoOptSwap(result, deliveries, depots, turn_penalty, truck_capacity, i);
//    }
//     
    auto now = std::chrono::steady_clock::now();
    cout<<"Total time: "<<chrono::duration_cast<chrono::seconds>(now - start).count()<<", total iterations: <<"<<iterations<<endl;

    return result;
}


void getCourierPathThreadHelper(const vector <DeliveryInfo> & deliveries, const vector <int> & depots, map <double, int> depotList,
       const float turn_penalty, const float truck_capacity, map <int, vector<int>> pickup, map <int, vector<int>> dropoff, vector<CourierSubpath>& returnedResult){
    
    vector<CourierSubpath> result;
    int elapsedCount;
    auto now =  std::chrono::steady_clock::now();
    double minTime;
    while (true){
        if (result.size() == 0){
            elapsedCount = INT_MAX;
        } else if (elapsedCount == 1){
            elapsedCount = 15;
        } else {
            elapsedCount--;
        }
        iterations++;
        vector<CourierSubpath> tempResult;
        if (rand() % 2 == 1)
            getCourierPath(depotList.begin()->second, deliveries, depots, turn_penalty, truck_capacity, pickup, dropoff, ref(tempResult), elapsedCount);
        else {
            auto it = depotList.begin();
            int numbers = rand() % depotList.size();
            for (int i = 0; i < numbers; i++, it++);
        
            getCourierPath(it->second, deliveries, depots, turn_penalty, truck_capacity, pickup, dropoff, ref(tempResult), elapsedCount);
        }
        if (result.size() == 0){
            //on first run
            minTime = calculatePathTime(tempResult, turn_penalty);
            result = tempResult;
            elapsedCount = 15;
            if (minTime < globalBestTime)
                globalBestTime = minTime;
        } else {
            double time = calculatePathTime(tempResult, turn_penalty);
            if (time < minTime){
                result = tempResult;
                minTime = time;
            }
            if (time < globalBestTime)
                globalBestTime = time;
        }
        now = std::chrono::steady_clock::now();
        if (chrono::duration_cast<chrono::milliseconds>(now - start).count()>=46750) {
            break;
        }
        
        //only run once if the number of deliveries is small
       // if (deliveries.size() < 10) break;
    }
    

    int range=result.size()-3;
    for(int r=2;r<range/5;r++){
        twoEdgeOperations(r,r,r,result, deliveries, depots, turn_penalty, truck_capacity);
    }
//    cout<<"trying swapping"<<endl;
//    for (int i = 1; i < fmin(5, result.size()-2); i++){
//        result.erase(result.begin());
//        twoOptRetry(result, deliveries, depots, turn_penalty, truck_capacity, 2);
//    }
//    cout<<"end trying swapping"<<endl;
    doTwoEdgeOperationsWholePath(result,deliveries, depots, turn_penalty, truck_capacity);
    for (int i = 1; i < 6; i++){
      //  cout<<"_____div: "<<i<<" _____"<<endl;
        twoOpt(result, deliveries, depots, turn_penalty, truck_capacity, i);
    }
    for (int i = 1; i < 3; i++){
      //  cout<<"_____div: "<<i<<" _____"<<endl;
        twoOpt(result, deliveries, depots, turn_penalty, truck_capacity, i);
        doTwoEdgeOperationsWholePath(result,deliveries, depots, turn_penalty, truck_capacity);
    }
    returnedResult = result;
}




void twoEdgeOperations(int length1, int length2, int length3, vector<CourierSubpath> & perturbationResult, const vector <DeliveryInfo> & deliveries, const vector <int> & depots, 
                                                            const float turn_penalty, const float truck_capacity){
    
    
    //invalid input, return empty
    if(length1<2 ||length2<2 ||length3<2){
        return;
    }
    
    int legal=0,notLegal=0;
    int totalLength=length1+length2+length3;
    int i=0;
    for(auto it=perturbationResult.begin();it!=perturbationResult.end() && i+totalLength+1<perturbationResult.size();it++, i++){

        //pointer to the second subpath
        auto second=it+length1;

        auto third=second+length2;
        
        auto end=it+totalLength;
        //do I need this???????
        //find original travel time
        double originalTime;
        for(int j=0;j<totalLength;j++){
            originalTime += compute_path_travel_time((it+j)->subpath,turn_penalty);
        }
        
        double originalLinkTime=travelTime[(second-1)->start_intersection][(second-1)->end_intersection]+ travelTime[(third-1)->start_intersection][(third-1)->end_intersection]
        +travelTime[(end-1)->start_intersection][(end-1)->end_intersection];
        
        //case I: connect first part to third part's head
        //1.1 third part's tail to second's head
        //compute new time, see if it's optimized
        double newLinkTime=travelTime[(second-1)->start_intersection][third->start_intersection] + travelTime[(end-1)->start_intersection][second->start_intersection]
        +travelTime[(third-1)->start_intersection][end->start_intersection];
        
        

        //if optimized, do the swap
        if(newLinkTime<originalLinkTime){
            // **************************consider manually swap units instead of making copy?????
            vector<CourierSubpath> tempPerturbationResult=perturbationResult;
//            cout<<"here is original"<<endl;
//            for(int q=0; q<10;q++){
//                cout<<perturbationResult[i+q].start_intersection<<" "<<perturbationResult[i+q].end_intersection<<endl;
//            }
            
            //find new link path
            vector<int> newpath1=travelPath[(second-1)->start_intersection][third->start_intersection];
            vector<int> newpath2=travelPath[(end-1)->start_intersection][second->start_intersection];
            vector<int> newpath3=travelPath[(third-1)->start_intersection][end->start_intersection];  
            
            //get original path in 3 parts
            vector<CourierSubpath> firstPart (perturbationResult.begin()+i,perturbationResult.begin()+i+length1-1);//-1: because link should not be counted; also (x,y) means <y
            vector<CourierSubpath> secondPart (perturbationResult.begin()+i+length1,perturbationResult.begin()+i+length1+length2-1);
            vector<CourierSubpath> thirdPart (perturbationResult.begin()+i+length1+length2,perturbationResult.begin()+i+totalLength-1);
//            cout<<"expecting 3 twos"<<firstPart.size()<<secondPart.size()<<thirdPart.size()<<endl;
            //link subpath, copy start intersection, pickup indices, then change end intersections
            CourierSubpath link1=perturbationResult[i+length1-1];
            CourierSubpath link2=perturbationResult[i+totalLength-1];
            CourierSubpath link3=perturbationResult[i+length1+length2-1];
            //end intersections
            link1.end_intersection=third->start_intersection;
            link2.end_intersection=second->start_intersection;
            link3.end_intersection=end->start_intersection;
            //subpaths
            link1.subpath=newpath1;
            link2.subpath=newpath2;
            link3.subpath=newpath3;
            
            //add links to parts, now they have new links
            firstPart.push_back(link1);
            thirdPart.push_back(link2);
            secondPart.push_back(link3);
            
            //adding up parts: total segments that's replacing original
            firstPart.insert(firstPart.end(),thirdPart.begin(),thirdPart.end());
            firstPart.insert(firstPart.end(),secondPart.begin(),secondPart.end());
            //now firstPart contains the whole path(length = totalLength)
            

//            cout<<"copy into result"<<endl;
            for(int k=0;k<totalLength;k++){
                perturbationResult[i+k]=firstPart[k];
            }
        
            //if not legal:
            if(courier_path_is_legal_with_capacity(deliveries, depots, perturbationResult, truck_capacity)!=true){

                //give up this attempt
                perturbationResult=tempPerturbationResult;
                notLegal+=1;
            }else{
//                cout<<"legal AS"<<endl;
                legal+=1;
            }
//                    bool isLegal = courier_path_is_legal_withcapacity(deliveries, depots, perturbationResult, truck_capacity);
//                    cout<<"legal: "<<isLegal<<endl;

        }              
          //increment i and it by 3 so that it doesn't overwrite
//        i=i+6;
//        it=it+6;
        
    }
    
    if(legal>0){//if made improvement and at least one of the paths is legal
//        cout<<"a 2-opt test made improvement"<<endl;
//        cout<<"this is the result for length1="<<length1<<", length2="<<length2<<", length3="<<length3<<endl;
//        cout<<"legal:"<<legal<<";   not legal:"<<notLegal<<endl;
    }
}



vector<CourierSubpath>::iterator search(vector<CourierSubpath>::iterator startSect, vector<CourierSubpath>::iterator endSect, CourierSubpath target){
    for (auto i = startSect; i != endSect; i++){
        
        if (i->start_intersection == target.start_intersection && i->end_intersection == target.end_intersection && i->pickUp_indices == target.pickUp_indices && i->subpath == target.subpath){
            return i;
        }
    }
    return endSect;
}



void twoOpt(vector<CourierSubpath> & result, const vector <DeliveryInfo> & deliveries, const vector <int> & depots, 
                                                            const float turn_penalty, const float truck_capacity, int cutoutSize){
    
    if (cutoutSize + 3 >= result.size()) return;
    for (auto first = result.begin(); first + cutoutSize + 3 != result.end(); first++){
        auto last = first + cutoutSize + 3;
        auto afterFirst = first + 2;
        auto beforeLast = first + cutoutSize + 1;
        
        vector <CourierSubpath> temp = result;
        
        //delete the two cutout paths
        temp.erase(search(temp.begin(), temp.end(), *first)+ 1);
        temp.erase(search(temp.begin(), temp.end(), *beforeLast) + 1);
        

        //insert a connection between tempFirst and tempBeforeLast
        CourierSubpath firstLink;
        firstLink.start_intersection = first->end_intersection;
        firstLink.end_intersection = beforeLast->end_intersection;
        firstLink.pickUp_indices = (first + 1)->pickUp_indices;
        firstLink.subpath = travelPath[firstLink.start_intersection][firstLink.end_intersection];
        temp.insert(search(temp.begin(), temp.end(), *first)+1, firstLink);
        
        //insert a connection between tempAfterFirst and tempLast
        CourierSubpath lastLink;
        lastLink.start_intersection = afterFirst->start_intersection;
        lastLink.end_intersection = last->start_intersection;
        lastLink.pickUp_indices = afterFirst->pickUp_indices;
        lastLink.subpath = travelPath[lastLink.start_intersection][lastLink.end_intersection];
        temp.insert(search(temp.begin(), temp.end(), *last), lastLink);
        
        //reverse the middle cutout segment. Forward and backward are in the sense of the original path before being reversed
        auto forward = search(temp.begin(), temp.end(), firstLink) + 1; 
        auto backward = search(temp.begin(), temp.end(), *beforeLast);
        while (forward <= backward){
            if (forward < backward){
                //switch pickup indice
                backward->pickUp_indices = (forward+1)->pickUp_indices;
                forward->pickUp_indices =  (last-1)->pickUp_indices;

                last--;

                //switch start and end intersection
                int savedStart = forward->start_intersection;
                int savedEnd = forward->end_intersection;
                forward->start_intersection = backward->end_intersection;
                forward->end_intersection = backward->start_intersection;
                backward->start_intersection = savedEnd;
                backward->end_intersection = savedStart;

                //compute new path
                forward->subpath = travelPath[forward->start_intersection][forward->end_intersection];
                backward->subpath = travelPath[backward->start_intersection][backward->end_intersection];

                forward++;
                backward--;
            } else if (forward == backward){
                
                forward->pickUp_indices = (last-1)->pickUp_indices;
                //switch its own start and end
                int savedStart = forward->start_intersection;
                forward->start_intersection = forward->end_intersection;
                forward->end_intersection = savedStart;
                
                forward->subpath = travelPath[forward->start_intersection][forward->end_intersection];
                forward++;
                backward--;
            }
        }

        double originalTime = 0, newTime = 0;
        for (int i = 0; i < result.size(); i++){
            originalTime += compute_path_travel_time(result[i].subpath, turn_penalty);
            newTime += compute_path_travel_time(temp[i].subpath, turn_penalty);
        }
       // cout<<"permutation: time: "<<newTime<<" , old time: "<<originalTime<<endl;
        if (newTime < originalTime && courier_path_is_legal_with_capacity(deliveries, depots, temp, truck_capacity)){
           // cout<<"permutation: better time found: "<<newTime<<endl;
           //the permutated version is better, so keep it
            result = temp;
        }
    }    
}

//before calling this, must remove the first CourierSubpath from result (corresponding to the subpath from depot to first pickup)
void twoOptSwap(vector<CourierSubpath> & result, const vector <DeliveryInfo> & deliveries, const vector <int> & depots, 
                                                            const float turn_penalty, const float truck_capacity, int cutoutSize){
    if (cutoutSize + 3 >= result.size()) return;
    for (auto first = result.begin(); first + cutoutSize + 3 != result.end(); first++){
        auto last = first + cutoutSize + 3;
        auto afterFirst = first + 2;
        auto beforeLast = first + cutoutSize + 1;
        
        vector <CourierSubpath> temp = result;
        if (reservedPickup.find(beforeLast->start_intersection) == reservedPickup.end()) continue; //if beforeLast is not a pickup
        //delete the two cutout paths
        temp.erase(search(temp.begin(), temp.end(), *beforeLast) + 1);
        
       
        CourierSubpath firstLink;
        firstLink.start_intersection = afterFirst->start_intersection;
        firstLink.end_intersection = result.begin()->start_intersection;
        firstLink.pickUp_indices = afterFirst->pickUp_indices;
        firstLink.subpath = travelPath[firstLink.start_intersection][firstLink.end_intersection];
        temp.insert(temp.begin(), firstLink);
        
       
        CourierSubpath lastLink;
        lastLink.start_intersection = first->end_intersection;
        lastLink.end_intersection = last->start_intersection;
        lastLink.pickUp_indices = (first + 1)->pickUp_indices;
        lastLink.subpath = travelPath[lastLink.start_intersection][lastLink.end_intersection];
        temp.insert(search(temp.begin(), temp.end(), *last), lastLink);


        auto forward = search(temp.begin(), temp.end(), *afterFirst); 
        auto backward = search(temp.begin(), temp.end(), *beforeLast); 
        
        while(forward <= backward){ //insert each node to the front from the start of seg2 to the end
            int tempPath = forward->start_intersection; // swap start and end
            forward->start_intersection = forward->end_intersection;
            forward->end_intersection = tempPath;
            forward->subpath = travelPath[forward->start_intersection][forward->end_intersection];
            temp.insert(temp.begin(), *forward); //insert at the front
            forward++;
            temp.erase(forward-1); //erase previous location in vector
        }
  
        double originalTime = 0, newTime = 0;
        for (int i = 0; i < result.size(); i++){
            originalTime += compute_path_travel_time(result[i].subpath, turn_penalty);
            newTime += compute_path_travel_time(temp[i].subpath, turn_penalty);
        }
        //cout<<"permutation: time: "<<newTime<<" , old time: "<<originalTime<<endl;
        if (newTime < originalTime){
            
            fitNewPath(deliveries, depots, turn_penalty, truck_capacity, temp);
            
            //find the closest depot to the start intersection
            double shortestDistance = DBL_MAX;
            int shortestDepot = interToDepots[temp.begin()->start_intersection].begin()->second;
            
            CourierSubpath first;
            first.start_intersection = shortestDepot;
            first.end_intersection = temp.begin()->start_intersection;
            first.subpath = find_path_between_intersections(first.start_intersection, first.end_intersection, turn_penalty);
            temp.insert(temp.begin(), first);
    
            bool pathIsLegal = courier_path_is_legal_with_capacity(deliveries, depots, temp, truck_capacity);
            //cout<<"last intersection: "<<(temp.end()-1)->end_intersection<<"path legal: "<<pathIsLegal<<endl;
            if (!pathIsLegal) continue;
            else {
                //cout<<"swap: better time found: original "<<originalTime<<" vs new "<<newTime<<endl;
               //the permutated version is better, so keep it
                result = temp;
                return;
            }
        }
    }    
}

//before calling this, must remove the first CourierSubpath from result (corresponding to the subpath from depot to first pickup)
void twoOptRetry(vector<CourierSubpath> & result, const vector <DeliveryInfo> & deliveries, const vector <int> & depots, 
                                                            const float turn_penalty, const float truck_capacity, int cutoutSize){

    //beforeFirst: the segment immediately before the first cut
    //afterFirst: the segment immediately after the first cut
    //beforeSecond: the segment immediately before the second cut
    //afterSecond: the segment immediately after the second cut
    //first: the segment of first cut
    //second: the segment of second cut
    
    int firstPosition = 1;
    for (auto first = result.begin() + 1; first + cutoutSize + 3 != result.end(); first++, firstPosition++){
        //this holds the permutated path
        vector <CourierSubpath> temp;
        
        auto beforeFirst = first - 1;
        auto afterFirst = first + 1;
        auto second = first + cutoutSize + 1;
        auto beforeSecond = second - 1;
        auto afterSecond = second + 1;
        
//        //if second's start intersection is a dropoff point: don't bother
//        if (reservedDropoff.find(second->start_intersection) != reservedDropoff.end()) continue;
//        
//        //reverse segment 2
//        for (auto i = beforeSecond; i != first; i--){
//            CourierSubpath newPath;
//            newPath.start_intersection = i->end_intersection;
//            newPath.end_intersection = i->start_intersection;
//            newPath.subpath = travelPath[newPath.start_intersection][newPath.end_intersection];
//            temp.push_back(newPath);
//        }
        if (reservedDropoff.find(afterFirst->start_intersection) != reservedDropoff.end()) continue;
        for (auto i = afterFirst; i != second; i++){
            CourierSubpath newPath;
            newPath.start_intersection = i->start_intersection;
            newPath.end_intersection = i->end_intersection;
            newPath.subpath = travelPath[newPath.start_intersection][newPath.end_intersection];
            temp.push_back(newPath);
        }
        //connect start of segment 2 to start of segment 1
        CourierSubpath link1;
        link1.start_intersection = second->start_intersection;
        link1.end_intersection = result.begin()->start_intersection;
        link1.subpath = travelPath[link1.start_intersection][link1.end_intersection];
        temp.push_back(link1);
        
        //insert the whole of segment 1 into temp
        for (auto i = result.begin(); i != first; i++){
            CourierSubpath newPath;
            newPath.start_intersection = i->start_intersection;
            newPath.end_intersection = i->end_intersection;
            newPath.subpath = travelPath[newPath.start_intersection][newPath.end_intersection];
            temp.push_back(newPath);
        }
        
        //connect the end of segment 1 to the start of segment 3
        CourierSubpath link2;
        link2.start_intersection = first->start_intersection;
        link2.end_intersection = afterSecond->start_intersection;
        link2.subpath = travelPath[link2.start_intersection][link2.end_intersection];
        temp.push_back(link2);
        
        //copy the rest of segment 3 into temp
        for (auto i = afterSecond; i != result.end(); i++) {    
            CourierSubpath newPath;
            newPath.start_intersection = i->start_intersection;
            newPath.end_intersection = i->end_intersection;
            if (i != result.end()-1)
                newPath.subpath = travelPath[newPath.start_intersection][newPath.end_intersection];
            else 
                newPath.subpath = find_path_between_intersections(newPath.start_intersection, newPath.end_intersection, turn_penalty);
            temp.push_back(newPath);
        }
        
        
        double originalTime = 0, newTime = 0;
        for (int i = 0; i < result.size(); i++){
            originalTime += compute_path_travel_time(result[i].subpath, turn_penalty);
            newTime += compute_path_travel_time(temp[i].subpath, turn_penalty);
        }
        //cout<<"permutation: time: "<<newTime<<" , old time: "<<originalTime<<endl;
        if (newTime < originalTime){
            
            fitNewPath(deliveries, depots, turn_penalty, truck_capacity, temp);
            
            //find the closest depot to the start intersection
            double shortestDistance = DBL_MAX;
            int shortestDepot = interToDepots[temp.begin()->start_intersection].begin()->second;
//            for (auto dep = depots.begin(); dep != depots.end(); dep++){
//                double distance = findDistance(intersectionGraph[temp.begin()->start_intersection].position, intersectionGraph[*dep].position);
//                if (shortestDistance > distance){
//                    shortestDistance = distance;
//                    shortestDepot = *dep;
//                }
//            }
            CourierSubpath startCourierSubpath;
            startCourierSubpath.start_intersection = shortestDepot;
            startCourierSubpath.end_intersection = temp.begin()->start_intersection;
            startCourierSubpath.subpath = find_path_between_intersections(startCourierSubpath.start_intersection, startCourierSubpath.end_intersection, turn_penalty);
            temp.insert(temp.begin(), startCourierSubpath);
    
            bool pathIsLegal = courier_path_is_legal_with_capacity(deliveries, depots, temp, truck_capacity);
            if (!pathIsLegal) continue;
            else {
                //cout<<"retry: better time found: original "<<originalTime<<" vs new "<<newTime<<endl;
               //the permutated version is better, so keep it
                result = temp;
                
                if (firstPosition >= result.size() - 4)
                    return;
                else {
                    result.erase(result.begin());
                    first = result.begin() + firstPosition;
                }
            }
        }
    }
    
    //if the first starting intersection is not a depot, then add in the closest depot
    if (find(depots.begin(), depots.end(), result.begin()->start_intersection )==depots.end()){
          //find the closest depot to the start intersection
            double shortestDistance = DBL_MAX;
            int shortestDepot = interToDepots[result.begin()->start_intersection].begin()->second;
//            for (auto dep = depots.begin(); dep != depots.end(); dep++){
//                double distance = findDistance(intersectionGraph[result.begin()->start_intersection].position, intersectionGraph[*dep].position);
//                if (shortestDistance > distance){
//                    shortestDistance = distance;
//                    shortestDepot = *dep;
//                }
//            }
            CourierSubpath startCourierSubpath;
            startCourierSubpath.start_intersection = shortestDepot;
            startCourierSubpath.end_intersection = result.begin()->start_intersection;
            startCourierSubpath.subpath = find_path_between_intersections(startCourierSubpath.start_intersection, startCourierSubpath.end_intersection, turn_penalty);
            result.insert(result.begin(), startCourierSubpath);
    }
}

void fitNewPath(const vector <DeliveryInfo> & deliveries, const vector <int> & depots, 
                    const float turn_penalty, const float truck_capacity, vector <CourierSubpath> & result){
    map <int, vector<int>> pickup = reservedPickup, dropoff = reservedDropoff;
    

    vector<int> truckContents;
    double remainingRoom = truck_capacity;
    for (int i = 0; i < result.size()-1; i++){
        //clear this subpath's pickup indice
        result[i].pickUp_indices.clear();
        
        //if the start_intersection is not found in pickup or dropoff intersections: we can delete it
//        if (pickup.find(result[i].start_intersection) == pickup.end() && dropoff.find(result[i].start_intersection) == dropoff.end()){
//            //change the last path's end_intersection and path
//            result[i-1].end_intersection = result[i].end_intersection;
//            result[i-1].subpath = travelPath[result[i-1].start_intersection][result[i-1].end_intersection];
//            result.erase(result.begin() + i);
//            i--;
//            
//            continue;
//        }
        
        //this intersection is a pickup or dropoff intersection
       
        pickupDropoff(result[i], deliveries, ref(pickup), ref(dropoff), ref(remainingRoom), ref(truckContents), result[i].start_intersection);

//        cout<<"the pickups at intersection are: "<<result[i].start_intersection<<" :";
//        for (auto ind = result[i].pickUp_indices.begin(); ind != result[i].pickUp_indices.end(); ind++){
//            cout<<" <"<<*ind<<">";
//        }
        cout<<endl;
       // if it didn't perform a pickup or dropoff then the node is likely redundant. Delete
//        if (somethingChanged == false){
//            result[i-1].end_intersection = result[i].end_intersection;
//            result[i-1].subpath = travelPath[result[i-1].start_intersection][result[i-1].end_intersection];
//            result.erase(result.begin() + i);
//            i--;
//            continue;
//        }

    }
}

void twoOptShift(vector<CourierSubpath> & result, const vector <DeliveryInfo> & deliveries, const vector <int> & depots, 
                                                   const float turn_penalty, const float truck_capacity, int firstSegSize){

    if (firstSegSize + 3 > result.size()) return;
    //modResult contains result, with last element (path to returning depot) removed
    vector<CourierSubpath> modResult = result;
    modResult.pop_back();
    int firstPosition = 1;
    //div points to the last CourierSubpath in the second segment (to be shifted)
    for (auto div = modResult.begin() + firstSegSize; div < modResult.end()-2; div++, firstPosition++){
        
        //div's end-intersection must be a dropoff-only intersection
        if (reservedPickup.find(div->end_intersection) != reservedPickup.end()) continue;
        
        
        //put the first segment as is into temp
        vector<CourierSubpath> temp;
        copy(modResult.begin(), modResult.begin() + firstSegSize, back_inserter(temp));
        
        //connection1 connects first and third segment
        CourierSubpath connection1;
        connection1.start_intersection = (modResult.begin() + firstSegSize)->start_intersection;
        connection1.end_intersection = (div+1)->start_intersection;
        connection1.subpath = travelPath[connection1.start_intersection][connection1.end_intersection];
        connection1.pickUp_indices = (modResult.begin() + firstSegSize)->pickUp_indices;
        temp.push_back(connection1);
        
        //insert the remainder of third segment into temp
        copy (div+1, modResult.end(), back_inserter(temp));
        
        //connection2 connects the end of third segment to start of second segment
        CourierSubpath connection2;
        connection2.start_intersection = (modResult.end()-1)->end_intersection;
        connection2.end_intersection = (modResult.begin() + firstSegSize)->end_intersection;
        connection2.subpath = travelPath[connection2.start_intersection][connection2.end_intersection];
        temp.push_back(connection2);
        
        //add the remainder of segment 2 into temp
        copy(modResult.begin() + firstSegSize + 1, div, back_inserter(temp));
        
        //find the closest depot to the last intersection
        double shortestDistance = DBL_MAX;
        int shortestDepot = interToDepots[div->start_intersection].begin()->second;
//        for (auto dep = depots.begin(); dep != depots.end(); dep++){
//            double distance = findDistance(intersectionGraph[div->end_intersection].position, intersectionGraph[*dep].position);
//            if (shortestDistance > distance){
//                shortestDistance = distance;
//                shortestDepot = *dep;
//            }
//        }

        CourierSubpath last;
        last.start_intersection = div->end_intersection;
        last.end_intersection = shortestDepot;
        last.subpath = find_path_between_intersections(last.start_intersection, last.end_intersection, turn_penalty);
        temp.push_back(last);
        double originalTime = 0, newTime = 0;
        for (int i = 0; i < result.size(); i++){
            originalTime += compute_path_travel_time(result[i].subpath, turn_penalty);
            newTime += compute_path_travel_time(temp[i].subpath, turn_penalty);
        }
        //cout<<"checking time: original time is "<<originalTime<<", perturbated time is: "<<newTime<<endl;
        if (newTime < originalTime && courier_path_is_legal_with_capacity(deliveries, depots, temp, truck_capacity)){
           fitNewPath(deliveries, depots, turn_penalty, truck_capacity, temp);
           //cout<<"permutation: better time found: "<<newTime<<endl;
           //the permutated version is better, so keep it
            result = temp;
        }
        

        
//        if (newTime < originalTime){
//            
//            fitNewPath(deliveries, depots, turn_penalty, truck_capacity, temp);
//            
//            //find the closest depot to the start intersection
//            double shortestDistance = DBL_MAX;
//            int shortestDepot = *depots.begin();
//            for (auto dep = depots.begin(); dep != depots.end(); dep++){
//                double distance = findDistance(intersectionGraph[div->end_intersection].position, intersectionGraph[*dep].position);
//                if (shortestDistance > distance){
//                    shortestDistance = distance;
//                    shortestDepot = *dep;
//                }
//            }
//            CourierSubpath last;
//            last.start_intersection = shortestDepot;
//            last.end_intersection = temp.begin()->start_intersection;
//            last.subpath = find_path_between_intersections(last.start_intersection, last.end_intersection, turn_penalty);
//            temp.push_back(last);
//    
//            bool pathIsLegal = courier_path_is_legal_with_capacity(deliveries, depots, temp, truck_capacity);
//            if (!pathIsLegal) continue;
//            else {
//                cout<<"shift swap: better time found: original "<<originalTime<<" vs new "<<newTime<<endl;
//               //the permutated version is better, so keep it
//                result = temp;
//                
//                if (firstPosition >= result.size() - 4)
//                    return;
////                else {
////                    result.erase(result.begin());
////                    first = result.begin() + firstPosition;
////                }
//            }
//        }
    }
}

void localPerturbation(vector<CourierSubpath> & perturbationResult, const vector <DeliveryInfo> & deliveries, const vector <int> & depots, 
                                                            const float turn_penalty, const float truck_capacity){
       int i=1;

       int legal=0, notLegal=0;
       
    for(auto it=perturbationResult.begin()+1;it+1!=perturbationResult.end() && i+6<perturbationResult.size();it++, i++){


        //pointer to the second subpath
        auto second=it+1;

        //cout<<"3 consecutive pickups"<<endl;
        //cout<<"i="<<i<<endl;
        auto third=second+1;
        //find original travel time
        double originalTime = travelTime[it->start_intersection][it->end_intersection]+travelTime[second->start_intersection][second->end_intersection]+travelTime[third->start_intersection][third->end_intersection];
//        //record 3 new paths, compute their time  
                double newtime1=travelTime[it->start_intersection][second->end_intersection];
                double newtime2=travelTime[second->end_intersection][it->end_intersection];
                double newtime3=travelTime[it->end_intersection][third->end_intersection];
        //compute new time, see if it's optimized
                double newTime=newtime1+newtime2+newtime3;
        //double newTime=compute_path_travel_time(newpath1,turn_penalty) + compute_path_travel_time(newpath2,turn_penalty)+ compute_path_travel_time(newpath3,turn_penalty);

        //if optimized, do the swap
        if(newTime<originalTime){
//            //cout<<"newTime<originalTime"<<endl;
//
//                        //find path
//            vector<int> newpath1=travelPath[it->start_intersection][second->end_intersection];
//            vector<int> newpath2=travelPath[second->end_intersection][it->end_intersection];
//            vector<int> newpath3=travelPath[it->end_intersection][third->end_intersection];  
//            cout<<"newTime<originalTime"<<endl;

            //find path
            vector<int> newpath1=travelPath[it->start_intersection][second->end_intersection];
            vector<int> newpath2=travelPath[second->end_intersection][it->end_intersection];
            vector<int> newpath3=travelPath[it->end_intersection][third->end_intersection];



            
            vector<CourierSubpath> tempPerturbationResult=perturbationResult;
            //intersections
            int temp1end=perturbationResult[i].end_intersection;
            perturbationResult[i].end_intersection=perturbationResult[i+1].end_intersection;
            perturbationResult[i+1].start_intersection=perturbationResult[i].end_intersection;
            perturbationResult[i+1].end_intersection=temp1end;
            perturbationResult[i+2].start_intersection=temp1end;
            //subpaths
            perturbationResult[i].subpath=newpath1;
            perturbationResult[i+1].subpath=newpath2;
            perturbationResult[i+2].subpath=newpath3;
            //indicesperturbationResult
            std::vector<unsigned> pickUp_indices2=perturbationResult[i+1].pickUp_indices;
            perturbationResult[i+1].pickUp_indices=perturbationResult[i+2].pickUp_indices;
            perturbationResult[i+2].pickUp_indices=pickUp_indices2;

            //if not legal:
            if(courier_path_is_legal_with_capacity(deliveries, depots, perturbationResult, truck_capacity)!=true){

                //give up this attempt
                perturbationResult=tempPerturbationResult;
                notLegal+=1;
            }else{
//                cout<<"legal AS"<<endl;
                legal+=1;
            }
//                    bool isLegal = courier_path_is_legal_withcapacity(deliveries, depots, perturbationResult, truck_capacity);
//                    cout<<"legal: "<<isLegal<<endl;

        }              
    }
    
       if(legal>1){
            //cout<<"local perturbation made improvement"<<endl;
            //cout<<"number of subpaths for this test is: "<<i<<endl;
            //cout<<"legal:"<<legal<<";   not legal:"<<notLegal<<endl;
       }
}



void calcTravelTime(vector<int> startingIntersec, const vector <DeliveryInfo> & deliveries, double turn_penalty, vector<int> depots){
    for (auto i = startingIntersec.begin(); i != startingIntersec.end(); i++){
        unordered_map<int, vector<int>> paths;
        travelTime.insert(make_pair(*i, find_distance_between_points(*i, deliveries, depots, turn_penalty, paths)));
        travelPath.insert(make_pair(*i, paths));
    }
}

void getCourierPath(int startingDepot, const vector <DeliveryInfo> & deliveries, const vector <int> & depots, 
                    const float turn_penalty, const float truck_capacity, map <int, vector<int>> pickup, map <int, vector<int>> dropoff,
                     vector <CourierSubpath> & result, int randomFactor){
    double remainingRoom;
    //this vector holds the delivery indices of all the pickups made (and not delivered)
    vector <int> truckContents;
    //first segment: go to the first pickup location
    CourierSubpath first;
    first.start_intersection = startingDepot;
    
    double minDist = DBL_MAX;
    for (auto i = deliveries.begin(); i != deliveries.end(); i++){
        if (dropoff.find(i->pickUp) != dropoff.end()) continue;
        double distance = findDistance(intersectionGraph[startingDepot].position, intersectionGraph[i->pickUp].position);
        if (distance < minDist){
            first.end_intersection = i->pickUp;
            minDist = distance;
        }
    }

    //possibility of random start (but must start at a pickup-only intersection)
    if (rand()% (randomFactor*2) == 0) {
        int randomPickup;
        do {
            randomPickup = deliveries[rand() % deliveries.size()].pickUp;
        } while (dropoff.find(randomPickup) != dropoff.end());
        first.end_intersection = randomPickup;
        
        //find the closest depot
        minDist = DBL_MAX;
        first.start_intersection = (interToDepots[randomPickup].begin())->second;
    }
    
    first.subpath = find_path_between_intersections(first.start_intersection, first.end_intersection, turn_penalty);
    result.push_back(first);
    
    
    remainingRoom = truck_capacity;
    int currentIntersection = first.end_intersection;
    

    //continue while more dropoffs need to be made (we're not done with the deliveries yet)
    while (dropoff.size() != 0){
        CourierSubpath nextPath;
        nextPath.start_intersection = currentIntersection;
       
        //see what to pick up and what to drop off
        pickupDropoff(nextPath, deliveries, pickup, dropoff, remainingRoom, truckContents, currentIntersection);
        if (dropoff.size() == 0) break;
        

        int shortestIntersection;
        
        //find the closest points by euclidian distance (from the list of pickup and dropoff)
        map<double, int> closest;
        for (auto i = pickup.begin(); i != pickup.end(); i++){
            
            bool tooHeavy = true;
            for (auto j = (i->second).begin(); j != (i->second).end(); j++){
                if (deliveries[*j].itemWeight < remainingRoom) tooHeavy = false;
            }
            if (tooHeavy) continue;
            double distance = travelTime[currentIntersection][i->first];
            closest.insert(make_pair(distance, i->first));
        }
        
        for (auto i = dropoff.begin(); i != dropoff.end(); i++){
            for (auto j = (i->second).begin(); j != (i->second).end(); j++){
                if (find(truckContents.begin(), truckContents.end(), *j) != truckContents.end()){
                    //this intersection is a dropoff intersection for something on the car
                    double distance = travelTime[currentIntersection][i->first];
                    closest.insert(make_pair(distance, i->first));
                    break;
                }
            }
        }
        

        auto iter = closest.begin();
        shortestIntersection = iter->second;
        int random = rand() % randomFactor;
        iter++;
        
        if (randomFactor != INT_MAX && random ==0 && iter != closest.end()){
            shortestIntersection = iter->second;
            iter++;
        }
       
        
        //get path to the "shortest intersection"
        nextPath.end_intersection = shortestIntersection;
        nextPath.subpath = travelPath[currentIntersection][shortestIntersection];
        result.push_back(nextPath);
        
        currentIntersection = shortestIntersection;
    }
    
    
//    double shortestDistance = DBL_MAX;
//    int shortestDepot = depots[0];
//    //all the dropoffs have been made: time to search for the closest depot to return to
//    for (auto dep = depots.begin(); dep != depots.end(); dep++){
//        double distance = findDistance(intersectionGraph[currentIntersection].position, intersectionGraph[*dep].position);
//        if (shortestDistance > distance){
//            shortestDistance = distance;
//            shortestDepot = *dep;
//        }
//    }
    
    CourierSubpath last;
    last.start_intersection = currentIntersection;
    last.end_intersection =  (interToDepots[currentIntersection].begin())->second;;
    last.subpath = travelPath[last.start_intersection][last.end_intersection];
    result.push_back(last);
    
    double resultTime = calculatePathTime(result, turn_penalty);
    
    if (resultTime > globalBestTime*1.02) return;
    twoOpt(result, deliveries, depots, turn_penalty, truck_capacity, 1);
    doTwoEdgeOperationsWholePath(result,deliveries, depots, turn_penalty, truck_capacity);
    return;
    //try two opt swap
    
        vector <CourierSubpath> experimental = result;
        experimental.erase(experimental.begin());
        twoOptRetry(experimental, deliveries, depots, turn_penalty, truck_capacity, 1);
//        experimental.erase(experimental.begin());
//        twoOptRetry(experimental, deliveries, depots, turn_penalty, truck_capacity, 4);
//        experimental.erase(experimental.begin());
//        twoOptRetry(experimental, deliveries, depots, turn_penalty, truck_capacity, 6);
//        twoOpt(experimental, deliveries, depots, turn_penalty, truck_capacity, 1);
//        twoOpt(experimental, deliveries, depots, turn_penalty, truck_capacity, 2);
//        twoOpt(experimental, deliveries, depots, turn_penalty, truck_capacity, 3);
    
    
    double experimentalTime = calculatePathTime(experimental, turn_penalty);
    double originalTime = calculatePathTime(result, turn_penalty);
    if (experimentalTime < originalTime) result = experimental;
    
//    vector<CourierSubpath> perturbationResult=result;
//    localPerturbation(perturbationResult, deliveries, depots, turn_penalty, truck_capacity);
//        for(int r=0;r<1;r++){
//        twoEdgeOperations(2,2,2,perturbationResult, deliveries, depots, turn_penalty, truck_capacity);
//        twoEdgeOperations(3,2,2,perturbationResult, deliveries, depots, turn_penalty, truck_capacity);
//        twoEdgeOperations(2,3,2,perturbationResult, deliveries, depots, turn_penalty, truck_capacity);
//        twoEdgeOperations(2,2,3,perturbationResult, deliveries, depots, turn_penalty, truck_capacity);
//        localPerturbation(perturbationResult, deliveries, depots, turn_penalty, truck_capacity);
//    }
    //localPerturbation(result, deliveries, depots, turn_penalty, truck_capacity);
//   twoOpt(result, deliveries, depots, turn_penalty, truck_capacity, 1);
//   twoOpt(result, deliveries, depots, turn_penalty, truck_capacity, 2);
//   twoOpt(result, deliveries, depots, turn_penalty, truck_capacity, 3);
//   twoOpt(result, deliveries, depots, turn_penalty, truck_capacity, 4);
   // result=perturbationResult;
}


bool pickupDropoff(CourierSubpath & nextPath, const vector <DeliveryInfo> & deliveries, map <int, vector<int>> & pickup, 
        map <int, vector<int>> & dropoff, double & remainingRoom, vector<int>&truckContents, int currentIntersection){
    
    bool somethingChanged = false;
     //if currentIntersection is a dropoff intersection:
        if (dropoff.find(currentIntersection) != dropoff.end()){
            //drop off all cargo that can be dropped off
            for (auto i = dropoff[currentIntersection].begin(); i != dropoff[currentIntersection].end(); i++){
                auto iter = find(truckContents.begin(), truckContents.end(), *i);
                if (iter != truckContents.end()){
                    somethingChanged = true;
                    remainingRoom += deliveries[*i].itemWeight;
                    truckContents.erase(iter);
                    i = dropoff[currentIntersection].erase(i);
                    i--;
                }
            }
            if (dropoff[currentIntersection].size() == 0)
                dropoff.erase(dropoff.find(currentIntersection));
        }
        
        
        //if currentIntersection is (also) a pickup intersection:
        if (pickup.find(currentIntersection) != pickup.end()){
            //pick up as much as possible
            for (auto i = pickup[currentIntersection].begin(); i != pickup[currentIntersection].end(); i++){
                
                float nextWeight = deliveries[*i].itemWeight;
                if (nextWeight <= remainingRoom){
                    somethingChanged = true;
                    //can put it in to the truck
                    remainingRoom -= nextWeight;
                    truckContents.push_back(*i);
                    nextPath.pickUp_indices.push_back(*i);
                    i = pickup[currentIntersection].erase(i);
                    i--;
                }
            }
            if (pickup[currentIntersection].size() == 0){
                //no more pickups required at that intersection
                pickup.erase(pickup.find(currentIntersection));
            }
        }
    return somethingChanged;
}

double calculatePathTime(vector<CourierSubpath> path, double turn_penalty){
    double result = 0;
    for (auto i = path.begin(); i != path.end(); i++){
        
        result += compute_path_travel_time(i->subpath, turn_penalty);
    }
    
    return result;
}
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

struct intersecData{
    double distance;
    double heuristics;
};

struct compareNode{
    bool operator()(const node & lhs, const node & rhs){
        
        return (lhs.heuristics > rhs.heuristics);
    }
};
vector<int> trackback(int endPoppedID,  vector <popped> poppedIntersections){
    
    vector <int> segResult;
    
    int currentPopped = endPoppedID;
    while (currentPopped != -1 && currentPopped != 0){
        segResult.push_back(poppedIntersections[currentPopped].currSeg);
        currentPopped = poppedIntersections[currentPopped].lastPoppedID;
    }
    
    reverse(segResult.begin(), segResult.end());
    return segResult;
}

unordered_map <int, double> find_distance_between_points(
		          const IntersectionIndex intersect_id_start, //input every current pickup or dropoff
                  const vector <DeliveryInfo> & deliveries,
        vector<int> depots,
                  const double turn_penalty, unordered_map<int, vector<int>>& paths){
    //<distance from this inter to depot, depot ID>
    map<double, int> depotCost;

    //set up the number of intersections that we need to visit
    int depotSize=depots.size();
    set <int> needToVisit;
    set <int> needToVisitDepots;
    for (auto i = deliveries.begin(); i != deliveries.end(); i++){
        needToVisit.insert(i->pickUp);
        needToVisit.insert(i->dropOff);
    }
    for(int i=0;i<depots.size();i++){
        needToVisitDepots.insert(depots[i]);
    }
    
    
    int inputsize=paths.size();
    unordered_map<int, double> result;
    priority_queue <node, vector<node>, compareNode> q;
    vector <popped> poppedIntersections;
    
    //we need this in order to perform SMT with find path function (for M4))
    vector <intersecData> intersections;
    intersections.resize(intersectionGraph.size());
    
    for (int i = 0; i < intersectionGraph.size(); i++){
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
        
        //for(auto it = deliveries.begin(); it != deliveries.end(); it++){
            //intersection to deliveries
            if(needToVisit.find(top.ID) != needToVisit.end()){
                vector<int> path = trackback(poppedID, poppedIntersections);
                paths.insert(make_pair(top.ID, path));
                
                double pathCost = compute_path_travel_time(path, turn_penalty);
                result.insert(make_pair(top.ID, pathCost));
                needToVisit.erase(needToVisit.find(top.ID));
            }
            //intersection to depots
            if(needToVisitDepots.find(top.ID) != needToVisitDepots.end()){
                vector<int> path = trackback(poppedID, poppedIntersections);
                paths.insert(make_pair(top.ID, path));
                
                double pathCost = compute_path_travel_time(path, turn_penalty);
                depotCost.insert(make_pair(pathCost, top.ID));
                //also put it in travelTime for convinience!!
                result.insert(make_pair(top.ID, pathCost));
                
                needToVisitDepots.erase(needToVisitDepots.find(top.ID));
            }
      //  }
        //deliveries and depots are empty
        if (needToVisit.size() == 0 && needToVisitDepots.size() == 0) break; 
        
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
                destID = currentSeg.from;
            } else {
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
    
    
    //update depot datastructure
    interToDepots.insert(make_pair(intersect_id_start,depotCost));
    
    int outputsize=result.size();
    //needTovisit!=0 means there are intersections that this one can't reach
    //needToVisitDepots.size()==depotSize means there are no depots found
    if(inputsize>outputsize || needToVisit.size()!=0 || needToVisitDepots.size()==depotSize){
        validPickupDropoff=false;
    }
    return result;
}
int wholePathImprove=0;
void doTwoEdgeOperationsWholePath(vector<CourierSubpath> & perturbationResult, const vector <DeliveryInfo> & deliveries, const vector <int> & depots, 
                                                            const float turn_penalty, const float truck_capacity){
    wholePathImprove=0;
    for(int mid=0;mid<perturbationResult.size()-2;mid++){
        twoEdgeOperationsWholePath(mid,perturbationResult, deliveries, depots, turn_penalty, truck_capacity);
    }
    
    //cout<<endl<<"number of improvements in doTwoEdgeOperationsWholePath: "<<wholePathImprove<<endl<<endl;
}

//two edge whole path version: 1->3->2, without reversing, then try alternating depot
//just fixed: interToDepot
void twoEdgeOperationsWholePath(int length, vector<CourierSubpath> & perturbationResult, const vector <DeliveryInfo> & deliveries, const vector <int> & depots, 
                                                            const float turn_penalty, const float truck_capacity){
    
    int length1, length2, length3, totalLength;
    totalLength=perturbationResult.size();
    length2=length;
    int legal=0, notLegal=0;
    //go through the loop, varying length1, with fixed length2, do the 2 opt
    for(length1=2;length1<totalLength;length1++){
        

        //length2 is by function perameter
        int length3=totalLength-length1-length2;
        //check input: if invalid input, go to next one
        if(length1<2 ||length2<2 ||length3<2){
            continue;
        }        
        int link1n=length1-1;        
        int link2n=length1+length2-1;
        int link3n=totalLength-1;
        
        //the third one is intertodepots!!!!
        double originalLinkTime=travelTime[perturbationResult[length1-1].start_intersection][perturbationResult[length1-1].end_intersection]+ travelTime[perturbationResult[length1+length2-1].start_intersection][perturbationResult[length1+length2-1].end_intersection]
        +travelTime[perturbationResult[link3n].start_intersection][perturbationResult[link3n].end_intersection];
        
        //case I: connect first part to third part's head
        //1.1 third part's tail to second's head
        //compute new time, see if it's optimized
//        double newLinkTime=travelTime[perturbationResult[length1-1].start_intersection][perturbationResult[length1+length2].start_intersection] + travelTime[perturbationResult[link3n].start_intersection][perturbationResult[length1].start_intersection]
//        +travelTime[perturbationResult[length1+length2-1].start_intersection][perturbationResult[link3n].end_intersection];
        //first two links
        double newLinkTime=travelTime[perturbationResult[length1-1].start_intersection][perturbationResult[length1+length2].start_intersection] + travelTime[perturbationResult[link3n].start_intersection][perturbationResult[length1].start_intersection];
        //third link go to closest depot
        newLinkTime=newLinkTime+interToDepots[perturbationResult[link2n].start_intersection].begin()->first;
        //depot number is interToDepots[perturbationResult[link2n].start_intersection].begin()->second
#ifdef ptdebug
        if(travelTime[perturbationResult[link3n].start_intersection][perturbationResult[link3n].end_intersection]<interToDepots[perturbationResult[link3n].start_intersection].begin()->first){
            cout<<"depots.size()"<<depots.size()<<endl;
            for(auto it=interToDepots[perturbationResult[link3n].start_intersection].begin();it!=interToDepots[perturbationResult[link3n].start_intersection].end();it++){
                cout<<"here is interToDepots"<<endl;
                cout<<"first(dis): "<<it->first<<"second(inter): "<<it->second<<endl;
            }
          
            cout<<"not expected, this is wrong"<<endl;
            cout<<"perturbationResult[link3n].start_intersection"<<perturbationResult[link3n].start_intersection<<endl;
            cout<<"perturbationResult[link3n].end_intersection"<<perturbationResult[link3n].end_intersection<<endl;
            cout<<"TravelTime[perturbationResult[link3n].start_intersection][perturbationResult[link3n].end_intersection]"<<travelTime[perturbationResult[link3n].start_intersection][perturbationResult[link3n].end_intersection]<<endl;
            cout<<"interToDepots[perturbationResult[link3n].start_intersection].begin()->first"<<interToDepots[perturbationResult[link3n].start_intersection].begin()->first<<endl;
            break;
        }
#endif
        
        //if optimized, do the swap
        if(newLinkTime<originalLinkTime){
            
//            double checkTime=0;
//            for(auto it=perturbationResult.begin();it!=perturbationResult.end();it++){
//                checkTime+=compute_path_travel_time(it->subpath,turn_penalty);
//            }

            
            vector<CourierSubpath> tempPerturbationResult=perturbationResult;
//            cout<<"here is original"<<endl;
//            for(int q=0; q<10;q++){
//                cout<<perturbationResult[i+q].start_intersection<<" "<<perturbationResult[i+q].end_intersection<<endl;
//            }
            int curDepot=interToDepots[perturbationResult[link2n].start_intersection].begin()->second;
            //find new link path
            vector<int> newpath1=travelPath[perturbationResult[length1-1].start_intersection][perturbationResult[length1+length2].start_intersection];
            vector<int> newpath2=travelPath[perturbationResult[link3n].start_intersection][perturbationResult[length1].start_intersection];
            //not necessary to find third path, only time maters(find path in the end):
            vector<int> newpath3=travelPath[perturbationResult[length1+length2-1].start_intersection][curDepot];  
            
            //get original path in 3 parts
            vector<CourierSubpath> firstPart (perturbationResult.begin(),perturbationResult.begin()+length1-1);//-1: because link should not be counted; also (x,y) means <y
            vector<CourierSubpath> secondPart (perturbationResult.begin()+length1,perturbationResult.begin()+length1+length2-1);
            vector<CourierSubpath> thirdPart (perturbationResult.begin()+length1+length2,perturbationResult.begin()+totalLength-1);
//            cout<<"expecting 3 twos"<<firstPart.size()<<secondPart.size()<<thirdPart.size()<<endl;
            
            //link subpath, copy start intersection, pickup indices, then change end intersections
            CourierSubpath link1=perturbationResult[length1-1];
            CourierSubpath link2=perturbationResult[totalLength-1];
            CourierSubpath link3=perturbationResult[length1+length2-1];
            
            //end intersections
            link1.end_intersection=perturbationResult[length1+length2].start_intersection;
            link2.end_intersection=perturbationResult[length1].start_intersection;
            link3.end_intersection=curDepot; //link3 to new depot

            
            //subpaths
            link1.subpath=newpath1;
            link2.subpath=newpath2;
            link3.subpath=newpath3;
            
            
            //add links to parts, now they have new links
            firstPart.push_back(link1);
            thirdPart.push_back(link2);
            secondPart.push_back(link3);
            
            //adding up parts: total segments that's replacing original
            firstPart.insert(firstPart.end(),thirdPart.begin(),thirdPart.end());
            firstPart.insert(firstPart.end(),secondPart.begin(),secondPart.end());
            //now firstPart contains the whole path(length = totalLength)
            
            
            
//            cout<<"here is after swap:"<<endl;
//            cout<<"size"<<firstPart.size();
//            for(int q=0; q<firstPart.size();q++){
//                cout<<perturbationResult[i+q].start_intersection<<" "<<perturbationResult[i+q].end_intersection<<endl;
//            }
            
            

                        
            //if not legal:
            if(courier_path_is_legal_with_capacity(deliveries, depots, perturbationResult, truck_capacity)!=true){

                //give up this attempt
                perturbationResult=tempPerturbationResult;
                notLegal+=1;
            }else{
                legal+=1;
                wholePathImprove+=1;
                
                //checktime correct
                
//                double timeAfter=0;
//                for(auto it=perturbationResult.begin();it!=perturbationResult.end();it++){
//                    timeAfter+=compute_path_travel_time(it->subpath,turn_penalty);
//                }
//                if(checkTime<timeAfter){
//                    cout<<"wrong time!!!"<<endl;
//                }else{
//                    cout<<"good";
//                }
                
                
                //cout<<"two opt whole path improvement, here are the parameters"<<endl;
                //cout<<"length1:"<<length1<<" length2:"<<length2<<" length3:"<<length3<<endl;
            }


        }              

        
        
        
        
    }
    
    
    
}

//void twoEdgeOperationsWholePath(int length, vector<CourierSubpath> & perturbationResult, const vector <DeliveryInfo> & deliveries, const vector <int> & depots, 
//                                                            const float turn_penalty, const float truck_capacity){
//    
//    int length1, length2, length3, totalLength;
//    totalLength=perturbationResult.size();
//    length2=length;
//    
//    //go through the loop, varying length1, with fixed length2, do the 2 opt
//    for(length1=2;length1<totalLength;length1++){
//        
//        
//        int legal=0, notLegal=0;
//        //length2 is by function perameter
//        int length3=totalLength-length1-length2;
//        //check input: if invalid input, go to next one
//        if(length1<2 ||length2<2 ||length3<2){
//            continue;
//        }        
//        int link1n=length1-1;        
//        int link2n=length1+length2-1;
//        int link3n=totalLength-1;
//        
//        double originalLinkTime=travelTime[perturbationResult[length1-1].start_intersection][perturbationResult[length1-1].end_intersection]+ travelTime[perturbationResult[length1+length2-1].start_intersection][perturbationResult[length1+length2-1].end_intersection]
//        +travelTime[perturbationResult[link3n].start_intersection][perturbationResult[link3n].end_intersection];
//        
//        //case I: connect first part to third part's head
//        //1.1 third part's tail to second's head
//        //compute new time, see if it's optimized
//        double newLinkTime=travelTime[perturbationResult[length1-1].start_intersection][perturbationResult[length1+length2].start_intersection] + travelTime[perturbationResult[link3n].start_intersection][perturbationResult[length1].start_intersection]
//        +interToDepot[perturbationResult[length1+length2-1].start_intersection][perturbationResult[link3n].end_intersection];
//        //first two links
////        double newLinkTime=travelTime[perturbationResult[length1-1].start_intersection][perturbationResult[length1+length2].start_intersection] + travelTime[perturbationResult[link3n].start_intersection][perturbationResult[length1].start_intersection];
////        //third link go to closest depot
////        newLinkTime=newLinkTime+interToDepots[perturbationResult[link3n].start_intersection].begin()->first;
//        
//        //if optimized, do the swap
//        if(newLinkTime<originalLinkTime){
//            // **************************consider manually swap units instead of making copy?????
//            vector<CourierSubpath> tempPerturbationResult=perturbationResult;
////            cout<<"here is original"<<endl;
////            for(int q=0; q<10;q++){
////                cout<<perturbationResult[i+q].start_intersection<<" "<<perturbationResult[i+q].end_intersection<<endl;
////            }
//            
//            //find new link path
//            vector<int> newpath1=travelPath[perturbationResult[length1-1].start_intersection][perturbationResult[length1+length2].start_intersection];
//            vector<int> newpath2=travelPath[perturbationResult[link3n].start_intersection][perturbationResult[length1].start_intersection];
//            vector<int> newpath3=travelPath[perturbationResult[length1+length2-1].start_intersection][perturbationResult[link3n].end_intersection];  
//            
//            //get original path in 3 parts
//            vector<CourierSubpath> firstPart (perturbationResult.begin(),perturbationResult.begin()+length1-1);//-1: because link should not be counted; also (x,y) means <y
//            vector<CourierSubpath> secondPart (perturbationResult.begin()+length1,perturbationResult.begin()+length1+length2-1);
//            vector<CourierSubpath> thirdPart (perturbationResult.begin()+length1+length2,perturbationResult.begin()+totalLength-1);
////            cout<<"expecting 3 twos"<<firstPart.size()<<secondPart.size()<<thirdPart.size()<<endl;
//            //link subpath, copy start intersection, pickup indices, then change end intersections
//            CourierSubpath link1=perturbationResult[length1-1];
//            CourierSubpath link2=perturbationResult[totalLength-1];
//            CourierSubpath link3=perturbationResult[length1+length2-1];
//            //end intersections
//            link1.end_intersection=perturbationResult[length1+length2].start_intersection;
//            link2.end_intersection=perturbationResult[length1].start_intersection;
//            link3.end_intersection=perturbationResult[link3n].end_intersection;
//            //subpaths
//            link1.subpath=newpath1;
//            link2.subpath=newpath2;
//            link3.subpath=newpath3;
//            
//            //add links to parts, now they have new links
//            firstPart.push_back(link1);
//            thirdPart.push_back(link2);
//            secondPart.push_back(link3);
//            
//            //adding up parts: total segments that's replacing original
//            firstPart.insert(firstPart.end(),thirdPart.begin(),thirdPart.end());
//            firstPart.insert(firstPart.end(),secondPart.begin(),secondPart.end());
//            //now firstPart contains the whole path(length = totalLength)
//            
//
//
////            //indices??????????????????????????????Do I have to change this???????
////            std::vector<unsigned> pickUp_indices2=perturbationResult[i+1].pickUp_indices;
////            perturbationResult[i+1].pickUp_indices=perturbationResult[i+2].pickUp_indices;
////            perturbationResult[i+2].pickUp_indices=pickUp_indices2;
//            
//            
////            cout<<"here is after swap:"<<endl;
////            cout<<"size"<<firstPart.size();
////            for(int q=0; q<firstPart.size();q++){
////                cout<<perturbationResult[i+q].start_intersection<<" "<<perturbationResult[i+q].end_intersection<<endl;
////            }
//            
//            
//
//                        
//            //if not legal:
//            if(courier_path_is_legal_with_capacity(deliveries, depots, perturbationResult, truck_capacity)!=true){
//
//                //give up this attempt
//                perturbationResult=tempPerturbationResult;
//                notLegal+=1;
//            }else{
//                legal+=1;
//                wholePathImprove+=1;
//            }
//            //cout<<"twooptwholepath improvement"<<endl;
//            //cout<<"legal: "<<legal<<endl;
//            //cout<<"length1:"<<length1<<" length2:"<<length2<<" length3:"<<length3<<endl;
//
//        }              
//
//        
//        
//        
//        
//    }
//    
//}