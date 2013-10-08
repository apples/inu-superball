#ifndef GALOSENGEN_H
#define GALOSENGEN_H

#include "DJ.h"

#include <map>
#include <sstream>
#include <string>
#include <set>
#include <vector>

class ChainChomp
{
public:
    enum State
    {
        N, T, F
    };

    State state;

    ChainChomp(State s)
        : state(s)
    {}

    template <typename A, typename B>
    ChainChomp& operator()(const A& a, const B& b)
    {
        if (state != N) return *this;
        if      (a<b)  state = T;
        else if (a==b) state = N;
        else           state = F;
        return *this;
    }

    bool get() const
    {
        return (state == T);
    }
};

inline ChainChomp releaseTheChains()
{
    return ChainChomp(ChainChomp::N);
}

template <typename T>
class Ptr
{
    struct Data
    {
        T* ptr;
        int refs;
    } *data;
public:
    Ptr()
        : data(NULL)
    {}

    Ptr(T* p)
        : data(new Data)
    {
        data->ptr = p;
        data->refs = 1;
    }

    Ptr(const Ptr& in)
        : data(in.data)
    {
        if (data) ++data->refs;
    }

    ~Ptr()
    {
        del();
    }

    Ptr& operator=(const Ptr& in)
    {
        del();
        data = in.data;
        if (data) ++data->refs;
    }

    void del()
    {
        if (data)
        {
            if (--data->refs == 0)
            {
                delete data->ptr;
                delete data;
            }
        }
        data = NULL;
    }

    void reset(T* p)
    {
        del();
        data = new Data;
        data->ptr = p;
        data->refs = 1;
    }

    T* operator->() const
    {
        return data->ptr;
    }

    T& operator*() const
    {
        return *(data->ptr);
    }
};

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

class Action
{
public:
    virtual ~Action() {}
    virtual std::string str() const = 0;
};

class Swap
    : public Action
{
    Loc data[2];
public:
    Swap(Loc a, Loc b)
        : data{a, b}
    {}

    virtual std::string str() const
    {
        std::stringstream ss;
        ss << "SWAP ";
        ss << data[0].r << " " << data[0].c << " ";
        ss << data[1].r << " " << data[1].c;
        return ss.str();
    }
};

class Score
    : public Action
{
    Loc data;
public:
    Score(Loc a)
        : data(a)
    {}

    virtual std::string str() const
    {
        std::stringstream ss;
        ss << "SCORE ";
        ss << data.r << " " << data.c;
        return ss.str();
    }
};

class GaloSengen
{
public:
    typedef char Cell;
    typedef std::string Array;
    typedef std::vector<Array> Board;

    typedef std::vector<Loc> Zone;
    typedef std::vector<Loc>::iterator ZoneIter;

    class BoardInfo
    {
    public:
        BoardInfo(int w, int h);
        int numGroups;
        int numEmpty;
        int need;
        int scoreVal;
        int bestSize;
        int numScoreGroups;
        int numScorable;
        int numWeakGroups;
        int numSmallGroups;
        int numFieldGroups;
        LocGroup groups;
        Loc bestLoc;
    };

    typedef std::vector<int BoardInfo::*> AISpec;
    typedef std::vector<int BoardInfo::*> SpecSet;

    static const Cell EMPTY;

    static Cell& cellify(Cell& c);

    const int width, height;
    const int minScore;
    Array colors;

    std::map<Cell, int> colorVals;
    Zone scoreZone;

    AISpec normalAI;
    AISpec panicAI;

    SpecSet inverseSpecs;

    GaloSengen(int w, int h, int ms, Array c);
    void loadAI(AISpec& ai, const char* filename);
    Ptr<Action> play(Board board);

    int fillGroups(const Board& board, LocGroup& groups) const;
    int weakGroups(const Board& board) const;
	Ptr<BoardInfo> getInfo(const Board& board);
	void clean(Board& board);
};

#endif // GALOSENGEN_H
