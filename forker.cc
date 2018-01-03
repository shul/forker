#include <sys/signalfd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

#include "common.h"
#include "Parent.hh"


static Parent parent;

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

    parent.run_epoll(sfd);

    return EXIT_SUCCESS;
}
