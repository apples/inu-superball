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

#ifndef CUSTOMCORE_H
#define CUSTOMCORE_H

#include "inugami/core.hpp"

#include "inugami/animatedsprite.hpp"
#include "inugami/mesh.hpp"
#include "inugami/shader.hpp"
#include "inugami/spritesheet.hpp"
#include "inugami/texture.hpp"

#include <set>

class CustomCore
    : public Inugami::Core
{
public:
    enum class Color
    {
        NONE
        , PURPLE
        , BLUE
        , YELLOW
        , RED
        , GREEN

        , WHITE

        , COUNT
    };

    class Loc
    {
    public:
        int r;
        int c;

        bool operator<(const Loc& in) const;
    };

    CustomCore(const RenderParams &params);

    void tick();
    void draw();

    void drawBoard();
    void drawScore();
    void drawLinks();
    void drawFlash();

    Color& cellAt(int r, int c);

    void selectCell(int r, int c);
    void clearSelect();

    void swapCells(int r1, int c1, int r2, int c2);
    void scoreCell(int r, int c);
    bool getGroup(int r, int c, Color k, std::set<Loc>& s);

    void flash();

    void spawnFive();

private:
    Inugami::Texture     colors[int(Color::COUNT)];
    Inugami::Spritesheet font;

    Inugami::Mesh    panel;
    Inugami::Mesh    piece;
    Inugami::Mesh    diamond;

    std::vector<Color> board;

    struct {int r; int c; bool on;} selection;
    struct {int timer; Color color;} flashing;

    int score;

    Loc hoverCell;
    std::set<Loc> hoverGroup;
    bool hoverScore;
};

#endif // CUSTOMCORE_H
