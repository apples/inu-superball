#ifndef LOC_HPP
#define LOC_HPP

#include "DJ.h"

#include "utils.inl"

#include <map>

class Loc
{
public:
    int r, c;

    Loc();
    Loc(int r, int c);
    Loc(const Loc& in);

    bool operator<(const Loc& in) const;
};

class LocGroup
{
public:
    typedef int Group;

    LocGroup(int w, int h);

    Group join(const Loc& a, const Loc& b);
    Group getGroup(const Loc& a);
    int getSize(Group a);
    Ptr<std::map<Group, int> > getSizes();
    int numRoots() const;

protected:
    int width;
    int height;

    Disjoint dj;
    int rootCount;

    int toIndex(const Loc& a) const;
    Loc fromIndex(int a) const;

    Group rawRoot(int a);
};

#endif // LOC_HPP
