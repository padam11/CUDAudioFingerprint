#include <iostream>
#include <stdlib.h>
#include <string.h>
#include <chrono>
#include <ctime>
#include "../common/globals.h"
// #include "../common/gpu_helpers.h"
#include "../common/cpu_helpers.h"
#include "../common/audiodatabase.h"

using namespace std;

int main(int argc, char *argv[])
{   
    /* STEP 0: Decide whether to run in CUDA mode or not */
    int CUDA_MODE = 0;
    /*if(argc==2 && (strcmp(argv[1], "-cuda") == 0)) {
        CUDA_MODE = 1;
    }*/

    /* STEP 1: Request the folder containing .wav files */
    char inputDirPath[100];
    cout << "Enter the name of folder containing WAVs to footprint: ";
    cin.getline(inputDirPath, sizeof inputDirPath);
    cout << endl;

    cout << "CUDA mode (1) or no (0)?: ";
    cin >> CUDA_MODE;
    cout << endl;

    chrono::time_point<std::chrono::system_clock> start, end;
 
    start = chrono::system_clock::now();

    /* STEP 2: Build the database */
    DB* database = new DB();
    database->initFromScratch(inputDirPath, CUDA_MODE);

    /* STEP 3: Serialize the database into text files */
    database->serializeToFiles((char *) "../../database/HASHES.txt", (char *) "../../database/FILENAMES.txt");

    end = chrono::system_clock::now();
    chrono::duration<double> elapsed_seconds = end - start;

    cout << "Elapsed Time: " << elapsed_seconds.count() << "s\n";

    return 0;
}
