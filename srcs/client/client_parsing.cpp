#include "Client.hpp"
#include "client_helpers.hpp"
#include <iostream>
#include <sstream>
#include <algorithm>

void Client::chargeHeader()
{
	ssize_t bytesRead = recv(this->_fd, this->_buffer, sizeof(this->_buffer) - 1, 0);
	if (bytesRead < 0)
	{
		if (errno == EAGAIN || errno == EWOULDBLOCK)
			return;
		std::cerr << "Error reading from socket (fd " << this->_fd << "): " << strerror(errno) << std::endl;
		return;
	}
	else if (bytesRead == 0)
	{
		if (!this->_headerContent.isChunked)
		{
			if (this->_headerContent.ContentLength != 0)
			{
				if (this->_body.size() >= this->_headerContent.ContentLength)
				{
					if (this->_body.size() > this->_headerContent.ContentLength)
						this->_body = this->_body.substr(0, this->_headerContent.ContentLength);
					this->_isBodyReady = true;
					this->_request[1] = this->_body;
					return;
				}
				this->_status = 400;
				return;
			}
			else
			{
				this->_isBodyReady = true;
				this->_request[1] = this->_body;
				return;
			}
		}
		else
		{
			if (this->_isBodyReady)
				return;
			this->_status = 400;
			return;
		}
	}
	else
	{
		this->_buffer[bytesRead] = '\0';
		this->_header += this->_buffer;
		size_t headerEnd = this->_header.find("\r\n\r\n");
		if (headerEnd == std::string::npos && _header.size() > 8192)
		{
			_status = 431;
			_isHeaderReady = true;
			_isBodyReady = true;
			return ;
		}
		if (headerEnd != std::string::npos)
		{
			std::string full = this->_header;
			this->_header = full.substr(0, headerEnd);
			this->parseHeader();
			try {
				this->setPath();
			} catch (const std::exception& e) {
				std::cerr << "Exception in setPath(): " << e.what() << std::endl;
				this->_status = 500;
			}
			if (this->_status == 413)
			{
				this->_isHeaderReady = true;
				this->_isBodyReady = true;
				return ;
			}
			if (this->_headerContent.method == "POST")
			{
				if (headerEnd + 4 < full.size())
				{
					std::string extra = full.substr(headerEnd + 4);
					if (this->_headerContent.isChunked)
					{
						this->_chunkLine += extra;
						chunkManagement();
						if (this->_isBodyReady)
						{
							this->_request[1] = this->_body;
						}
					}
					else
					{
						this->_body = extra;
						if (this->_headerContent.ContentLength != 0 && this->_body.size() >= this->_headerContent.ContentLength)
						{
							if (this->_body.size() > this->_headerContent.ContentLength)
								this->_body = this->_body.substr(0, this->_headerContent.ContentLength);
							this->_isBodyReady = true;
							this->_request[1] = this->_body;
						}
						else if (this->_headerContent.ContentLength == 0)
						{
							_status = 411;
							this->_isBodyReady = true;
							return ;
						}
					}
				}
			}
			this->_request[0] = this->_header.substr(0, this->_header.find("\r\n"));
			_isHeaderReady = true;
			if (_headerContent.method != "POST")
				_isBodyReady = true;
		}
	}
}

void	Client::chunkManagement()
{
    size_t        i;
    size_t        sublen;
    std::string    hexLen;

    this->_chunkLine += this->_buffer;
    while (true)
    {
        if (this->_chunkLen == 0)
        {
            i = this->_chunkLine.find("\r\n");
            if (i == std::string::npos)
                return ;
            hexLen = this->_chunkLine.substr(0, i);
            this->_chunkLen = hexToDecimal(hexLen);
            this->_chunkLine.erase(0, i + 2);
            if (this->_chunkLen == 0)
            {
                if (this->_chunkLine.size() >= 2 && this->_chunkLine[0] == '\r' && this->_chunkLine[1] == '\n')
                    this->_chunkLine.erase(0, 2);
                this->_isBodyReady = true;
                return ;
            }
        }
        sublen = this->_chunkLen;
        if (this->_chunkLine.size() < this->_chunkLen + 2)
        {
            sublen = this->_chunkLine.size();
            this->_chunkLen -= sublen;
            this->_body += this->_chunkLine.substr(0, sublen);
            this->_chunkLine.erase(0, sublen);
            return ;
        }
        if (this->_chunkLine[this->_chunkLen] != '\r'
            || this->_chunkLine[this->_chunkLen + 1] != '\n')
        {
            this->_status = 400;
            return ;
        }
        this->_body += this->_chunkLine.substr(0, sublen);
        this->_chunkLine.erase(0, this->_chunkLen + 2);
        this->_chunkLen = 0;
    }
}

void	Client::chargeBody()
{
    if (this->_headerContent.method != "POST")
    {
        return ;
    }
    if (this->_status == 413)
        return;
    ssize_t bytesRead = recv(this->_fd, this->_buffer, sizeof(this->_buffer), 0);
    if (bytesRead < 0)
    {
        if (errno == EAGAIN || errno == EWOULDBLOCK)
            return;
        std::cerr << "Error reading from socket (fd " << this->_fd << "): " << strerror(errno) << std::endl;
        return;
    }
    else if (bytesRead == 0)
    {
        if (!this->_headerContent.isChunked)
        {
            if (this->_headerContent.ContentLength != 0)
            {
                if (this->_body.size() >= this->_headerContent.ContentLength)
                {
                    if (this->_body.size() > this->_headerContent.ContentLength)
                        this->_body = this->_body.substr(0, this->_headerContent.ContentLength);
                    this->_isBodyReady = true;
                    this->_request[1] = this->_body;
                    return;
                }
                this->_status = 400;
                return;
            }
            else
            {
                this->_isBodyReady = true;
                this->_request[1] = this->_body;
                return;
            }
        }
        else
        {
            if (this->_isBodyReady)
                return;
            this->_status = 400;
            return;
        }
    }
    else
    {
        if (this->_headerContent.isChunked == true)
        {
            this->_chunkLine.append(this->_buffer, bytesRead);
            chunkManagement();
            ft_bzero(this->_buffer, sizeof(this->_buffer));
            return ;
        }
        this->_body.append(this->_buffer, bytesRead);
        if (this->_headerContent.ContentLength <= this->_body.size())
        {
            if (this->_body.size() > this->_headerContent.ContentLength)
                this->_body = this->_body.substr(0, this->_headerContent.ContentLength);
            this->_isBodyReady = true;
            this->_request[1] = this->_body;
        }
        ft_bzero(this->_buffer, sizeof(this->_buffer));
    }
}

bool	Client::checkMethod(const std::string& method, const std::vector<std::string>& vec) const
{
    if (std::find(vec.begin(), vec.end(), method) == vec.end())
        return false;
    return true; 
}

void Client::parseHeader()
{
    std::vector<std::string>	tokens;
    std::string				token;
    std::string				line;
    size_t				headerEnd;

    if (this->_status != 200)
        return ;
    headerEnd = this->_header.find("\r\n");
    line = this->_header.substr(0, headerEnd);
    this->_header = this->_header.substr(headerEnd + 2, this->_header.size());
    if (wordCounter(line, ' ') != 3)
    {
        this->_status = 400;
        return ;
    }
    std::string::iterator	begin = line.begin();
    std::string::iterator	end = line.end();
    while (*begin == ' ')
        ++begin;
    for (; begin != end; ++begin)
    {
        if (*begin == ' ')
            break;
        token.push_back(*begin);
    }
    if (token != "GET" && token != "POST" && token != "DELETE" && token != "HEAD")
    {   
        this->_status = 405;
        return ;
    }
    if (token == "HEAD")
    {
        std::string getToken = "GET";
        if (checkMethod(getToken, _listener.getConfig().allowed_methods) == false)
        {
            _status = 405;
            return ;
        }
    }
    else if (checkMethod(token, _listener.getConfig().allowed_methods) == false)
    {
        _status = 405;
        return ;
    }
    this->_headerContent.method = token;
    token = "";
    while (*begin == ' ')
        ++begin;
    for (; begin != end; ++begin)
    {
        if (*begin == ' ')
            break;
        token.push_back(*begin);
    }
    if (token[0] != '/')
    {
        this->_status = 400;
        return ;
    }
    this->_headerContent.path = urlDecode(token);
    token = "";
    while (*begin == ' ')
        ++begin;
    for (; begin != end; ++begin)
        token.push_back(*begin);
    if (token != "HTTP/1.0" && token != "HTTP/1.1")
    {
        this->_status = 505;
        return ;
    }
    this->_headerContent.protocol = token;
    std::string lower = strToLower(this->_header);
    size_t pos = lower.find("host:");
    if (pos != std::string::npos)
    {
        size_t valStart = pos + 5;
        while (valStart < this->_header.size() && (this->_header[valStart] == ' ' || this->_header[valStart] == '\t'))
            ++valStart;
        size_t valEnd = this->_header.find("\r\n", valStart);
        if (valEnd == std::string::npos) valEnd = this->_header.size();
        std::string hostVal = this->_header.substr(valStart, valEnd - valStart);
        size_t colon = hostVal.find(':');
        if (colon != std::string::npos)
            hostVal = hostVal.substr(0, colon);
        while (!hostVal.empty() && (hostVal[0] == ' ' || hostVal[0] == '\t'))
            hostVal.erase(hostVal.begin());
        while (!hostVal.empty() && (hostVal[hostVal.size() - 1] == ' ' || hostVal[hostVal.size() - 1] == '\t'))
            hostVal.erase(hostVal.end()-1);
        this->_headerContent.host = hostVal;
    }
    while (*begin == ' ')
        ++begin;
    begin += 2;
    {
        std::string hdrs = this->_header;
        std::vector<std::string> header_lines;
        while (!hdrs.empty())
        {
            size_t p = hdrs.find("\r\n");
            if (p == std::string::npos)
            {
                header_lines.push_back(hdrs);
                hdrs.clear();
            }
            else
            {
                header_lines.push_back(hdrs.substr(0, p));
                hdrs = hdrs.substr(p + 2);
            }
        }
        for (size_t hi = 0; hi < header_lines.size(); ++hi)
        {
            std::string line = header_lines[hi];
            if (line.empty()) continue;
            size_t colonPos = line.find(":");
            if (colonPos == std::string::npos)
            {
                this->_status = 400;
                return;
            }
            std::string name = line.substr(0, colonPos);
            std::string value = line.substr(colonPos + 1);
            while (!name.empty() && (name[0] == ' ' || name[0] == '\t'))
                name.erase(name.begin());
            while (!name.empty() && (name[name.size() - 1] == ' ' || name[name.size() - 1] == '\t'))
                name.erase(name.end()-1);
            while (!value.empty() && (value[0] == ' ' || value[0] == '\t'))
                value.erase(value.begin());
            while (!value.empty() && (value[value.size() - 1] == ' ' || value[value.size() - 1] == '\t'))
                value.erase(value.end()-1);
            std::string lname = strToLower(name);
            if (lname == "content-length")
            {
                if (this->_headerContent.isChunked == true)
                {
                    this->_status = 404;
                    return;
                }
                size_t num = 0;
                std::istringstream ss(value);
                ss >> num;
                checkContentLength(num);
                this->_headerContent.ContentLength = num;
            }
            else if (lname == "transfer-encoding")
            {
                if (strToLower(value).find("chunked") != std::string::npos)
                    this->_headerContent.isChunked = true;
            }
        }
    }
}
