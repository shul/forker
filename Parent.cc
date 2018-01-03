#include "Parent.hh"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <signal.h>
#include <unistd.h>
#include <sys/epoll.h>

#include "mmaper.h"
#include "common.h"

#include "Handler.hh"
#include "ParentSignalHandler.hh"
#include "ChildSignalHandler.hh"
#include "InotifyHandler.hh"
#include "EchoServerHandler.hh"

#define MAX_EVENTS MAX_CHILDREN

int Parent::get_child_idx(pid_t pid)
{
    int res = -1;

    for (int i = 0; i < MAX_CHILDREN; ++i) {
        if (children[i] == pid) {
            res = i;
            break;
        }
    }

    assert(res != -1);

    return res;
}

int Parent::find_empty_child_idx(void)
{
    int res = -1;

    for (int i = 0; i < MAX_CHILDREN; ++i) {
        if (children[i] == 0) {
            res = i;
            break;
        }
    }

    assert(res != -1);

    return res;
}

void Parent::clear_child(pid_t pid)
{
    children[get_child_idx(pid)] = 0;
    --n_children;
}

void Parent::notify_children(int sig)
{
    for (int i = 0; i < MAX_CHILDREN; ++i) {
        if (children[i] != 0) {
            fprintf(stderr, "%d: Sending signal to %d\n", getpid(), children[i]);
            kill(children[i], sig);
        }
    }
}

void Parent::add_new_child(pid_t pid)
{
    children[find_empty_child_idx()] = pid;
    ++n_children;
}

void Parent::set_new_child(int child_idx, pid_t pid)
{
    assert(children[child_idx] == 0);

    children[child_idx] = pid;
    ++n_children;
}

void Parent::child_fn(int idx)
{
    map_memory(idx);
    run_epoll(0);
}

void Parent::respawn(int child_idx)
{
    pid_t pid = fork();
    if (pid == -1)
        handle_error("fork respawn");

    if (pid) {
        fprintf(stderr, "Respawned %d\n", pid);
        set_new_child(child_idx, pid);
    } else {
        child_fn(child_idx);
    }
}

void Parent::do_forks(int num)
{
    pid_t pid;
    int i;
    int idx;

    for (i = 0; i < num; ++i) {
        idx = find_empty_child_idx();
        pid = fork();
        if (pid == -1)
            handle_error("fork");

        if (pid) {
            fprintf(stderr, "Forked %d\n", pid);
            set_new_child(idx, pid);
        } else {
            child_fn(idx);
        }
    }
}

void Parent::run_epoll(int is_parent)
{
    struct epoll_event ev, events[MAX_EVENTS];
    int nfds, n;

    int epollfd = epoll_create1(EPOLL_CLOEXEC);
    if (epollfd == -1) {
        handle_error("epoll_create1");
    }

    ev.events = EPOLLIN;
    if (is_parent) {
        struct epoll_event inotify_ev;

        ev.data.ptr = new ParentSignalHandler(sfd, this);
        set_epoll_fd(epollfd);

        InotifyHandler *ino_handler = new InotifyHandler();

        inotify_ev.events = EPOLLIN;
        inotify_ev.data.ptr = ino_handler;

        int inotify_fd = ino_handler->init("/tmp");

        if (epoll_ctl(epollfd, EPOLL_CTL_ADD, inotify_fd, &inotify_ev) == -1) {
            handle_error("epoll_ctl: inotify_fd");
        }
    } else {
        ev.data.ptr = new ChildSignalHandler(sfd);

        struct epoll_event accept_ev;

        EchoServerHandler *echo_handler = new EchoServerHandler();

        accept_ev.events = EPOLLIN;
        accept_ev.data.ptr = echo_handler;

        int accept_fd = echo_handler->init();

        if (epoll_ctl(epollfd, EPOLL_CTL_ADD, accept_fd, &accept_ev) == -1) {
            handle_error("epoll_ctl: accept_fd");
        }
    }

    if (epoll_ctl(epollfd, EPOLL_CTL_ADD, sfd, &ev) == -1) {
        handle_error("epoll_ctl: signalfd");
    }

    for (;;) {
        nfds = epoll_wait(epollfd, events, MAX_EVENTS, -1);
        if (nfds == -1) {
            handle_error("epoll_wait");
        }

        for (n = 0; n < nfds; ++n) {
            Handler *handler = (Handler*)events[n].data.ptr;
            handler->handle();
        }
    }
}
