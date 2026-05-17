#include "main/event_loop.hpp"
#include "main/cgi_fds.hpp"
#include "main/signals.hpp"
#include <unistd.h>
#include <iostream>
#include <algorithm>

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
			break;
		}
	}
}

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

int runEventLoop(ServerSocket& sockman,
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
