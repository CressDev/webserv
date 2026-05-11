#include "Parser.hpp"
#include <sstream>
#include <limits>
#include <cctype>

bool	Parser::check_ipv4(std::string t)
{
	int prev = 0;
	int pos;
	int len = 0;
	size_t i = 0;

	if (t == "localhost")
		return (true);
	for (; i < t.length(); i++)
	{
		pos = 0;
		if (std::isdigit(t[i]) && t[i] == '0')
		{
			pos = 1;
			len++;
		}
		else if (std::isdigit(t[i]) && t[i] == '1')
		{
			pos = 2;
			len++;
		}
		else if (std::isdigit(t[i]) && t[i] == '2')
		{
			pos = 3;
			len++;
		}
		else if (std::isdigit(t[i]) && ((t[i] == '3') || (t[i] == '4')))
		{
			pos = 4;
			len++;
		}
		else if (std::isdigit(t[i]) && t[i] == '5')
		{
			pos = 5;
			len++;
		}
		else if (t[i] == '.')
		{
			pos = 6;
			len = 0;
		}
		else if (std::isdigit(t[i]) && t[i] > '5' && t[i] <= '9')
		{
			pos = 7;
			len++;
		}
		prev = getIPV4State(prev, pos);
		if (prev == 0 || len > 3)
			return (false);
	}
	if (!std::isdigit(t[i - 1]))
		return false;
	return (true);
}

bool	Parser::check_port(std::string t)
{
	std::stringstream       parse;
	long long               n;
	std::string::iterator   it = t.begin();
	std::string::iterator   end = t.end();

	while (it != end)
	{
		if (!std::isdigit(*it))
			return false;
		++it;
	}
	parse << t;
	parse >> n;
	if (n > 65535 || n < 0)
		return false;
	return true;
}

bool	Parser::checkclientmaxbodysize(std::string t)
{
	int prev = 0;
	int pos;
	std::string::iterator   it = t.begin();
	std::string::iterator   end = t.end();

	for (; it != end; ++it)
	{
		pos = 0;
		if (std::isdigit(*it))
			pos = 1;
		else if (*it == 'g' || *it == 'G' || *it == 'k' || *it == 'K' || *it == 'm' || *it == 'M')
			pos = 2;
		prev = getClientMaxBodySizeState(prev, pos);
		if (prev == 0)
			return false;
	}
	return true;
}
