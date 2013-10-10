#ifndef ACTION_HPP
#define ACTION_HPP

#include "loc.hpp"

#include <sstream>
#include <string>

class Action
{
public:
    virtual ~Action();
    virtual std::string str() const = 0;
};

class Swap
    : public Action
{
    Loc data[2];

public:
    Swap(Loc a, Loc b);
    virtual std::string str() const;
};

class Score
    : public Action
{
    Loc data;

public:
    Score(Loc a);
    virtual std::string str() const;
};

#endif // ACTION_HPP
