#pragma once

#include "Handler.hh"

class InotifyHandler: public Handler
{
public:
    InotifyHandler();

    int init(const char* path);

    virtual void handle();

protected:
    int inotify_fd;
    int watch_descriptor;
    const char* path;
};

