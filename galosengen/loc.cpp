#include "loc.hpp"

/* Loc --                             --                               -- Loc */

Loc::Loc()
    : r()
    , c()
{}

Loc::Loc(int r, int c)
    : r(r)
    , c(c)
{}

Loc::Loc(const Loc& in)
    : r(in.r)
    , c(in.c)
{}

bool Loc::operator<(const Loc& in) const
{
    if (r < in.r) return true;
    if (r > in.r) return false;
    return (c < in.c);
}

/* LocGroup --                        --                          -- LocGroup */

LocGroup::LocGroup(int w, int h)
    : width(w)
    , height(h)
    , dj(width*height)
    , rootCount(width*height)
{}

LocGroup::Group LocGroup::join(const Loc& a, const Loc& b)
{
    Group rootA = getGroup(a);
    Group rootB = getGroup(b);
    if (rootA == rootB) return rootA;
    --rootCount;
    return dj.Union(rootA, rootB);
}

LocGroup::Group LocGroup::getGroup(const Loc& a)
{
    return rawRoot(toIndex(a));
}

int LocGroup::getSize(Group a)
{
    int rval = 0;

    const int wh = width*height;
    for (int i=0; i<wh; ++i)
    {
        if (rawRoot(i) == a) ++rval;
    }

    return rval;
}

Ptr<std::map<LocGroup::Group, int> > LocGroup::getSizes()
{
    std::map<Group, int>* rval = new std::map<Group, int>;

    const int wh = width*height;
    for (int i=0; i<wh; ++i)
    {
        ++(*rval)[rawRoot(i)];
    }

    return rval;
}

int LocGroup::numRoots() const
{
    return rootCount;
}

int LocGroup::toIndex(const Loc& a) const
{
    return (a.r*width + a.c);
}

Loc LocGroup::fromIndex(int a) const
{
    return Loc(a/width, a%width);
}

int LocGroup::rawRoot(int a)
{
    return dj.Find(a);
}
