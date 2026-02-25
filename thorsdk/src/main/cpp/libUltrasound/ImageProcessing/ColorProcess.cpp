//
// Copyright 2017 EchoNous Inc.
//
//
#include <ColorProcess.h>

//-----------------------------------------------------------------------------
ColorProcess::ColorProcess(const std::shared_ptr<UpsReader>& upsReader) :
    ImageProcess(upsReader),
    mFrameCount(0)
{
}

//-----------------------------------------------------------------------------
ColorProcess::~ColorProcess()
{
}

//-----------------------------------------------------------------------------
ThorStatus ColorProcess::init()
{
    ThorStatus  retVal = THOR_OK;

    mFrameCount = 0;

    return(retVal); 
}

//-----------------------------------------------------------------------------
ThorStatus ColorProcess::setProcessParams(ProcessParams& params)
{
    ThorStatus          retVal = THOR_ERROR;
    Colorflow::Params   colorParams;

    getUpsReader()->getCScanConverterParams(params.imagingCaseId, mCConverterParams);
    if (getUpsReader()->getColorflowParams(params.imagingCaseId, colorParams))
    {
        // set CVD as default mode
        colorParams.colorMode = COLOR_MODE_CVD;
        mColorflow.setParams(colorParams);
        mThresholds = colorParams.threshold;
        retVal = THOR_OK;
    }

    return(retVal); 
}

//-----------------------------------------------------------------------------
ThorStatus ColorProcess::setProcessParams(uint32_t imagingCaseId, ScanConverterParams scp,
                                          uint32_t colorMode)
{
    ThorStatus          retVal = THOR_ERROR;
    Colorflow::Params   colorParams;

    mCConverterParams = scp;

    if (getUpsReader()->getColorflowParams(imagingCaseId, colorParams))
    {
        colorParams.colorMode = colorMode;
        mColorflow.setParams(colorParams);
        mThresholds = colorParams.threshold;
        retVal = THOR_OK;
    }

    return(retVal); 
}

//-----------------------------------------------------------------------------
void ColorProcess::resetFrameCount()
{
    mFrameCount = 0;
}

//-----------------------------------------------------------------------------
ThorStatus ColorProcess::setGain(int gain)
{
    float rangeDb = mThresholds.gainRange;
    float threshDb = (mThresholds.power + rangeDb*0.5) - float(gain)*rangeDb*0.01;
    mColorflow.setPowerThreshold( threshDb );

    return(THOR_OK); 
}


//-----------------------------------------------------------------------------
ThorStatus ColorProcess::process(uint8_t* inputPtr, uint8_t* outputPtr)
{
    ThorStatus  retVal = THOR_OK;

    mColorflow.processFrame(inputPtr, 
                            mCConverterParams.numRays,
                            mCConverterParams.numSamples,
                            mFrameCount++,
                            outputPtr);

    return(retVal); 
}

//-----------------------------------------------------------------------------
void ColorProcess::terminate()
{
}


