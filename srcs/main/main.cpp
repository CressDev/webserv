#include "ServerSocket.hpp"
#include "main/print_helpers.hpp"
#include "Parser.hpp"
#include "Server.hpp"
#include "Client.hpp"
#include "main/signals.hpp"
#include "main/event_loop.hpp"
#include <poll.h>
#include <map>
#include <vector>
#include <string>
#include <typeinfo>
#include <cerrno>
#include <cstring>
#include <iostream>


int main (int argc, char** argv, char** env)
{
	if (argc != 2)
	{
		std::cerr << "Invalid arguments\n";
		return 1;
	}
	try
	{
		std::vector<Server> servers;
		Parser              parser(argv[1], servers);
		(void)parser;
		//PRINTEO DE LOS SERVER PARSEADOS
		printParsedServers(servers);

		ServerSocket sockman;
		if (!sockman.createListeners(servers))
		{
			std::cerr << "No listeners created\n";
			return 1;
		}
		std::vector<struct pollfd> pollfds = sockman.getPollfds();
		int sigfd = setupSignalFd(pollfds);
		std::vector<Client> clients;
		std::map<int, int> cgiFdToClientIdx;
		int rc = runEventLoop(sockman, servers, pollfds, sigfd, clients, cgiFdToClientIdx, env);
		if (rc != 0)
			return rc;
	}
	catch (const std::exception& e)
	{
		std::cerr << "Uncaught exception (" << typeid(e).name() << "): " << e.what() << std::endl;
		return 1;
	}
}
