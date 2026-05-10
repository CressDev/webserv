#ifndef CGI_FDS_HPP
#define CGI_FDS_HPP

#include "Client.hpp"
#include <vector>
#include <map>

void registerCgiFd(std::vector<struct pollfd>& pollfds, std::map<int, int>& map, int fd, int clientSock, short events);
void unregisterCgiFds(std::vector<struct pollfd>& pollfds, std::map<int, int>& map, int infd, int outfd);
void maybeRegisterClientCgiFds(std::vector<struct pollfd>& pollfds, std::map<int, int>& cgiFdToClientIdx, const Client& client);

#endif // CGI_FDS_HPP
