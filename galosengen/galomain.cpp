#include "galosengen.hpp"

#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

using namespace std;

int main(int argc, char* argv[])
{
    const unsigned height   = atoi(argv[1]);
    const unsigned width    = atoi(argv[2]);
    const unsigned minScore = atoi(argv[3]);
    const string   colors   =      argv[4] ;

    GaloSengen gs(width, height, minScore, colors);

    vector<string> bored;

    for (unsigned i=0; i<height; ++i)
    {
        string line;
        getline(cin, line);
        bored.push_back(line);
    }

    cout << gs.play(bored)->str() << endl;
}
