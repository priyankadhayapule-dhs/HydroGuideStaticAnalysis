//
// Copyright 2017 EchoNous Inc.
//
//

#include <DeInterleaveProcess.h>


//-----------------------------------------------------------------------------
DeInterleaveProcess::

DeInterleaveProcess(const std::shared_ptr<UpsReader>& upsReader,
                    const std::shared_ptr<DauInputManager>& inputManager) :
    ImageProcess(upsReader),
    mInputManagerSPtr(inputManager)
{
}

//-----------------------------------------------------------------------------
DeInterleaveProcess::~DeInterleaveProcess()
{
}

//-----------------------------------------------------------------------------
ThorStatus DeInterleaveProcess::init()
{
    ThorStatus  retVal = THOR_OK;

    return(retVal); 
}

//-----------------------------------------------------------------------------
ThorStatus DeInterleaveProcess::

setProcessParams(ProcessParams& params)
{
    UNUSED(params);

    ThorStatus  retVal = THOR_OK;

    return(retVal); 
}

//-----------------------------------------------------------------------------
ThorStatus DeInterleaveProcess::process(uint8_t* inputPtr, uint8_t* outputPtr)
{
    ThorStatus  retVal = THOR_OK;

    mInputManagerSPtr->deinterleaveBData(inputPtr, outputPtr);

    return(retVal); 
}

//-----------------------------------------------------------------------------
void DeInterleaveProcess::terminate()
{
}


