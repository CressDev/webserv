#include "Parser.hpp"
#include <stdexcept>
#include <iostream>
#include <sstream>

int	Parser::chooseState(std::vector<std::string>& tokens)
{
	int prev = 0;
	int  is_serv = 0   ;
	int  is_loc = 0;

	for (size_t i = 0; i < tokens.size(); i++)
	{
		int pos = 0;
		if (tokens[i] == "server")
			pos = 1;
		else if (tokens[i] == "{" && prev == 2)
		{
			pos = 2;
			is_serv = 1;
		}
		else if (tokens[i] == "{" && prev == 6)
		{
			pos = 6;
			is_loc = 1;
		}
		else if (tokens[i] == "}")
		{
			if ((prev == 3 || prev == 8 || prev == 11) && is_serv)
			{
				pos = 3;
				if (!is_loc)
					is_serv = 0;
			}
			if ((prev == 7 || prev == 11) && is_loc)
			{
				pos = 7;
				is_loc = 0;
			}
		}
		else if (tokens[i] == "location")
			pos = 4;
		else if (isConfigWord(tokens[i]))
			pos = 8;
		else if (tokens[i] == ";")
			pos = 10;
		else
		{
			pos = 9;
			if (prev == 5)
			{
				if (tokens[i] == "=" && tokens[i + 1] != "{")
					i++;
				else if (tokens[i] == "=" && tokens[i + 1] == "{")
				{
					std::cerr << "Syntax error: " << tokens[i] << "\n";
					return 1;
				}
				pos = 5;
			}
		}
		prev = getState(prev, pos);
		if (prev == 1)
		{
			std::cerr << "Syntax error\n";
			return 1;
		}
	}
	return 0;
}

void	Parser::rmComments(std::ifstream& config_file)
{
	std::string line;

	while (std::getline(config_file, line))
	{
		if (line.empty())
			this->_config_file = this->_config_file + " ";
		if (!line.empty())
		{
			size_t  pos = line.find('#');
			if (pos != std::string::npos)
				line = line.substr(0, pos);
			std::string cleanLine = rtrim(line);
			if (!cleanLine.empty())
				this->_config_file += cleanLine + " ";
		}
	}
}

void	Parser::listenParser(std::vector<std::string>::iterator& it, Server& server)
{
	if (*(it + 2) != ";")
		throw std::runtime_error("Parser: configuration error");
	++it;
	std::string t = *it;
	size_t  pos = t.find(':');
	if (pos != std::string::npos)
	{
		std::string ip = t.substr(0, pos);
		std::string port = t.substr(pos + 1);
			
		if (ip.size() == 0 || port.size() == 0 || !check_ipv4(ip) || !check_port(port))
		{
			throw std::runtime_error("Parser: configuration error");
		}
		if (ip == "localhost")
			ip = "127.0.0.1";
		++it;
		_listens.insert(std::pair<std::string, std::string>(ip, port));
		server.addListen(ip, port);
	}
	else
	{
		bool	ip = false;
		bool	port = false;
		std::string	ipPort;
			
		ip = check_ipv4(t);
		port = check_port(t);
		if (ip)
		{
			if (t == "localhost")
				t = "127.0.0.1";
			ipPort = "80";
			server.addListen(t, ipPort);
			++it;
			return ;
		}
		if (port)
		{
			ipPort = "127.0.0.1";
			server.addListen(ipPort, t);
			++it;
			return ;
		}
		throw std::runtime_error("Parser: configuration error");
	}
}

void	Parser::clientmaxbodysizeParser(std::vector<std::string>::iterator& it,Server& server)
{
	std::stringstream   parse;
	long long           n;
	char                c;

	if (*(it + 2) != ";")
		throw std::runtime_error("Parser: configuration error");
	++it;
	if (checkclientmaxbodysize(*it) == false)
		throw std::runtime_error("Parser: configuration error");
	parse << *it;
	parse >> n;
	parse >> c;
	if (n < 0 || static_cast<size_t>(n) > std::numeric_limits<size_t>::max())
		throw std::runtime_error("Parser: configuration error");
	server.setClientMaxBodySize(n, c);
}

void	Parser::autoindexParser(std::vector<std::string>::iterator& it, Server& server)
{
	if (*(it + 2) != ";")
		throw std::runtime_error("Parser: configuration error");
	++it;
	if (*it != "on" && *it != "off")
		throw std::runtime_error("Parser: configuration error");
	if (*it == "on")
		server.setAutoindex(true);
	else
		server.setAutoindex(false);
	++it;
}

void	Parser::allowedParser(std::vector<std::string>::iterator& it, Server& server)
{
	std::vector<std::string>    methods;

	++it;
	while (*it != ";")
	{
		if (*it != "GET" && *it != "POST" && *it != "DELETE")
			throw std::runtime_error("Parser: configuration error");
		else
			methods.push_back(*it);
		++it;
	}
	server.setAllowedMethods(methods);
}

void	Parser::serverNameParser(std::vector<std::string>::iterator& it,Server& server)
{
	std::string		str;
	int				prev = 0;
	int				pos;

	it++;
	while (*it != ";")
	{
		size_t	i = 0;
		str = *it;
		prev = 0;
		if (!check_ipv4(str))
		{
			while (i < str.length())
			{
				pos = 0;
				if (str[i] == '.')
					pos = 1;
				else if (str[i] == 'c')
					pos = 2;
				else if (str[i] == 'o')
					pos = 3;
				else if (str[i] == 'm')
					pos = 4;
				else if (std::isalpha(str[i]) && str[i] != '*')
					pos = 5;
				else
					pos = 6;
				prev = getServerNameState(prev, pos);
				if (prev == 1)
					throw std::runtime_error("Parser: configuration error");
				i++;
			}
			if (prev != 6)
				throw std::runtime_error("Parser: configuration error");
		}
		else
			throw std::runtime_error(std::string("Parser: unknown token inside location block: ") + *it);
		++it;
	}
	server.setServerName(*(it - 1));
}
