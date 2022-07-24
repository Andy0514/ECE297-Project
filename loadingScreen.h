/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   loadingScreen.h
 * Author: huan1386
 *
 * Created on February 22, 2020, 3:27 PM
 */

#ifndef LOADINGSCREEN_H
#define LOADINGSCREEN_H
#include <iostream>
#include <string>
#include "OSMID.h"
#include "LatLon.h"
#include "m1.h"
#include "m2.h"
#include "cURL.h"
#include "StreetsDatabaseAPI.h"
#include <dirent.h>
#include <sys/types.h>
#include <set>
#include <vector>
#include "loadingScreen.h"
#include "globalData.h"

vector <string> getMapNames();

vector <string> getMapNames(){
   
   //gets a list of all the available maps
   set <string> result;
   vector <string> mapNames;
      
   struct dirent *entry;
   DIR *dir = opendir("/cad2/ece297s/public/maps/");
   
   if (dir == NULL) {
      return mapNames;
   }
   
   mapNames.clear();
   while ((entry = readdir(dir)) != NULL) {
       
       //parse map names and put all unique maps into a "map" structure
       string currentFile(entry->d_name);
       int dotIndex = currentFile.find(".");
       result.insert(currentFile.substr(0, dotIndex));

   }
   closedir(dir);

   

   for(auto it = result.begin(); it != result.end(); ++it ) {
        mapNames.push_back(*it);
    }
   
   mapNames = vector<string> (mapNames.begin()+1, mapNames.end());

   return mapNames;
}

#endif /* LOADINGSCREEN_H */

