#pragma once

#include "Handler.hh"

class ChildSignalHandler: public Handler
{
public:
    ChildSignalHandler(int fd)
    : Handler(fd)
    {}

    virtual void handle();
};

