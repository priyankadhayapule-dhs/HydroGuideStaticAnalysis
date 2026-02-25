//
// Copyright 2017 EchoNous Inc.
//
//

#include <DeInterleaveMProcess.h>


//-----------------------------------------------------------------------------
DeInterleaveMProcess::

DeInterleaveMProcess(const std::shared_ptr<UpsReader>& upsReader,
                    const std::shared_ptr<DauInputManager>& inputManager) :
    ImageProcess(upsReader),
    mInputManagerSPtr(inputManager)
{
}

//-----------------------------------------------------------------------------
DeInterleaveMProcess::~DeInterleaveMProcess()
{
}

//-----------------------------------------------------------------------------
ThorStatus DeInterleaveMProcess::init()
{
    ThorStatus  retVal = THOR_OK;

    return(retVal); 
}

//-----------------------------------------------------------------------------
ThorStatus DeInterleaveMProcess::setProcessParams(ProcessParams& params)
{
    UNUSED(params);

    ThorStatus  retVal = THOR_OK;

    return(retVal); 
}

//-----------------------------------------------------------------------------
void DeInterleaveMProcess::setMultilineSelection(uint32_t multilineSelection)
{
    mMultilineSelection = multilineSelection;
}

//-----------------------------------------------------------------------------
ThorStatus DeInterleaveMProcess::process(uint8_t* inputPtr, uint8_t* outputPtr)
{
    ThorStatus  retVal = THOR_OK;

    mInputManagerSPtr->deinterleaveMData(inputPtr, mMultilineSelection, outputPtr);
    return(retVal); 
}

//-----------------------------------------------------------------------------
void DeInterleaveMProcess::terminate()
{
}


