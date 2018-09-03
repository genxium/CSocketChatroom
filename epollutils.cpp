#include "epollutils.h"
#include <cstddef>

// epoll part
int add_fd(int &ep_fd, std::set<int> &monitored_fds, int &fd, epoll_event &ev) {
	if (monitored_fds.find(fd) == monitored_fds.end()) monitored_fds.insert(fd);
	return epoll_ctl(ep_fd, EPOLL_CTL_ADD, fd, &ev);
}

int delete_fd(int &ep_fd, std::set<int> &monitored_fds, int &fd) {
	if (monitored_fds.find(fd) != monitored_fds.end()) monitored_fds.erase(fd);
	return epoll_ctl(ep_fd, EPOLL_CTL_DEL, fd, NULL);
}

int update_fd(int &ep_fd, int &fd, epoll_event &ev) {
	return epoll_ctl(ep_fd, EPOLL_CTL_MOD, fd, &ev);
}

