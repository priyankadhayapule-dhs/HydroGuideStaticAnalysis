//
// Copyright 2017 EchoNous Inc.
//
//
#define LOG_TAG "SpeckleReductionProcess"

#include <ThorUtils.h>
#include <SpeckleReductionProcess.h>


//-----------------------------------------------------------------------------
SpeckleReductionProcess::

SpeckleReductionProcess(const std::shared_ptr<UpsReader>& upsReader) :
    ImageProcess(upsReader)
{
	ALOGD("SpeckleReduction Version = 0x%X", 
	      mSpeckleReduction.getVersion());
}

//-----------------------------------------------------------------------------
SpeckleReductionProcess::~SpeckleReductionProcess()
{
}

//-----------------------------------------------------------------------------
ThorStatus SpeckleReductionProcess::init()
{
    ThorStatus  retVal = THOR_OK;

    return(retVal); 
}

//-----------------------------------------------------------------------------
ThorStatus SpeckleReductionProcess::

setProcessParams(ProcessParams& params)
{
    ThorStatus  retVal = THOR_OK;
    const int   ver = 1; // algorithm selection

    getUpsReader()->getSpeckleReductionParams(params.imagingCaseId, mSpecParams);

    if (0 == (params.numRays & 0x7))  // numRays is a multiple of 8
    {
        mSpeckleReduction.setParams(mSpecParams,
                                    params.numRays,
                                    params.numSamples,
                                    ver);
    }
    else
    {
        mSpeckleReduction.setParams(mSpecParams,
                                    params.numRays + (8 - (params.numRays & 0x7)), // round it up to nearest multiple of 8
                                    params.numSamples,
                                    ver);
    }

    return(retVal); 
}

//-----------------------------------------------------------------------------
ThorStatus SpeckleReductionProcess::process(uint8_t* inputPtr, uint8_t* outputPtr)
{
    ThorStatus  retVal = THOR_OK;

    mSpeckleReduction.processFrame(inputPtr, outputPtr);

    return(retVal); 
}

//-----------------------------------------------------------------------------
void SpeckleReductionProcess::terminate()
{
}


