#include <sys/signalfd.h>
#include <sys/epoll.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include "server.h"

#define handle_error(msg)                                                                                              \
    do {                                                                                                               \
        perror(msg);                                                                                                   \
        exit(EXIT_FAILURE);                                                                                            \
    } while (0)

#define MAX_CHILDREN 10

static pid_t children[MAX_CHILDREN];
static int n_children;

static int get_child_idx(pid_t pid)
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

static int find_empty_child_idx(void)
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

static void clear_child(pid_t pid)
{
    children[get_child_idx(pid)] = 0;
    --n_children;
}

#define MAX_EVENTS MAX_CHILDREN

void run_epoll(int sfd, int parent);

static void do_forks(int num, int sfd)
{
    pid_t pid;
    int i;

    for (i = 0; i < num; ++i) {
        pid = fork();
        if (pid == -1)
            handle_error("fork");

        if (pid) {
            fprintf(stderr, "Forked %d\n", pid);
            children[find_empty_child_idx()] = pid;
            ++n_children;
        } else {
            run_server();
        }
    }
}

static int exiting;

static void notify_children(int sig)
{
    for (int i = 0; i < MAX_CHILDREN; ++i) {
        if (children[i] != 0) {
            fprintf(stderr, "%d: Sending signal to %d\n", getpid(), children[i]);
            kill(children[i], sig);
        }
    }
}

void run_epoll(int sfd, int parent)
{
    struct epoll_event ev, events[MAX_EVENTS];
    int nfds, n, status;
    pid_t pid;
    struct signalfd_siginfo fdsi;
    ssize_t sz;

    int epollfd = epoll_create1(EPOLL_CLOEXEC);
    if (epollfd == -1) {
        handle_error("epoll_create1");
    }

    ev.events = EPOLLIN;
    ev.data.fd = sfd;
    if (epoll_ctl(epollfd, EPOLL_CTL_ADD, sfd, &ev) == -1) {
        handle_error("epoll_ctl: signalfd");
    }

    for (;;) {
        nfds = epoll_wait(epollfd, events, MAX_EVENTS, -1);
        if (nfds == -1) {
            handle_error("epoll_wait");
        }

        for (n = 0; n < nfds; ++n) {
            sz = read(events[n].data.fd, &fdsi, sizeof(struct signalfd_siginfo));
            if (sz != sizeof(struct signalfd_siginfo))
                handle_error("read");

            if (fdsi.ssi_signo == SIGINT) {
                fprintf(stderr, "%d: Got SIGINT from %d\n", getpid(), fdsi.ssi_pid);
            } else if (fdsi.ssi_signo == SIGQUIT) {
                fprintf(stderr, "%d: Got SIGQUIT from %d\n", getpid(), fdsi.ssi_pid);
                if (parent) {
                    exiting = 1;
                    notify_children(SIGQUIT);
                } else {
                    exit(EXIT_SUCCESS);
                }
            } else if (fdsi.ssi_signo == SIGTERM) {
                fprintf(stderr, "%d: Got SIGTERM from %d\n", getpid(), fdsi.ssi_pid);
                if (parent) {
                    exiting = 1;
                    notify_children(SIGTERM);
                } else {
                    exit(EXIT_SUCCESS);
                }
            } else if (fdsi.ssi_signo == SIGCHLD) {
                fprintf(stderr, "%d: Got SIGCHLD from %d\n", getpid(), fdsi.ssi_pid);
                do {
                    pid = waitpid(-1, &status, WNOHANG);
                    if (pid > 0) {
                        fprintf(stderr, "%d: Process %d exited\n", getpid(), pid);
                        clear_child(pid);

                        if (!exiting) {
                            do_forks(1, sfd);
                        } else {
                            if (n_children == 0) {
                                fprintf(stderr, "%d: All children exited\n", getpid());
                                exit(EXIT_SUCCESS);
                            }
                        }
                    }
                } while (pid > 0);
            } else {
                fprintf(stderr, "%d: Read unexpected signal from %d\n", getpid(), fdsi.ssi_pid);
            }
        }
    }
}

int main(int argc, char *argv[])
{
    sigset_t mask;
    int sfd, epollfd;
    struct epoll_event ev;

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

    // forking after epoll created leades to world of pain
    do_forks(MAX_CHILDREN, sfd);

    run_epoll(sfd, 1);

    return EXIT_SUCCESS;
}
