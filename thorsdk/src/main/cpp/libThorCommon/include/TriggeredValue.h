//
// Copyright 2017 EchoNous, Inc.
//
// A threadsafe numeric value that sets an event each time it is changed.
// poll() / select() can be used to wait on the event (see getFd()).
//
#pragma once

#include <stdint.h>
#include <Event.h>
#include <CriticalSection.h>

class TriggeredValue
{
public: // Methods
    TriggeredValue();
    ~TriggeredValue();

    bool        init();

    void        set(uint64_t value);
    void        reset();

    bool        wait(int msecTimeout = -1);
    uint64_t    read();

    int         getFd();

private: // Properties
    uint64_t            mValue;
    Event               mEvent;
    CriticalSection     mLock;
};
