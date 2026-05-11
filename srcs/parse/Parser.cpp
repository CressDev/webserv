#include "Parser.hpp"

int	Parser::getTypeOfItem(std::string& str)
{
	if (str.size() == 3
		&& strIsDigit(str) 
		&& (str[0] == '3'
			|| str[0] == '4'
			|| str[0] == '5'))
		return 1;
	if (str.size() == 4
		&& str[0] == '='
		&& strIsDigit(str.substr(1, 3))
		&& (str[1] == '1'
			|| str[1] == '2'
			|| str[1] == '3'
			|| str[1] == '4'
			|| str[1] == '5'))
	{
		return 2;
	}
	return 3;
}

int	Parser::getErrorPageParserState(int prev, int pos)
{
	static int matrix[][4] = {
		{EP_ERR, EP_COD, EP_ERR, EP_ERR}, // INI
		{EP_ERR, EP_COD, EP_OVR, EP_URI}, // EP_COD
		{EP_ERR, EP_ERR, EP_ERR, EP_URI}, // EP_OVR
		{EP_ERR, EP_ERR, EP_ERR, EP_ERR}  // EP_URI
	};
	return matrix[prev][pos];
}

void	Parser::errorpageParser(std::vector<std::string>::iterator& it, Server& server)
{
	int                                 pos;
	int                                 prev = 0;
	std::vector<std::pair<int, int> >   ovr;

	++it;
	while (*it != ";")
	{
		pos = getTypeOfItem(*it);
		prev = getErrorPageParserState(prev, pos);
		if (prev == 0)
			throw std::runtime_error("Parser: configuration error");
		if (prev == 1)
			ovr.push_back(std::make_pair(std::atoi(it->c_str()), std::atoi(it->c_str())));
		if (prev == 2)
		{
			for (size_t i = 0; i < ovr.size(); i++)
				ovr[i].second = std::atoi(it->substr(1, it->length()).c_str());
		}
		++it;
	}
	if (getTypeOfItem(*(it - 1)) != 3)
		throw std::runtime_error("Parser: configuration error");
	else
		server.addErrorPage(ovr, *(it - 1));
}

int		Parser::getIndexState(int prev, int pos)
{
	static int	matrix[][9] = {
		{1, 1, 10, 10, 10, 10, 10, 10},
		{1, 1,  1,  1,  1,  1,  1,  1}, //1 error
		{1, 1,  3,  6,  1,  1,  1,  1}, //2 .
		{1, 1,  1,  4,  1,  1,  1,  1}, //3 p
		{1, 1,  5,  1,  1,  1,  1,  1}, //4 h
		{1, 1,  1,  1,  1,  1,  1,  1}, //5 p2
		{1, 1,  1,  1,  7,  1,  1,  1}, //6 h2
		{1, 1,  1,  1,  1,  8,  1,  1}, //7 t
		{1, 1,  1,  1,  1,  1,  9,  1}, //8 m
		{1, 1,  1,  1,  1,  1,  1,  1}, //9 l
		{1, 2, 10, 10, 10, 10, 10, 10}, //10 alpha
	//	 e  .  p  h  t  m  l  a
	};
	return (matrix[prev][pos]);
}


void	Parser::chooseIndexState(std::string str)
{
	std::string::iterator	it = str.begin();
	std::string::iterator	end= str.end();
	int	pos;
	int	prev = 0;
	while (it != end)
	{
		pos = 0;
		if (*it == '.')
			pos = 1;
		else if (*it == 'p')
			pos = 2;
		else if (*it ==  'h')
			pos = 3;
		else if (*it == 't')
			pos = 4;
		else if (*it == 'm')
			pos = 5;
		else if (*it == 'l')
			pos = 6;
		else if (std::isalpha(*it))
			pos = 7;
		prev = getIndexState(prev, pos);
		/*if (prev == 1)
			throw std::runtime_error("Parser: configuration error");*/
		it++;
	}
	/*if (prev != 5 && prev != 8 && prev != 9)
			throw std::runtime_error("Parser: configuration error");*/
	
	//NO TOCAR ESTOS COMENTARIOS SOLO ELIMINAR ESTE <--
}

void	Parser::indexParser(std::vector<std::string>::iterator& it,Server& server)
{
	std::string			str;
	++it;
	while (*it != ";")
	{
		str = *it;
		chooseIndexState(str);
        server.addIndex(*it);
		++it;
	}
}

int    isCgiWord(std::string& token)
{
    for (size_t i = 0; i < 15; i++)
    {
        if (token == cgikeys[i])
            return 1;
    }
    return 0;
}

void	Parser::cgiParser(std::vector<std::string>::iterator& it, Server& server)
{
	std::string			str;
	++it;
    str = *it;
    if (*(it + 2) != ";" || !isCgiWord(str))
        throw std::runtime_error("Parser: configuration error");
    ++it;
    if (access((*it).c_str(), F_OK) || access((*it).c_str(), X_OK))
        throw std::runtime_error("Parser: configuration error");
    server.addCgi(*(it - 1), *it);
	++it;
}

void    Parser::rootParser(std::vector<std::string>::iterator& it, Server& server, int n)
{
    if (*(it + 2) != ";" || server.getConfig().isRoot == true || server.getConfig().isAlias == true)
        throw std::runtime_error("Parser: configuration error");
    ++it;
    if (n == 1)
        throw std::runtime_error("Parser: configuration error");
    server.setRoot(*it, n);
}

void    Parser::rootLocParser(std::vector<std::string>::iterator& it, LocationConfig& loc, int i)
{
    if (*(it + 2) != ";")
        throw std::runtime_error("Parser: configuration error");
    ++it;
    if (loc.isRoot == false && loc.isAlias == false)
    {
		loc.root = *it;
        if (i == 0)
            loc.isRoot = true;
        if (i == 1)
            loc.isAlias = true;
    }
    else
    {
        std::cerr << "root\n";
        throw std::runtime_error("Parser: configuration error");
    }
	++it;
}

void	Parser::indexLocParser(std::vector<std::string>::iterator& it, LocationConfig& loc)
{
	std::string			str;

	++it;
	loc.index.clear();
	while (*it != ";")
	{
		str = *it;
		chooseIndexState(str);
        loc.index.push_back(*it);
		++it;
	}

}

void	Parser::cgiLocParser(std::vector<std::string>::iterator& it, LocationConfig& loc)
{
	std::string			str;

	++it;
    str = *it;
	loc.cgi.clear();
    if (*(it + 2) != ";" || !isCgiWord(str))
	{
		std::cout << "CGI 1\n";
        throw std::runtime_error("Parser: configuration error");
	}
    ++it;
    if (access((*it).c_str(), F_OK) || access((*it).c_str(), X_OK))
    {
		std::cout << "CGI 2\n";
        throw std::runtime_error("Parser: configuration error");
    }
	loc.cgi.insert(std::pair<std::string, std::string>(*(it - 1), *it));
	++it;
}

void    Parser::autoindexLocParser(std::vector<std::string>::iterator& it, LocationConfig& loc)
{
    if (*(it + 2) != ";")
	{
		std::cout << "AUTOINDEX PARSER 1\n";
        throw std::runtime_error("Parser: configuration error");
	}
    ++it;
    if (*it != "on" && *it != "off")
	{
		std::cout << "AUTOINDEX PARSER 2\n";
        throw std::runtime_error("Parser: configuration error");
	}
    if (*it == "on")
        loc.autoindex = true;
    else
        loc.autoindex = false;
    loc.isAutoindex = true;
    ++it;
}

void	Parser::allowedLocParser(std::vector<std::string>::iterator& it, LocationConfig& loc)
{
    std::vector<std::string>    methods;

	++it;
	loc.allowed_methods.clear();
	while (*it != ";")
	{
		if (*it != "GET" && *it != "POST" && *it != "DELETE")
			throw std::runtime_error("Parser: configuration error");
        else
            methods.push_back(*it);
		++it;
	}
    loc.allowed_methods = methods;
}

void    Parser::errorPageLocParser(std::vector<std::string>::iterator& it, LocationConfig& loc)
{
    int                 pos;
    int                 prev = 0;
	std::pair<int, int>	ovr;

    ++it;
	loc.error_pages.clear();
    while (*it != ";")
    {
        pos = getTypeOfItem(*it);
        prev = getErrorPageParserState(prev, pos);
        if (prev == 0)
            throw std::runtime_error("Parser: configuration error");
		if (prev == 1)
        {
            ovr.first = std::atoi(it->c_str());
            ovr.second = std::atoi(it->c_str());
        }
		if (prev == 2)
			ovr.second = std::atoi(it->substr(1, it->length()).c_str());
        ++it;
    }
    if (getTypeOfItem(*(it - 1)) != 3)
        throw std::runtime_error("Parser: configuration error");
	else
    {
		loc.error_pages.insert(std::pair<std::pair<int, int>, std::string>(ovr, *(it - 1)));
        loc.areErrorPages = true;
    }
}

int Parser::getLocationState(int prev, int pos)
{
    static int matrix[][3] = {
        {LO_ERR, LO_EQU, LO_PAT}, // INI
        {LO_ERR, LO_ERR, LO_PAT}, // LO_EQU
        {LO_ERR, LO_ERR, LO_ERR}  // LO_PAT
    };
    
    return (matrix[prev][pos]);
}

void	Parser::cmbsLocParser(std::vector<std::string>::iterator& it, LocationConfig& loc)
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
	{
        throw std::runtime_error("Parser: configuration error");
	}
	if (c == 'K' || c == 'k')
		n *= 1024;
	if (c == 'M' || c == 'm')
		n *= 1048576;
	if (c == 'G' || c == 'g')
		n *= 1073741824;
    loc.client_max_body_size = n;
}

void    Parser::locationParser(std::vector<std::string>::iterator& it, Server& server)
{
    int prev = 0;
    int pos;
	LocationConfig	loc;

    ++it;
    while (*it != "{")
    {
        if (*it == "=")
            pos = 1;
        else
            pos = 2;
        prev = getLocationState(prev, pos);
        if (prev == 0)
            throw std::runtime_error("Parser: configuration error");
        ++it;
    }
	loc.path = *(it - 1);
	++it;
    loc.isAutoindex = false;
	loc.isRoot = false;
	loc.isAlias = false;
	loc.allowed_methods = server.getConfig().allowed_methods;
	loc.index = server.getConfig().index;
	loc.cgi = server.getConfig().cgi;
	loc.autoindex = server.getConfig().autoindex;
	loc.error_pages = server.getConfig().error_pages;
    loc.areErrorPages = server.getConfig().areErrorPages;
	loc.isAutoindex = server.getConfig().isAutoindex;
	loc.client_max_body_size =  server.getConfig().client_max_body_size;
	while (*it != "}")
	{
		if (*it == "root")
			rootLocParser(it, loc, 0);
		else if (*it == "index")
			indexLocParser(it, loc);
        else if (*it == "cgi")
			cgiLocParser(it, loc);
		else if (*it == "autoindex")
			autoindexLocParser(it, loc);
		else if (*it == "alias")
			rootLocParser(it, loc, 1);
		else if (*it == "allowed_methods")
			allowedLocParser(it, loc);
		else if (*it == "error_page")
			errorPageLocParser(it, loc);
		else if (*it == "client_max_body_size")
			cmbsLocParser(it, loc);
		else if (*it == ";")
			++it;
		else
			throw std::runtime_error("Parser: configuration error");
    }
	if (loc.root.empty() == true)
	{
		std::cout << loc.root << std::endl;
		loc.root = server.getConfig().root;
		loc.isRoot = server.getConfig().isRoot;
		loc.isAlias = server.getConfig().isAlias;
	}
	server.addLocation(loc);
}

void    Parser::checkListen(std::vector<Server>& servers)
{
    std::set<std::pair<std::string, std::string> >  listens;
    std::vector<Server>::iterator                   it = servers.begin();
    std::vector<Server>::iterator                   end = servers.end();

    while (it != end)
    {
        std::multimap<std::string, std::string>::const_iterator lit = it->getConfig().listen.begin();
        std::multimap<std::string, std::string>::const_iterator lend = it->getConfig().listen.end();
        while (lit != lend)
        {
            std::pair<std::string, std::string> iport = std::make_pair(lit->first, lit->second);
            if (listens.find(iport) != listens.end())
                throw std::runtime_error("Parser: duplicate listen address/port between servers");
            listens.insert(iport);
            ++lit;
        }
        ++it;
    }
}

void	Parser::checkServerNames(std::vector<Server>& servers)
{
    std::set<std::string> seen;
    for (size_t i = 0; i < servers.size(); ++i)
    {
        const std::string &name = servers[i].getConfig().server_name;
        if (name.empty())
            continue;
        std::pair<std::set<std::string>::iterator, bool> res = seen.insert(name);
        if (!res.second)
        {
            std::cerr << "Duplicate server_name '" << name << "' between servers (first occurrence and index " << i << ")\n";
        }
    }
}

Parser::Parser(const char* in_file, std::vector<Server>& servers) : _serverCounter(0)
{
	_infile.open(in_file);
    std::vector<std::string>::iterator  it;
    std::vector<std::string>::iterator  end;

    if (!_infile.is_open())
        throw std::runtime_error("Could not open config file\n");
    this->rmComments(_infile);
    if (this->tokenize() == 1)
    {
        _infile.close();
        throw std::runtime_error("Parser: tokenize failed (mismatched braces or too many braces)");
    }
    this->chooseState(this->_tokens);
    it = _tokens.begin();
    end = _tokens.end();
    int i = 0;
    while (i < _serverCounter)
    {
        Server  server;
        servers.push_back(server);
        ++i;
    }
    i = -1;
    for (; it != end; ++it)
    {
        if (*it == "server")
            ++i;
        if (*it == "listen")
            this->listenParser(it, servers[i]);
        if (*it == "autoindex")
            this->autoindexParser(it, servers[i]);
        if (*it == "allowed_methods")
            this->allowedParser(it, servers[i]);
		if (*it == "server_name")
			this->serverNameParser(it, servers[i]);
		if (*it == "client_max_body_size")
			this->clientmaxbodysizeParser(it, servers[i]);
        if (*it == "error_page")
            this->errorpageParser(it, servers[i]);
        if (*it == "index")
            this->indexParser(it, servers[i]);
        if (*it == "cgi")
            this->cgiParser(it, servers[i]);
        if (*it == "root")
            this->rootParser(it, servers[i], 0);
        if (*it == "alias")
            throw std::runtime_error("Parser: 'alias' directive not supported in this parser");
		if (*it == "location")
		{
			while (*it != "}")
				++it;
		}
    }
    it = _tokens.begin();
	i = -1;
    for (; it != end; ++it)
    {
		if (*it == "server")
			++i;
        if (*it == "location")
            this->locationParser(it, servers[i]);
    }
    checkListen(servers);
	checkServerNames(servers);
    _infile.close();
}

Parser::~Parser()
{
    _tokens.clear();
    _listens.clear();
	 if (_infile.is_open())
        _infile.close();
}

int    Parser::tokenize()
{
    std::string token;
    int         curly_braces = 0;

    for (size_t i = 0; i < this->_config_file.size();++i)
    {
        switch(this->_config_file[i])
        {
            case '{':
                if (!token.empty())
                {
                    _tokens.push_back(token);
                    token.clear();
                }
                _tokens.push_back("{");
                ++curly_braces;
                if (curly_braces > 2)
                    return 1;
                break ;
            case '}':
                if (!token.empty())
                {
                    _tokens.push_back(token);
                    token.clear();
                }
                _tokens.push_back("}");
                --curly_braces;
                break;
            case ';':
                if (!token.empty())
                {
                    _tokens.push_back(token);
                    token.clear();
                }
                _tokens.push_back(";");
                break ;
            case ' ':
            case '\t':
            case '\n':
            case '\r':
                if (!token.empty())
                {
                    _tokens.push_back(token);
                    if (token == "server")
                        ++_serverCounter;
                    token.clear();
                }
                break;
            default:
                token.push_back(this->_config_file[i]);
                break;
        }
    }
    if (!token.empty())
        _tokens.push_back(token);
    if (curly_braces != 0)
        return 1;
    return 0;
}

std::vector<std::string>    Parser::get_tokens(){ return _tokens; }
