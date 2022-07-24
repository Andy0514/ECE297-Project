/* 
 * Copyright 2018 University of Toronto
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
#include <map>
#include <iostream>
#include <sstream>
#include <iterator>
#include <algorithm>
#include <cassert>
#include <map>
#include <unordered_set>
#include "StreetsDatabaseAPI.h"
#include "unit_test_util.h"
#include "globalData.h"
#include "courierVerify.h"

//
//
//
// File-scope function declarations
//
//
//

namespace ece297 {

} //namespace

//
//
//
// Function Implementations
//
//
//

namespace ece297 {


bool courier_path_is_legal_with_capacity(const std::vector<DeliveryInfo>& deliveries,
                           const std::vector<IntersectionIndex>& depots,
                           const std::vector<CourierSubpath>& path, 
                           const float truck_capacity) {

    //
    //Ensuree we have a valid problem specification
    //
    if(!valid_courier_problem(deliveries, depots)) {
        return false;
    }

    if (path.empty()) {
        //If it is a valid path it must contain at-least one segment
        //(since we gaurentee there is at-least one depot and delivery)
        std::cerr << "Invalid courier path: path was empty\n";

        return false; 

    } else {
        assert(!path.empty());

        //
        // Check that start_intersection of the first subpath is a depot
        //
        if (!is_depot(depots, path[0].start_intersection)) {
            // Not a valid start intersection
            //std::cerr << "Invalid courier path: start_intersection of first CourierSubpath is not a depot" << std::endl;
            return false;
        }

        //
        //We should end at a depot
        //
        if (!is_depot(depots, path[path.size() - 1].end_intersection)) {
            //Not at a valid end intersection
            //std::cerr << "Invalid courier path: end_intersection of last CourierSubpath is not a depot" << std::endl;
            return false;
        }

        // Make sure that overall path of segments is not empty 
        std::vector<StreetSegmentIndex> overall_path;
        for (size_t sub_idx = 0; sub_idx < path.size(); sub_idx++)
            overall_path.insert(overall_path.end(), path[sub_idx].subpath.begin(), path[sub_idx].subpath.end());

        if (overall_path.empty()) {
            //std::cerr << "Invalid courier path: Overall courier route is empty!" << std::endl;
            return false;
        }

        //We store whether each delivery has been picked-up or dropped-off
        std::vector<bool> deliveries_picked_up(deliveries.size(), false);
        std::vector<bool> deliveries_dropped_off(deliveries.size(), false);

        

        
        // 
        // Validating CourierSubpath and capacity
        //
        float current_total_weight = 0.0;
        for (size_t sub_idx = 0; sub_idx < path.size(); sub_idx++) {

            CourierSubpath courier_subpath = path[sub_idx];
            IntersectionIndex start_intersection = courier_subpath.start_intersection;
            IntersectionIndex end_intersection = courier_subpath.end_intersection;
            std::vector<unsigned> pickup_indices = courier_subpath.pickUp_indices;

            

            if (courier_subpath.subpath.empty() && courier_subpath.start_intersection != courier_subpath.end_intersection) {
               //std::cerr<< "subpath is empty, but start_intersection != end_intersection"<<std::endl;
                return false;
            }

            // Check if start_intersection = end_intersection of previous CourierSubpath
            if (sub_idx > 0)
            {
                if (start_intersection != path[sub_idx - 1].end_intersection)
                {
                   //std::cerr<<"start_intersection is not equal to end_intersection of previous subpath"<<std::endl;
                    return false;
                }
                // Check that start intersection is at least a pickup intersection or drop-off intersection (already checked if first one is depot above)
                if (intersections_to_pick_up.find(start_intersection) == intersections_to_pick_up.end() &&
                    intersections_to_drop_off.find(start_intersection) == intersections_to_drop_off.end()) {
                    //std::cerr<< "start_intersection is not a pick-up or drop-off intersection!"<<std::endl;
                    return false;
                }
            }

            // check that start_intersection of each subpath is actually at the beginning of the subpath
            if (!is_start_intersection_correct(courier_subpath))
            {
                //std::cerr<< "start_intersection is not the start of its subpath"<<std::endl;
                return false;
            }

            // check that end_intersection of each subpath is actually at the end of the subpath
            if (!is_end_intersection_correct(courier_subpath))
            {
                //std::cerr<<  "end_intersection is not the end of its subpath"<<std::endl;
                return false;
            }

            // Checking capacity
            /*
             * Must ensure that the total weight of all picked up packages at each subpath does not 
             * exceed the truck capacity
             */
            
            // Process Packages
            if (!pick_up_at_intersection_capacity(deliveries, 
                                            intersections_to_pick_up, 
                                            pickup_indices, 
                                            start_intersection, 
                                            deliveries_picked_up, 
                                            current_total_weight))
                return false;

            if (!drop_off_at_intersection_capacity(deliveries, 
                                              intersections_to_drop_off,
                                              deliveries_picked_up, 
                                              start_intersection, 
                                              deliveries_dropped_off, 
                                              current_total_weight))
                return false;

            if (current_total_weight > truck_capacity + FLOAT_EPSILON)
            {
                //std::cerr << "Invalid Courier Path. Total weight of picked up items exceeds truck_capacity!" << std::endl;
                return false;
            }

            //We verify the subpath by walking along each segment and:
            //  * Checking that the next step along the path is valid (see traverse_segment())
            IntersectionIndex curr_intersection = start_intersection;
            for (size_t subpath_idx = 0; subpath_idx < courier_subpath.subpath.size(); ++subpath_idx) {

                IntersectionIndex next_intersection; //Set by traverse_segment

                if(!traverse_segment(courier_subpath.subpath, subpath_idx, curr_intersection, next_intersection)) {
                    return false;
                }

                //Advance
                curr_intersection = next_intersection;
            }
            
            // must end at end_intersection
            assert (curr_intersection == end_intersection);
        }

        //
        //Check everything was delivered
        //
        if(!delivered_all_packages(deliveries, deliveries_picked_up, deliveries_dropped_off)) {
            return false;
        }

        // current_total_weight should be zero
        if (current_total_weight < -FLOAT_EPSILON ||
            current_total_weight > FLOAT_EPSILON) {
            //std::cerr << "Invalid courier route: Truck is not empty at the end depot!" << std::endl;
            return false;
        }
    }

    //Everything validated
    return true;
}


bool valid_courier_problem(const std::vector<DeliveryInfo>& deliveries_vec,
                           const std::vector<IntersectionIndex>& depots_vec) {
    if(deliveries_vec.empty()) {
        //std::cerr << "Invalid courier problem: Must have at-least one delivery" << "\n";
        return false;
    }

    if(depots_vec.empty()) {
        //std::cerr << "Invalid courier problem: Must have at-least one depot" << "\n";
        return false;
    }

    // Check for duplicates among:
    //   * Pick-Ups and depots
    //   * Drop-Offs and depots

    std::vector<IntersectionIndex> depots = depots_vec; //Copy since we need to sort the depots

    std::vector<IntersectionIndex> delivery_pick_ups;
    std::vector<IntersectionIndex> delivery_drop_offs;

    for(const DeliveryInfo& info : deliveries_vec) {
        delivery_pick_ups.push_back(info.pickUp);
        delivery_drop_offs.push_back(info.dropOff);
    }

    //Sort all the ids so we can quickly find the set intersections
    std::sort(delivery_pick_ups.begin(), delivery_pick_ups.end());
    std::sort(delivery_drop_offs.begin(), delivery_drop_offs.end());
    std::sort(depots.begin(), depots.end());

    //Verify that there are is not commonality between:
    //  * pick_ups and depots
    //  * drop_offs and depots
    //
    // Note: we allow duplicates between pick_ups and drop_offs
    std::vector<IntersectionIndex> common_pick_up_depots;
    std::vector<IntersectionIndex> common_drop_off_depots;

    //Common between pick-ups and depots
    std::set_intersection(delivery_pick_ups.begin(), delivery_pick_ups.end(),
                          depots.begin(), depots.end(),
                          std::back_inserter(common_pick_up_depots));
    
    //Common between drop-offs and depots
    std::set_intersection(delivery_drop_offs.begin(), delivery_drop_offs.end(),
                          depots.begin(), depots.end(),
                          std::back_inserter(common_drop_off_depots));

    if(!common_pick_up_depots.empty()) {
     //
        //std::cerr << "Invalid Courier Problem: common pick-up and depot itersections " << common_pick_up_depots << "\n";
        return false;
    }

    if(!common_drop_off_depots.empty()) {
        //std::cerr << "Invalid Courier Problem: common drop-off and depot itersections " << common_drop_off_depots << "\n";
        return false;
    }

    //
    //Sanity check on id bounds
    //
    //Since vectors are sorted, can just query first and last elements
    assert(*(--depots.end()) <= (getNumIntersections()));
    assert(*(--delivery_pick_ups.end()) <= (getNumIntersections()));
    assert(*(--delivery_drop_offs.end()) <= (getNumIntersections()));

    return true;
}

bool determine_start_intersection(const std::vector<StreetSegmentIndex>& path,
                                  const std::vector<IntersectionIndex>& depots,
                                  IntersectionIndex& start_intersection) {
    assert(path.size() > 0);

    InfoStreetSegment first_seg = getInfoStreetSegment(path[0]);

    //Look for the from and to in the depot list
    auto to_iter = std::find(depots.begin(), depots.end(), first_seg.to);
    auto from_iter = std::find(depots.begin(), depots.end(), first_seg.from);

    //Initilize the current intersection
    if(to_iter != depots.end() && from_iter == depots.end()) {

        //To is exclusively a depot
        start_intersection = *to_iter;

    } else if(to_iter == depots.end() && from_iter != depots.end()) {

        //From is exclusivley a depot
        start_intersection = *from_iter;

    } else if(to_iter != depots.end() && from_iter != depots.end()) {
        //Both to and from are depots.
        //
        //This is the unlikely case where two depots are adjacent, and the student
        //returns the single segment joining them as the first element of the path.
        if(path.size() == 1) {
            //If we have a path consisting of only a single segment
            //we only need to take care when the segment is a one-way (i.e.
            //we should pick the start intersection to not violate the one-way)
            //
            //If it is not a one-way, we can pick an arbitrary start intersection.
            //
            //As a result we always pick the 'from' intersection as the start, as this
            //is gaurnteed to be correct for a one-way segment
            start_intersection = *from_iter;

        } else {
            //Multiple segments
            //
            //We can determine the start intersection by identify the common 
            //intersection between the two segments.
            assert(path.size() > 1);

            InfoStreetSegment second_seg = getInfoStreetSegment(path[1]);

            if(first_seg.to == second_seg.to || first_seg.to == second_seg.from) {
                //first_seg to connects to the second seg via it's 'to' intersection
                start_intersection = first_seg.from;
            } else {
                assert(first_seg.from == second_seg.to || first_seg.from == second_seg.from);

                //first_seg to connects to the second seg via it's 'from' intersection
                start_intersection = first_seg.to;
            }
        }

    } else {
        assert(to_iter == depots.end() && from_iter == depots.end());

        //Neither 'to' nor 'from' are depots
        //std::cerr << "Invalid courier path: first path segment not connected to a depot\n";
        return false;
    }
#ifdef DEBUG_PICK_UP_DROP_OFF
    std::cout << "Starting at depot " <<  start_intersection << std::endl;
#endif
    return true;
}

bool is_depot(const std::vector<IntersectionIndex>& depots,
              const IntersectionIndex intersection_id) {

    auto iter = std::find(depots.begin(), depots.end(), intersection_id);

    return (iter != depots.end());
}

bool traverse_segment(const std::vector<StreetSegmentIndex>& path,
                      const unsigned path_idx,
                      const IntersectionIndex curr_intersection,
                      IntersectionIndex& next_intersection) {
    assert(path_idx < path.size());

    InfoStreetSegment seg_info = getInfoStreetSegment(path[path_idx]);

    //
    //Are we moving forward or back along the segment?
    //
    bool seg_traverse_forward;
    if (seg_info.from == static_cast<int>(curr_intersection)) {
        //We take care to check 'from' first. This ensures
        //we get a reasonable traversal direction even in the 
        //case of a self-looping one-way segment

        //Moving forwards
        seg_traverse_forward = true;

    } else if (seg_info.to == static_cast<int>(curr_intersection)) {
        //Moving backwards
        seg_traverse_forward = false;

    } else {
        assert(seg_info.from != static_cast<int>(curr_intersection) && seg_info.to != static_cast<int>(curr_intersection));
#ifdef pterror
        //This segment isn't connected to the current intersection
        std::cerr << "Invalid courier path: path disconnected"
                  << " (intersection " << curr_intersection 
                  << " does not connect to street segment " << path[path_idx] 
                  << " at path index " << path_idx << ")\n";
#endif
        return false;
    }

    //
    //Are we going the wrong way along the segment?
    //
    if (!seg_traverse_forward && seg_info.oneWay) {
        #ifdef pterror
        std::cerr << "Invalid courier path: Attempting to traverse against one-way"
                  << " (from intersection " << curr_intersection 
                  << " along street segment " << path[path_idx] 
                  << " at path index " << path_idx << ")\n";
#endif
        return false;
    }

    //
    //Advance to the next intersection
    //
    next_intersection = (seg_traverse_forward) ? seg_info.to : seg_info.from;

    return true;
}

void pick_up_at_intersection(const std::vector<DeliveryInfo>& /*deliveries*/,
                             const std::multimap<IntersectionIndex,size_t>& intersections_to_pick_up,
                             const IntersectionIndex curr_intersection,
                             std::vector<bool>& deliveries_picked_up) {
    //
    //Check if we are picking up packages
    //

    //Find all the deliveries picking-up from this intersection
    auto range_pair = intersections_to_pick_up.equal_range(curr_intersection);

    //Mark each delivery as picked-up
    for(auto key_value_iter = range_pair.first; key_value_iter != range_pair.second; ++key_value_iter) {
        size_t delivery_idx = key_value_iter->second; 

        deliveries_picked_up[delivery_idx] = true;

#ifdef DEBUG_PICK_UP_DROP_OFF
        std::cerr << "Info: Picked-up package for delivery " << delivery_idx
                  << " at intersection " << curr_intersection << "\n";
#endif

    }
}

bool pick_up_at_intersection_capacity(const std::vector<DeliveryInfo>& deliveries,
                             const std::multimap<IntersectionIndex,size_t>& intersections_to_pick_up,
                             const std::vector<unsigned> & pickup_indices,
                             const IntersectionIndex curr_intersection,
                             std::vector<bool>& deliveries_picked_up, 
                             float & current_total_weight) {

    // check if start_intersection is a pickup order
    if (pickup_indices.size() > 0) // this is a pickup intersection
    {
        if(intersections_to_pick_up.find(curr_intersection) ==
            intersections_to_pick_up.end()) {
            std::cerr << "Invalid pick-up intersection!" << std::endl;
            return false;
        }

        for (unsigned idx = 0; idx < pickup_indices.size(); idx++)
        {
            if (deliveries_picked_up[pickup_indices[idx]])
                continue;

            deliveries_picked_up[pickup_indices[idx]] = true;

            current_total_weight += deliveries[pickup_indices[idx]].itemWeight;
        }
    }
    return true;
}

void drop_off_at_intersection(const std::vector<DeliveryInfo>& /*deliveries*/,
                              const std::multimap<IntersectionIndex,size_t>& intersections_to_drop_off,
                              const std::vector<bool>& deliveries_picked_up,
                              const IntersectionIndex curr_intersection,
                              std::vector<bool>& deliveries_dropped_off) {
    //
    //Check if we are dropping-off packages
    //

    //Find all the deliveries dropping-off to this intersection
    auto range_pair = intersections_to_drop_off.equal_range(curr_intersection);

    //Mark each delivery dropped-off
    for(auto key_value_iter = range_pair.first; key_value_iter != range_pair.second; ++key_value_iter) {
        size_t delivery_idx = key_value_iter->second; 

        if(deliveries_picked_up[delivery_idx]) {
            //Can only drop-off if the delivery was already picked-up
            deliveries_dropped_off[delivery_idx] = true;

#ifdef DEBUG_PICK_UP_DROP_OFF
            std::cerr << "Info: Dropped-off package for delivery " << delivery_idx
                      << " at intersection " << curr_intersection << "\n";
        } else {
            std::cerr << "Info: Did not drop-off package for delivery " << delivery_idx
                      << " at intersection " << curr_intersection 
                      << " since it has not been picked-up" << "\n";
#endif

        }
    }
}

bool drop_off_at_intersection_capacity(const std::vector<DeliveryInfo>& deliveries,
                              const std::multimap<IntersectionIndex,size_t>& intersections_to_drop_off,
                              const std::vector<bool>& deliveries_picked_up,
                              const IntersectionIndex curr_intersection,
                              std::vector<bool>& deliveries_dropped_off,
                              float & current_total_weight) {

	auto drop_off_it = intersections_to_drop_off.equal_range(curr_intersection);

    for (auto itr = drop_off_it.first; itr != drop_off_it.second; ++itr)
    {
        unsigned delivery_index = itr->second;
        if (deliveries_picked_up[delivery_index]) // cannot drop off before picking up
        {
            if (deliveries_dropped_off[delivery_index]) // Already dropped it off
                continue;

            deliveries_dropped_off[delivery_index] = true;
            current_total_weight = current_total_weight - deliveries[delivery_index].itemWeight;
        } 
    }

    if (current_total_weight < -FLOAT_EPSILON) {
        #ifdef pterror
        std::cerr << "Invalid drop-off. Total weight in truck is less than 0" << std::endl;
#endif
        return false;
    }
    return true;
}

bool delivered_all_packages(const std::vector<DeliveryInfo>& deliveries,
                            const std::vector<bool>& deliveries_picked_up,
                            const std::vector<bool>& deliveries_dropped_off) {

    //
    //Check how many undelivered packages there are
    //
    size_t undelivered_packages = 0;
    for(size_t delivery_idx = 0; delivery_idx < deliveries.size(); ++delivery_idx) {
        if(!deliveries_dropped_off[delivery_idx]) {
            ++undelivered_packages;
        } else {
            //If it was dropped-off it must have been picked-up
            assert(deliveries_picked_up[delivery_idx]);
        }
    }

    //
    //Report to the user the missing packages
    //
    if(undelivered_packages > 0) {
        #ifdef pterror
        std::cerr << "Invalid courier path: " << undelivered_packages << " packages were never delivered:\n";
#endif
        for(size_t delivery_idx = 0; delivery_idx < deliveries.size(); ++delivery_idx) {
            //Un-picked up
            if(!deliveries_picked_up[delivery_idx]) {
                #ifdef pterror
                std::cerr << "\tDelivery " << delivery_idx
                          << " was never picked-up at intersection " << deliveries[delivery_idx].pickUp
                          << "\n";
#endif
            }

            //Un-delivered up
            if(!deliveries_dropped_off[delivery_idx]) {
                #ifdef pterror
                std::cerr << "\tDelivery " << delivery_idx
                          << " was never dropped-off at intersection " << deliveries[delivery_idx].dropOff
                          << "\n";
#endif
            }
        }

        //Some un-delivered
        return false;
    }

    //All delivered
    return true;
}


bool is_start_intersection_correct(const CourierSubpath& cs) {
    if (cs.subpath.empty()) {
        return cs.start_intersection == cs.end_intersection;
    }

    InfoStreetSegment seg_info = getInfoStreetSegment(cs.subpath.at(0));
	if (seg_info.from != static_cast<int>(cs.start_intersection) &&
		seg_info.to != static_cast<int>(cs.start_intersection))
		return false;
	
	return true;
}

bool is_end_intersection_correct(const CourierSubpath& cs) {
    if (cs.subpath.empty()) {
        return cs.start_intersection == cs.end_intersection;
    }

	InfoStreetSegment seg_info = getInfoStreetSegment(cs.subpath.at(cs.subpath.size() - 1));
	if (seg_info.from != static_cast<int>(cs.end_intersection) &&
		seg_info.to != static_cast<int>(cs.end_intersection))
		return false;

	return true;
}

double compute_courier_path_travel_time(const std::vector<CourierSubpath>& courier_route, 
                                        const float turn_penalty) {
    // extract the whole route 
    std::vector<StreetSegmentIndex> path; 
    for (size_t i = 0; i < courier_route.size(); i++)
        path.insert(path.end(), courier_route[i].subpath.begin(), courier_route[i].subpath.end());

    // Call compute path function 
    return compute_path_travel_time(path, turn_penalty);
}

} //namespace
