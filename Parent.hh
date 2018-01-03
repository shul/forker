#pragma once
#include <sys/types.h>
#include <string.h>

#define MAX_CHILDREN 10

class Parent
{
    friend class ParentSignalHandler;
public:
    Parent()
    : exiting(0), n_children(0), sfd(-1), epoll_fd(-1)
    {
        memset(children, 0, sizeof(children));
    }

    void set_sfd(int sfd)
    { this->sfd = sfd; }

    void set_epoll_fd(int fd)
    { epoll_fd = fd; }

    void notify_children(int sig);
    int get_child_idx(pid_t pid);
    int find_empty_child_idx(void);
    void clear_child(pid_t pid);
    void add_new_child(pid_t pid);
    void do_forks(int num);

protected:
    int exiting;

    pid_t children[MAX_CHILDREN];
    int n_children;
    int sfd;
    int epoll_fd;
};
