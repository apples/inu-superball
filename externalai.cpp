#include "externalai.hpp"

#include "meta.hpp"

#include "libexecstream/exec-stream.h"

#include <sstream>

ExternalAI::ExternalAI(std::string en, int w, int h, int ms, std::string c)
    : execname(en)
    , args()
{
    std::stringstream ss;
    const char* s = " ";
    ss << h  << s;
    ss << w  << s;
    ss << ms << s;
    ss << c  << s;
    args = ss.str();
}

std::string ExternalAI::play(std::vector<std::string> const& board)
{
    exec_stream_t process;
    process.set_wait_timeout(exec_stream_t::s_all, 1000*30);

    try
    {
        process.start(execname, args);

        for (auto&& line : board)
        {
            process.in() << line << "\n";
            logger->log<1>(line);
        }
        process.in() << "\n";
        process.in().flush();
        process.close_in();

        std::stringstream ss;
        std::string word;
        while (process.out() >> word) ss << word << " ";

        return ss.str();
    }
    catch (exec_stream_t::error_t const& e)
    {
        process.kill();
        logger->log<1>(e.what());
        logger->log<1>("Player stderr:");
        std::string line;
        while (getline(process.err(), line)) logger->log<1>(line);
        return "";
    }
}
