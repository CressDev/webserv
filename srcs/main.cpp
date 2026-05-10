#include "ServerSocket.hpp"
#include <poll.h>
#include <map>
#include <vector>
#include <string>
#include "Parser.hpp"
#include "Server.hpp"
#include "Client.hpp"
#include <typeinfo>
#include <signal.h>
#include <sys/signalfd.h>
#include <unistd.h>
#include <cerrno>
#include <cstring>


//AUXILIARES PARA PRINTEO DE SERVER PARSEADO
static void printVector(const std::vector<std::string>& v)
{
    for (std::vector<std::string>::size_type i = 0; i < v.size(); ++i)
    {
        std::cout << v[i];
        if (i + 1 < v.size()) std::cout << ", ";
    }
}

static void printLocationConfig(const LocationConfig& loc, std::vector<LocationConfig>::size_type j)
{
    std::cout << "  -- Location #" << j << " --\n";
    std::cout << "    Path: " << loc.path << "\n";
    std::cout << "    Root: " << loc.root << "\n";
    std::cout << "    Index: ";
    printVector(loc.index);
    std::cout << "\n";
    std::cout << "    Allowed methods: ";
    printVector(loc.allowed_methods);
    std::cout << "\n";
    std::cout << "    CGI:\n";
    for (std::map<std::string, std::string>::const_iterator it = loc.cgi.begin(); it != loc.cgi.end(); ++it)
        std::cout << "      " << it->first << " -> " << it->second << "\n";
    std::cout << "    Error pages:\n";
    for (std::map<std::pair<int, int>, std::string>::const_iterator it = loc.error_pages.begin(); it != loc.error_pages.end(); ++it)
        std::cout << "      " << it->first.first << " | " << it->first.second << " -> " << it->second << "\n";
    std::cout << "    Autoindex: " << loc.autoindex << "\n";
}

static void printServerConfig(const ServerConfig& cfg, std::vector<Server>::size_type i)
{
    std::cout << "=== Server #" << i << " ===\n";

    std::cout << "Listen:\n";
    for (std::multimap<std::string, std::string>::const_iterator it = cfg.listen.begin(); it != cfg.listen.end(); ++it)
        std::cout << "  " << it->first << ":" << it->second << "\n";

    std::cout << "CGI:\n";
    for (std::map<std::string, std::string>::const_iterator it = cfg.cgi.begin(); it != cfg.cgi.end(); ++it)
        std::cout << "  " << it->first << " -> " << it->second << "\n";

    std::cout << "Index: ";
    printVector(cfg.index);
    std::cout << "\n";

    std::cout << std::boolalpha;
    std::cout << "Autoindex: " << cfg.autoindex << "\n";
    std::cout << "Root: " << cfg.root << "\n";
    std::cout << "Server name: " << cfg.server_name << "\n";
    std::cout << "Client max body size: " << cfg.client_max_body_size << "\n";

    std::cout << "Allowed methods: ";
    printVector(cfg.allowed_methods);
    std::cout << "\n";

    std::cout << "Error pages:\n";
    for (std::map<std::pair<int, int>, std::string>::const_iterator it = cfg.error_pages.begin(); it != cfg.error_pages.end(); ++it)
        std::cout << "  " << it->first.first << " | " << it->first.second << " -> " << it->second << "\n";

    std::cout << "Locations (" << cfg.locations.size() << "):\n";
    for (std::vector<LocationConfig>::size_type j = 0; j < cfg.locations.size(); ++j)
        printLocationConfig(cfg.locations[j], j);

    std::cout << std::noboolalpha << "======================\n\n";
}

static void printParsedServers(const std::vector<Server>& servers)
{
    for (std::vector<Server>::size_type i = 0; i < servers.size(); ++i)
        printServerConfig(servers[i].getConfig(), i);
}

//CONFIGURACION SEÑALES
static int setupSignalFd(std::vector<struct pollfd>& pollfds)
{
    int sigfd = -1;
    sigset_t mask;
    sigemptyset(&mask);
    sigaddset(&mask, SIGINT);
    sigaddset(&mask, SIGTERM);
    sigaddset(&mask, SIGHUP);
    if (sigprocmask(SIG_BLOCK, &mask, NULL) == 0)
	{
        sigfd = signalfd(-1, &mask, SFD_NONBLOCK | SFD_CLOEXEC);
        if (sigfd >= 0)
		{
            struct pollfd p;
            p.fd = sigfd; p.events = POLLIN;
            p.revents = 0;
            pollfds.push_back(p);
        }
        else
            std::cerr << "Warning: failed to create signalfd: " << strerror(errno) << "\n";
    }
    else
        std::cerr << "Warning: failed to block signals for signalfd: " << strerror(errno) << "\n";
    return sigfd;
}

//REGISTROS CGI_FDS
static void registerCgiFd(std::vector<struct pollfd>& pollfds, std::map<int, int>& map, int fd, int clientSock, short events)
{
	if (fd < 0)
		return;
	struct pollfd p; p.fd = fd; p.events = events; p.revents = 0; pollfds.push_back(p);
	map[fd] = clientSock;
	std::cerr << "registerCgiFd: registered fd=" << fd << " for clientSock=" << clientSock << " events=" << events << "\n";
}

static void unregisterCgiFds(std::vector<struct pollfd>& pollfds, std::map<int, int>& map, int infd, int outfd)
{
	if (infd >= 0) map.erase(infd);
	if (outfd >= 0) map.erase(outfd);
	for (size_t k = 0; k < pollfds.size(); )
	{
		if (pollfds[k].fd == infd || pollfds[k].fd == outfd)
		{
			std::cerr << "unregisterCgiFds: removing fd=" << pollfds[k].fd << "\n";
			std::swap(pollfds[k], pollfds.back());
			pollfds.pop_back();
		}
		else
			++k;
	}
}

static void maybeRegisterClientCgiFds(std::vector<struct pollfd>& pollfds, std::map<int, int>& cgiFdToClientIdx, const Client& client)
{
	int inFd = client.getCgiInFd();
	int outFd = client.getCgiOutFd();
	if (inFd >= 0 && cgiFdToClientIdx.count(inFd) == 0)
		registerCgiFd(pollfds, cgiFdToClientIdx, inFd, client.getClientFd(), POLLOUT);
	if (outFd >= 0 && cgiFdToClientIdx.count(outFd) == 0)
		registerCgiFd(pollfds, cgiFdToClientIdx, outFd, client.getClientFd(), POLLIN);
}

// Manejo de signalfd
static bool handleSignalEvent(int sigfd, bool &stop)
{
    struct signalfd_siginfo fdsi;
    ssize_t s = read(sigfd, &fdsi, sizeof(fdsi));
    if (s == sizeof(fdsi))
    {
        if (fdsi.ssi_signo == SIGINT || fdsi.ssi_signo == SIGTERM)
        {
            std::cerr << "Received signal to terminate (" << fdsi.ssi_signo << "), shutting down...\n";
            stop = true;
            return true;
        }
        else if (fdsi.ssi_signo == SIGHUP)
        {
            std::cerr << "Received SIGHUP (" << fdsi.ssi_signo << ") - ignoring for now\n";
        }
    }
    return false;
}

// Manejo de POLLHUP
static void handlePollHup(size_t i, std::vector<struct pollfd>& pollfds, std::map<int, int>& cgiFdToClientIdx, std::vector<Client>& clients)
{
    int fd = pollfds[i].fd;
    if (cgiFdToClientIdx.count(fd))
    {
        int clientSock = cgiFdToClientIdx[fd];
        for (size_t ci = 0; ci < clients.size(); ++ci)
        {
            if (clients[ci].getClientFd() == clientSock)
            {
                clients[ci].handleCgiFdEvent(fd, pollfds[i].revents);
                if (!clients[ci].isCgiRunning())
                {
                    int infd = clients[ci].getCgiInFd();
                    int outfd = clients[ci].getCgiOutFd();
                    unregisterCgiFds(pollfds, cgiFdToClientIdx, infd, outfd);
                }
                break;
            }
        }
        return;
    }
    close(pollfds[i].fd);
    std::swap(pollfds[i], pollfds.back());
    pollfds.pop_back();
}

// Manejo cuando un listener acepta un nuevo cliente
static void handleListenerEvent(int fd, ServerSocket& sockman, std::vector<Server>& servers, std::vector<struct pollfd>& pollfds, std::vector<Client>& clients, std::map<int, int>& cgiFdToClientIdx, char** env)
{
    int client_fd = sockman.acceptNewClient(fd);
    std::cout << "listen fd: " << fd << " | client fd: " << client_fd << "\n";
    if (client_fd <= 0) return;
    struct pollfd newp;
    newp.fd = client_fd;
    newp.events = POLLIN;
    newp.revents = 0;
    pollfds.push_back(newp);
    for (size_t j = 0; j < servers.size(); ++j)
    {
        if (servers[j].getFd() == fd)
        {
            std::cout << "fd server = " << fd << std::endl;
            try
            {
                clients.push_back(Client(servers[j], client_fd));
                clients.back().setEnv(env);
            }
            catch (const std::exception& e)
            {
                std::cerr << "Failed to store client: " << e.what() << std::endl;
                close(client_fd);
                return;
            }
            std::cout << "cualquier tonteria AQUI\n";
            clients.back().chargeHeader();
            std::string reqHost = clients.back().getHeaderHost();
            if (!reqHost.empty())
            {
                for (size_t k = 0; k < servers.size(); ++k)
                {
                    const ServerConfig& cfg = servers[k].getConfig();
                    if (cfg.server_name == reqHost)
                    {
                        clients.back().setListener(servers[k]);
                        break;
                    }
                }
            }
            clients.back().chargeBody();
            if (clients.back().getIsHeaderReady() == true
                && (clients.back().getMethod() != "POST" || clients.back().getIsBodyReady() == true))
            {
                clients.back().sendResponse();
                if (clients.back().isCgiRunning())
                    maybeRegisterClientCgiFds(pollfds, cgiFdToClientIdx, clients.back());
                if (clients.back().getIsSent() == true)
                {
                    close(pollfds[pollfds.size() - 1].fd);
                    std::swap(clients.back(), clients[clients.size() - 1]);
                    clients.pop_back();
                    std::swap(pollfds[pollfds.size() - 1], pollfds.back());
                    pollfds.pop_back();
                }
                else
                {
                    if (clients.back().isCgiRunning())
                        pollfds[pollfds.size() - 1].events = POLLIN;
                    else
                        pollfds[pollfds.size() - 1].events = POLLOUT;
                }
            }
            std::cout << "cualquier tonteria AQUI2\n";
            break;
        }
    }
}

// Manejo de lectura desde socket cliente (no listener)
static void handleClientInEvent(int fd, size_t i, std::vector<struct pollfd>& pollfds, std::vector<Client>& clients, std::map<int, int>& cgiFdToClientIdx)
{
    for (size_t j = 0; j < clients.size(); ++j)
    {
        if (clients[j].getClientFd() == fd)
        {
            if (clients[j].getIsHeaderReady() == false)
            {
                clients[j].chargeHeader();
                if (clients[j].isCgiRunning()) {
                    maybeRegisterClientCgiFds(pollfds, cgiFdToClientIdx, clients[j]);
                }
                if (clients[j].getIsBodyReady() == true)
                {
                    if (clients[j].isCgiRunning())
                        pollfds[i].events = POLLIN;
                    else
                        pollfds[i].events = POLLOUT;
                }
            }
            else
            {
                clients[j].chargeBody();
                if (clients[j].isCgiRunning()) {
                    maybeRegisterClientCgiFds(pollfds, cgiFdToClientIdx, clients[j]);
                }
                if (clients[j].getIsBodyReady() == true)
                {
                    clients[j].sendResponse();
                    pollfds[i].events = POLLOUT;
                }
            }
            break;
        }
    }
}

// Manejo de evento en FD asociado a CGI
static void handleCgiFdEvent(int fd, size_t i, std::vector<struct pollfd>& pollfds, std::vector<Client>& clients, std::map<int, int>& cgiFdToClientIdx)
{
    int clientSock = cgiFdToClientIdx[fd];
    size_t clientIdx = static_cast<size_t>(-1);
    for (size_t ci = 0; ci < clients.size(); ++ci) {
        if (clients[ci].getClientFd() == clientSock) { clientIdx = ci; break; }
    }
    if (clientIdx == static_cast<size_t>(-1)) {
        unregisterCgiFds(pollfds, cgiFdToClientIdx, fd, -1);
    } else {
        int revents = pollfds[i].revents;
        clients[clientIdx].handleCgiFdEvent(fd, revents);
        if (!clients[clientIdx].isCgiRunning()) {
            int infd = clients[clientIdx].getCgiInFd();
            int outfd = clients[clientIdx].getCgiOutFd();
            unregisterCgiFds(pollfds, cgiFdToClientIdx, infd, outfd);
            clients[clientIdx].sendResponse();
            int clientFd = clients[clientIdx].getClientFd();
            for (size_t k = 0; k < pollfds.size(); ++k) {
                if (pollfds[k].fd == clientFd) {
                    pollfds[k].events = POLLOUT;
                    break;
                }
            }
        }
    }
}

// Manejo de POLLOUT
static void handlePollOutEvent(size_t i, std::vector<struct pollfd>& pollfds, std::vector<Client>& clients, std::map<int, int>& cgiFdToClientIdx)
{
    for (size_t j = 0; j < clients.size(); ++j)
    {
        if (clients[j].getClientFd() == pollfds[i].fd)
        {
            std::cout << "hace el pollout\n";
            clients[j].sendResponse();
            if (clients[j].isCgiRunning()) {
                maybeRegisterClientCgiFds(pollfds, cgiFdToClientIdx, clients[j]);
            }
            if (clients[j].getIsSent() == true)
            {
                close(pollfds[i].fd);
                std::swap(clients[j], clients.back());
                clients.pop_back();
                std::swap(pollfds[i], pollfds.back());
                pollfds.pop_back();
                if (i > 0)
                    --i;
            }
            else
            {
                if (clients[j].isCgiRunning())
                {
                    pollfds[i].events = POLLIN;
                }
            }
        }
    }
}

// LOOP PRINCIPAL
static int runEventLoop(ServerSocket& sockman,
                        std::vector<Server>& servers,
                        std::vector<struct pollfd>& pollfds,
                        int sigfd,
                        std::vector<Client>& clients,
                        std::map<int, int>& cgiFdToClientIdx,
                        char** env)
{
    bool stop = false;
    while (!stop)
    {
        nfds_t  nfds = static_cast<nfds_t>(pollfds.size());
        int ret = poll(pollfds.data(), nfds, -1);
        if (ret < 0)
        {
            std::cerr << "Poll not ready: " << strerror(errno) << "\n";
            return 1;
        }
        for (size_t ci = 0; ci < clients.size(); ++ci) {
            maybeRegisterClientCgiFds(pollfds, cgiFdToClientIdx, clients[ci]);
        }

        for(std::vector<struct pollfd>::size_type i = 0; i < pollfds.size(); ++i)
        {
            if (sigfd >= 0 && pollfds[i].fd == sigfd && (pollfds[i].revents & POLLIN))
            {
                if (handleSignalEvent(sigfd, stop))
                    break;
                continue;
            }
            if (pollfds[i].revents & POLLHUP)
            {
                handlePollHup(i, pollfds, cgiFdToClientIdx, clients);
                continue;
            }
            if(pollfds[i].revents & POLLIN)
            {
                int fd = pollfds[i].fd;
                bool    is_listen = sockman.isListener(fd);
                if (is_listen)
                {
                    handleListenerEvent(fd, sockman, servers, pollfds, clients, cgiFdToClientIdx, env);
                }
                else
                {
                    handleClientInEvent(fd, i, pollfds, clients, cgiFdToClientIdx);
                    if (cgiFdToClientIdx.count(fd)) {
                        handleCgiFdEvent(fd, i, pollfds, clients, cgiFdToClientIdx);
                    }
                }
            }
            if (pollfds[i].revents & POLLOUT)
            {
                handlePollOutEvent(i, pollfds, clients, cgiFdToClientIdx);
            }
        }
    }
    return 0;
}



int main (int argc, char** argv, char** env)
{
    if (argc != 2)
    {
        std::cerr << "Invalid arguments\n";
        return 1;
    }
    try
    {
        std::vector<Server> servers;
        Parser              parser(argv[1], servers);
        (void)parser;
		//PRINTEO DE LOS SERVER PARSEADOS
        printParsedServers(servers);

        ServerSocket sockman;
        if (!sockman.createListeners(servers))
        {
            std::cerr << "No listeners created\n";
            return 1;
        }
        std::vector<struct pollfd> pollfds = sockman.getPollfds();
        int sigfd = setupSignalFd(pollfds);
        std::vector<Client> clients;
        std::map<int, int> cgiFdToClientIdx;
        int rc = runEventLoop(sockman, servers, pollfds, sigfd, clients, cgiFdToClientIdx, env);
        if (rc != 0)
            return rc;
    }
    catch (const std::exception& e)
    {
		std::cerr << "Uncaught exception (" << typeid(e).name() << "): " << e.what() << std::endl;
        return 1;
    }
}
