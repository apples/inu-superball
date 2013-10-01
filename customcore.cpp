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

#include "meta.hpp"

#include "inugami/camera.hpp"
#include "inugami/geometry.hpp"
#include "inugami/image.hpp"
#include "inugami/interface.hpp"
#include "inugami/loaders.hpp"
#include "inugami/math.hpp"
#include "inugami/shader.hpp"
#include "inugami/shaderprogram.hpp"
#include "inugami/transform.hpp"
#include "inugami/utility.hpp"

#include <fstream>
#include <utility>
#include <random>

#include <sstream>
#include <iomanip>

#include <tuple>

#include <ctime>

using namespace Inugami;

bool CustomCore::Loc::operator<(const Loc& in) const
{
    return (std::tie(r, c) < std::tie(in.r, in.c));
}

CustomCore::CustomCore(const RenderParams &params)
    : Core(params)

    , colors
    {
          {{1, 1, {128, 128, 128, 255}}, false, false} // NONE
        , {{1, 1, {255,   0, 255, 255}}, false, false} // MAGENTA
        , {{1, 1, {  0,   0, 255, 255}}, false, false} // BLUE
        , {{1, 1, {255, 255,   0, 255}}, false, false} // YELLOW
        , {{1, 1, {  0, 255,   0, 255}}, false, false} // GREEN
        , {{1, 1, {255,   0,   0, 255}}, false, false} // RED
        , {{1, 1, {  0, 255, 255, 255}}, false, false} // CYAN
        , {{1, 1, {128,   0, 128, 255}}, false, false} // VIOLET
        , {{1, 1, {  0,   0,   0, 255}}, false, false} // BLACK
        , {{1, 1, {255, 255, 255, 255}}, false, false} // WHITE
    }
    , font(Image::fromPNG("data/font.png"), 16, 16)
    , panel(Geometry::fromRect(1.f, 1.f))
    , piece(Geometry::fromDisc(0.9f, 0.9f, 16))
    , diamond(Geometry::fromDisc(0.3f, 0.3f, 4))

    , rules{5, 5, 3, 10, 8}

    , board(rules.boardWidth*rules.boardHeight, Color::NONE)

    , selection{-1, -1, false}
    , flashing{-1, Color::NONE}

    , score(0)

    , hoverCell{-1, -1}
    , hoverGroup()
    , hoverScore(false)

    , rng(time(nullptr))

    , swapAnim{0.f, 0.f, 0.f, {{-1, -1}, {-1, -1}}}
    , spawning()
    , spawnScale(0.f)

    , screenShake(0.f)

    , prevScore(-1)
    , highScore(-1)
    , pointList()
{
    ScopedProfile prof(profiler, "CustomCore: Constructor");

    logger->log<5>("Adding callbacks...");
    addCallback([&]{tick();draw();}, 60.0);

    setWindowTitle("Inu SuperBall", true);

    spawn(rules.swapSpawn);
}

void CustomCore::tick()
{
    ScopedProfile prof(profiler, "CustomCore: Tick");

    //Keybinds can be stored in proxies
    auto keyAI    = iface->getProxy(' '_ivk);
    auto keyFast  = iface->getProxy('L'_ivkShift);
    auto keyESC   = iface->getProxy(0_ivkFunc);
    auto keyFlood = iface->getProxy(1_ivkFunc);
    auto keyReset = iface->getProxy(5_ivkFunc);

    //Poll must be called every frame
    iface->poll();

    //Key Proxies can be cast to bool
    if (keyESC || shouldClose())
    {
        running = false;
        return;
    }

    if (keyReset.pressed())
    {
        gameOver();
    }

    if (keyAI.pressed() || (keyAI && keyFast))
    {
        galoSengen();
    }

    if (keyFlood.pressed())
    {
        for (Color& c : board) c = Color::RED;
    }

    auto mPos = iface->getMousePos();
    float mx = mPos.x/10.0;
    float my = mPos.y/10.0-5.f;

    int cc = mx/6.f;
    int cr = my/6.f;

    hoverCell.r = cr;
    hoverCell.c = cc;

    hoverGroup.clear();

    hoverScore = false;

    bool inBoard = (
            cc>=0 && cc<rules.boardWidth
         && cr>=0 && cr<rules.boardHeight
    );

    if (inBoard)
    {
        Color cat = cellAt(cr, cc);

        if (cat != Color::NONE)
        {
            hoverScore = getGroup(cr, cc, cat, hoverGroup);
        }
    }

    if (iface->mousePressed(0))
    {
        if (inBoard)
        {
            selectCell(cr, cc);
        }
    }

    if (iface->mousePressed(1))
    {
        clearSelect();
    }
}

void CustomCore::draw()
{
    ScopedProfile prof(profiler, "CustomCore: Draw");

    //beginFrame() sets the OpenGL context to the proper initial state
    beginFrame();

    {
        ScopedProfile prof(profiler, "2D");

        std::uniform_real_distribution<float> shake(-screenShake, screenShake);
        screenShake *= 0.9f;

        float s1 = shake(rng);
        float s2 = shake(rng);

        Camera cam;
        cam.ortho(-40.f+s1, 40.f+s1, -30.f+s2, 30.f+s2, -1.f, 1.f);
        applyCam(cam);

        if (flashing.timer <= 0)
        {
            drawBoard();
            drawLinks();
            drawScore();
        }
        else
        {
            drawFlash();
        }
    }

    //endFrame() swaps the buffer to the screen
    endFrame();
}

void CustomCore::drawBoard()
{
    Transform mat;

    constexpr float panelSize = 5.f;

    mat.translate(Vec3{-40.f, 30.f, 0.f});
    mat.scale(Vec3{panelSize, panelSize, 1.f});
    mat.translate(Vec3{0.6f, -1.6f, 0.f});

    for (int r=0; r<rules.boardHeight; ++r)
    {
        for (int c=0; c<rules.boardWidth; ++c)
        {
            mat.push();

            Vec3 toPanel{c*1.2f, r*-1.2f, 0.f};

            mat.translate(toPanel);
            modelMatrix(mat);

            Color pc = Color::NONE;

            if (selection.on && selection.r == r && selection.c == c)
            {
                pc = Color::WHITE;
            }

            colors[int(pc)].bind(0);
            panel.draw();

            Color& cell = cellAt(r, c);

            if ((swapAnim.c[0].r!=r || swapAnim.c[0].c!=c)
             && (swapAnim.c[1].r!=r || swapAnim.c[1].c!=c)
             && cell != Color::NONE)
            {
                colors[int(cell)].bind(0);

                if (spawning.find(&cell) != end(spawning))
                {
                    mat.push();

                    mat.scale(Vec3{spawnScale, spawnScale, 1.f});
                    modelMatrix(mat);

                    piece.draw();

                    mat.pop();
                    modelMatrix(mat);
                }
                else
                {
                    piece.draw();
                }
            }

            if (r>=2 && r<6
             && (c<2 || c>=8))
            {
                colors[int(Color::WHITE)].bind(0);
                diamond.draw();
            }

            mat.pop();
        }
    }

    if (spawning.size() > 0)
    {
        spawnScale *= 1.25f;

        if (spawnScale >= 1.f) spawning.clear();
    }

    if (swapAnim.c[0].r != -1) //lazy check
    {
        Vec3 toRotCent{swapAnim.px*1.2f, swapAnim.py*-1.2f, 0.f};
        for (int i=0; i<2; ++i)
        {
            Loc& l = swapAnim.c[i];
            Color col = cellAt(swapAnim.c[1-i].r, swapAnim.c[1-i].c);

            colors[int(col)].bind(0);

            mat.push();

            Vec3 toPanel{l.c*1.2f, l.r*-1.2f, 0.f};

            float rate = (2.f-sind(swapAnim.deg))/2.f;

            mat.translate(toRotCent);
            mat.rotate(swapAnim.deg, Vec3{0.f, 0.f, 1.f});
            mat.translate((-toRotCent+toPanel)*rate);
            modelMatrix(mat);

            piece.draw();

            swapAnim.deg += 5.f;
            if (swapAnim.deg >= 180.f)
            {
                swapAnim.c[0].r = -1;
                swapAnim.c[0].c = -1;
                swapAnim.c[1].r = -1;
                swapAnim.c[1].c = -1;
            }

            mat.pop();
        }
    }
}

void CustomCore::drawScore()
{
    Transform mat;

    mat.translate(Vec3{20.f, 0.f, 0.f});
    mat.scale(Vec3{0.2f, 0.2f, 1.f});
    mat.translate(Vec3{4.f, 24.f+8.f*pointList.size(), 0.f});

    auto drawString = [&](const std::string& in)
    {
        mat.push();
        mat.scale(Vec3{0.5f, 0.5f, 1.f});

        for (char c : in)
        {
            modelMatrix(mat);
            font.draw(c/16, c%16);
            mat.translate(Vec3{16.f, 0.f, 0.f});
        }

        mat.pop();
    };

    auto drawnum = [&](int num)
    {
        mat.push();
        mat.translate(Vec3{4.f, 0.f, 0.f});
        std::stringstream ss;
        ss << std::setw(12) << std::setfill(' ') << num;
        drawString(ss.str());
        mat.pop();
    };

    auto newline = [&]
    {
        mat.translate(Vec3{0.f, -8.f, 0.f});
    };

    auto drawval = [&](const std::string& name, int val)
    {
        drawString(name);
        newline();

        drawnum(val);
        newline();
    };

    for (int i : pointList)
    {
        drawnum(i);
        newline();
    }

    drawval("Score:",      score);
    drawval("Last Score:", prevScore);
    drawval("High Score:", highScore);

    for (int i=0; i<rules.numColors; ++i)
    {
        mat.push();

        mat.translate(Vec3{4.f, -8.f, 0.f});
        mat.push();
        mat.scale(Vec3{12.f, 12.f, 1.f});
        colors[i+1].bind(0);
        modelMatrix(mat);
        piece.draw();
        mat.pop();
        std::stringstream ss;
        ss << colorVal(Color(i+1));
        drawString(ss.str());
        mat.pop();

        mat.translate(Vec3{20.f, 0.f, 0.f});
        if (i%5 == 0 && i>0) mat.translate(Vec3{-120.f, -20.f, 0.f});
    }
}

void CustomCore::drawLinks()
{
    if (hoverCell.r<0 || hoverCell.r>=8
     || hoverCell.c<0 || hoverCell.c>=10)
    {
        return;
    }

    Transform mat;

    constexpr float panelSize = 5.f;

    Color rain = cellAt(hoverCell.r, hoverCell.c);
    colors[int(rain)].bind(0);

    mat.translate(Vec3{-40.f, 30.f, 0.f});
    mat.scale(Vec3{panelSize, panelSize, 1.f});
    mat.translate(Vec3{0.6f, -1.6f, 0.f});

    for (const Loc& l : hoverGroup)
    {
        mat.push();
        mat.translate(Vec3{l.c*1.2f, l.r*-1.2f, 0.f});

        if (hoverGroup.find({l.r+1, l.c}) != end(hoverGroup))
        {
            mat.push();
            mat.translate(Vec3{0.f, -0.6f, 0.f});
            modelMatrix(mat);
            diamond.draw();
            mat.pop();
        }

        if (hoverGroup.find({l.r, l.c+1}) != end(hoverGroup))
        {
            mat.push();
            mat.translate(Vec3{0.6f, 0.f, 0.f});
            modelMatrix(mat);
            diamond.draw();
            mat.pop();
        }

        mat.pop();
    }
}

void CustomCore::drawFlash()
{
    Transform mat;
    mat.scale(Vec3{80.f, 60.f, 1.f});
    modelMatrix(mat);

    Color fc = flashing.color;

    colors[int(fc)].bind(0);

    panel.draw();

    if (flashing.color == Color::RED) flashing.color = Color::WHITE;
    else flashing.color = Color::RED;

    --flashing.timer;
}

CustomCore::Color& CustomCore::cellAt(int r, int c)
{
    return board[r*10+c];
}

void CustomCore::selectCell(int r, int c)
{
    if (selection.on)
    {
        if (r == selection.r && c == selection.c) scoreCell(r, c);
        else swapCells(r, c, selection.r, selection.c);
        selection.on = false;
    }
    else
    {
        selection.r = r;
        selection.c = c;
        selection.on = true;
    }
}

void CustomCore::clearSelect()
{
    selection.on = false;
}

void CustomCore::swapCells(int r1, int c1, int r2, int c2, bool force)
{
    Color& cell1 = cellAt(r1, c1);
    Color& cell2 = cellAt(r2, c2);

    if (cell1 == Color::NONE || cell2 == Color::NONE) return flash();
    if (!force && cell1 == cell2) return flash();

    std::swap(cell1, cell2);

    swapAnim.c[0].r = r1;
    swapAnim.c[0].c = c1;
    swapAnim.c[1].r = r2;
    swapAnim.c[1].c = c2;
    swapAnim.deg = 0.f;
    swapAnim.px = (c1+c2)/2.f;
    swapAnim.py = (r1+r2)/2.f;

    return spawn(5);
}

void CustomCore::scoreCell(int r, int c)
{
    Color& cell = cellAt(r, c);
    if (cell == Color::NONE) return flash();

    std::set<Loc> group;

    bool isScoring = getGroup(r, c, cell, group);

    if (!isScoring)       return flash();
    if (group.size() < 5) return flash();

    int s = colorVal(cell) * group.size();

    score += s;
    if (pointList.size() == 10) pointList.erase(begin(pointList));
    pointList.push_back(s);

    for (const Loc& l : group) cellAt(l.r, l.c) = Color::NONE;

    shake_n_bake(s);

    return spawn(3);
}

bool CustomCore::getGroup(int r, int c, Color k, std::set<Loc>& s)
{
    Color& cell = cellAt(r, c);
    if (cell != k) return false;

    if (!s.insert({r, c}).second) return false;

    bool isScoring = (r>=2 && r<6 && (c<2 || c>=8));

    int a = rules.boardHeight-1;
    int b = rules.boardWidth-1;

    if (r>0) isScoring += getGroup(r-1, c, k, s);
    if (r<a) isScoring += getGroup(r+1, c, k, s);
    if (c>0) isScoring += getGroup(r, c-1, k, s);
    if (c<b) isScoring += getGroup(r, c+1, k, s);

    return isScoring;
}

void CustomCore::flash()
{
    flashing.timer = 10;
    flashing.color = Color::RED;
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

    spawning.clear();
    spawnScale = 0.05f;

    for (int i=0; i<n; ++i)
    {
        *nones[i] = Color(pick(rng));
        spawning.insert(nones[i]);
    }
}

void CustomCore::galoSengen()
{
    Loc scoreLoc;
    int scoreSize = 0;
    int scoreVal = 0;

    for (int r=2; r<6; ++r)
    {
        auto doit = [&](int c)
        {
            Color& cell = cellAt(r, c);
            if (cell == Color::NONE) return;

            std::set<Loc> group;
            getGroup(r, c, cell, group);

            int cscore = colorVal(cellAt(r, c));

            if (cscore*group.size() > scoreVal)
            {
                scoreLoc.r = r;
                scoreLoc.c = c;
                scoreSize = group.size();
                scoreVal = cscore*group.size();
            }
        };

        for (int c=0; c<2; ++c) doit(c);
        for (int c=8; c<10; ++c) doit(c);
    }

    if (scoreSize >= 5) return scoreCell(scoreLoc.r, scoreLoc.c);

    auto weight = [&](const Loc& l)
    {
        int s = int(cellAt(l.r, l.c));
        int rval = 0;

        std::set<Color> nbor;

        auto comp = [&](Color c, int w)
        {
            int i = int(c);
            if (i==s) rval += w;
            else nbor.insert(c);
        };

        int x = 3;

        for (int i=-x; i<=x; ++i)
        {
            for (int j=-x; j<=x; ++j)
            {
                if (i==0 && j==0) continue;

                int r = l.r+i;
                int c = l.c+j;

                if (r<0 || r>=rules.boardHeight
                 || c<0 || c>=rules.boardWidth)
                {
                    continue;
                }

                int w = 1+2*x-(((i<0)?-i:i)+((j<0)?-j:j));

                comp(cellAt(r, c), w);
            }
        }

        for (Color i : nbor) rval -= int(i)/2;

        return rval;
    };

    auto pred = [&](const Loc& a, const Loc& b)
    {
        return (weight(a) < weight(b));
    };

    std::vector<Loc> swappables;

    for (int r=0; r<rules.boardHeight; ++r)
    {
        for (int c=0; c<rules.boardHeight; ++c)
        {
            if (cellAt(r, c) != Color::NONE) swappables.push_back({r, c});
        }
    }

    std::shuffle(begin(swappables), end(swappables), rng);
    std::sort(begin(swappables), end(swappables), pred);

    const Loc& loc1 = swappables[0];

    int i = 1;

    while (cellAt(loc1.r, loc1.c) == cellAt(swappables[i].r, swappables[i].c))
    {
        ++i;
        if (i >= swappables.size()-1)
        {
            i = swappables.size()-1;
            break;
        }
    }

    const Loc& loc2 = swappables[i];


    return swapCells(loc1.r, loc1.c, loc2.r, loc2.c, true);
}

void CustomCore::shake_n_bake(int s)
{
    screenShake += s/10.f;
}

int CustomCore::colorVal(Color c) const
{
    return int(c)-int(Color::NONE)+1;
}

void CustomCore::gameOver()
{
    prevScore = score;
    if (score > highScore) highScore = score;
    score = 0;
    for (Color& c : board) c = Color::NONE;
    spawn(rules.swapSpawn);
    pointList.clear();
}
