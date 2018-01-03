#include "ParentSignalHandler.hh"
#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <signal.h>
#include <sys/signalfd.h>
#include <stdlib.h>
#include <time.h>
#include <sys/wait.h>
#include "common.h"

void ParentSignalHandler::handle()
{
    struct signalfd_siginfo fdsi;
    ssize_t sz;
    pid_t pid;
    int status;

    sz = read(fd, &fdsi, sizeof(struct signalfd_siginfo));
    if (sz != sizeof(struct signalfd_siginfo))
	handle_error("read parent signal handler");

    if (fdsi.ssi_signo == SIGINT) {
	fprintf(stderr, "%d: Got SIGINT from %d\n", getpid(), fdsi.ssi_pid);
    } else if (fdsi.ssi_signo == SIGQUIT) {
	fprintf(stderr, "%d: Got SIGQUIT from %d\n", getpid(), fdsi.ssi_pid);
	parent->exiting = 1;
	parent->notify_children(SIGQUIT);
    } else if (fdsi.ssi_signo == SIGTERM) {
	fprintf(stderr, "%d: Got SIGTERM from %d\n", getpid(), fdsi.ssi_pid);
	parent->exiting = 1;
	parent->notify_children(SIGTERM);
    } else if (fdsi.ssi_signo == SIGCHLD) {
	time_t t = time(NULL);
	struct tm *tm = localtime(&t);

	fprintf(stderr, "%d: Got SIGCHLD from %d (%s)\n", getpid(), fdsi.ssi_pid, asctime(tm));
	do {
	    pid = waitpid(-1, &status, WNOHANG);
	    if (pid > 0) {
                int child_idx = parent->get_child_idx(pid);
		fprintf(stderr, "%d: Process %d exited (idx:%d)\n", getpid(), pid, child_idx);
		parent->clear_child(pid);

		if (!parent->exiting) {
		    parent->respawn(child_idx);
		} else {
		    if (parent->n_children == 0) {
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

