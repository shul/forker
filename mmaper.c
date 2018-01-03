#include <sys/mman.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>

static void * mmap_addr;

#define handle_error(msg)                                                                                              \
    do {                                                                                                               \
        perror(msg);                                                                                                   \
        exit(EXIT_FAILURE);                                                                                            \
    } while (0)

#define FILESZ (30L * 1024L * 1024L * 1024L)

#define CORE_FILTER "0x3f"

int allow_mmap_in_core()
{
    int fd = open("/proc/self/coredump_filter", O_WRONLY);
    if (fd < 0)
        handle_error("open");

    ssize_t written = write(fd, CORE_FILTER, sizeof(CORE_FILTER);
    if (written != sizeof(CORE_FILTER))
        handle_error("write");

    close(fd);
    return 0;
}

int map_memory(void)
{
    static char template[] = "/tmp/myfileXXXXXX";
    int res;

    allow_mmap_in_core();

    int fd = mkstemp(template);
    if (fd < 0)
        handle_error("mkstemp");
 
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

