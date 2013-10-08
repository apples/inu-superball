#include <iostream>
#include <vector>
#include <string>
#include <algorithm>
#include <sstream>

using namespace std;

struct Data
{
    float avg;
    int high;
    string name;
};

bool sortAvg(const Data& a, const Data& b)
{
    return (a.avg>b.avg);
}

bool sortHigh(const Data& a, const Data& b)
{
    return (a.high>b.high);
}

int main()
{
    string line;
    vector<Data> vec;
    while (getline(cin, line))
    {
        auto cpos = line.find(':');
        
        Data datum;
        datum.name = line;
        
        stringstream ss(line.substr(cpos+1));
        if (ss >> datum.avg >> datum.high) vec.push_back(datum);
    }
    sort(vec.begin(), vec.end(), sortAvg);
    for (int i=0; i<vec.size()&&i<10; ++i) cout << vec[i].name << endl;
    sort(vec.begin(), vec.end(), sortHigh);
    //for (int i=0; i<vec.size()&&i<10; ++i) cout << vec[i].name << endl;
}
