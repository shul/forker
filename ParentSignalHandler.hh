#pragma once

#include "Handler.hh"
#include "Parent.hh"

class ParentSignalHandler: public Handler
{
public:
    ParentSignalHandler(int fd, Parent* parent)
    : Handler(fd), parent(parent)
    {}

    virtual void handle();

protected:
    Parent* parent;
};

