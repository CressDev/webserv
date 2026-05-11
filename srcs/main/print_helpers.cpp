#include "main/print_helpers.hpp"
#include <iostream>
#include <map>

void printVector(const std::vector<std::string>& v)
{
	for (std::vector<std::string>::size_type i = 0; i < v.size(); ++i)
	{
		std::cout << v[i];
		if (i + 1 < v.size()) std::cout << ", ";
	}
}

void printLocationConfig(const LocationConfig& loc, std::vector<LocationConfig>::size_type j)
{
	std::cout << "  -- Location #" << j << " --\n";
	std::cout << "    Path: " << loc.path << "\n";
	std::cout << "    Root: " << loc.root << "\n";
	std::cout << "    Index: ";
	printVector(loc.index);
	std::cout << "\n";
	std::cout << "    Allowed methods: ";
	printVector(loc.allowed_methods);
	std::cout << "\n";
	std::cout << "    CGI:\n";
	for (std::map<std::string, std::string>::const_iterator it = loc.cgi.begin(); it != loc.cgi.end(); ++it)
		std::cout << "      " << it->first << " -> " << it->second << "\n";
	std::cout << "    Error pages:\n";
	for (std::map<std::pair<int, int>, std::string>::const_iterator it = loc.error_pages.begin(); it != loc.error_pages.end(); ++it)
		std::cout << "      " << it->first.first << " | " << it->first.second << " -> " << it->second << "\n";
	std::cout << "    Autoindex: " << loc.autoindex << "\n";
}

void printServerConfig(const ServerConfig& cfg, std::vector<Server>::size_type i)
{
	std::cout << "=== Server #" << i << " ===\n";

	std::cout << "Listen:\n";
	for (std::multimap<std::string, std::string>::const_iterator it = cfg.listen.begin(); it != cfg.listen.end(); ++it)
		std::cout << "  " << it->first << ":" << it->second << "\n";

	std::cout << "CGI:\n";
	for (std::map<std::string, std::string>::const_iterator it = cfg.cgi.begin(); it != cfg.cgi.end(); ++it)
		std::cout << "  " << it->first << " -> " << it->second << "\n";

	std::cout << "Index: ";
	printVector(cfg.index);
	std::cout << "\n";

	std::cout << std::boolalpha;
	std::cout << "Autoindex: " << cfg.autoindex << "\n";
	std::cout << "Root: " << cfg.root << "\n";
	std::cout << "Server name: " << cfg.server_name << "\n";
	std::cout << "Client max body size: " << cfg.client_max_body_size << "\n";

	std::cout << "Allowed methods: ";
	printVector(cfg.allowed_methods);
	std::cout << "\n";

	std::cout << "Error pages:\n";
	for (std::map<std::pair<int, int>, std::string>::const_iterator it = cfg.error_pages.begin(); it != cfg.error_pages.end(); ++it)
		std::cout << "  " << it->first.first << " | " << it->first.second << " -> " << it->second << "\n";

	std::cout << "Locations (" << cfg.locations.size() << "):\n";
	for (std::vector<LocationConfig>::size_type j = 0; j < cfg.locations.size(); ++j)
		printLocationConfig(cfg.locations[j], j);

	std::cout << std::noboolalpha << "======================\n\n";
}

void printParsedServers(const std::vector<Server>& servers)
{
	for (std::vector<Server>::size_type i = 0; i < servers.size(); ++i)
		printServerConfig(servers[i].getConfig(), i);
}
