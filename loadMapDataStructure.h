#pragma once



//FORWARD DECLARATION FOR MAP LOADING FUNCTIONS
bool load_dotBin_files(std::string mapPath);
void loadIntersectionGraph(int numIntersections);
void loadStreetDatabases(int numberOfSegments, int numberOfStreets, vector <pair<std::string, int>> & tempArrayOfStreetNames);
void sortStreetSegments();
void loadFeatureDatabases(int numberOfFeatures);
void loadPOIDatabases(int numberOfPOIs);
void loadSortedStreetName (vector <pair<std::string, int>> & tempArrayOfStreetNames);
void loadOSMDatabases();


