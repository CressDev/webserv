#include "Client.hpp"
#include "client_helpers.hpp"
#include <iostream>
#include <sstream>
#include <unistd.h>
#include <sys/wait.h>
#include <limits.h>

bool Client::parentProcess(int *inpipe, int *outpipe, pid_t pid)
{
	close(inpipe[0]);
	close(outpipe[1]);
	_cgi.pid = pid;
	_cgi.in_fd = inpipe[1];
	_cgi.out_fd = outpipe[0];
	_cgi.write_buf = this->_body;
	if (_headerContent.ContentLength != 0 && _cgi.write_buf.size() > _headerContent.ContentLength)
		_cgi.write_buf = _cgi.write_buf.substr(0, _headerContent.ContentLength);
	this->_body.clear();
	_cgi.write_pos = 0;
	_cgi.read_buf.clear();
	_cgiContentType.clear();
	_cgi.in_closed = false;
	_cgi.out_closed = false;
	_cgi.finalized = false;
	setNonBlocking(_cgi.in_fd);
	setNonBlocking(_cgi.out_fd);
	if (_cgi.write_buf.empty() && _cgi.in_fd >= 0)
	{
		close(_cgi.in_fd);
		_cgi.in_fd = -1;
		_cgi.in_closed = true;
	}
	return true;
}

bool Client::startCgiNonBlocking(const std::string& scriptPath, const std::string& interpreter)
{
	int inpipe[2];
	int outpipe[2];
	if (pipe(inpipe) == -1)
		return false;
	if (pipe(outpipe) == -1)
	{
		close(inpipe[0]); close(inpipe[1]);
		return false;
	}
	pid_t pid = fork();
	if (pid < 0)
	{
		close(inpipe[0]); close(inpipe[1]); close(outpipe[0]); close(outpipe[1]);
		return false;
	}
	else if (pid == 0)
	{
		if (dup2(inpipe[0], STDIN_FILENO) == -1) _exit(127);
		if (dup2(outpipe[1], STDOUT_FILENO) == -1) _exit(127);
		close(inpipe[0]); close(inpipe[1]); close(outpipe[0]); close(outpipe[1]);
		std::string dir = getDirectory(scriptPath);
		bool changedCwd = false;
		if (!dir.empty())
		{
			char resolved[PATH_MAX];
			if (realpath(dir.c_str(), resolved) != NULL)
			{
				if (chdir(resolved) != 0) _exit(127);
			}
			else
			{
				if (chdir(dir.c_str()) != 0) _exit(127);
			}
			changedCwd = true;
		}
		std::vector<std::string> argv_store;
		if (!interpreter.empty())
		{
			argv_store.push_back(interpreter);
			std::string li = interpreter;
			if (li.find("python") != std::string::npos)
				argv_store.push_back(std::string("-u"));
			std::string childScript = scriptPath;
			while (childScript.size() >= 2 && childScript[0] == '.' && childScript[1] == '/')
				childScript = childScript.substr(2);
			if (changedCwd)
			{
				size_t bp = childScript.find_last_of('/');
				if (bp != std::string::npos)
					childScript = childScript.substr(bp + 1);
			}
			argv_store.push_back(childScript);
		}
		else
			argv_store.push_back(scriptPath);
		size_t argc = argv_store.size();
		char **argv_exec = new char*[argc + 1];
		for (size_t ai = 0; ai < argc; ++ai)
			argv_exec[ai] = const_cast<char*>(argv_store[ai].c_str());
		argv_exec[argc] = NULL;
		std::map<std::string,std::string> extras;
		extras["REQUEST_METHOD"] = _headerContent.method;
		extras["SERVER_PROTOCOL"] = _headerContent.protocol;
		std::string lower = strToLower(this->_header);
		size_t ct_pos = lower.find("content-type:");
		if (ct_pos != std::string::npos) {
			size_t vs = ct_pos + 13;
			while (vs < _header.size() && (_header[vs] == ' ' || _header[vs] == '\t')) ++vs;
			size_t ve = _header.find("\r\n", vs);
			if (ve == std::string::npos) ve = _header.size();
			extras["CONTENT_TYPE"] = _header.substr(vs, ve - vs);
		}
		if (_headerContent.ContentLength > 0)
		{
			std::ostringstream __tmp_ss; __tmp_ss << _headerContent.ContentLength; extras["CONTENT_LENGTH"] = __tmp_ss.str();
		}
		else if (_headerContent.method == "POST")
		{
			std::ostringstream __tmp_ss; __tmp_ss << _body.size(); extras["CONTENT_LENGTH"] = __tmp_ss.str();
		}
		if (!this->_headerContent.host.empty()) extras["SERVER_NAME"] = this->_headerContent.host;
		extras["SCRIPT_FILENAME"] = scriptPath;
		std::string request_uri;
		if (!this->_request[0].empty())
		{
			std::istringstream rs(this->_request[0]);
			std::string method_token, uri_token, proto_token;
			rs >> method_token >> uri_token >> proto_token;
			request_uri = uri_token;
		}
		if (request_uri.empty()) request_uri = _headerContent.path;
		size_t qpos = request_uri.find('?');
		if (qpos != std::string::npos)
		{
			extras["SCRIPT_NAME"] = request_uri.substr(0, qpos);
			extras["QUERY_STRING"] = request_uri.substr(qpos + 1);
		}
		else
		{
			extras["SCRIPT_NAME"] = request_uri;
			extras["QUERY_STRING"] = std::string();
		}
		extras["REQUEST_URI"] = request_uri;
		if (_isLocation) extras["DOCUMENT_ROOT"] = _location.root; else extras["DOCUMENT_ROOT"] = _listener.getConfig().root;
		extras["GATEWAY_INTERFACE"] = "CGI/1.1";
		extras["SERVER_SOFTWARE"] = "webserv/0.1";
		for (size_t ai = 0; ai < argc; ++ai)
		{
			std::string li = argv_store[ai];
			if (li.find("python") != std::string::npos)
			{
				extras["PYTHONUNBUFFERED"] = "1";
				break;
			}
		}
		char **child_envp = buildEnvpFromMapAndParent(extras, env);
		execve(argv_exec[0], argv_exec, child_envp);
		int savedErrno = errno;
		std::cerr << "execve failed: errno=" << savedErrno << " (" << strerror(savedErrno) << ")\n";
		freeEnvp(child_envp);
		delete [] argv_exec;
		_exit(127);
	}
	else
		return(parentProcess(inpipe, outpipe, pid));
	return false;
}

int Client::getCgiInFd() const
{
	return _cgi.in_fd;
}

std::string Client::getMethod() const
{
	return _headerContent.method;
}

int Client::getCgiOutFd() const
{
	return _cgi.out_fd;
}

bool Client::isCgiRunning() const
{
	return (_cgi.pid > 0);
}

void Client::handleCgiFdEvent(int fd, short revents)
{
	if (!isCgiRunning())
		return ;
	if (revents & (POLLHUP | POLLERR | POLLNVAL))
	{
		if (fd == _cgi.in_fd && _cgi.in_fd >= 0)
		{
			close(_cgi.in_fd);
			_cgi.in_fd = -1;
			_cgi.in_closed = true;
		}
		if (fd == _cgi.out_fd && _cgi.out_fd >= 0)
		{
			close(_cgi.out_fd);
			_cgi.out_fd = -1;
			_cgi.out_closed = true;
		}
		finalizeCgiIfDone();
		return;
	}
	if (fd == _cgi.in_fd && (revents & POLLOUT))
	{
		while (_cgi.write_pos < _cgi.write_buf.size())
		{
			const char* buf = _cgi.write_buf.c_str() + _cgi.write_pos;
			size_t to_write = _cgi.write_buf.size() - _cgi.write_pos;
			ssize_t w = write(_cgi.in_fd, buf, to_write);
			if (w > 0)
				_cgi.write_pos += (size_t)w;
			else if (w < 0)
				break;
			else
			{
				close(_cgi.in_fd);
				_cgi.in_fd = -1;
				_cgi.in_closed = true;
				break ;
			}
		}
		if(_cgi.write_pos >= _cgi.write_buf.size() && _cgi.in_fd >= 0)
		{
			close(_cgi.in_fd);
			_cgi.in_fd = -1;
			_cgi.in_closed = true;
		}
	}
	if (fd == _cgi.out_fd && (revents & POLLIN))
	{
		char buf[4096];
		ssize_t r = read(_cgi.out_fd, buf, sizeof(buf));
		if (r > 0)
			_cgi.read_buf.append(buf, r);
		else if (r == 0)
		{
			close(_cgi.out_fd);
			_cgi.out_fd = -1;
			_cgi.out_closed = true;
		}
		else
		{
			if (_cgi.out_fd >= 0) close(_cgi.out_fd);
			_cgi.out_fd = -1;
			_cgi.out_closed = true;
		}
	}
	finalizeCgiIfDone();
}

void Client::finalizeCgiIfDone()
{
	if (!isCgiRunning())
	{
		if (_cgi.finalized) return;
		return ;
	}
	int status = 0;
	pid_t w = waitpid(_cgi.pid, &status, WNOHANG);
	if (w == 0)
	{
		bool haveSep = (_cgi.read_buf.find("\r\n\r\n") != std::string::npos) || (_cgi.read_buf.find("\n\n") != std::string::npos);
		if (!haveSep) return ;
		w = waitpid(_cgi.pid, &status, 0);
	}
	_cgi.pid = -1;
	if (_cgi.out_fd >= 0)
	{
		char tmpbuf[4096];
		while (true)
		{
			ssize_t r = read(_cgi.out_fd, tmpbuf, sizeof(tmpbuf));
			if (r > 0) _cgi.read_buf.append(tmpbuf, r);
			else if (r == 0) { close(_cgi.out_fd); _cgi.out_fd = -1; _cgi.out_closed = true; break; }
			else break;
		}
	}
	std::string &out = _cgi.read_buf;
	size_t sep = out.find("\r\n\r\n");
	std::string cgiHeaders;
	std::string cgiBody;
	if (sep != std::string::npos)
	{
		cgiHeaders = out.substr(0, sep);
		cgiBody = out.substr(sep + 4);
	}
	else
	{
		sep = out.find("\n\n");
		if (sep != std::string::npos)
		{
			cgiHeaders = out.substr(0, sep);
			cgiBody = out.substr(sep + 2);
		}
		else
			cgiBody = out;
	}
	int cgiStatus = 200;
	if (!cgiHeaders.empty())
	{
		size_t pos = 0;
		while (pos < cgiHeaders.size())
		{
			size_t lineEnd = cgiHeaders.find('\n', pos);
			if (lineEnd == std::string::npos) lineEnd = cgiHeaders.size();
			size_t len = lineEnd - pos;
			if (len > 0 && cgiHeaders[lineEnd - 1] == '\r') --len;
			std::string line = cgiHeaders.substr(pos, len);
			size_t i = 0;
			while (i < line.size() && std::isspace(static_cast<unsigned char>(line[i]))) ++i;
			if (i < line.size()) line = line.substr(i);
			if (line.size() >= 7 && line.find("Status:") == 0)
			{
				size_t j = 7; while (j < line.size() && std::isspace(static_cast<unsigned char>(line[j]))) ++j;
				std::string num; while (j < line.size() && std::isdigit(static_cast<unsigned char>(line[j]))) { num.push_back(line[j]); ++j; }
				if (!num.empty()) { std::istringstream ss(num); ss >> cgiStatus; }
			}
			else if (line.size() >= 13 && strncasecmp(line.c_str(), "Content-Type:", 13) == 0)
			{
				size_t k = 13; while (k < line.size() && std::isspace(static_cast<unsigned char>(line[k]))) ++k; _cgiContentType = line.substr(k);
			}
			pos = lineEnd + 1;
		}
	}
	this->_body = cgiBody;
	this->_status = cgiStatus;
	this->_isBodyReady = true;
	_cgi.finalized = true;
	if (_cgi.in_fd >= 0) { close(_cgi.in_fd); _cgi.in_fd = -1; }
	if (_cgi.out_fd >= 0) { close(_cgi.out_fd); _cgi.out_fd = -1; }
	_cgi.in_closed = true; _cgi.out_closed = true;
	this->sendResponse();
}

bool Client::handleCgiIfNeeded()
{
	std::map<std::string,std::string> cgiMap = _listener.getConfig().cgi;
	if (this->_isLocation)
	{
		std::map<std::string,std::string>::const_iterator it = _location.cgi.begin();
		for (; it != _location.cgi.end(); ++it) cgiMap[it->first] = it->second;
	}
	std::string ext = getExtension(this->_headerContent.path);
	if (cgiMap.count(ext))
	{
		if (this->_headerContent.method == "POST" && !this->_isBodyReady)
			return false;
		std::string interp = cgiMap[ext];
		if (!startCgiNonBlocking(this->_headerContent.path, interp)) { _status = 500; return false; }
		return true;
	}
	return false;
}
