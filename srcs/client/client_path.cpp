#include "Client.hpp"
#include "client_helpers.hpp"
#include <sys/stat.h>
#include <unistd.h>

void Client::checkContentLength(size_t num)
{
    const std::vector<LocationConfig>& locations = _listener.getConfig().locations;
    if (!locations.empty())
    {
        std::vector<LocationConfig>::const_iterator it = locations.begin();
        std::vector<LocationConfig>::const_iterator end = locations.end();
        for (; it != end; ++it)
        {
            if (it->path.size() <= _headerContent.path.size()
                && _headerContent.path.compare(0, it->path.size(), it->path) == 0
                && it->client_max_body_size != 0
                && num > it->client_max_body_size)
            {
                this->_status = 413;
                return ;
            }
        }
    }
    if (this->_listener.getConfig().client_max_body_size != 0
        && num > this->_listener.getConfig().client_max_body_size)
        this->_status = 413;
}

void Client::setPath()
{
    const std::vector<LocationConfig>& locations = _listener.getConfig().locations;
    if (!locations.empty())
    {
        std::vector<LocationConfig>::const_iterator it = locations.begin();
        std::vector<LocationConfig>::const_iterator end = locations.end();
        std::vector<std::string> index;
        bool autoindex = false;
        for (; it != end; ++it)
        {
            if (it->path.size() <= _headerContent.path.size() && _headerContent.path.compare(0, it->path.size(), it->path) == 0)
            {
                _isLocation = true;
                _location = *it;
                if (checkMethod(_headerContent.method, it->allowed_methods) == false)
                {
                    _status = 405;
                    return ;
                }
                if (!_listener.getConfig().index.empty()) index = _listener.getConfig().index;
                if (!it->index.empty()) index = it->index;
                if (_listener.getConfig().isAutoindex == true) autoindex = _listener.getConfig().autoindex;
                if (it->isAutoindex == true) autoindex = it->autoindex;
                if (it->isAlias)
                {
                    std::string rest = _headerContent.path.substr(it->path.size());
                    _headerContent.path = it->root + rest;
                    _headerContent.root = it->root;
                    checkPathValidity(_headerContent.path, index, autoindex, it->root);
                    return ;
                }
                if (it->isRoot)
                {
                    _headerContent.path = it->root + _headerContent.path;
                    _headerContent.root = it->root;
                    checkPathValidity(_headerContent.path, index, autoindex, it->root);
                    return ;
                }
                _headerContent.path = _listener.getConfig().root + _headerContent.path;
                _headerContent.root = _listener.getConfig().root;
                checkPathValidity(_headerContent.path, index, autoindex, _listener.getConfig().root);
                return ;
            }
        }
        if (_headerContent.path == "/")
        {
            if (!_listener.getConfig().index.empty()) index = _listener.getConfig().index;
            if (_listener.getConfig().isAutoindex == true) autoindex = _listener.getConfig().autoindex;
            _headerContent.path = _listener.getConfig().root + _headerContent.path;
            _headerContent.root = _listener.getConfig().root;
            checkPathValidity(_headerContent.path, index, autoindex, _listener.getConfig().root);
            return ;
        }
    }
    std::vector<std::string> index;
    bool autoindex = false;
    if (!_listener.getConfig().index.empty()) index = _listener.getConfig().index;
    if (_listener.getConfig().isAutoindex == true) autoindex = _listener.getConfig().autoindex;
    if (!_headerContent.path.empty() && _headerContent.path[0] == '/')
    {
        _headerContent.path = _listener.getConfig().root + _headerContent.path;
        _headerContent.root = _listener.getConfig().root;
        checkPathValidity(_headerContent.path, index, autoindex, _listener.getConfig().root);
    }
}

void Client::checkPathValidity(std::string& path, std::vector<std::string>& index, bool autoindex, const std::string& root)
{
    struct stat st;
    if (stat(path.c_str(), &st) == 0)
    {
        if (S_ISREG(st.st_mode))
        {
            if (access(path.c_str(), R_OK) != 0) { _status = 404; return ; }
            if (!isWithinRoot(path, root)) { _status = 403; return ; }
            return ;
        }
        if (S_ISDIR(st.st_mode))
        {
            if (!index.empty())
            {
                std::vector<std::string>::iterator it = index.begin();
                std::vector<std::string>::iterator end = index.end();
                for (; it != end; ++it)
                {
                    if (access(joinPath(path, *it).c_str(), F_OK) == 0 || this->_headerContent.method == "POST")
                    {
                        if (access(joinPath(path, *it).c_str(), R_OK) != 0 && this->_headerContent.method != "POST") { _status = 404; return ; }
                        if (!isWithinRoot(joinPath(path, *it), root) && this->_headerContent.method != "POST") { _status = 403; return ; }
                        if (this->_headerContent.method != "POST") _headerContent.path = joinPath(_headerContent.path, *it);
                        return ;
                    }
                }
            }
            if (autoindex == true) _headerContent.isAutoindexResponse = true;
            else _status = 403;
            return ;
        }
    }
    if (this->_headerContent.method == "POST") return;
    _status = 404;
}

std::string Client::joinPath(const std::string& a, const std::string& b)
{
    if (a[a.size() - 1] == '/') return a + b;
    return a + "/" + b;
}
