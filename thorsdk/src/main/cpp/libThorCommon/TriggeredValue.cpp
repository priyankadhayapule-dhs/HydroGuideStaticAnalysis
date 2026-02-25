//
// Copyright 2017 EchoNous, Inc.
//

#define LOG_TAG "TriggeredValue"

#include <ThorUtils.h>
#include <TriggeredValue.h>


//-----------------------------------------------------------------------------
TriggeredValue::TriggeredValue() :
    mValue(0)
{
    ALOGD("*** TriggeredValue - constructor");
}

//-----------------------------------------------------------------------------
TriggeredValue::~TriggeredValue()
{
    ALOGD("*** TriggeredValue - destructor");
}

//-----------------------------------------------------------------------------
bool TriggeredValue::init()
{
    bool    retVal = false;

    mLock.enter();
    mValue = 0;
    retVal = mEvent.init(EventMode::ManualReset, false);
    mLock.leave();

    return(retVal);
}

//-----------------------------------------------------------------------------
void TriggeredValue::set(uint64_t value)
{
    mLock.enter();
    mValue = value;
    mEvent.set();
    mLock.leave();
}

//-----------------------------------------------------------------------------
void TriggeredValue::reset()
{
    mLock.enter();
    mValue = 0;
    mEvent.reset();
    mLock.leave();
}

//-----------------------------------------------------------------------------
bool TriggeredValue::wait(int msecTimeout)
{
    return(mEvent.wait(msecTimeout));
}

//-----------------------------------------------------------------------------
uint64_t TriggeredValue::read()
{
    uint64_t    retValue;

    mLock.enter();
    retValue = mValue;
    mEvent.reset();
    mLock.leave();

    return(retValue);
}

//-----------------------------------------------------------------------------
int TriggeredValue::getFd()
{
    return(mEvent.getFd());
}

