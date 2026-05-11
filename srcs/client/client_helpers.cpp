#include "client_helpers.hpp"
#include "Client.hpp"
#include <cstring>
#include <cctype>
#include <sstream>
#include <map>
#include <unistd.h>
#include <limits.h>
#include <stdlib.h>
#include <errno.h>

std::string getMimeType(const std::string& path)
{
	if (path.size() >= 4 && path.substr(path.size() - 4) == ".jpg") return "image/jpeg";
	if (path.size() >= 5 && path.substr(path.size() - 5) == ".jpeg") return "image/jpeg";
	if (path.size() >= 4 && path.substr(path.size() - 4) == ".png") return "image/png";
	if (path.size() >= 4 && path.substr(path.size() - 4) == ".gif") return "image/gif";
	if (path.size() >= 4 && path.substr(path.size() - 4) == ".svg") return "image/svg+xml";
	if (path.size() >= 5 && path.substr(path.size() - 5) == ".html") return "text/html";
	if (path.size() >= 4 && path.substr(path.size() - 4) == ".htm") return "text/html";
	if (path.size() >= 4 && path.substr(path.size() - 4) == ".txt") return "text/plain";
	if (path.size() >= 5 && path.substr(path.size() - 5) == ".json") return "application/json";
	if (path.size() >= 4 && path.substr(path.size() - 4) == ".xml") return "application/xml";
	if (path.size() >= 4 && path.substr(path.size() - 4) == ".css") return "text/css";
	if (path.size() >= 3 && path.substr(path.size() - 3) == ".js") return "application/javascript";
	if (path.size() >= 4 && path.substr(path.size() - 4) == ".pdf") return "application/pdf";
	return "application/octet-stream";
}

const char* reasonPhrase(int code)
{
	switch (code)
	{
		/* 1xx Informational */
		case 100: return "Continue";
		case 101: return "Switching Protocols";
		case 102: return "Processing";
		case 103: return "Early Hints";

		/* 2xx Success */
		case 200: return "OK";
		case 201: return "Created";
		case 202: return "Accepted";
		case 203: return "Non-Authoritative Information";
		case 204: return "No Content";
		case 205: return "Reset Content";
		case 206: return "Partial Content";
		case 207: return "Multi-Status";
		case 208: return "Already Reported";
		case 226: return "IM Used";
		
		/* 3xx Redirection */
		case 300: return "Multiple Choices";
		case 301: return "Moved Permanently";
		case 302: return "Found";
		case 303: return "See Other";
		case 304: return "Not Modified";
		case 305: return "Use Proxy";
		case 306: return "Unused";
		case 307: return "Temporary Redirect";
		case 308: return "Permanent Redirect";
		
		/* 4xx Client Errors */
		case 400: return "Bad Request";
		case 401: return "Unauthorized";
		case 402: return "Payment Required";
		case 403: return "Forbidden";
		case 404: return "Not Found";
		case 405: return "Method Not Allowed";
		case 406: return "Not Acceptable";
		case 407: return "Proxy Authentication Required";
		case 408: return "Request Timeout";
		case 409: return "Conflict";
		case 410: return "Gone";
		case 411: return "Length Required";
		case 412: return "Precondition Failed";
		case 413: return "Payload Too Large";
		case 414: return "URI Too Long";
		case 415: return "Unsupported Media Type";
		case 416: return "Range Not Satisfiable";
		case 417: return "Expectation Failed";
		case 418: return "I'm a teapot";
		case 421: return "Misdirected Request";
		case 422: return "Unprocessable Content";
		case 423: return "Locked";
		case 424: return "Failed Dependency";
		case 425: return "Too Early";
		case 426: return "Upgrade Required";
		case 428: return "Precondition Required";
		case 429: return "Too Many Requests";
		case 431: return "Request Header Fields Too Large";
		case 451: return "Unavailable For Legal Reasons";
		
		/* 5xx Server Errors */
		case 500: return "Internal Server Error";
		case 501: return "Not Implemented";
		case 502: return "Bad Gateway";
		case 503: return "Service Unavailable";
		case 504: return "Gateway Timeout";
		case 505: return "HTTP Version Not Supported";
		case 506: return "Variant Also Negotiates";
		case 507: return "Insufficient Storage";
		case 508: return "Loop Detected";
		case 510: return "Not Extended";
		case 511: return "Network Authentication Required";
		default: return "Unknown HTTP Status";
	}
}

int hexVal(char c)
{
	if (c >= '0' && c <= '9') return c - '0';
	if (c >= 'a' && c <= 'f') return c - 'a' + 10;
	if (c >= 'A' && c <= 'F') return c - 'A' + 10;
	return -1;
}

std::string urlDecode(const std::string& s)
{
	std::string out;
	out.reserve(s.size());
	for (size_t i = 0; i < s.size(); ++i)
	{
		if (s[i] == '%' && i + 2 < s.size())
		{
			int hi = hexVal(s[i + 1]);
			int lo = hexVal(s[i + 2]);
			if (hi >= 0 && lo >= 0)
			{
				char decoded = (char)((hi << 4) | lo);
				out.push_back(decoded);
				i += 2;
				continue;
			}
		}
		out.push_back(s[i]);
	}
	return out;
}

std::string getDirectory(const std::string& path)
{
	if (path.empty())
		return std::string();
	std::string p = path;
	while (p.size() >= 2 && p[0] == '.' && p[1] == '/')
		p = p.substr(2);
	size_t pos = p.find_last_of('/');
	if (pos == std::string::npos)
		return std::string();
	if (pos == 0)
		return std::string("/");
	return p.substr(0, pos);
}

bool setNonBlocking(int fd)
{
	int flags = fcntl(fd, F_GETFL, 0);
	if (flags == -1) 
		return false;
	flags |= O_NONBLOCK;
	return (fcntl(fd, F_SETFL, flags) != -1);
}

static void freeEnvp_internal(char **envp)
{
	if (!envp)
		return;
	for (char **p = envp; *p != NULL; ++p)
		delete [] *p;
	delete [] envp;
}

char **buildEnvpFromMapAndParent(const std::map<std::string, std::string>& extras, char **parent_env)
{
	std::map<std::string, std::string> merged;
	if (parent_env)
	{
		for (char **p = parent_env; *p != NULL; ++p)
		{
			std::string s(*p);
			size_t eq = s.find('=');
			if (eq == std::string::npos)
				continue;
			std::string k = s.substr(0, eq);
			std::string v = s.substr(eq + 1);
			merged[k] = v;
		}
	}
	for (std::map<std::string, std::string>::const_iterator it = extras.begin(); it != extras.end(); ++it)
		merged[it->first] = it->second;
	char **envp = new char*[merged.size() + 1];
	try
	{
		size_t i = 0;
		for (std::map<std::string,std::string>::const_iterator it = merged.begin(); it != merged.end(); ++it)
		{
			std::string kv = it->first + "=" + it->second;
			envp[i] = new char[kv.size() + 1];
			std::memcpy(envp[i], kv.c_str(), kv.size() + 1);
			++i;
		}
		envp[i] = NULL;
		return envp;
	}
	catch(const std::bad_alloc& e)
	{
		freeEnvp_internal(envp);
		std::cerr << "Failed to allocate memory for envp: " << e.what() << '\n';
		return NULL;
	}
}

void freeEnvp(char **envp)
{
	freeEnvp_internal(envp);
}

std::string normalizePath(const std::string& p)
{
	std::string path = p;
	if (path.empty())
		return std::string("/");
	bool absolute = (path[0] =='/');
	std::vector<std::string> parts;
	size_t i = 0;
	while (i < path.size())
	{
		while (i < path.size() && path[i] == '/')
			++i;
		if (i >= path.size())
			break;
		size_t j = i;
		while (j < path.size() && path[j] != '/')
			++j;
		std::string token = path.substr(i, j - i);
		i = j;
		if (token == "." || token.empty())
			continue;
		else if (token == "..")
		{
			if (!parts.empty() && parts.back() != "..")
				parts.pop_back();
			else if (!absolute)
				parts.push_back("..");
		}
		else
			parts.push_back(token);
	}
	std::string out;
	if (absolute)
		out = "/";
	for (size_t k = 0; k < parts.size(); ++k)
	{
		if (!(absolute && k == 0) && out.size() > 0 && out[out.size() -1] != '/')
			out += "/";
		out += parts[k];
	}
	if (out.empty())
	{
		if (absolute)
			out = std::string("/");
		else
			out = std::string(".");
	}
	if (out.size() > 1 && out[out.size() - 1] == '/')
		out.erase(out.size() - 1);
	return out;
}

bool isWithinRoot(const std::string& candidate, const std::string& root)
{
	std::string r = normalizePath(root);
	std::string c = normalizePath(candidate);

	if (c.empty() || ((c[0] != '/') && (c[0] == '.' && c[1] != '/')))
		return false;
	if (r == "/")
		return true;
	if (r.size() > 1 && r[r.size() - 1] == '/')
		r.erase(r.size() - 1);
	if (c == r)
		return true;
	if (c.size() > r.size() && c.compare(0, r.size(), r) == 0 && c[r.size()] == '/')
		return true;
	return false;
}
