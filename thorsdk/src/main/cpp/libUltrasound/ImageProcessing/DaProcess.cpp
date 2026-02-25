//
// Copyright 2019 EchoNous Inc.
//
//
#define LOG_TAG "DaProcess"

#include "DaProcess.h"
#include "../include/ImageProcessing/DaProcess.h"
#include <DauInputManager.h>
#include <ThorUtils.h>
#include <math.h>

//import numpy as np
//import matplotlib.pyplot as plt
//import scipy
//from scipy import signal
//import math
//
//baseClk = 2.5e9;
//sysClk = baseClk/24;
//afeClk = sysClk/4;
//
//afeClksPerDaSample = 2048;
//fSampleRate = afeClk/afeClksPerDaSample;
//f1 = 30
//f2 = 600
//afCoef = scipy.signal.firwin(numtaps=360, cutoff=[f1, f2], window="hamming", scale=True, fs=fSampleRate, pass_zero=False)
//wa, ha = signal.freqz(afCoef)
//fig = plt.figure()
//
//plt.title('DA FIR Filter Response')
//plt.plot(fSampleRate*wa/(2.0*math.pi), 20*np.log10(abs(ha)), 'b')
//plt.show()

#ifndef BYPASS_FILTER
// 360 taps for - 6db cutoff at 30Hz
static float daCoeffs[] = \
{
        -3.89639215e-05, -1.23028510e-06,  3.12016312e-05,  5.53429184e-05,
        6.87905726e-05,  6.99227303e-05,  5.80442780e-05,  3.34709819e-05,
        -2.45670923e-06, -4.74351944e-05, -9.83349999e-05, -1.51409034e-04,
        -2.02561525e-04, -2.47661861e-04, -2.82882564e-04, -3.05036618e-04,
        -3.11886905e-04, -3.02399939e-04, -2.76917921e-04, -2.37227382e-04,
        -1.86509374e-04, -1.29165004e-04, -7.05204908e-05, -1.64270597e-05,
        2.72180400e-05,  5.49934367e-05,  6.24751790e-05,  4.67106151e-05,
        6.59733767e-06, -5.68755878e-05, -1.40557927e-04, -2.39260925e-04,
        -3.46076931e-04, -4.52882573e-04, -5.50993660e-04, -6.31923054e-04,
        -6.88179019e-04, -7.14032638e-04, -7.06180057e-04, -6.64229358e-04,
        -5.90952932e-04, -4.92263822e-04, -3.76897464e-04, -2.55806709e-04,
        -1.41305760e-04, -4.60250660e-05,  1.82383433e-05,  4.16746087e-05,
        1.75107741e-05, -5.71494038e-05, -1.80789082e-04, -3.47316733e-04,
        -5.46320195e-04, -7.63740776e-04, -9.82932541e-04, -1.18603324e-03,
        -1.35553914e-03, -1.47594975e-03, -1.53533318e-03, -1.52666068e-03,
        -1.44877089e-03, -1.30685040e-03, -1.11235519e-03, -8.82345514e-04,
        -6.38259753e-04, -4.04206914e-04, -2.04907284e-04, -6.34513405e-05,
        9.25812155e-07, -2.51530148e-05, -1.47626518e-04, -3.64007589e-04,
        -6.63115328e-04, -1.02559288e-03, -1.42520847e-03, -1.83086414e-03,
        -2.20916699e-03, -2.52735738e-03, -2.75634414e-03, -2.87357311e-03,
        -2.86545575e-03, -2.72911002e-03, -2.47321604e-03, -2.11785950e-03,
        -1.69332265e-03, -1.23787679e-03, -7.94725326e-04, -4.08332015e-04,
        -1.20437819e-04,  3.38857300e-05,  2.97840953e-05, -1.44576645e-04,
        -4.86244699e-04, -9.76970807e-04, -1.58406120e-03, -2.26258779e-03,
        -2.95882416e-03, -3.61465737e-03, -4.17262464e-03, -4.58114913e-03,
        -4.79951012e-03, -4.80208339e-03, -4.58143111e-03, -4.14990382e-03,
        -3.53953488e-03, -2.80015186e-03, -1.99578620e-03, -1.19962052e-03,
        -4.87856265e-04,  6.69993215e-05,  4.02851615e-04,  4.74138302e-04,
        2.56770767e-04, -2.48604904e-04, -1.01540311e-03, -1.99216001e-03,
        -3.10587826e-03, -4.26746925e-03, -5.37891072e-03, -6.34156592e-03,
        -7.06498030e-03, -7.47539574e-03, -7.52321039e-03, -7.18866836e-03,
        -6.48518495e-03, -5.45989352e-03, -4.19122525e-03, -2.78358532e-03,
        -1.35944816e-03, -4.94370108e-05,  1.01884169e-03,  1.73229171e-03,
        2.00315602e-03,  1.77819796e-03,  1.04534517e-03, -1.62917217e-04,
        -1.76987183e-03, -3.65878267e-03, -5.68084003e-03, -7.66635083e-03,
        -9.43838526e-03, -1.08278399e-02, -1.16887037e-02, -1.19122321e-02,
        -1.14387619e-02, -1.02660301e-02, -8.45309600e-03, -6.11928493e-03,
        -3.43795694e-03, -6.25319160e-04,  2.07507846e-03,  4.41114019e-03,
        6.14297025e-03,  7.06399827e-03,  7.02080722e-03,  5.93000766e-03,
        3.79065680e-03,  6.91000694e-04, -3.19129424e-03, -7.59578544e-03,
        -1.21926239e-02, -1.66019763e-02, -2.04183349e-02, -2.32381364e-02,
        -2.46887943e-02, -2.44570775e-02, -2.23147404e-02, -1.81394381e-02,
        -1.19292496e-02, -3.80954077e-03,  5.96857631e-03,  1.70383439e-02,
        2.89355649e-02,  4.11238671e-02,  5.30254548e-02,  6.40553563e-02,
        7.36568960e-02,  8.13360009e-02,  8.66920051e-02,  8.94428343e-02,
        8.94428343e-02,  8.66920051e-02,  8.13360009e-02,  7.36568960e-02,
        6.40553563e-02,  5.30254548e-02,  4.11238671e-02,  2.89355649e-02,
        1.70383439e-02,  5.96857631e-03, -3.80954077e-03, -1.19292496e-02,
        -1.81394381e-02, -2.23147404e-02, -2.44570775e-02, -2.46887943e-02,
        -2.32381364e-02, -2.04183349e-02, -1.66019763e-02, -1.21926239e-02,
        -7.59578544e-03, -3.19129424e-03,  6.91000694e-04,  3.79065680e-03,
        5.93000766e-03,  7.02080722e-03,  7.06399827e-03,  6.14297025e-03,
        4.41114019e-03,  2.07507846e-03, -6.25319160e-04, -3.43795694e-03,
        -6.11928493e-03, -8.45309600e-03, -1.02660301e-02, -1.14387619e-02,
        -1.19122321e-02, -1.16887037e-02, -1.08278399e-02, -9.43838526e-03,
        -7.66635083e-03, -5.68084003e-03, -3.65878267e-03, -1.76987183e-03,
        -1.62917217e-04,  1.04534517e-03,  1.77819796e-03,  2.00315602e-03,
        1.73229171e-03,  1.01884169e-03, -4.94370108e-05, -1.35944816e-03,
        -2.78358532e-03, -4.19122525e-03, -5.45989352e-03, -6.48518495e-03,
        -7.18866836e-03, -7.52321039e-03, -7.47539574e-03, -7.06498030e-03,
        -6.34156592e-03, -5.37891072e-03, -4.26746925e-03, -3.10587826e-03,
        -1.99216001e-03, -1.01540311e-03, -2.48604904e-04,  2.56770767e-04,
        4.74138302e-04,  4.02851615e-04,  6.69993215e-05, -4.87856265e-04,
        -1.19962052e-03, -1.99578620e-03, -2.80015186e-03, -3.53953488e-03,
        -4.14990382e-03, -4.58143111e-03, -4.80208339e-03, -4.79951012e-03,
        -4.58114913e-03, -4.17262464e-03, -3.61465737e-03, -2.95882416e-03,
        -2.26258779e-03, -1.58406120e-03, -9.76970807e-04, -4.86244699e-04,
        -1.44576645e-04,  2.97840953e-05,  3.38857300e-05, -1.20437819e-04,
        -4.08332015e-04, -7.94725326e-04, -1.23787679e-03, -1.69332265e-03,
        -2.11785950e-03, -2.47321604e-03, -2.72911002e-03, -2.86545575e-03,
        -2.87357311e-03, -2.75634414e-03, -2.52735738e-03, -2.20916699e-03,
        -1.83086414e-03, -1.42520847e-03, -1.02559288e-03, -6.63115328e-04,
        -3.64007589e-04, -1.47626518e-04, -2.51530148e-05,  9.25812155e-07,
        -6.34513405e-05, -2.04907284e-04, -4.04206914e-04, -6.38259753e-04,
        -8.82345514e-04, -1.11235519e-03, -1.30685040e-03, -1.44877089e-03,
        -1.52666068e-03, -1.53533318e-03, -1.47594975e-03, -1.35553914e-03,
        -1.18603324e-03, -9.82932541e-04, -7.63740776e-04, -5.46320195e-04,
        -3.47316733e-04, -1.80789082e-04, -5.71494038e-05,  1.75107741e-05,
        4.16746087e-05,  1.82383433e-05, -4.60250660e-05, -1.41305760e-04,
        -2.55806709e-04, -3.76897464e-04, -4.92263822e-04, -5.90952932e-04,
        -6.64229358e-04, -7.06180057e-04, -7.14032638e-04, -6.88179019e-04,
        -6.31923054e-04, -5.50993660e-04, -4.52882573e-04, -3.46076931e-04,
        -2.39260925e-04, -1.40557927e-04, -5.68755878e-05,  6.59733767e-06,
        4.67106151e-05,  6.24751790e-05,  5.49934367e-05,  2.72180400e-05,
        -1.64270597e-05, -7.05204908e-05, -1.29165004e-04, -1.86509374e-04,
        -2.37227382e-04, -2.76917921e-04, -3.02399939e-04, -3.11886905e-04,
        -3.05036618e-04, -2.82882564e-04, -2.47661861e-04, -2.02561525e-04,
        -1.51409034e-04, -9.83349999e-05, -4.74351944e-05, -2.45670923e-06,
        3.34709819e-05,  5.80442780e-05,  6.99227303e-05,  6.87905726e-05,
        5.53429184e-05,  3.12016312e-05, -1.23028510e-06, -3.89639215e-05
};
#else
// for testing purposes: bypass output
static float daCoeffs[] = \
{
        0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0.,
        0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0.,
        0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0.,
        0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0.,
        0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0.,
        0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0.,
        0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0.,
        0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0.,
        0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0.,
        0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0.,
        0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0.,
        0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0.,
        0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0.,
        0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0.,
        0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0.,
        0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0.,
        0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0.,
        0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0.,
        0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0.,
        0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0.,
        0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0.,
        0., 0., 1.
};
#endif

// IIR filter coefficients (for DC offset)
// cutoff 4 Hz - 2nd order
static float b[] = \
{
        0.8910317, -1.7820634,  0.8910317
};

static float a[] = \
{
        1.        , -1.99950791,  0.99950813
};


//-----------------------------------------------------------------------------
DaProcess::DaProcess(const std::shared_ptr<DauMemory>& daum,
                     const std::shared_ptr<UpsReader> &upsReader) :
    mDaumSPtr(daum),
    mUpsReaderSPtr(upsReader),
    mGain(1.0f)
{
    LOGD("DaProcess - constructor");
}



//-----------------------------------------------------------------------------
DaProcess::~DaProcess()
{
#ifdef DA_SIGNAL_TESTING
    delete mSineOsc;
#endif

    LOGD("DaProcess - destructor");
}

//-----------------------------------------------------------------------------
ThorStatus DaProcess::init()
{
    ThorStatus retVal = THOR_OK;

    return(retVal);
}

//-----------------------------------------------------------------------------
ThorStatus DaProcess::setProcessParams(uint32_t samplesPerFrame, uint32_t organId)
{
    LOGD("DaProcess: samples per frame: %d  organId: %d", samplesPerFrame, organId);

    mSamplesPerFrame = samplesPerFrame;

    //////////////////////////////////////////////////////////////
    // IMPORTANT: for VVR Release - organId is hardcoded.
    //////////////////////////////////////////////////////////////
    mFilterSize = mUpsReaderSPtr->getDaCoeff(0, mDaCoeffs);  //organId -> 0
    LOGD("DaProcess: Filter Size %d", mFilterSize);

    return(THOR_OK);
}

//-----------------------------------------------------------------------------
ThorStatus DaProcess::initializeProcessIndices()
{

    for (int i = 0; i < mFilterSize; i++)
    {
        mProcessingBuffer[i] = 0.0f;
    }

    mInputDataIndex = mFilterSize - 1;
    mProcessingIndex = 0;

    mXn_1 = 0.0f;
    mXn_2 = 0.0f;
    mYn_1 = 0.0f;
    mYn_2 = 0.0f;
    mHardLimit = 1.0f;

#ifdef DA_SIGNAL_TESTING
    mSineOsc = new SineGenerator();
    mSineOsc->setup(10.0, 12716, 0.25);
#endif

    return (THOR_OK);
}

//-----------------------------------------------------------------------------
ThorStatus DaProcess::setCineBuffer(CineBuffer *cineBuffer)
{
    mCineBuffer = cineBuffer;
    return (THOR_OK);
}

//-----------------------------------------------------------------------------
ThorStatus DaProcess::setGain(float dBGain)
{
    mGain = pow(10.0f, dBGain / 20.0f);
    return (THOR_OK);
}

//-----------------------------------------------------------------------------
ThorStatus DaProcess::process(uint8_t* inputPtr, uint32_t frameNum)
{
    uint8_t *outputPtr = mCineBuffer->getFrame(frameNum, DAU_DATA_TYPE_DA, CineBuffer::FrameContents::IncludeHeader);
    CineBuffer::CineFrameHeader* cineHeaderPtr = reinterpret_cast<CineBuffer::CineFrameHeader*>(outputPtr);
    cineHeaderPtr->frameNum = frameNum;
    cineHeaderPtr->timeStamp = reinterpret_cast<const DauInputManager::FrameHeader *>(inputPtr)->timeStamp;

    uint32_t *inputWordPtr = (uint32_t *)inputPtr;
    outputPtr =  mCineBuffer->getFrame(frameNum, DAU_DATA_TYPE_DA, CineBuffer::FrameContents::DataOnly);
    float *outputFloatPtr = (float *)outputPtr;

    uint32_t val;
    mDaumSPtr->read( SYS_TEST_ADDR, &val, 1);
    if ( val > 3 || val < 1)
    {
        LOGE("invalid DA channel selection specified in test register %d", val);
        val = 0;
    }

#ifndef DA_SIGNAL_TESTING
    inputWordPtr += 4; // skip past header
    for (int i = 0; i < mSamplesPerFrame; i++)
    {
        uint32_t uval = *(inputWordPtr + val);
        float fval = 2.0 * (((uval & 0x3FF) / 1024.0) - 0.5);

        // 2nd iir filter
        float iir_fval = b[0] * fval  + b[1] * mXn_1 + b[2] * mXn_2
                         - a[1] * mYn_1 - a[2] * mYn_2;

        mXn_2 = mXn_1;
        mXn_1 = fval;

        mYn_2 = mYn_1;
        mYn_1 = iir_fval;

        // apply gain
        iir_fval *= mGain;

        // apply hard Limiter
        if (iir_fval > mHardLimit)
            iir_fval = mHardLimit;

        if (iir_fval < -mHardLimit)
            iir_fval = -mHardLimit;

        mProcessingBuffer[mInputDataIndex] = iir_fval;
        mInputDataIndex++;

        if (mInputDataIndex > DA_PROCESSING_BUFFER_SIZE - 1)
        {
            mInputDataIndex -= DA_PROCESSING_BUFFER_SIZE;
        }
        inputWordPtr += 4; // 4 words per sample
    }
#else
    float test_sig[mSamplesPerFrame];
    mSineOsc->render(test_sig, 1, mSamplesPerFrame);
    for (int i = 0; i < mSamplesPerFrame; i++)
    {
        float fval = test_sig[i];

        mProcessingBuffer[mInputDataIndex] = fval;
        mInputDataIndex++;

        if (mInputDataIndex > DA_PROCESSING_BUFFER_SIZE - 1)
        {
            mInputDataIndex -= DA_PROCESSING_BUFFER_SIZE;
        }
    }
#endif

    for (int i = 0; i < mSamplesPerFrame; i++)
    {
        float fval = 0.0f;
        int current_data_index = mProcessingIndex;

        for (int j = 0; j < mFilterSize; j++)
        {
            fval += mProcessingBuffer[current_data_index] * mDaCoeffs[j];
            current_data_index++;

            if (current_data_index > DA_PROCESSING_BUFFER_SIZE - 1)
            {
                current_data_index -= DA_PROCESSING_BUFFER_SIZE;
            }
        }

        *(outputFloatPtr + i) = fval;

        mProcessingIndex++;

        if (mProcessingIndex > DA_PROCESSING_BUFFER_SIZE - 1)
        {
            mProcessingIndex -= DA_PROCESSING_BUFFER_SIZE;
        }
    }

    return( THOR_OK );
}


