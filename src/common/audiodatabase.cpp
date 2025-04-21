#include <string.h>
#include <stdlib.h>
#include <dirent.h>
#include <iostream>
#include <unordered_map>
#include <list>
#include <vector>
#include <fstream>
#include <sstream>
#include "../common/globals.h"
#include "../common/audiodatabase.h"
#include "../common/cpu_helpers.h"
#include "../common/gpu_helpers.h"
#include <map>

using namespace std;


/* Constructor */
DB::DB(){
  numFiles = 0;
}

int DB::initFromScratch(char * folderPath, int cudaMode){

  /* Step 1: For all .wavs in this folder, assign a filename to each file ID */

  DIR*    dir;
  dirent* pdir;
  vector<string> files;

  dir = opendir(folderPath);
  pdir = readdir(dir);

  int i=0;
  while (pdir){
    string filename = pdir->d_name;
    if(filename.find(".wav") != string::npos){
      fileNames.push_back(std::string(pdir->d_name));
      i++;
    }
    pdir = readdir(dir);
  }
  numFiles = i;

  /* Step 2: Compute hashes and their respective datapoints */
  
  for(int i=0; i<fileNames.size(); i++){
    
    //char* filepath;
    //filepath = (char *) malloc(strlen(folderPath)+strlen(fileNames[i])+1); 
    //strcpy(filepath, folderPath ); 
    //strcat(filepath, fileNames[i]);


    Complex * audio;
    int audioSize, numChunks;
    std::string fullpath = std::string(folderPath) + fileNames[i];
    wavToComplex(fullpath.c_str() , &audio, &audioSize);
    numChunks = audioSize / CHUNK_SAMPLES;
    unsigned long * hashes;

    if(cudaMode == 0){
      audioToHashes(audio, numChunks, &hashes);
    }
    else{
      audioToHashes_CUDA(audio, numChunks, &hashes);
    }
    

    fileLengths.push_back(numChunks);
    std::cout << "Added file ID " << i << ": " << fileNames[i] << std::endl;

    for(int j=0; j<numChunks; j++){

        DataPoint dp;
        dp.file_id = i;
        dp.time = j;
        unsigned long hash = hashes[j];

        hashmap[hash].push_back(dp);

    }
  }


  return 0;
}

/*
  serializeToFiles
  ====================
  Takes the contents of the database and serializes them to text files
  that can be used to initialize the database again at 

  @param - folderPath: string containing input file for WAV

  @return - 0 for success, 1 for error
*/

int DB::serializeToFiles(char * outputHashesFile, char * outputfileNamesFile){
  
  fstream fileNamesFile(outputfileNamesFile, ios_base::out);
  
  for(int i=0; i<fileNames.size(); i++){
    fileNamesFile << i << " " << fileNames[i] << " " << fileLengths[i] << " " << endl;
  }

  fstream hashFile(outputHashesFile, ios_base::out);

  for (auto it : hashmap){
    long hash = it.first;
    
    list<DataPoint> dpList = hashmap[hash];

    hashFile << std::hex << hash;

    for(auto &i : dpList){
      DataPoint dp = i;
      hashFile << std::dec << " ( " << dp.file_id << " " << dp.time << " )";
    }
    hashFile << endl;
  }


  fileNamesFile.close();
  hashFile.close();

  return 0;
    
}


/*
dbInitFromFile
====================
Initializes database from a previously serialized instance of a database.
This is useful so we don't have to perform FFTs and determine frequencies
from hours of audio all over again.

@param - hashesFile: file containing serialized hashmap
        - fileNamesFile: file containing serialized fileIDs and fileNames

@return - 0 for success, 1 for error
*/
int DB::initFromFile(char * hashesFile, char * fileNamesFile){

  //Step 1: Read in the fileNames and assign file IDs
  
  fstream fstreamfileNames(fileNamesFile, ios_base::in);
  if (!fstreamfileNames) {
    cout << "Unable to open fileNames file";
    exit(1); // terminate with error
  }

  int id;
  int length;
  string fname;

  while ( fstreamfileNames >> id >> fname >> length ){
    char * fnameCstr = new char[fname.length() + 1];
    strcpy(fnameCstr, fname.c_str());
    fileNames.push_back(fnameCstr);
    fileLengths.push_back(length);
    numFiles++;
  }

  fstreamfileNames.close();

  //Step 2: Read in the hash table

  fstream fstreamHashes(hashesFile, ios_base::in);
  if (!fstreamHashes) {
    cout << "Unable to open hashes file";
    exit(1); // terminate with error
  }

  string line;
  while (getline( fstreamHashes, line))
  {
    stringstream stream(line);

    unsigned long hash;
    stream >> std::hex >> hash >> std::dec;


    list<DataPoint> dpList;
    char openParentheses;
    char closeParentheses;
    unsigned int dataPointTime;
    unsigned int dataPointFileId;
    int i = 0;


    while( stream >> openParentheses >> dataPointFileId >> dataPointTime >> closeParentheses){          
      if(openParentheses!='(' || closeParentheses!=')'){
        cout << "HASHES text file not formatted correctly at line: '" << line << "'" << endl;
        exit(1); // terminate with error
      }
      DataPoint * dp = new DataPoint;
      dp->file_id = dataPointFileId;
      dp->time = dataPointTime;
      
      dpList.push_back(*dp);
    }
    
    hashmap[hash] = dpList;
  }

  return 0;
}


int DB::getBestMatchingSongNaive(int numHashes, unsigned long * hashes){

  //NOTE: THIS IS A NAIVE IMPLEMENTATION... FOR BETTER ACCURACY, CONSIDER OFFSET
  
  //Compute histogram
  int * histogram = (int *) malloc(numFiles * sizeof(int));  //Histogram of what files match each audio chunk
  for(int i=0; i<numFiles; i++) histogram[i] = 0;
  int curOffset = 0;

  cout << "NUM HASHES = " << numHashes << endl;

  for(int i=0; i<numHashes; i++){
    unsigned long hash = hashes[i];
    list<DataPoint> datapoints = hashmap[hash];
    for(auto dp : datapoints){
      histogram[dp.file_id]++;
    }
    curOffset++;
  }

  //Get best song from the histogram
  int best = 0;
  int bestScore = 0;

  cout << "---HISTOGRAM RESULTS---" << endl;

  for(int i=0; i<numFiles; i++){

    cout << "FILE ID " << i << " HAS SCORE OF " << histogram[i] << " ( " << fileNames[i] << " ) " <<endl;

    if(histogram[i] > bestScore){
      best = i;
      bestScore = histogram[i];
    }
  }

  cout << "The audiofile " << fileNames[best] << " was the BEST matching song!";

  return best;
}

int DB::getBestMatchingSong(int numHashes, unsigned long * hashes){

  int biggestFileLength = 0;
  for(int i=0; i<fileLengths.size(); i++) if(fileLengths[i] > biggestFileLength) biggestFileLength = fileLengths[i];
  
  //Compute histogram

  int histogram[numFiles];  //Histogram of what files match each audio chunk
  memset(histogram, 0, numFiles * sizeof(int));

  int offsetHistory[numFiles][biggestFileLength]; //Keeps track of at what point in time did we last see the hashes
  memset(offsetHistory, -1, sizeof(offsetHistory[0][0]) * numFiles * biggestFileLength);

  for(int t=0; t<numHashes; t++){

    bool histogramCandidates[numFiles]; //Histogram candidates for this iteration
    memset(histogramCandidates, false, sizeof(bool)*numFiles);

    unsigned long hash = hashes[t];
    list<DataPoint> datapoints = hashmap[hash];

    for(auto dp : datapoints){
      if(dp.time > 0 && offsetHistory[dp.file_id][dp.time-1] == (t-1)){ 
        histogramCandidates[dp.file_id] = true; //We found a candidate
      }
      offsetHistory[dp.file_id][dp.time] = t;
    }

    for(int i=0; i<numFiles; i++) if(histogramCandidates[i] == true) histogram[i]++;
  }

  //Get best song from the histogram
  int best = 0;
  int bestScore = 0;
  cout << endl << "---HISTOGRAM RESULTS---" << endl;
  for(int i=0; i<numFiles; i++){
    cout << "FILE ID " << i << " HAS SCORE OF " << histogram[i] << " ( " << fileNames[i] << " ) " <<endl;
    if(histogram[i] > bestScore){
      best = i;
      bestScore = histogram[i];
    }
  }
  cout << endl << "The audiofile " << fileNames[best] << " was the BEST matching song! " << endl << endl;


  return best;
}

//from that guy on youtube

int DB::getBestMatchingSongTimeDelta(int numHashes, unsigned long *hashes)
{
  map<int, int> scorebyFile;

  for (int i = 0; i < numHashes - 1; ++i)
  {
    unsigned long hashA = hashes[i];
    unsigned long hashB = hashes[i+1];

    const auto& listA = hashmap[hashA];
    const auto& listB = hashmap[hashB];

    for (const auto& dpA : listA) {
      for (const auto& dpB : listB) {
        if (dpA.file_id == dpB.file_id) {
          int deltaQuery = 1; //because i+1 - 1 =1 chunk
          int deltaCandidate = dpB.time - dpA.time;

          float timeDeltaQuerySec = deltaQuery * CHUNK_SECONDS;
          float timeDeltaCandidateSec = deltaCandidate * CHUNK_SECONDS;

          if (abs(timeDeltaQuerySec - timeDeltaCandidateSec) < 0.1f) {
            scorebyFile[dpA.file_id]++;
          }
        }
      }
    }
  }

  //find the file with the highest score
  int bestId = -1;
  int bestScore = 0;

  std::cout << "--- TIME-DELTA MATCHING RESULTS ---" << std::endl;
  for (const auto& entry : scorebyFile) {
    cout << "File ID " << entry.first << " score: " << entry.second << " (" << fileNames[entry.first] << ")" << endl;
    if (entry.second > bestScore)
    {
      bestScore = entry.second;
      bestId = entry.first;
    }
  }

  cout << "\nBest match: " << fileNames[bestId] << endl;
  return bestId;
}
