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
#include "externalai.hpp"

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

bool CustomCore::Loc::operator==(const Loc& in) const
{
    return (std::tie(r, c) == std::tie(in.r, in.c));
}

bool CustomCore::Loc::operator!=(const Loc& in) const
{
    return (std::tie(r, c) != std::tie(in.r, in.c));
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

    , rules{5, 5, 3, 10, 8, 5}

    , board(rules.width*rules.height, Color::NONE)

    , selection{{-1, -1}, false}
    , flashing{-1, Color::NONE}

    , score(0)

    , hoverCell{-1, -1}
    , hoverGroup()
    , hoverScore(false)

    , isGameOver(false)

    , rng(time(nullptr))

    , swapAnim{0.f, 0.f, 0.f, {{-1, -1}, {-1, -1}}}
    , spawning()
    , spawnScale(0.f)

    , screenShake(0.f)

    , prevScore(-1)
    , highScore(-1)
    , pointList()

    , scoreZone()
{
    ScopedProfile prof(profiler, "CustomCore: Constructor");

    logger->log<5>("Adding callbacks...");
    addCallback([&]{tick();draw();}, 60.0);

    setWindowTitle("Inu SuperBall", true);

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
        if (!isGameOver) gameOver();
        gameOver();
    }

    if (!isGameOver && keyFast) executeAI();

    if (keyAI.pressed())
    {
        if (isGameOver) gameOver();
        else executeAI();
    }

    if (keyFlood.pressed())
    {
        for (Color& c : board) c = Color::RED;
    }

    auto mPos = iface->getMousePos();
    float mx = mPos.x/10.0;
    float my = mPos.y/10.0-5.f;

    Loc loc{my/6.f, mx/6.f};

    hoverCell.r = loc.r;
    hoverCell.c = loc.c;

    hoverGroup.clear();

    hoverScore = false;

    bool inBoard = (
            loc.c>=0 && loc.c<rules.width
         && loc.r>=0 && loc.r<rules.height
    );

    if (inBoard)
    {
        Color cat = cellAt(loc);

        if (cat != Color::NONE)
        {
            hoverScore = getGroup(loc, cat, hoverGroup);
        }
    }

    if (iface->mousePressed(0))
    {
        if (inBoard)
        {
            selectCell(loc);
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

    for (int r=0; r<rules.height; ++r)
    {
        for (int c=0; c<rules.width; ++c)
        {
            const Loc loc = {r, c};

            mat.push();

            Vec3 toPanel{c*1.2f, r*-1.2f, 0.f};

            mat.translate(toPanel);
            modelMatrix(mat);

            Color pc = isGameOver? Color::BLACK : Color::NONE;

            if (selection.on && selection.loc == loc)
            {
                pc = Color::WHITE;
            }

            colors[int(pc)].bind(0);
            panel.draw();

            Color& cell = cellAt(loc);

            if ((swapAnim.c[0] != loc)
             && (swapAnim.c[1] != loc)
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

            if (isScoreTile(loc))
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

        if (spawnScale >= 1.f)
        {
            spawnScale = 0.05f;
            spawning.clear();
        }
    }

    if (swapAnim.c[0].r != -1) //lazy check
    {
        Vec3 toRotCent{swapAnim.px*1.2f, swapAnim.py*-1.2f, 0.f};
        for (int i=0; i<2; ++i)
        {
            Loc& l = swapAnim.c[i];
            Color col = cellAt(swapAnim.c[1-i]);

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
                swapAnim.deg = 0.f;
                if (!isGameOver)
                {
                    swapAnim.c[0] = {-1, -1};
                    swapAnim.c[1] = {-1, -1};
                }
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
    if (hoverCell.r<0 || hoverCell.r>=rules.height
     || hoverCell.c<0 || hoverCell.c>=rules.width)
    {
        return;
    }

    Transform mat;

    constexpr float panelSize = 5.f;

    Color rain = cellAt(hoverCell);
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

CustomCore::Color& CustomCore::cellAt(const Loc& loc)
{
    return board[loc.r*rules.width+loc.c];
}

void CustomCore::selectCell(const Loc& loc)
{
    if (selection.on)
    {
        if (loc == selection.loc) scoreCell(loc);
        else swapCells(loc, selection.loc);
        selection.on = false;
    }
    else
    {
        selection.loc = loc;
        selection.on = true;
    }
}

void CustomCore::clearSelect()
{
    selection.on = false;
}

void CustomCore::swapCells(const Loc& a, const Loc& b, bool force)
{
    Color& cell1 = cellAt(a);
    Color& cell2 = cellAt(b);

    if (cell1 == Color::NONE || cell2 == Color::NONE) return flash();
    if (!force && cell1 == cell2) return flash();

    std::swap(cell1, cell2);

    swapAnim.c[0] = a;
    swapAnim.c[1] = b;
    swapAnim.deg = 0.f;
    swapAnim.px = (a.c+b.c)/2.f;
    swapAnim.py = (a.r+b.r)/2.f;

    return spawn(5);
}

void CustomCore::scoreCell(const Loc& loc)
{
    Color& cell = cellAt(loc);
    if (cell == Color::NONE) return flash();

    std::set<Loc> group;

    bool isScoring = getGroup(loc, cell, group);

    if (!isScoring)       return flash();
    if (group.size() < 5) return flash();

    int s = colorVal(cell) * group.size();

    score += s;
    if (pointList.size() == 10) pointList.erase(begin(pointList));
    pointList.push_back(s);

    for (const Loc& l : group) cellAt(l) = Color::NONE;

    shake_n_bake(s);

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

void CustomCore::executeAI()
{
    ExternalAI ai("./sb-play", rules.width, rules.height, rules.minScore, "pbygr");

    std::vector<std::string> bored(rules.height, std::string(rules.width, '.'));

    std::map<Color, char> conv = {
          {Color::NONE   , '.'}
        , {Color::MAGENTA, 'p'}
        , {Color::BLUE   , 'b'}
        , {Color::YELLOW , 'y'}
        , {Color::GREEN  , 'g'}
        , {Color::RED    , 'r'}
    };

    std::map<char, char> conv2 = {
          {'.', '*'}
        , {'p', 'P'}
        , {'b', 'B'}
        , {'y', 'Y'}
        , {'g', 'G'}
        , {'r', 'R'}
    };

    for (unsigned r=0; r<rules.height; ++r)
    {
        for (unsigned c=0; c<rules.width; ++c)
        {
            bored[r][c] = conv[cellAt({r, c})];
            if (scoreZone.find({r,c}) != scoreZone.end()) bored[r][c] = conv2[bored[r][c]];
        }
    }

    auto str = ai.play(bored);

    logger->log<3>(str);

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

    return flash();
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
    if (!isGameOver)
    {
        isGameOver = true;
        return;
    }

    isGameOver = false;
    prevScore = score;
    if (score > highScore) highScore = score;
    score = 0;
    for (Color& c : board) c = Color::NONE;
    spawn(rules.swapSpawn);
    pointList.clear();
}

bool CustomCore::isScoreTile(const Loc& loc) const
{
    return (scoreZone.find(loc) != end(scoreZone));
}
