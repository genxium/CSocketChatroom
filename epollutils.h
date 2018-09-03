#ifndef _EPOLLUTILSH_
#define _EPOLLUTILSH_

#include <sys/epoll.h>
#include <set>

#define MAXEVENTS 64

// epoll part
int add_fd(int &ep_fd, std::set<int>& monitored_fds, int &fd, epoll_event &ev);
int delete_fd(int &ep_fd, std::set<int>& monitored_fds, int &fd);
int update_fd(int &ep_fd, int &fd, epoll_event &ev);

#endif
