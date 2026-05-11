#include "main/signals.hpp"
#include <signal.h>
#include <sys/signalfd.h>
#include <unistd.h>
#include <cerrno>
#include <cstring>
#include <iostream>

int setupSignalFd(std::vector<struct pollfd>& pollfds)
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

bool handleSignalEvent(int sigfd, bool &stop)
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
