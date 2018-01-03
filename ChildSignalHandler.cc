#include "ChildSignalHandler.hh"
#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <signal.h>
#include <sys/signalfd.h>
#include <stdlib.h>
#include "common.h"

void ChildSignalHandler::handle()
{
    struct signalfd_siginfo fdsi;
    ssize_t sz;

    sz = read(fd, &fdsi, sizeof(struct signalfd_siginfo));
    if (sz != sizeof(struct signalfd_siginfo))
	handle_error("read child signal handler");

    if (fdsi.ssi_signo == SIGINT) {
	fprintf(stderr, "%d: Got SIGINT from %d\n", getpid(), fdsi.ssi_pid);
        exit(EXIT_SUCCESS);
    } else if (fdsi.ssi_signo == SIGQUIT) {
	fprintf(stderr, "%d: Got SIGQUIT from %d\n", getpid(), fdsi.ssi_pid);
        exit(EXIT_SUCCESS);
    } else if (fdsi.ssi_signo == SIGTERM) {
	fprintf(stderr, "%d: Got SIGTERM from %d\n", getpid(), fdsi.ssi_pid);
        exit(EXIT_SUCCESS);
    } else {
	fprintf(stderr, "%d: Read unexpected signal from %d\n", getpid(), fdsi.ssi_pid);
    }
}

