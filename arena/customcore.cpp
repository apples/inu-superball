/*******************************************************************************
 * Inugami - An OpenGL framework designed for rapid game development
 * Version: 0.2.0
 * https://github.com/DBRalir/Inugami
 *
 * Copyright (c) 2012 Jeramy Harrison <dbralir@gmail.com>
 *
 * This software is provided 'as-is', without any express or implied warranty.
 * In no event will the authors be held liable for any damages arising from the
 * use of this software.
 *
 * Permission is granted to anyone to use this software for any purpose,
 * including commercial applications, and to alter it and redistribute it
 * freely, subject to the following restrictions:
 *
 *  1. The origin of this software must not be misrepresented; you must not
 *     claim that you wrote the original software. If you use this software
 *     in a product, an acknowledgment in the product documentation would be
 *     appreciated but is not required.
 *
 *  2. Altered source versions must be plainly marked as such, and must not be
 *     misrepresented as being the original software.
 *
 *  3. This notice may not be removed or altered from any source distribution.
 *
 ******************************************************************************/

#include "customcore.hpp"
#include "galosengen.hpp"

#include <fstream>
#include <utility>
#include <random>

#include <sstream>
#include <iomanip>

#include <tuple>

#include <ctime>
#include <algorithm>

bool CustomCore::Loc::operator<(const Loc& in) const
{
    return (std::tie(r, c) < std::tie(in.r, in.c));
}

bool CustomCore::Loc::operator==(const Loc& in) const
{
    return (std::tie(r, c) == std::tie(in.r, in.c));
}

bool CustomCore::Loc::operator!=(const Loc& in) const
{
    return (std::tie(r, c) != std::tie(in.r, in.c));
}

CustomCore::CustomCore(int i, int j)
    : rules{5, 5, 3, 10, 8, 5}

    , board(rules.width*rules.height, Color::NONE)

    , score(0)

    , rng(j)

    , highScore(-1)
    , totalScore(0)
    , runs(i)
    , run(0)

    , scoreZone()
{
#if 1
    for (int r=2; r<rules.height-2; ++r)
    {
        for (int c=0; c<2; ++c) scoreZone.insert({r, c});
        for (int c=rules.width-2; c<rules.width; ++c) scoreZone.insert({r, c});
    }
#else
    {
        std::uniform_int_distribution<int> rd(0, rules.height-1);
        std::uniform_int_distribution<int> cd(0, rules.width-1);
        while (scoreZone.size() < 10)
        {
            scoreZone.insert({rd(rng), cd(rng)});
        }
    }
#endif

    spawn(rules.swapSpawn);
}

void CustomCore::go()
{
    while (true) tick();
}

void CustomCore::tick()
{
    galoSengen();
}

CustomCore::Color& CustomCore::cellAt(const Loc& loc)
{
    return board[loc.r*rules.width+loc.c];
}

void CustomCore::swapCells(const Loc& a, const Loc& b, bool force)
{
    Color& cell1 = cellAt(a);
    Color& cell2 = cellAt(b);

    std::swap(cell1, cell2);

    return spawn(5);
}

void CustomCore::scoreCell(const Loc& loc)
{
    Color& cell = cellAt(loc);
    
    std::set<Loc> group;

    bool isScoring = getGroup(loc, cell, group);

    int s = colorVal(cell) * group.size();

    score += s;
    
    for (const Loc& l : group) cellAt(l) = Color::NONE;

    return spawn(3);
}

bool CustomCore::getGroup(const Loc& loc, Color k, std::set<Loc>& s)
{
    Color& cell = cellAt(loc);
    if (cell != k) return false;

    if (!s.insert(loc).second) return false;

    bool isScoring = isScoreTile(loc);

    int a = rules.height-1;
    int b = rules.width-1;

    if (loc.r>0) isScoring += getGroup({loc.r-1, loc.c}, k, s);
    if (loc.r<a) isScoring += getGroup({loc.r+1, loc.c}, k, s);
    if (loc.c>0) isScoring += getGroup({loc.r, loc.c-1}, k, s);
    if (loc.c<b) isScoring += getGroup({loc.r, loc.c+1}, k, s);

    return isScoring;
}

void CustomCore::spawn(int n)
{
    std::vector<Color*> nones;

    for (Color& c : board)
    {
        if (c == Color::NONE) nones.push_back(&c);
    }

    if (nones.size() < n) return gameOver();

    std::shuffle(nones.begin(), nones.end(), rng);

    std::uniform_int_distribution<int> pick(1,rules.numColors);

    for (int i=0; i<n; ++i)
    {
        *nones[i] = Color(pick(rng));
    }
}

void CustomCore::galoSengen()
{
    GaloSengen gs(rules.width, rules.height, rules.minScore, "pbygr");

    std::vector<std::string> bored(rules.height, std::string(rules.width, '.'));

    std::map<Color, char> conv = {
          {Color::NONE   , '.'}
        , {Color::MAGENTA, 'p'}
        , {Color::BLUE   , 'b'}
        , {Color::YELLOW , 'y'}
        , {Color::GREEN  , 'g'}
        , {Color::RED    , 'r'}
    };

    for (unsigned r=0; r<rules.height; ++r)
    {
        for (unsigned c=0; c<rules.width; ++c)
        {
            bored[r][c] = conv[cellAt({r, c})];
        }
    }

    auto act = gs.play(bored);

    auto str = act->str();

    std::stringstream ss(str);

    std::string action;
    ss >> action;

    if (action == "SWAP")
    {
        Loc locs[2];
        ss >> locs[0].r >> locs[0].c >> locs[1].r >> locs[1].c;

        return swapCells(locs[0], locs[1], true);
    }

    if (action == "SCORE")
    {
        Loc loc;
        ss >> loc.r >> loc.c;

        return scoreCell(loc);
    }
}

int CustomCore::colorVal(Color c) const
{
    return int(c)-int(Color::NONE)+1;
}

void CustomCore::gameOver()
{
    totalScore += score;
    if (score > highScore) highScore = score;
    
    if (++run >= runs) throw GameOver(double(totalScore)/double(runs), highScore);
    
    score = 0;
    for (Color& c : board) c = Color::NONE;
    spawn(rules.swapSpawn);
}

bool CustomCore::isScoreTile(const Loc& loc) const
{
    return (scoreZone.find(loc) != end(scoreZone));
}
