#pragma once

class Handler
{
public:
    Handler(int fd)
    : fd(fd)
    {}

    virtual void handle() = 0;

protected:
    int fd;
};

