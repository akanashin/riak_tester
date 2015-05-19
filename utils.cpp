#include "utils.hpp"

#include <sstream>
#include <stdexcept>

std::vector<std::string> const split(const std::string &s, char delim)
{   
    std::vector<std::string> elems;
    std::stringstream ss(s);
    std::string item;
    while (std::getline(ss, item, delim)) {
        elems.push_back(item);
    }
    return elems;
}

bool validate_address(std::string const& addr, std::string *a_host, int *a_port)
{
    std::vector<std::string> pair = split(addr, ':');

    // there must be 2 non-empty parts
    if ( pair.size() != 2
         || pair[0].empty()
         || pair[1].empty()
        )
        return false;

    try{
        int portnum = std::stoi(pair[1]);

        // port cannot be empty
        if (portnum == 0)
            return false;

        // store value if it is Ok
        if (a_host)
            *a_host = pair[0];
        if (a_port)
            *a_port = portnum;

        return true;
    }
    catch (std::invalid_argument const&) {}
    catch (...) {}

    return false;
}
