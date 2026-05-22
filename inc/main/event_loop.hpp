#ifndef EVENT_LOOP_HPP
#define EVENT_LOOP_HPP

#include "ServerSocket.hpp"
#include "Server.hpp"
#include "Client.hpp"
#include <vector>
#include <map>

int runEventLoop(ServerSocket& sockman,
				std::vector<Server>& servers,
				std::vector<struct pollfd>& pollfds,
				int sigfd,
				std::vector<Client>& clients,
				std::map<int, int>& cgiFdToClientIdx,
				char** env);

#endif // EVENT_LOOP_HPP
