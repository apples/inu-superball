#include "action.hpp"

#include <sstream>

Action::~Action()
{}

Swap::Swap(Loc a, Loc b)
    : data{a, b}
{}

std::string Swap::str() const
{
    std::stringstream ss;
    ss << "SWAP ";
    ss << data[0].r << " " << data[0].c << " ";
    ss << data[1].r << " " << data[1].c;
    return ss.str();
}

Score::Score(Loc a)
    : data(a)
{}

std::string Score::str() const
{
    std::stringstream ss;
    ss << "SCORE ";
    ss << data.r << " " << data.c;
    return ss.str();
}
