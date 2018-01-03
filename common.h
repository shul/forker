#pragma once
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>

#define handle_error(msg)                                                                                              \
    do {                                                                                                               \
        perror(msg);                                                                                                   \
        exit(EXIT_FAILURE);                                                                                            \
    } while (0)


