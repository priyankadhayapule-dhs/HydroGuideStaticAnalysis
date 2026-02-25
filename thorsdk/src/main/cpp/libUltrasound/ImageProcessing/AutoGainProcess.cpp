//
// Copyright 2017 EchoNous Inc.
//
//

#include <AutoGainProcess.h>


//-----------------------------------------------------------------------------
AutoGainProcess::AutoGainProcess(const std::shared_ptr<UpsReader>& upsReader) :
    ImageProcess(upsReader),
    mFrameCount(0)
{
}

//-----------------------------------------------------------------------------
AutoGainProcess::~AutoGainProcess()
{
}

//-----------------------------------------------------------------------------
ThorStatus AutoGainProcess::init()
{
    ThorStatus  retVal = THOR_OK;

    mFrameCount = 0;

    return(retVal); 
}

//-----------------------------------------------------------------------------
ThorStatus AutoGainProcess::

setProcessParams(ProcessParams& params)
{
    ThorStatus  retVal = THOR_OK;

    getUpsReader()->getAutoGainParams(params.imagingCaseId,
                                      mAutoechoParams,
                                      mGainNoiseFrame);
    mAutoechoParams.numSamples = params.numSamples;
    mAutoechoParams.numRays = params.numRays;
    mAutoechoParams.dilationType = 0;

    mAutoecho.setParams(mAutoechoParams);
    mAutoecho.setNoiseSignal(mGainNoiseFrame);

    return(retVal); 
}

//-----------------------------------------------------------------------------
ThorStatus AutoGainProcess::process(uint8_t* inputPtr, uint8_t* outputPtr)
{
    ThorStatus  retVal = THOR_OK;

    if (0 == (mFrameCount++ % mAutoechoParams.gainCalcInterval))
    {
        mAutoecho.estimateGain(inputPtr);
    }

    mAutoecho.applyGain(inputPtr, outputPtr);

    return(retVal); 
}

//-----------------------------------------------------------------------------
void AutoGainProcess::terminate()
{
}

//-----------------------------------------------------------------------------
void AutoGainProcess::setUserGain(float userGain)
{
    mAutoecho.setUserGain(userGain);
}

