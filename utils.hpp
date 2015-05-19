#ifndef UTILS_HPP
#define UTILS_HPP

#include <string>
#include <vector>

std::vector<std::string> const split(const std::string &s, char delim);

bool validate_address(std::string const& addr, std::string *a_host = 0, int *a_port = 0);

#endif //UTILS_HPP
