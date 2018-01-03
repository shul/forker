#include "Parent.hh"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <signal.h>
#include <unistd.h>
#include "server.h"
#include "mmaper.h"
#include "common.h"

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
    run_server(sfd);
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

