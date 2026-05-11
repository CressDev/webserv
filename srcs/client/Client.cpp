// Minimal Client.cpp: keep only constructor/destructor and trivial accessors.
#include "Client.hpp"
#include "client_helpers.hpp"
#include <new>
#include <cstring>
#include <strings.h>
#include <sstream>
#include <sys/wait.h>
#include <signal.h>
#include <unistd.h>

Client::Client(Server& listener, int fd)
    : _listener(listener), _fd(fd), _status(200), _body(""), _isHeaderReady(false), _isBodyReady(false), _chunkLen(0), _chunkLine(""), _isSent(false), _isLocation(false)
{
    _headerContent.isChunked = false;
    _headerContent.ContentLength = 0;
    _headerContent.isAutoindexResponse = false;
    _hasErrorPageResolved = false;
    _cgi.pid = -1;
    _cgi.in_fd = -1;
    _cgi.out_fd = -1;
    _cgi.write_pos = 0;
    _cgi.in_closed = false;
    _cgi.out_closed = false;
    _cgi.finalized = false;
    env = NULL;
}

Client::~Client()
{
    if (_cgi.in_fd >= 0) { close(_cgi.in_fd); _cgi.in_fd = -1; }
    if (_cgi.out_fd >= 0) { close(_cgi.out_fd); _cgi.out_fd = -1; }
    if (_cgi.pid > 0)
    {
        int status = 0;
        pid_t w = waitpid(_cgi.pid, &status, WNOHANG);
        if (w == 0)
        {
            kill(_cgi.pid, SIGKILL);
            waitpid(_cgi.pid, &status, 0);
        }
        _cgi.pid = -1;
    }
}

void Client::setEnv(char **envp) { env = envp; }

void Client::setListener(const Server& s) { this->_listener = s; }

std::string Client::getHeaderHost() const { return this->_headerContent.host; }

Server Client::getListener() const { return _listener; }

void Client::setClientFd(int fd) { this->_fd = fd; }

int Client::getClientFd() const { return this->_fd; }

bool Client::getIsHeaderReady() const { return this->_isHeaderReady; }

bool Client::getIsBodyReady() const { return this->_isBodyReady; }
