#include "customcore.hpp"

#include <iostream>
#include <cstdlib>
#include <thread>
#include <future>
#include <random>

using namespace std;

int main(int argc, char* argv[])
{
    if (argc != 2) return -1;
    int runs = atoi(argv[1]);
    
    auto oneRun = [](int i)
    {
        try
        {
            CustomCore core(1, i);
            core.go();
        }
        catch (GameOver& go)
        {
            return go.high;
        }
        catch (...)
        {
            return 0;
        }
        return 0;
    };
    
    vector<future<int>> futes;
    mt19937 rng(time(nullptr));
    
    for (int i=0; i<runs; ++i) futes.push_back(async(launch::async, oneRun, rng()));
    
    int tote = 0;
    int high = 0;
    for (int i=0; i<runs; ++i)
    {
        int scr = futes[i].get();
        tote += scr;
        if (scr>high) high = scr;
    }
    
    double avg = double(tote)/double(runs);
    
    cout << avg << " " << high << endl;
}
