#ifndef PRINT_HELPERS_HPP
#define PRINT_HELPERS_HPP

#include "Server.hpp"
#include <vector>

void printVector(const std::vector<std::string>& v);
void printLocationConfig(const LocationConfig& loc, std::vector<LocationConfig>::size_type j);
void printServerConfig(const ServerConfig& cfg, std::vector<Server>::size_type i);
void printParsedServers(const std::vector<Server>& servers);

#endif // PRINT_HELPERS_HPP
