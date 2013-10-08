#include <iostream>
#include <string>
#include <fstream>

using namespace std;

int main(int argc, char* argv[])
{
    if (argc != 3) return -1;
    
    string a = argv[1];
    string b = argv[2];
    
    ifstream file(argv[1]);
    if (!file) return -2;
    
    string line;
    while (getline(file, line))
    {
        if (b.find(line) != string::npos) return 0;
    }
    
    return 1;
}
