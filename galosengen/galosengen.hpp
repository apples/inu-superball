#ifndef GALOSENGEN_H
#define GALOSENGEN_H

#include "DJ.h"

#include "action.hpp"
#include "loc.hpp"

#include "utils.inl"

#include <map>
#include <sstream>
#include <string>
#include <set>
#include <vector>

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
        int numExtendedGroups;
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
    Zone extendedZone;

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
