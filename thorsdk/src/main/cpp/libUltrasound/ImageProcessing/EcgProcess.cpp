//
// Copyright 2019 EchoNous Inc.
//
//
#define LOG_TAG "EcgProcess"

#define FILTER_SIZE 36     // FIR FILTER SIZE 36 for 397.36 sampling rate

#define PEAK_THRESHOLD 0.80      // peak threshold wrt max value
#define STD_THRESHOLD 1.5        // std threshold value
#define ABS_THRESHOLD 0.07       // minimum peak qualifier - on pre-gain

#define MAX_HEARTRATE 300.0
#define MIN_HEARTRATE 30.0

#define HR_DIFFERENCE_REJECT 0.2

#include "EcgProcess.h"
#include <DauInputManager.h>
#include <ThorUtils.h>
#include <math.h>

// Typical cutoff for ECG - 0.5 Hz for High Pass
// QRS: 10 – 50 Hz so need to set FIR cutoff > 50

// Python Script
//from scipy import signal
//import matplotlib.pyplot as plt
//import numpy as np
//import math
//
//# ecgSampleRate = 3178.9184 older
//ecgSampleRate = 397.364807
//iirCutOff = 1.5
//firCutOff = 52.5
//
//ecgCoeffs = signal.firwin(numtaps = 36, cutoff = firCutOff, scale=True, nyq=ecgSampleRate/2.0, window='hamming')
//wa, ha = signal.freqz(ecgCoeffs)
//fig = plt.figure()
//plt.title('ECG FIR Filter Response')
//plt.plot(ecgSampleRate*wa/(2.0*math.pi), 20*np.log10(abs(ha)), 'b')
//plt.show()
//
//
//b, a = signal.cheby1(2, 1, iirCutOff*2/ecgSampleRate, 'highpass')
//w, h = signal.freqs(b, a)
//plt.semilogx(w, 20 * np.log10(abs(h)))
//plt.show()


/*
// cutoff 35.2 (recommended by HD Med)
static float ecgCoeffs[] = \
{
    -0.00045096,  0.00040205,  0.00160505,  0.00310742,  0.00429788,
    0.00401989,  0.00104421, -0.00512032, -0.01342961, -0.0209657 ,
   -0.02338304, -0.01616169,  0.00368171,  0.03604722,  0.0770284 ,
    0.11942944,  0.15449527,  0.17435279,  0.17435279,  0.15449527,
    0.11942944,  0.0770284 ,  0.03604722,  0.00368171, -0.01616169,
   -0.02338304, -0.0209657 , -0.01342961, -0.00512032,  0.00104421,
    0.00401989,  0.00429788,  0.00310742,  0.00160505,  0.00040205,
   -0.00045096
};*/

// cutoff 47.5 Hz
static float ecgCoeffs[] = \
{
        0.00079532, -0.00029158, -0.00179486, -0.00317191, -0.00299534,
        0.00023443,  0.00632289,  0.01206932,  0.01207507,  0.00211869,
        -0.01652144, -0.03483348, -0.0384186 , -0.01433883,  0.04075102,
        0.11598646,  0.18866706,  0.23334577,  0.23334577,  0.18866706,
        0.11598646,  0.04075102, -0.01433883, -0.0384186 , -0.03483348,
        -0.01652144,  0.00211869,  0.01207507,  0.01206932,  0.00632289,
        0.00023443, -0.00299534, -0.00317191, -0.00179486, -0.00029158,
        0.00079532
};

// bypass
/*static float ecgCoeffs[] = \
{
    0.,  0.,  0.,  0.,  0.,  0.,  0.,  0.,  0.,  0.,  0.,  0.,  0.,
    0.,  0.,  0.,  0.,  0.,  0.,  0.,  0.,  0.,  0.,  0.,  0.,  0.,
    0.,  0.,  0.,  0.,  0.,  0.,  0.,  0.,  0.,  1.
};*/

// cutoff 0.65 Hz
static float b[] = \
{
        0.88669268, -1.77338536,  0.88669268
};

static float a[] = \
{
        1.        , -1.98972343,  0.98981876
};

// no iir filter
/*static float b[] = \
{
        1.0f      ,        0.0f,  0.0f
};

static float a[] = \
{
        1.0f      ,        0.0f,  0.0f
};*/

//-----------------------------------------------------------------------------
EcgProcess::EcgProcess(const std::shared_ptr<UpsReader> &upsReader, bool isUsb) :
    mUpsReaderSPtr(upsReader),
    mADCMax(8388608.0f),
    mGain(1.0f),
    mSamplesPerSecond(300.0f),
    mMinSeparation(0),
    mPrevEstimatedHR(-1.0f),
    mHeartRateUpdateInterval(64),
    mIsUsbContext(isUsb)
{
    LOGD("EcgProcess - constructor");
}

//-----------------------------------------------------------------------------
EcgProcess::~EcgProcess()
{
    LOGD("EcgProcess - destructor");
}

//-----------------------------------------------------------------------------
ThorStatus EcgProcess::init()
{
    ThorStatus retVal = THOR_OK;

    return(retVal);
}

//-----------------------------------------------------------------------------
ThorStatus EcgProcess::setProcessParams(uint32_t samplesPerFrame, float samplesPerSecond)
{
    mSamplesPerFrame = samplesPerFrame;
    mSamplesPerSecond = samplesPerSecond;
    mNotReadyCount = 0;
    mLastValidSample = 0.0f;

    mMinSeparation = (uint32_t) (samplesPerSecond * 0.025f);
    mMinQRSSample = samplesPerSecond * 0.01635f;

    mFilterSize = FILTER_SIZE;

    // heart rate update interval
    mHeartRateUpdateInterval = (uint32_t) ((float) HEART_RATE_UPDATE_INTERVAL_SAMPLES)/((float) mSamplesPerFrame) - 1;
    if (mHeartRateUpdateInterval < 2)
        mHeartRateUpdateInterval = 2;

    float hrUpdateIntervalSec = ((float) (mHeartRateUpdateInterval + 1) * mSamplesPerFrame)/((float) mSamplesPerSecond);

    LOGD("EcgProcess: samples per frame %d, samples per second: %f", samplesPerFrame, samplesPerSecond);
    LOGD("EcgProcess: Heart Rate Update Interval: %d frames, %f sec", mHeartRateUpdateInterval, hrUpdateIntervalSec);

    return(THOR_OK);
}

//-----------------------------------------------------------------------------
ThorStatus EcgProcess::setADCMax(uint32_t adcMax)
{
    mADCMax = (float) adcMax;

    LOGD("EcgProcess: ADC_MAX: %f", mADCMax);

    return(THOR_OK);
}

//-----------------------------------------------------------------------------
ThorStatus EcgProcess::initializeProcessIndices()
{
    for (int i = 0; i < mFilterSize; i++)
    {
        mProcessingBuffer[i] = 0.0f;
    }

    mInputDataIndex = mFilterSize - 1;
    mProcessingIndex = 0;

    // init prev x, y values (for feedback)
#if IIR_FILTER_ORDER > 2
    for (int i = 0; i < IIR_FILTER_ORDER; i++)
    {
        mX[i] = 0.0f;
        mY[i] = 0.0f;
    }
#else
    mXn_1 = 0.0f;
    mXn_2 = 0.0f;
    mYn_1 = 0.0f;
    mYn_2 = 0.0f;
#endif

    mHeartRateUpdateIndex = 0;
    mHeartRate = -1.0f;
    mPrevPeakLoc = -1;          // -1: not valid
    mPrevPositive = true;

    return (THOR_OK);
}

//-----------------------------------------------------------------------------
ThorStatus EcgProcess::setCineBuffer(CineBuffer *cineBuffer)
{
    mCineBuffer = cineBuffer;
    return (THOR_OK);
}

//-----------------------------------------------------------------------------
ThorStatus EcgProcess::setGain(float dBGain)
{
    mGain = pow(10.0f, dBGain / 20.0f);
    return (THOR_OK);
}

//-----------------------------------------------------------------------------
ThorStatus EcgProcess::process(uint8_t* inputPtr, uint32_t frameNum)
{
    uint8_t *outputPtr = mCineBuffer->getFrame(frameNum, DAU_DATA_TYPE_ECG, CineBuffer::FrameContents::IncludeHeader);
    CineBuffer::CineFrameHeader* cineHeaderPtr = reinterpret_cast<CineBuffer::CineFrameHeader*>(outputPtr);
    cineHeaderPtr->frameNum = frameNum;
    cineHeaderPtr->timeStamp = reinterpret_cast<const DauInputManager::FrameHeader *>(inputPtr)->timeStamp;
    cineHeaderPtr->heartRate = mHeartRate;

    uint32_t *inputWordPtr = (uint32_t *)inputPtr;
    outputPtr =  mCineBuffer->getFrame(frameNum, DAU_DATA_TYPE_ECG, CineBuffer::FrameContents::DataOnly);
    float *outputFloatPtr = (float *)outputPtr;

    /*
     *  Byte    Meaning
     *  0       ECG data lower byte
     *  1       ECG data middle byte
     *  2       ECG data upper byte
     *  3       Status:
     *          Bit 0: 0
     *          Bit 1: ALARM
     *          Bit 2: PACE Channel 1 Ready
     *          Bit 3: PACE Channel 2 Ready
     *          Bit 4: PACE Channel 3 Ready
     *          Bit 5: ECG Channel 1 Ready
     *          Bit 6: ECG Channel 2 Ready
     *          Bit 7: ECG Channel 3 Ready
     */

    if(mIsUsbContext) // In USB ECG Signal offset is different as per PCIe.
        inputWordPtr += 5; // skip past header + uint32 for USB
    else
        inputWordPtr += 4; // skip past header


    for (int i = 0; i < mSamplesPerFrame; i++)
    {
        uint32_t uval = *inputWordPtr;
        float  fval;
        uint32_t st;
        float  iir_fval;

        st = (uval & 0xff000000) >> 24;

        if ( (st & 0x20) == 0 )
        {
            mNotReadyCount++;
            fval = mLastValidSample;
        }
        else
        {
            uval = uval & 0xffffff;
            fval = (float)uval/mADCMax;
            // TODO: constants used in the following expression need to
            // be parameterized once we finalize the settings of TI ADS 1293
            // 0.825 is the fudge factor used to get 0.4 aspect ratio at 25 mm/sec sweep speed
            fval = 750.0*(fval - 0.5) * (2.0 * 2.4/3.5) * 0.826;
            mLastValidSample = fval;
        }

#if IIR_FILTER_ORDER > 2
        iir_fval = b[0] * fval;

        for (int j = 0; j < IIR_FILTER_ORDER; j++)
        {
            iir_fval += b[j+1] * mX[j] - a[j+1] * mY[j];
        }

        for (int j = IIR_FILTER_ORDER - 1; j > 0; j--)
        {
            mX[j] = mX[j-1];
            mY[j] = mY[j-1];
        }

        mX[0] = fval;
        mY[0] = iir_fval;
#else
        // 2nd order case (HD medical example)
        iir_fval = b[0] * fval  + b[1] * mXn_1 + b[2] * mXn_2
                                - a[1] * mYn_1 - a[2] * mYn_2;

        mXn_2 = mXn_1;
        mXn_1 = fval;

        mYn_2 = mYn_1;
        mYn_1 = iir_fval;
#endif

        // put data into processing buffer
        mProcessingBuffer[mInputDataIndex] = iir_fval;
        mInputDataIndex++;

        if (mInputDataIndex > ECG_PROCESSING_BUFFER_SIZE - 1)
        {
            mInputDataIndex -= ECG_PROCESSING_BUFFER_SIZE;
        }

        inputWordPtr += 4; // 4 words per sample
    }

    // FIR filtering
    for (int i = 0; i < mSamplesPerFrame; i++)
    {
        float fval = 0.0f;
        int current_data_index = mProcessingIndex;

        for (int j = 0; j < mFilterSize; j++)
        {
            fval += mProcessingBuffer[current_data_index] * ecgCoeffs[j];
            current_data_index++;

            if (current_data_index > ECG_PROCESSING_BUFFER_SIZE - 1)
            {
                current_data_index -= ECG_PROCESSING_BUFFER_SIZE;
            }
        }

        *(outputFloatPtr + i) = fval * mGain;

        mProcessingIndex++;

        if (mProcessingIndex > ECG_PROCESSING_BUFFER_SIZE - 1)
        {
            mProcessingIndex -= ECG_PROCESSING_BUFFER_SIZE;
        }
    }

    mHeartRateUpdateIndex++;

    if (mHeartRateUpdateIndex > mHeartRateUpdateInterval)
    {
        // update HeartRate
        estimateHR(frameNum);
        mHeartRateUpdateIndex = 1;

#ifdef ECG_PERFORMANCE_TEST
        int32_t* testBufferPtr;
        testBufferPtr = (int32_t*) mDmaBufferSPtr->getBufferPtr(DmaBufferType::Diagnostics);

        *testBufferPtr = round(mHeartRate);
#endif
    }

    if ( (frameNum % 1800) == 0 )  // approximately once a minute
    {
        LOGD("ECG NOT READY COUNT = %d", mNotReadyCount);
    }

    return( THOR_OK );
}

//-----------------------------------------------------------------------------
void EcgProcess::estimateHR(uint32_t frameNum)
{
    int   minFrameNum;
    float sampleMin = 1000.0f;
    float sampleMax = -1000.0f;
    float sampleSum = 0.0f;

    uint8_t *samplePtr;
    float *sampleFloatPtr;
    float curSample;
    float sampleAvg;
    int   idxCnt = 0;
    float absTh;
    float distSum;
    int   distCnt;
    int   dist = 0;
    float peakInterval;
    float sqrSum = 0.0f;
    int   midThCnt = 0;
    float sign_adj = 1.0f;

    std::vector<int> topSpl;
    std::vector<int> peakSpl;
    std::vector<int> sepLoc;
    std::vector<float> topSplVal;

    minFrameNum = frameNum - mHeartRateUpdateInterval;

    if (minFrameNum < 0)
        minFrameNum = 0;

    for (int i = minFrameNum; i <= frameNum; i++)
    {
        samplePtr = mCineBuffer->getFrame(i, DAU_DATA_TYPE_ECG, CineBuffer::FrameContents::DataOnly);
        sampleFloatPtr = (float*) samplePtr;

        for (int j = 0; j < mSamplesPerFrame; j++)
        {
            curSample = *(sampleFloatPtr + j);

            if (curSample > sampleMax)
                sampleMax = curSample;

            if (curSample < sampleMin)
                sampleMin = curSample;

            sampleSum += curSample;
        }
    }

    sampleAvg = sampleSum / (mSamplesPerFrame * (mHeartRateUpdateInterval + 1));

    if (abs(sampleMax - sampleAvg) > abs(sampleMin - sampleAvg))
    {
        absTh = abs(sampleMax - sampleAvg) * PEAK_THRESHOLD;  // wrt sampleAvg
        if (mPrevPositive)
        {
            sign_adj = 1.0f;
        }
        else
        {
            mPrevPositive = true;
            mHeartRate = -1.0f;
            mPrevEstimatedHR = mHeartRate;
            return;
        }

    }
    else
    {
        absTh = abs(sampleMin - sampleAvg) * PEAK_THRESHOLD;
        if (!mPrevPositive)
        {
            sign_adj = -1.0f;
        }
        else
        {
            mPrevPositive = false;
            mHeartRate = -1.0f;
            mPrevEstimatedHR = mHeartRate;
            return;
        }
    }


    for (int i = minFrameNum; i <= frameNum; i++)
    {
        samplePtr = mCineBuffer->getFrame(i, DAU_DATA_TYPE_ECG, CineBuffer::FrameContents::DataOnly);
        sampleFloatPtr = (float*) samplePtr;

        for (int j = 0; j < mSamplesPerFrame; j++)
        {
            curSample = sign_adj * ( *(sampleFloatPtr + j) - sampleAvg );       // wrt sampleAvg

            if (curSample > absTh)
            {
                topSpl.push_back(idxCnt);
                topSplVal.push_back(curSample);
            }

            if (curSample > absTh * 0.5)
                midThCnt++;

            idxCnt++;
            sqrSum += curSample * curSample;
        }
    }

    float stdThr = STD_THRESHOLD * sqrt(sqrSum/idxCnt);

    if ( (absTh < stdThr) || (absTh/mGain < ABS_THRESHOLD) )
    {
        // signal too small or too noisy
        mHeartRate = -1.0f;
        mPrevPeakLoc = -1;
        mPrevEstimatedHR = mHeartRate;

        return;
    }

    // need to have at least two candidate peaks
    if (topSpl.size() > 1)
    {
        for (int i = 1; i < topSpl.size(); i++)
        {
            dist = topSpl.at(i) - topSpl.at(i-1);
            // need to have the minimum separation
            if (dist > mMinSeparation)
            {
                sepLoc.push_back(i);
            }
        }

        // check QRS width
        if (((float) midThCnt)/((float) (sepLoc.size() + 1)) < mMinQRSSample)
        {
            // QRS is too narrow
            mHeartRate = -1.0f;
            mPrevPeakLoc = -1;
            mPrevEstimatedHR = mHeartRate;

            return;
        }

        // find local maxima based on the separators
        // when there are at least two separators
        if (sepLoc.size() > 0)
        {
            float maxVal = topSplVal.at(0);
            int maxIdx = 0;
            int sepCnt = 0;

            for (int i = 1; i < topSpl.size(); i++)
            {
                if (i == sepLoc.at(sepCnt))
                {
                    if (maxIdx < topSpl.size())
                        peakSpl.push_back(topSpl.at(maxIdx));

                    // update counter and max values;
                    sepCnt++;
                    maxIdx = i;
                    maxVal = topSplVal.at(i);

                    if (sepCnt >= sepLoc.size())
                    {
                        sepCnt = sepLoc.size() - 1;
                    }
                }
                else
                {
                    if (topSplVal.at(i) > maxVal)
                    {
                        maxIdx = i;
                        maxVal = topSplVal.at(i);
                    }
                }
            }

            // last peak location
            if (maxIdx < topSpl.size())
                peakSpl.push_back(topSpl.at(maxIdx));

            // need at least two peaks to estimate heart rate
            if (peakSpl.size() > 1)
            {
                distSum = 0;
                distCnt = 0;

                for (int i = 1; i < peakSpl.size(); i++) {
                    dist = peakSpl.at(i) - peakSpl.at(i-1);
                    distSum += (float) dist;
                    distCnt++;
                }

                // peak Interval
                peakInterval = (distSum) / (distCnt);
                mHeartRate = round(mSamplesPerSecond * 60.0f / peakInterval);

                if (mHeartRate > MAX_HEARTRATE + 1)
                    mHeartRate = -1;

                if (mHeartRate < MIN_HEARTRATE - 1)
                    mHeartRate = -1;

                // last peak location
                mPrevPeakLoc = peakSpl.at(peakSpl.size() - 1);
            }
        }
        else if (sepLoc.size() == 0)
        {
            // When it only has one peak, but has enough QRS width
            // max location
            float maxVal = topSplVal.at(0);
            int maxIdx = 0;
            int peakLoc;

            for (int i = 1; i < topSpl.size(); i++)
            {
                if (topSplVal.at(i) > maxVal)
                {
                    maxIdx = i;
                    maxVal = topSplVal.at(i);
                }
            }

            peakLoc = topSpl.at(maxIdx);

            // When it has valid previous peak location
            if (mPrevPeakLoc >= 0)
            {
                distSum = peakLoc - mPrevPeakLoc + mSamplesPerFrame * (mHeartRateUpdateInterval + 1);
                mPrevPeakLoc = peakLoc;

                if (distSum > mMinSeparation)
                {
                    peakInterval = distSum;
                    mHeartRate = round(mSamplesPerSecond * 60.0f / peakInterval);

                    if (mHeartRate > MAX_HEARTRATE + 1)
                        mHeartRate = -1;

                    if (mHeartRate < MIN_HEARTRATE - 1)
                        mHeartRate = -1;
                }
                else
                {
                    // separation is too small (likely to be one peak got separated
                    mHeartRate = -1.0f;
                    mPrevPeakLoc = -1;
                }
            }
            else
            {
                // it has a peak but previous peak is not valid
                mHeartRate = -1.0f;
                mPrevPeakLoc = peakLoc;
            }
        }
    }
    else
    {
        // for the candidate peaks 1 or none
        mHeartRate = -1.0f;
        mPrevPeakLoc = -1;
    }

    float currentEstimatedHR = mHeartRate;

    if ((mPrevEstimatedHR - mHeartRate)/mPrevEstimatedHR > HR_DIFFERENCE_REJECT)
    {
        ALOGD("Rejest estimated HR - Current Estimated HR: %f, mPrevEstimateHR: %f",
              currentEstimatedHR, mPrevEstimatedHR);
        mHeartRate = -1.0f;
    }

    mPrevEstimatedHR = currentEstimatedHR;
}



//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
