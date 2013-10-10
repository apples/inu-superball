#ifndef EXTERNALAI_HPP
#define EXTERNALAI_HPP

#include <string>
#include <vector>

class ExternalAI
{
    std::string execname;
    std::string args;

public:
    ExternalAI(std::string en, int w, int h, int ms, std::string c);
    std::string play(std::vector<std::string> const& board);
};

#endif // EXTERNALAI_HPP
