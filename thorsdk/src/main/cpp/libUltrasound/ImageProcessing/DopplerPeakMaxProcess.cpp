//
// Copyright 2020 EchoNous Inc.
//
//
#define LOG_TAG "DopplerPeakMaxProcess"

#include <DopplerPeakMaxProcess.h>
#include <ThorUtils.h>

#define  MAX_THRESHOLD 255
#define  EXT_SEARCH_THRESHOLD 0.90
#define  MAX_SEARCH_RANGE 225       // should be <= 256
#define  CONSECUTIVE_NT_THRESHOLD 10
#define  MIN_CONSECUTIVE_THRESHOLD 5

struct {
    bool operator()(std::pair<int, int> a, std::pair<int, int> b) const { return a.first > b.first; }
} customGreater;


DopplerPeakMaxProcess::DopplerPeakMaxProcess(const std::shared_ptr<UpsReader> &upsReader) :
        ImageProcess(upsReader)
{
    mNumFFT = 256;
    mWFGain = 5.62f;
    mWFGainRef = 5.62f;
    mThreshold = 255;
    mThresholdFloat = 0.2f;
}

ThorStatus DopplerPeakMaxProcess::init()
{
    return THOR_OK;
}

ThorStatus DopplerPeakMaxProcess::setProcessParams(ProcessParams &params)
{
    mParams = params;
    LOGD("DopplerPeakMaxProcess::setProcessParams(): numRays = %d", mParams.numRays);
    return THOR_OK;
}

void DopplerPeakMaxProcess::setProcessIndices(uint32_t numFFT, float refGaindB, float thresholdFloat)
{
    // Number of FFT:
    // Expected: PW: 256, CW: 512
    mNumFFT = numFFT;

    // Reference Gain
    mWFGainRef = pow(10.0f, (refGaindB) / 20.0f);

    // threshold 0 ~ 1 (default 0.2)
    mThresholdFloat = thresholdFloat;
    float gainAdj = (mWFGain / mWFGainRef);
    if (gainAdj > 1.0f)
        gainAdj = sqrt(gainAdj);

    mThreshold = (uint32_t) (255.0f * gainAdj * mThresholdFloat);

    mPrevDistIdx = 127;

    LOGD("DopplerPeakMaxProcess::setProcessIndices(): mWFGainRef: %f, numFFT: %d, threshold: %d",
         mWFGainRef, mNumFFT, mThreshold);
}


void DopplerPeakMaxProcess::setWFGain(float dBGain)
{
    // TODO: update WFGain for the normalization process
    mWFGain = pow(10.0f, (dBGain) / 20.0f);
    float gainAdj = (mWFGain / mWFGainRef);
    if (gainAdj > 1.0f)
        gainAdj = sqrt(gainAdj);

    mThreshold = (uint32_t) (255.0f * gainAdj * mThresholdFloat);
    LOGD("DopplerPeakMaxProcess::setWFGain(): mWFGain: %f, threshold: %d", mWFGain, mThreshold);
}


ThorStatus DopplerPeakMaxProcess::process(uint8_t* inputPtr, uint8_t* outputPtr)
{
    // input: 8-bit unsigned integer (FFT Smoothed Output)
    // output float between 0 ~ 1.
    // Peak and Mean - interleaved.

    // input header pointer
    CineBuffer::CineFrameHeader* inputHeaderPtr =
            reinterpret_cast<CineBuffer::CineFrameHeader*>(inputPtr - sizeof(CineBuffer::CineFrameHeader));

    // output header pointer
    CineBuffer::CineFrameHeader* outputHeaderPtr =
            reinterpret_cast<CineBuffer::CineFrameHeader*>(outputPtr - sizeof(CineBuffer::CineFrameHeader));

    // Number of Lines Per Frame from header
    uint32_t numLinesPerFrame = inputHeaderPtr->numSamples;
    // output header update
    outputHeaderPtr->numSamples = numLinesPerFrame;
    outputHeaderPtr->frameNum = inputHeaderPtr->frameNum;

    float*   outputFloatPtr = (float*) outputPtr;

    // TODO: may need to skip this
    if (mThreshold > MAX_THRESHOLD)
    {
        for (int i = 0; i < numLinesPerFrame; i++)
        {
            *outputFloatPtr++ = 0.0f;       // Peak Max
            *outputFloatPtr++ = 0.0f;       // Peak Max Negative
        }

        // skip to the frame
        return THOR_OK;
    }

    // TODO: add adaptive threshold

    int   halfDistance = (mNumFFT >> 1);    // distance to check the threshold
    int   lineIdx, distIdx, idxShift;
    float maxVal, maxNVal;      // max and max Negative Value
    bool  maxFound;
    float scaleAdj = 1.0f/((float) halfDistance);
    int   maxIdx = halfDistance - 1;
    int   NSearchRange;
    int   consecutiveNT;
    int   negDistIdx;
    std::vector<std::pair<int, int>> conseqNT;

    int   minNTVal;
    int   cur_dist;
    bool  rejectMaxVal;
    bool  NTCountNeeded;
    int   dist_band = (int) ((1.0f - EXT_SEARCH_THRESHOLD) * halfDistance + 0.99f);

    for (lineIdx = 0; lineIdx < numLinesPerFrame; lineIdx++)
    {
        idxShift = mNumFFT * lineIdx;
        maxVal = 0.0f;
        maxNVal = 0.0f;

        // search the data range from maxIdx to 0
        distIdx = maxIdx;
        negDistIdx = maxIdx + 1;
        maxFound = false;

        NTCountNeeded = false;

        // check whether NTCountNeeded (Non-Threshold)
        if (mNumFFT == 256) {
            for (int ix = dist_band - halfDistance; ix < dist_band + halfDistance; ix++)
            {
                if (inputPtr[idxShift + ix] > mThreshold)
                {
                    NTCountNeeded = true;
                    break;
                }
            }
        }

        // mNumFFT == 512 or NT not needed.
        if (!NTCountNeeded)
        {
            while (distIdx >= 0 && !maxFound) {
                if (inputPtr[idxShift + distIdx] > mThreshold) {
                    rejectMaxVal = false;

                    if (distIdx > MIN_CONSECUTIVE_THRESHOLD) {
                        for (int ix = distIdx; ix > distIdx - MIN_CONSECUTIVE_THRESHOLD; ix--) {
                            if (inputPtr[idxShift + ix] <= mThreshold) {
                                rejectMaxVal = true;
                                break;
                            }
                        }
                    }

                    if (!rejectMaxVal) {
                        maxVal = ((float) distIdx) * scaleAdj;
                        maxFound = true;
                    }
                }

                distIdx--;
            }

            if (!maxFound)
                maxVal = 0.0f;
        }

        // when the detected envelop is near the border line
        if ( ((maxVal > EXT_SEARCH_THRESHOLD) || NTCountNeeded) && (mNumFFT == 256) )
        {
            conseqNT.clear();
            consecutiveNT = 0;

            for (int i = 0; i < mNumFFT; i++)
            {
                // reduce threshold value for Non threshold process
                if (inputPtr[idxShift + i] <= mThreshold/2)
                {
                    consecutiveNT++;
                }
                else
                {
                    if (consecutiveNT > CONSECUTIVE_NT_THRESHOLD)
                        conseqNT.emplace_back(consecutiveNT, i);

                    consecutiveNT = 0;
                }
            }

            if (consecutiveNT > CONSECUTIVE_NT_THRESHOLD)
                conseqNT.emplace_back(consecutiveNT, mNumFFT);

            if (conseqNT.size() < 1) {
                conseqNT.emplace_back(1, halfDistance);
            }

            // sorting descending order
            std::sort(conseqNT.begin(), conseqNT.end(), customGreater);
            auto max_ele = conseqNT.begin();
            auto tmp_ele = conseqNT.begin();

            // minimum value
            cur_dist = abs( mPrevDistIdx - (max_ele->second - max_ele->first/2) );

            if (cur_dist > halfDistance/2)
            {
                minNTVal = cur_dist;

                if (conseqNT.size() > 1) {

                    for (std::vector<std::pair<int, int>>::iterator it = conseqNT.begin();
                         it != conseqNT.end(); ++it)
                    {
                        cur_dist = abs( mPrevDistIdx - (it->second - it->first/2) );

                        if (cur_dist < minNTVal) {
                            minNTVal = cur_dist;
                            tmp_ele = it;
                        }
                    }

                    // replace only when the next element has significant NT values
                    if (max_ele->first * 0.55f < tmp_ele->first * 1.0f)
                    {
                        max_ele = tmp_ele;
                    }
                }
            }

            distIdx = max_ele->second - max_ele->first + 3;
            negDistIdx = max_ele->second - 3;

            // check limits
            distIdx = MIN(MAX(distIdx, 0), mNumFFT - 1);
            negDistIdx = MIN(MAX(negDistIdx, 0), mNumFFT - 1);

            // update mPrevDistIdx
            mPrevDistIdx = max_ele->second - max_ele->first/2;

            // re-search for positive side.
            maxFound = false;
            while (distIdx >= 0 && !maxFound)
            {
                if (inputPtr[idxShift + distIdx] > mThreshold)
                {
                    rejectMaxVal = false;

                    if (distIdx > MIN_CONSECUTIVE_THRESHOLD)
                    {
                        for (int ix = distIdx; ix > distIdx - MIN_CONSECUTIVE_THRESHOLD; ix--)
                        {
                            if (inputPtr[idxShift + ix] <= mThreshold)
                            {
                                rejectMaxVal = true;
                                break;
                            }
                        }
                    }

                    if (!rejectMaxVal)
                    {
                        maxVal = ((float) distIdx) * scaleAdj;
                        maxFound = true;
                    }
                }

                distIdx--;
            }

            // when max was not found - assign search range
            if (!maxFound)
            {
                maxVal = 0.0f;
            }
        }  // end of NFFT = 255

        // search the data in the negative side
        maxFound = false;

        // set distIdx for negative direction search
        distIdx = negDistIdx;

        while (distIdx < mNumFFT && !maxFound)
        {
            if (inputPtr[idxShift + distIdx] > mThreshold)
            {
                rejectMaxVal = false;

                if (distIdx + MIN_CONSECUTIVE_THRESHOLD < mNumFFT)
                {
                    for (int ix = distIdx; ix < distIdx + MIN_CONSECUTIVE_THRESHOLD; ix++)
                    {
                        if (inputPtr[idxShift + ix] <= mThreshold)
                        {
                            rejectMaxVal = true;
                            break;
                        }
                    }
                }

                if (!rejectMaxVal)
                {
                    // keep it negative
                    maxNVal = - ((float) (mNumFFT - distIdx)) * scaleAdj;
                    maxFound = true;
                }
            }

            distIdx++;
        }

        if (!maxFound)
        {
            if (maxVal > 1.0f) {
                NSearchRange = (int) round(maxVal / scaleAdj);
                maxNVal = ((float) (mNumFFT - NSearchRange)) * scaleAdj;
            } else
                maxNVal = 0.0f;
        }

        *outputFloatPtr++ = maxVal;      // Peak Max
        *outputFloatPtr++ = maxNVal;     // Peak N Max
    }

    return THOR_OK;
}

void DopplerPeakMaxProcess::resetCounter()
{
    mPrevDistIdx = 127;
}

void DopplerPeakMaxProcess::terminate() {}
