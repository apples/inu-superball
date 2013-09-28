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
        , {{1, 1, {255,   0, 255, 255}}, false, false} // PURPLE
        , {{1, 1, {  0,   0, 255, 255}}, false, false} // BLUE
        , {{1, 1, {255, 255,   0, 255}}, false, false} // YELLOW
        , {{1, 1, {255,   0,   0, 255}}, false, false} // RED
        , {{1, 1, {  0, 255,   0, 255}}, false, false} // GREEN
        , {{1, 1, {255, 255, 255, 255}}, false, false} // WHITE
    }
    , font(Image::fromPNG("data/font.png"), 8, 8)
    , panel(Geometry::fromRect(1.f, 1.f))
    , piece(Geometry::fromDisc(0.9f, 0.9f, 16))
    , diamond(Geometry::fromDisc(0.3f, 0.3f, 4))

    , board(80, Color::NONE)

    , selection{-1, -1, false}
    , flashing{-1, Color::NONE}

    , score(0)

    , hoverCell{-1, -1}
    , hoverGroup()
    , hoverScore(false)
{
    ScopedProfile prof(profiler, "CustomCore: Constructor");

    logger->log<5>("Adding callbacks...");
    addCallback([&]{tick();draw();}, 1200.0);

    setWindowTitle("Inu SuperBall", true);

    spawnFive();
}

void CustomCore::tick()
{
    ScopedProfile prof(profiler, "CustomCore: Tick");

    //Keybinds can be stored in proxies
    auto keyESC   = iface->getProxy(0_ivkFunc);
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
        for (Color& c : board) c = Color::NONE;
        spawnFive();
        score = 0;
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

    if (cc>=0 && cc<10
     && cr>=0 && cr<8)
    {
        Color cat = cellAt(cr, cc);

        if (cat != Color::NONE)
        {
            hoverScore = getGroup(cr, cc, cat, hoverGroup);
        }
    }

    if (iface->mousePressed(0))
    {
        if (cc>=0 && cc<10
         && cr>=0 && cr<8)
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

        Camera cam;
        cam.ortho(-40.f, 40.f, -30.f, 30.f, -1.f, 1.f);
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

    for (int r=0; r<8; ++r)
    {
        mat.push();
        for (int c=0; c<10; ++c)
        {
            modelMatrix(mat);

            Color pc = Color::NONE;

            if (selection.on && selection.r == r && selection.c == c)
            {
                pc = Color::WHITE;
            }

            colors[int(pc)].bind(0);
            panel.draw();

            Color cell = cellAt(r, c);
            if (cell != Color::NONE)
            {
                colors[int(cell)].bind(0);
                piece.draw();
            }

            if (r>=2 && r<6
             && (c<2 || c>=8))
            {
                colors[int(Color::WHITE)].bind(0);
                diamond.draw();
            }

            mat.translate(Vec3{1.2f, 0.f, 0.f});
        }
        mat.pop();
        mat.translate(Vec3{0.f, -1.2f, 0.f});
    }
}

void CustomCore::drawScore()
{
    Transform mat;

    mat.translate(Vec3{20.f, 0.f, 0.f});
    mat.scale(Vec3{0.2f, 0.2f, 1.f});
    mat.translate(Vec3{4.f, 8.f, 0.f});

    auto drawString = [&](const std::string& in)
    {
        mat.push();

        for (char c : in)
        {
            modelMatrix(mat);
            font.draw(c/16, c%16);
            mat.translate(Vec3{8.f, 0.f, 0.f});
        }

        mat.pop();
    };

    drawString("Score:");
    mat.translate(Vec3{4.f, -8.f, 0.f});

    std::stringstream ss;
    ss << std::setw(12) << std::setfill(' ') << score;
    drawString(ss.str());
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

void CustomCore::swapCells(int r1, int c1, int r2, int c2)
{
    Color& cell1 = cellAt(r1, c1);
    Color& cell2 = cellAt(r2, c2);

    if (cell1 == Color::NONE || cell2 == Color::NONE) return flash();
    if (cell1 == cell2) return flash();

    std::swap(cell1, cell2);

    return spawnFive();
}

void CustomCore::scoreCell(int r, int c)
{
    Color& cell = cellAt(r, c);

    if (!hoverScore)           return flash();
    if (hoverGroup.size() < 5) return flash();

    score += (int(cell)-int(Color::PURPLE)+2) * hoverGroup.size();

    for (const Loc& l : hoverGroup) cellAt(l.r, l.c) = Color::NONE;

    return spawnFive();
}

bool CustomCore::getGroup(int r, int c, Color k, std::set<Loc>& s)
{
    Color& cell = cellAt(r, c);
    if (cell != k) return false;

    if (!s.insert({r, c}).second) return false;

    bool isScoring = (r>=2 && r<6 && (c<2 || c>=8));

    if (r>0) isScoring += getGroup(r-1, c, k, s);
    if (r<7) isScoring += getGroup(r+1, c, k, s);
    if (c>0) isScoring += getGroup(r, c-1, k, s);
    if (c<9) isScoring += getGroup(r, c+1, k, s);

    return isScoring;
}

void CustomCore::flash()
{
    flashing.timer = 10;
    flashing.color = Color::RED;
}

void CustomCore::spawnFive()
{
    static std::mt19937 rng(std::random_device{}());

    std::vector<Color*> nones;

    for (Color& c : board)
    {
        if (c == Color::NONE) nones.push_back(&c);
    }

    if (nones.size() < 5) return;//return gameover();

    std::shuffle(nones.begin(), nones.end(), rng);

    std::uniform_int_distribution<int> pick(1,5);

    for (int i=0; i<5; ++i) *nones[i] = Color(pick(rng));
}
