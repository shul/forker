#include <sys/signalfd.h>
#include <sys/epoll.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

#include "Parent.hh"
#include "Handler.hh"
#include "ParentSignalHandler.hh"
#include "ChildSignalHandler.hh"

#define handle_error(msg)                                                                                              \
    do {                                                                                                               \
        perror(msg);                                                                                                   \
        exit(EXIT_FAILURE);                                                                                            \
    } while (0)

#define MAX_EVENTS MAX_CHILDREN

void run_epoll(int sfd, int is_parent);

static Parent parent;

void run_epoll(int sfd, int is_parent)
{
    struct epoll_event ev, events[MAX_EVENTS];
    int nfds, n;

    int epollfd = epoll_create1(EPOLL_CLOEXEC);
    if (epollfd == -1) {
        handle_error("epoll_create1");
    }

    ev.events = EPOLLIN;
    if (is_parent)
        ev.data.ptr = new ParentSignalHandler(sfd, &parent);
    else
        ev.data.ptr = new ChildSignalHandler(sfd);

    if (epoll_ctl(epollfd, EPOLL_CTL_ADD, sfd, &ev) == -1) {
        handle_error("epoll_ctl: signalfd");
    }

    for (;;) {
        nfds = epoll_wait(epollfd, events, MAX_EVENTS, -1);
        if (nfds == -1) {
            handle_error("epoll_wait");
        }

        for (n = 0; n < nfds; ++n) {
            Handler *handler = (Handler*)ev.data.ptr;
            handler->handle();
        }
    }
}

int main(int argc, char *argv[])
{
    sigset_t mask;
    int sfd;

    sigemptyset(&mask);
    sigaddset(&mask, SIGINT);
    sigaddset(&mask, SIGTERM);
    sigaddset(&mask, SIGQUIT);
    sigaddset(&mask, SIGCHLD);

    /* Block signals so that they aren't handled
       according to their default dispositions */

    if (sigprocmask(SIG_BLOCK, &mask, NULL) == -1)
        handle_error("sigprocmask");

    sfd = signalfd(-1, &mask, SFD_CLOEXEC | SFD_NONBLOCK);
    if (sfd == -1)
        handle_error("signalfd");

    parent.set_sfd(sfd);

    // forking after epoll created leades to world of pain
    parent.do_forks(MAX_CHILDREN);

    run_epoll(sfd, 1);

    return EXIT_SUCCESS;
}
