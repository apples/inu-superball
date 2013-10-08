#include "galosengen.hpp"

#include <map>
#include <set>
#include <vector>
#include <cstdlib>
#include <fstream>
#include <algorithm>

//#define INU_PROFILE
//#define SUPER_SAIYAN

#ifdef INU_PROFILE
    #include "meta.hpp"
    using namespace Inugami;
#endif // INU_PROFILE

#ifdef SUPER_SAIYAN
    #include <thread>
#endif // SUPER_SAIYAN

const GaloSengen::Cell GaloSengen::EMPTY = '.';

GaloSengen::Cell& GaloSengen::cellify(Cell& c) //static
{
    if (c>='A'&&c<='Z') c = c-'A'+'a';
    if (c=='*') c = '.';
    return c;
}

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

/* GaloSengen --                      --                        -- GaloSengen */

GaloSengen::BoardInfo::BoardInfo(int w, int h)
    : numGroups(0)
    , numEmpty(0)
    , need(w*h)
    , scoreVal(0)
    , bestSize(0)
    , numScoreGroups(0)
    , numScorable(0)
    , numWeakGroups(0)
    , numSmallGroups(0)
    , numFieldGroups(0)
    , groups(w, h)
    , bestLoc(-1, -1)
{}

template <typename T1>
static bool comp(T1 a1, T1 b1)
{
    return (a1 < b1);
}

template <typename T1, typename T2>
static bool comp(T1 a1, T2 a2, T1 b1, T2 b2)
{
    if (a1 < b1) return true;
    if (a1 == b1) return comp(a2, b2);
    return false;
}

template <typename T1, typename T2, typename T3>
static bool comp(T1 a1, T2 a2, T3 a3, T1 b1, T2 b2, T3 b3)
{
    if (a1 < b1) return true;
    if (a1 == b1) return comp(a2, a3, b2, b3);
    return false;
}

GaloSengen::GaloSengen(int w, int h, int ms, Array c)
    : width(w)
    , height(h)
    , minScore(std::max(ms,3))
    , colors(c)
    , colorVals()
    , scoreZone()
    , normalAI()
    , panicAI()
    , inverseSpecs()
{
    for (int i=0; i<colors.size(); ++i)
    {
        colorVals[c[i]] = i+2;
    }

    for (int r=2; r<height-2; ++r)
    {
        for (int c=0; c<2; ++c)           scoreZone.push_back(Loc(r, c));
        for (int c=width-2; c<width; ++c) scoreZone.push_back(Loc(r, c));
    }

    panicAI.push_back(&BoardInfo::need);

    loadAI(normalAI, "galo-normal.txt");
    loadAI(panicAI , "galo-panic.txt");

    inverseSpecs.push_back(&BoardInfo::bestSize);
    inverseSpecs.push_back(&BoardInfo::numScorable);
    inverseSpecs.push_back(&BoardInfo::scoreVal);
}

void GaloSengen::loadAI(AISpec& ai, const char* filename)
{
    std::ifstream file(filename);
    if (!file)
    {
        ai.push_back(&BoardInfo::numScorable);
        ai.push_back(&BoardInfo::numGroups);
        ai.push_back(&BoardInfo::need);
    }
    else
    {
        std::string line;
        int act;
        while (file >> act)
        {
            switch (act)
            {
                case 0:
                {
                    ai.push_back(&BoardInfo::numGroups);
                break;}

                case 1:
                {
                    ai.push_back(&BoardInfo::numEmpty);
                break;}

                case 2:
                {
                    ai.push_back(&BoardInfo::need);
                break;}

                case 3:
                {
                    ai.push_back(&BoardInfo::scoreVal);
                break;}

                case 4:
                {
                    ai.push_back(&BoardInfo::bestSize);
                break;}

                case 5:
                {
                    ai.push_back(&BoardInfo::numScoreGroups);
                break;}

                case 6:
                {
                    ai.push_back(&BoardInfo::numScorable);
                break;}

                case 7:
                {
                    ai.push_back(&BoardInfo::numWeakGroups);
                break;}

                case 8:
                {
                    ai.push_back(&BoardInfo::numSmallGroups);
                break;}

                case 9:
                {
                    ai.push_back(&BoardInfo::numFieldGroups);
                break;}

                default:
                {
                    throw "up";
                }
            }
        }
    }
}

Ptr<Action> GaloSengen::play(Board board)
{
#ifdef INU_PROFILE
    ScopedProfile _sp(profiler, "play()");
#endif // INU_PROFILE

    clean(board);

    Ptr<BoardInfo> before = getInfo(board);

    if (before->numEmpty < 5 && before->bestSize >= minScore
     || before->bestSize >= minScore)
    {
        return new Score(before->bestLoc);
    }

    bool panicMode = (before->need >= before->numEmpty/5);

    {
#ifdef INU_PROFILE
        ScopedProfile _sp(profiler, "Swap AI");
#endif // INU_PROFILE

        std::vector<Loc> locs;

        for (int r=0; r<height; ++r)
        {
            for (int c=0; c<width; ++c)
            {
                const Cell& cell = board[r][c];

                if (cell != EMPTY) locs.push_back(Loc(r, c));
            }
        }

		std::vector<std::pair<Loc,Loc> > swp;
        Ptr<BoardInfo> swpInfo = before;
		swp.push_back(std::make_pair(locs[0], locs[1]));

        Board newBoard = board;

        {
#ifdef INU_PROFILE
            ScopedProfile _sp(profiler, "Best Swap");
#endif // INU_PROFILE

            for (unsigned i=0; i<locs.size()-1; ++i)
            {
                for (unsigned j=i+1; j<locs.size(); ++j)
                {
                    Cell& cellA = newBoard[locs[i].r][locs[i].c];
                    Cell& cellB = newBoard[locs[j].r][locs[j].c];
                    if (cellA == cellB) continue;

#ifdef INU_PROFILE
                    ScopedProfile _sp(profiler, "Test Swap");
#endif // INU_PROFILE

                    std::swap(cellA, cellB);

                    Ptr<BoardInfo> after = getInfo(newBoard);

                    const AISpec& spec = (panicMode? panicAI : normalAI);

                    ChainChomp cc(ChainChomp::N);

                    for (int act=0; act<spec.size(); ++act)
                    {
                        int BoardInfo::* param = spec[act];
                        SpecSet::iterator ssl = std::find(inverseSpecs.begin()
                                                          , inverseSpecs.end()
                                                          , param
                        );
                        if (ssl == inverseSpecs.end())
                        {
                            cc((*after).*param, (*swpInfo).*param);
                        }
                        else
                        {
                            cc((*swpInfo).*param, (*after).*param);
                        }
                    }

                    if (cc.get())
                    {
                        swp.clear();
                        swp.push_back(std::make_pair(locs[i], locs[j]));
                        swpInfo = after;
                    }
#if 0
                    else if (cc.state == ChainChomp::N)
                    {
                        swp.push_back(std::make_pair(locs[i], locs[j]));
                    }
#endif
                    std::swap(cellA, cellB);
                }
            }
        }

		unsigned index = 0; //rand()%(swp.size());

        return new Swap(swp[index].first, swp[index].second);
    }
}

int GaloSengen::fillGroups(const Board& board, LocGroup& groups) const
{
    int emptyCount = 0;

    for (int r=0; r<height; ++r)
    {
        for (int c=0; c<width; ++c)
        {
            const Cell& cell = board[r][c];

            if (cell == EMPTY)
            {
                ++emptyCount;
                continue;
            }

            const Loc l (r, c);

            if (r>0)
            {
                const Loc u (r-1, c);

                if (cell == board[u.r][u.c])
                {
                    groups.join(l, u);
                }
            }

            if (c>0)
            {
                const Loc u (r, c-1);

                if (cell == board[u.r][u.c])
                {
                    groups.join(l, u);
                }
            }
        }
    }

    return emptyCount;
}

int GaloSengen::weakGroups(const Board& board) const
{
#ifdef INU_PROFILE
    ScopedProfile _sp(profiler, "weakGroups()");
#endif // INU_PROFILE

    class Iter
    {
    public:
        Loc orig;
        int pos;
        Iter(const Loc& l) : orig(l), pos(0) {}
        Loc get() const
        {
            switch (pos)
            {
                case  0: return Loc(orig.r  , orig.c-1);
                case  1: return Loc(orig.r-1, orig.c-1);
                case  2: return Loc(orig.r-1, orig.c  );
                default: return Loc(orig.r-1, orig.c+1);
            }
        }
        bool next() { return (++pos < 5); }
    };

    LocGroup groups(width, height);

    for (int r=0; r<height; ++r)
    {
        for (int c=0; c<width; ++c)
        {
            const Cell& cell = board[r][c];

            if (cell == EMPTY) continue;

            const Loc l (r, c);

            Iter i (l);
            do
            {
                const Loc& u = i.get();

                if (u.r>=0 && u.r<height && u.c>00 && u.c<width)
                {
                    if (cell == board[u.r][u.c]) groups.join(l, u);
                }
            } while (i.next());
        }
    }

    return groups.numRoots();
}

Ptr<GaloSengen::BoardInfo> GaloSengen::getInfo(const Board& board)
{
#ifdef INU_PROFILE
    ScopedProfile _sp(profiler, "getInfo");
#endif // INU_PROFILE

    typedef std::set<LocGroup::Group> SG;

    BoardInfo* rval = new BoardInfo(width, height);

    rval->numEmpty = fillGroups(board, rval->groups);

    rval->numGroups = rval->groups.numRoots();

#ifdef SUPER_SAIYAN
    std::thread threadWeakGroups([&]
#else
    (
#endif // SUPER_SAIYAN
    {
        rval->numWeakGroups = weakGroups(board);
    });

    std::map<Loc, LocGroup::Group> loc2grp;

    std::map<LocGroup::Group, int> grp2gsz;

    {
#ifdef INU_PROFILE
        ScopedProfile _sp(profiler, "Group Sizes");
#endif // INU_PROFILE

        std::set<LocGroup::Group> grps;
        for (unsigned r=0; r<height; ++r)
        {
            for (unsigned c=0; c<width; ++c)
            {
                if (board[r][c] == EMPTY) continue;
                LocGroup::Group p;
                const Loc l (r, c);
                {
#ifdef INU_PROFILE
                    ScopedProfile _sp(profiler, "Get Group");
#endif // INU_PROFILE
                    p = rval->groups.getGroup(l);
                    loc2grp[l] = p;
                }
                int sz;
                std::map<LocGroup::Group, int>::iterator szi;
                {
#ifdef INU_PROFILE
                    ScopedProfile _sp(profiler, "Get Size");
#endif // INU_PROFILE
                    szi = grp2gsz.find(p);
                    if (szi == grp2gsz.end())
                    {
                        sz = rval->groups.getSize(p);
                        grp2gsz.insert(szi, std::make_pair(p, sz));
                    }
                    else
                    {
                        sz = szi->second;
                    }
                }
                if (sz < 5) grps.insert(p);
            }
        }
        rval->numSmallGroups = grps.size();
    }

    SG scoreGroups;

    int bestScore = 0;

    {
#ifdef INU_PROFILE
        ScopedProfile _sp(profiler, "Score Zone Groups");
#endif // INU_PROFILE

        for (ZoneIter i=scoreZone.begin(); i!=scoreZone.end(); ++i)
        {
            const Cell& cell = board[i->r][i->c];
            if (cell == EMPTY) continue;

            LocGroup::Group group = loc2grp[*i];

            if (scoreGroups.insert(group).second)
            {
                int cscore = colorVals[cell];
                int gsize  = grp2gsz[group];
                int gscore = cscore*gsize;

                if (gsize >= minScore)
                {
                    rval->bestLoc = *i;
                    rval->bestSize = gsize;
                    bestScore = gscore;
                    ++rval->numScorable;
                }

                rval->scoreVal += gsize;

                int need = minScore - gsize;

                if (need < rval->need) rval->need = need;
            }
        }
    }

    rval->scoreVal *= 10;
    if (scoreGroups.size()>0) rval->scoreVal /= scoreGroups.size();

    rval->numScoreGroups = scoreGroups.size();
    rval->numFieldGroups = rval->numGroups - rval->numScoreGroups;

#ifdef SUPER_SAIYAN
    threadWeakGroups.join();
#endif // SUPER_SAIYAN

    return rval;
}

void GaloSengen::clean(Board& board)
{
    for (int r=0; r<height; ++r)
    {
        for (int c=0; c<width; ++c)
        {
            cellify(board[r][c]);
        }
    }
}
