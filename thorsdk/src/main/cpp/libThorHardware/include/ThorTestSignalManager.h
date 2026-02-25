//
// Copyright 2017 EchoNous, Inc.
//
// ----------------------------------------------------------------------------
//
// This class is to provide access to GPIOs on Thor that can be used for
// test signals.
//
// ----------------------------------------------------------------------------
#pragma once

#include <DauContext.h>
#include <CriticalSection.h>

enum class ThorTestSignal
{
    Out1 = 0,
    Out2,
    Out3,
    Out4,
    Out5,
};

class ThorTestSignalManager
{
public: // Methods
    ~ThorTestSignalManager();

    static ThorTestSignalManager& getInstance();

    bool open();

    bool open(DauContext* dauContextPtr);

    void close();

    void setSignal(const ThorTestSignal& signal, bool isSet);
    void pulseSignal(const ThorTestSignal& signal);

private: // Methods
    ThorTestSignalManager();

private: // Properties
    static const int    mSignalCount = (int) ThorTestSignal::Out5 + 1;
    int                 mFds[mSignalCount];
    int                 mOpenCount; // refcount for open()
    CriticalSection     mLock;
};
