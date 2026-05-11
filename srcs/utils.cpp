#include "utils.hpp"


void	ft_bzero(void *s, size_t n)
{
	size_t          i;
	unsigned char*  buff;

	buff = static_cast<unsigned char*>(s);
	i = 0;
	while (i < n)
	{
		buff[i] = 0;
		++i;
	}
}

std::string rtrim(std::string& line)
{
	size_t end = line.size() - 1;

	if (line.empty())
		return line;
	while ((end > 0 && std::isspace(static_cast<unsigned char>(line[end]))))
		end--;
	if (end == 0 && std::isspace(static_cast<unsigned char>(line[0])))
		return "";
	return line.substr(0, end + 1);
}

std::string trim(std::string& line)
{
	size_t i = 0;

	if (line.empty())
		return line;
	while (line[i] && std::isspace(static_cast<unsigned char>(line[i])))
		i++;
	if (i == line.size())
		return "";
	return line.substr(0, line.size() - i);
}

bool	strIsDigit(std::string str)
{
	std::string::iterator   it = str.begin();
	std::string::iterator   end = str.end();

	while (it != end)
	{
		if (!std::isdigit(*it))
			return false;
		++it;
	}
	return  true;
}

size_t	wordCounter(std::string& line, char splitter)
{
	std::string::iterator	it = line.begin();
	std::string::iterator	end = line.end();
	size_t					n;

	n = 0;
	for (; it != end; ++it)
	{
		while (it != end && *it == splitter)
			++it;
		while (it != end && *it != splitter)
			++it;
		while (it != end && *it == splitter)
			++it;
		++n;
		--it;
	}
	return n;
}

std::string strToLower(std::string& s)
{
	std::string             ret;
	std::string::iterator   it = s.begin();
	std::string::iterator   end = s.end();
	for (; it != end; ++it)
		ret.push_back(std::tolower(*it));
	return ret;
}

bool	checkExtention(std::string& name, std::string extention)
{
	size_t	index[2];

	index[0] = name.size() - 1;
	index[1] = extention.size() - 1;
	while (name[index[0]] && extention[index[1]])
	{
		if (name[index[0]--] != extention[index[1]--])
			return false;
	}
	return true;
}

std::string getExtension(const std::string& path)
{
	size_t dot = path.find_last_of('.');
	size_t slash = path.find_last_of('/');
	if (dot == std::string::npos || (slash != std::string::npos && dot < slash))
		return "";
	return path.substr(dot);
}

size_t hexToDecimal(const std::string &hexStr)
{
	std::stringstream	ss;
	size_t				result;

	ss << std::hex << hexStr;
	ss >> result;
	return result;
}
