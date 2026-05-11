#include "main/cgi_fds.hpp"
#include <iostream>
#include <algorithm>

void registerCgiFd(std::vector<struct pollfd>& pollfds, std::map<int, int>& map, int fd, int clientSock, short events)
{
	if (fd < 0)
		return;
	struct pollfd p; p.fd = fd; p.events = events; p.revents = 0; pollfds.push_back(p);
	map[fd] = clientSock;
	std::cerr << "registerCgiFd: registered fd=" << fd << " for clientSock=" << clientSock << " events=" << events << "\n";
}

void unregisterCgiFds(std::vector<struct pollfd>& pollfds, std::map<int, int>& map, int infd, int outfd)
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

void maybeRegisterClientCgiFds(std::vector<struct pollfd>& pollfds, std::map<int, int>& cgiFdToClientIdx, const Client& client)
{
	int inFd = client.getCgiInFd();
	int outFd = client.getCgiOutFd();
	if (inFd >= 0 && cgiFdToClientIdx.count(inFd) == 0)
		registerCgiFd(pollfds, cgiFdToClientIdx, inFd, client.getClientFd(), POLLOUT);
	if (outFd >= 0 && cgiFdToClientIdx.count(outFd) == 0)
		registerCgiFd(pollfds, cgiFdToClientIdx, outFd, client.getClientFd(), POLLIN);
}
