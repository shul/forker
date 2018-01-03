#include <sys/mman.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <limits.h>

#include "common.h"

static void * mmap_addr;

//#define FILESZ (30L * 1024L * 1024L * 1024L)
#define FILESZ (30L * 1024L)

#define CORE_FILTER "0x3f"

int allow_mmap_in_core()
{
    int fd = open("/proc/self/coredump_filter", O_WRONLY);
    if (fd < 0)
        handle_error("open");

    ssize_t written = write(fd, CORE_FILTER, sizeof(CORE_FILTER));
    if (written != sizeof(CORE_FILTER))
        handle_error("write");

    close(fd);
    return 0;
}

int map_memory(int idx)
{
    static char template[] = "/tmp/myfileXXXXXX";
    int res;

    allow_mmap_in_core();

#if 0
    int fd = mkstemp(template);
    if (fd < 0)
        handle_error("mkstemp");
#endif

    char fname[PATH_MAX];
    snprintf(fname, sizeof(fname), "/tmp/mmaped_file_%d", idx);
    int fd = open(fname, O_CREAT | O_RDWR, S_IWUSR | S_IRUSR);
    if (fd < 0)
        handle_error("open");

    res = ftruncate(fd, FILESZ);
    if (res)
        handle_error("ftruncate");

#if 0  
    res = posix_fallocate(fd, 0, FILESZ);
    if (res)
        handle_error("posix_fallocate");
#endif

    mmap_addr = mmap(NULL, FILESZ, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (mmap_addr == MAP_FAILED)
        handle_error("mmap");

    close(fd);

    return 0;
}

