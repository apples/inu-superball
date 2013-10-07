#ifndef DJ_H
#define DJ_H

#include <vector>
#include <iostream>
using namespace std;

class Disjoint {
  public:
    Disjoint(int nelements);
    int Union(int s1, int s2);
    int Find(int element);
    void Print();
  protected:
    vector <int> links;
    vector <int> ranks;
};

#endif // DJ_H
