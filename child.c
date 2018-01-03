#include <signal.h>
#include <stdio.h>
#include <string.h>

#include "common.h"

static int child_idx;

static void signal_handler(int sig, siginfo_t *si, void *arg)
{
    fprintf(stderr, "Child %d got signal: %d\n", child_idx, sig);
    kill(getpid(), sig);
}

typedef void (*sighandler_t)(int);

void set_child_idx(int idx)
{
    struct sigaction act;

    memset(&act, 0, sizeof(sigaction));
    sigemptyset(&act.sa_mask);
    act.sa_flags = (int)(SA_NODEFER | SA_RESETHAND | SA_SIGINFO);
    act.sa_sigaction = signal_handler;
    
    sigaction(SIGSEGV, &act, NULL);

    child_idx = idx;

}

