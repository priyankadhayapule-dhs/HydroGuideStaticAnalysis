//
// Copyright 2016 EchoNous, Inc.
//
#pragma once

#include <pthread.h>

class CriticalSection
{
public: // Methods
    CriticalSection();
    ~CriticalSection();

    void enter();
    bool tryEnter();
    void leave();

private: // Properties
    pthread_mutex_t     mMutex;
};
