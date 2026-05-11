#include "Client.hpp"
#include "client_helpers.hpp"
#include <dirent.h>
#include <fstream>
#include <sstream>
#include <iostream>
#include <sys/stat.h>
#include <unistd.h>

std::string Client::generateDirectoryListing(const std::string& dirPath, const std::string& requestPath)
{
    try {
        DIR* dir = opendir(dirPath.c_str());
        if (!dir) return std::string();
        std::string html;
        html += "<!doctype html><html><head><meta charset=\"utf-8\"><title>Index of ";
        html += requestPath + "</title></head><body>";
        html += "<h1>Index of " + requestPath + "</h1>\n<hr>\n<pre>";
        struct dirent* ent;
        std::vector<std::string> names;
        while ((ent = readdir(dir)) != NULL)
        {
            std::string n(ent->d_name);
            if (n == ".") continue;
            names.push_back(n);
        }
        closedir(dir);
        std::sort(names.begin(), names.end());
        for (size_t i = 0; i < names.size(); ++i)
        {
            std::string name = names[i];
            std::string href = requestPath;
            if (href.empty() || href[0] != '/') href = "/" + href;
            if (!href.empty() && href[href.size() - 1] != '/') href += "/";
            std::string fullPath = joinPath(dirPath, name);
            struct stat s;
            if (stat(fullPath.c_str(), &s) == 0)
                if (S_ISDIR(s.st_mode)) name += "/";
            html += "<a href=\"" + href + name + "\">" + name + "</a>\n";
        }
        html += "</pre><hr></body></html>";
        return html;
    } catch (const std::bad_alloc& e) {
        std::cerr << "autoindex: std::bad_alloc while generating directory listing for " << dirPath << "\n";
        return std::string();
    }
}

void Client::flushResponse()
{
    ssize_t bytesSent;
    while (!_sendBuffer.empty())
    {
        bytesSent = send(this->_fd, _sendBuffer.c_str(), _sendBuffer.size(), 0);
        if (bytesSent > 0) _sendBuffer.erase(0, bytesSent);
        else if (bytesSent < 0 && (errno == EAGAIN || errno == EWOULDBLOCK)) break ;
        else break ;
    }
    _errorResolvedPath.clear();
    _headerContent.path.clear();
    _isSent = _sendBuffer.empty();
}

void Client::handleDelete()
{
    struct stat s;
    std::string root = _listener.getConfig().root;
    if (!isWithinRoot(_headerContent.path, root)) { _status = 403; return; }
    if (stat(_headerContent.path.c_str(), &s) != 0) { if (errno == ENOENT) _status = 404; else _status = 500; return; }
    if (S_ISDIR(s.st_mode)) { _status = 403; return; }
    if (std::remove(_headerContent.path.c_str()) != 0)
    {
        if (errno == ENOENT) _status = 404;
        else if (errno == EACCES || errno == EPERM) _status = 403;
        else _status = 500;
        return;
    }
    _status = 204;
}

void Client::chargeStatusData(const std::map<std::pair<int, int>, std::string>& errorPages)
{
    std::map<std::pair<int, int>, std::string>::const_iterator it = errorPages.begin();
    std::map<std::pair<int, int>, std::string>::const_iterator end = errorPages.end();
    for (; it != end; ++it)
    {
        if (it->first.first != _status) continue;
        std::string target = it->second;
        while (!target.empty() && (target[0] == ' ' || target[0] == '\t')) target.erase(0, 1);
        while (!target.empty() && (target[target.size() - 1] == ' ' || target[target.size() - 1] == '\t')) target.erase(target.size() - 1);
        if (it->first.first != it->first.second)
        {
            bool isSelfRedirect = (target.find("http://localhost") == 0 || target.find("http://127.0.0.1") == 0);
            if (isSelfRedirect)
            {
                std::string path = target.substr(target.find('/', 7));
                std::string root = _listener.getConfig().root;
                std::string resolved = root + path;
                if (access(resolved.c_str(), R_OK) != 0) return ;
            }
            _status = it->first.second;
            _redirectLocation = target;
            _hasErrorPageResolved = true;
            return;
        }
        std::string root = _listener.getConfig().root;
        if (_isLocation) root = _location.root;
        std::string resolved;
        if (!target.empty() && target[0] == '/')
        {
            if (!root.empty() && root[root.size() - 1] == '/') resolved = root + target.substr(1); else resolved = root + target;
        }
        else
        {
            if (!root.empty() && root[root.size() - 1] == '/') resolved = root + target; else resolved = root + "/" + target;
        }
        if (!resolved.empty() && isWithinRoot(resolved, root) && access(resolved.c_str(), R_OK) == 0)
        {
            _errorResolvedPath = resolved;
            _hasErrorPageResolved = true;
            return ;
        }
    }
}

void Client::chargeStatusData(std::map<std::pair<int, int>, std::string>& errorPages)
{
    chargeStatusData(static_cast<const std::map<std::pair<int,int>,std::string>&>(errorPages));
}

std::string Client::chargeDefaultErrorPage()
{
    std::stringstream bs;
    bs << "<html><head><title>" << _status << " " << reasonPhrase(_status) << "</title></head><body><center><h1>" << _status << " " << reasonPhrase(_status) << "</h1></center><hr><center>webserv</center></body></html>";
    return bs.str();
}

void Client::handleErrors()
{
    if (_isLocation && _location.areErrorPages) { chargeStatusData(_location.error_pages); return ; }
    if (_listener.getConfig().areErrorPages) { chargeStatusData(_listener.getConfig().error_pages); return ; }
}

void Client::sendResponse()
{    
    if (_sendBuffer.empty())
    {
        handleErrors();
        std::cout << "STATUS: " << _status << "" << std::endl;
        if (_hasErrorPageResolved && !_redirectLocation.empty())
        {
            int redirectStatus = _status;
            if (!(redirectStatus >= 300 && redirectStatus < 400))
                redirectStatus = 302;
            std::ostringstream hs;
            hs << this->_headerContent.protocol << " " << redirectStatus << " " << reasonPhrase(redirectStatus) << "\r\n";
            hs << "Location: " << _redirectLocation << "\r\n";
            hs << "Connection: close\r\n";
            hs << "Content-Length: 0\r\n\r\n";
            _sendBuffer += hs.str();
            _isSent = true;
            flushResponse();
            return;
        }
        if (this->_headerContent.method == "DELETE")
        {
            handleDelete();
            handleErrors();
            std::ostringstream hs;
            hs << this->_headerContent.protocol << " 204 " << reasonPhrase(204) << "\r\n";
            hs << "Connection: close\r\n";
            hs << "Content-Length: 0\r\n\r\n";
            _sendBuffer += hs.str();
            _isSent = true;
            flushResponse();
            return;
        }

        if (this->_headerContent.method == "POST")
        {
            if (this->_status == 413)
            {
                std::ostringstream hs_err;
                hs_err << this->_headerContent.protocol << " " << this->_status << " " << reasonPhrase(this->_status) << "\r\n";
                hs_err << "Connection: close\r\n";
                hs_err << "Content-Length: 0\r\n\r\n";
                _sendBuffer += hs_err.str();
                _isSent = true;
                flushResponse();
                return;
            }
            std::string serverRoot = _listener.getConfig().root;
            if (!isWithinRoot(_headerContent.path, serverRoot))
            {
                this->_status = 403;
                std::ostringstream hs;
                hs << this->_headerContent.protocol << " " << this->_status << " " << reasonPhrase(this->_status) << "\r\n";
                hs << "Connection: close\r\n";
                hs << "Content-Length: 0\r\n\r\n";
                _sendBuffer += hs.str();
                _isSent = true;
                flushResponse();
                return;
            }
            struct stat st;
            bool existed = (stat(this->_headerContent.path.c_str(), &st) == 0);
            if (existed && S_ISDIR(st.st_mode))
            {
                _status = 403;
                return;
            }
            std::string ext = getExtension(_headerContent.path);
            std::map<std::string,std::string> cgiMap;
            if (_isLocation) cgiMap = _location.cgi; else cgiMap = _listener.getConfig().cgi;
            if (cgiMap.count(ext))
            {
                if (this->_isBodyReady)
                {
                    int respStatus = this->_status;
                    std::string contentType = _cgiContentType.empty() ? "text/plain" : _cgiContentType;
                    std::ostringstream hs;
                    hs << this->_headerContent.protocol << " " << respStatus << " " << reasonPhrase(respStatus) << "\r\n";
                    hs << "Connection: close\r\n";
                    hs << "Content-Type: " << contentType << "\r\n";
                    hs << "Content-Length: " << this->_body.size() << "\r\n\r\n";
                    _sendBuffer += hs.str();
                    _sendBuffer += this->_body;
                    _isSent = true;
                    flushResponse();
                    return;
                }
                else
                {
                    if (!isCgiRunning())
                    {
                        if (!startCgiNonBlocking(this->_headerContent.path, cgiMap[ext]))
                        {
                            this->_status = 500;
                            return;
                        }
                    }
                    return;
                }
            }
            int fd = open(this->_headerContent.path.c_str(), O_CREAT | O_WRONLY | O_TRUNC, 0644);
            if (fd < 0)
            {
                std::cerr << "sendResponse: open failed for " << this->_headerContent.path << " errno=" << errno << " (" << strerror(errno) << ")\n";
            }
            if (fd < 0)
            {
                if (errno == EACCES || errno == EPERM) this->_status = 403;
                else if (errno == ENOENT) this->_status = 404;
                else this->_status = 500;
                return ;
            }
            ssize_t to_write = _body.size();
            const char* buf = _body.c_str();
            ssize_t written = 0;
            while (to_write > 0)
            {
                ssize_t w = write(fd, buf + written, to_write);
                if (w <= 0)
                {
                    close(fd);
                    this->_status = 500;
                    return;
                }
                written += w;
                to_write -= w;
            }
            close (fd);
            int status;
            if (existed) status = 200; else status = 201;
            std::string requestUri = this->_headerContent.path;
            if (requestUri.find(serverRoot) == 0) requestUri = requestUri.substr(serverRoot.size());
            if (requestUri.empty()) requestUri = "/";
            std::ostringstream hs;
            hs << this->_headerContent.protocol << " " << status << " " << reasonPhrase(status) << "\r\n";
            if (!existed) hs << "Location: " << requestUri << "\r\n";
            hs << "Connection: close\r\n";
            hs << "Content-Length: 0\r\n\r\n";
            _sendBuffer += hs.str();
            _isSent = true;
            flushResponse();
            return;
        }
        std::string body;
        struct stat st;
        if (stat(this->_headerContent.path.c_str(), &st) == 0 && S_ISDIR(st.st_mode) && _headerContent.isAutoindexResponse)
        {
            try {
                std::string requestPath = this->_headerContent.path;
                std::string root = _listener.getConfig().root;
                if (requestPath.find(root) == 0) requestPath = requestPath.substr(root.size());
                if (requestPath.empty()) requestPath = "/";
                body = generateDirectoryListing(this->_headerContent.path, requestPath);
            } catch (const std::bad_alloc& e) {
                std::cerr << "sendResponse: std::bad_alloc while preparing autoindex for " << this->_headerContent.path << "\n";
                this->_status = 500;
                body.clear();
            }
        }
        else
        {
            if (_hasErrorPageResolved && !_errorResolvedPath.empty()) body = loadContent(_errorResolvedPath); else body.clear();
            if (this->_headerContent.method == "GET" || this->_headerContent.method == "HEAD")
            {
                std::string ext = getExtension(_headerContent.path);
                std::map<std::string,std::string> cgiMap;
                if (_isLocation) cgiMap = _location.cgi; else cgiMap = _listener.getConfig().cgi;
                if (cgiMap.count(ext))
                {
                    if (this->_isBodyReady && !this->_body.empty()) body = this->_body;
                    else
                    {
                        if (!isCgiRunning())
                        {
                            if (!startCgiNonBlocking(this->_headerContent.path, cgiMap[ext])) { this->_status = 500; return; }
                        }
                        return;
                    }
                }
            }
            if (body.empty()) body = loadContent(this->_headerContent.path);
        }
        if (!(_status >= 300 && _status < 600)) _status = 200; else body = chargeDefaultErrorPage();
        std::ostringstream hs;
        hs << this->_headerContent.protocol << " " << _status << " " << reasonPhrase(_status) << "\r\n";
        hs << "Content-Length: " << body.size() << "\r\n";
        if (!this->_cgiContentType.empty()) hs << "Content-Type: " << this->_cgiContentType << "\r\n";
        else if (this->_headerContent.isAutoindexResponse || (_status > 299 && _status < 600)) hs << "Content-Type: text/html\r\n";
        else hs << "Content-Type: " << getMimeType(this->_headerContent.path) << "\r\n";
        hs << "Connection: close\r\n";
        hs << "\r\n";
        _sendBuffer += hs.str();
        std::cout << "body: " << body << std::endl;
        if (this->_headerContent.method != "HEAD") _sendBuffer += body;
    }
    flushResponse();
}

std::string Client::loadContent(const std::string& filename) const
{
    std::ifstream file(filename.c_str(), std::ios::binary);
    if (!file.is_open()) return std::string();
    file.seekg(0, std::ios::end);
    std::streamoff soff = file.tellg();
    if (soff <= 0) return std::string();
    std::streamsize size = static_cast<std::streamsize>(soff);
    file.seekg(0, std::ios::beg);
    std::string buffer; buffer.resize(size);
    file.read(&buffer[0], size);
    return buffer;
}

bool Client::getIsSent() const { return _isSent; }
