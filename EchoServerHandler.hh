#pragma once

#include "Handler.hh"

class EchoServerHandler: public Handler
{
public:
    EchoServerHandler();

    int init();

    virtual void handle();

protected:
    int accept_fd;
};

