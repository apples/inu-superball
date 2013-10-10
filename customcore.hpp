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
        , MAGENTA
        , BLUE
        , YELLOW
        , GREEN
        , RED
        , CYAN
        , VIOLET
        , BLACK

        , WHITE

        , COUNT
    };

    class Loc
    {
    public:
        int r;
        int c;

        bool operator<(const Loc& in) const;
        bool operator==(const Loc& in) const;
        bool operator!=(const Loc& in) const;
    };

    CustomCore(const RenderParams &params);

    void tick();
    void draw();

    void drawBoard();
    void drawScore();
    void drawLinks();
    void drawFlash();

    Color& cellAt(const Loc& l);

    void selectCell(const Loc& l);
    void clearSelect();

    void swapCells(const Loc& a, const Loc& b, bool force=false);
    void scoreCell(const Loc& l);
    bool getGroup(const Loc& l, Color k, std::set<Loc>& s);

    void flash();

    void spawn(int n);

    void executeAI();

    void shake_n_bake(int s);

    int colorVal(Color c) const;

    void gameOver();

    bool isScoreTile(const Loc& l) const;

private:
    Inugami::Texture     colors[int(Color::COUNT)];
    Inugami::Spritesheet font;

    Inugami::Mesh    panel;
    Inugami::Mesh    piece;
    Inugami::Mesh    diamond;

    struct
    {
        int numColors;
        int swapSpawn;
        int scoreSpawn;
        int width;
        int height;
        int minScore;
    } rules;

    std::vector<Color> board;

    struct {Loc loc; bool on;} selection;
    struct {int timer; Color color;} flashing;

    int score;

    Loc hoverCell;
    std::set<Loc> hoverGroup;
    bool hoverScore;

    bool isGameOver;

    std::mt19937 rng;

    struct {float px, py, deg; Loc c[2];} swapAnim;
    std::set<Color*> spawning;
    float spawnScale;

    float screenShake;

    int prevScore;
    int highScore;

    std::vector<int> pointList;

    std::set<Loc> scoreZone;
};

#endif // CUSTOMCORE_H
