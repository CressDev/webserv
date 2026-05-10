#ifndef SIGNALS_HPP
#define SIGNALS_HPP

#include <vector>
#include <poll.h>

int setupSignalFd(std::vector<struct pollfd>& pollfds);
bool handleSignalEvent(int sigfd, bool &stop);

#endif // SIGNALS_HPP
