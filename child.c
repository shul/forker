#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>
#include <fcntl.h>

#include "common.h"

static int child_idx;

static void signal_handler(int sig, siginfo_t *si, void *arg)
{
    char path[PATH_MAX];
    pid_t pid = getpid();

    snprintf(path, sizeof(path), "/tmp/child_%d_%d", pid, child_idx);
    int fd = creat(path, S_IWUSR | S_IRUSR);
    if (fd < 0)
        handle_error("creat in signal handler");
    close(fd);

    fprintf(stderr, "Child idx %d pid: %d got signal: %d\n", child_idx, pid, sig);
    kill(pid, sig);
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

