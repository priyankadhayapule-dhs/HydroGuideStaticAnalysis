//
// Copyright 2016 EchoNous, Inc.
//
#include <CriticalSection.h>

CriticalSection::CriticalSection()
{
    pthread_mutexattr_t attr;

    pthread_mutexattr_init(&attr);
    pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
    pthread_mutex_init(&mMutex, &attr);
}

CriticalSection::~CriticalSection()
{
    pthread_mutex_destroy(&mMutex);
}

void CriticalSection::enter()
{
    pthread_mutex_lock(&mMutex);
}

bool CriticalSection::tryEnter()
{
    return (pthread_mutex_trylock(&mMutex) == 0);
}

void CriticalSection::leave()
{
    pthread_mutex_unlock(&mMutex);
}
