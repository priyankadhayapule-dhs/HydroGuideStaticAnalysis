//
// Copyright 2018 EchoNous Inc.
//
//
#define LOG_TAG "DopplerSpectralProcess"

#include <DopplerSpectralProcess.h>
#include <ThorUtils.h>

//#define DEBUG_FFT_WINDOW_COEFFS
//#define DEBUG_COMP_COEFFS
//#define FFT_SPECTRAL_FLOAT_OUT
//#define FFT_UINT8_TEST_PATTERN
//#define FFT_UINT8_OUTPUT_DEBUG

#define MAX_BLANKFRAMES 512
#define PWBLANKFRAMES 128
#define CWBLANKFRAMES 512

DopplerSpectralProcess::DopplerSpectralProcess(const std::shared_ptr<UpsReader> &upsReader) :
    ImageProcess(upsReader)
{
    mNumSamplesFft = 10;
    mNumDopplerSamplesPerFrame = 48;
    mFFTSize = 256;
    mPFFFTPtr = nullptr;
    mNumCompCoeffs = 256;

    // initial transition params
    mBlankFrames     = 0;
    mBlankGainStart  = 0;       // First FFT frame to start the gain ramp
    mBlankGainInc    = 0.0;     // Gain increment per FFT frame
    mBlankGain       = 0.0;     // Current ramp gain
}

ThorStatus DopplerSpectralProcess::init()
{
    return THOR_OK;
}

ThorStatus DopplerSpectralProcess::setProcessParams(ProcessParams &params)
{
    mParams = params;
    LOGD("DopplerSpectralProcess::setProcessParams(): numRays = %d", mParams.numRays);
    return THOR_OK;
}

uint32_t DopplerSpectralProcess::getCompressionIndex(float xList[], uint32_t first, uint32_t last, float item)
{
    // based on binary search
    // assuming item is in float

    bool found = false;
    uint32_t midpoint = 0;

    if (item <= xList[first])
    {
        midpoint = first;
        found = true;
    }
    else if (item >= xList[last])
    {
        midpoint = last;
        found = true;
    }

    // cannot be the last one so subtract 1 here
    last -= 1;

    while (first <= last && !found)
    {
        midpoint = (first + last)/2;
        if ((xList[midpoint] <= item) && (item < xList[midpoint+1]))
            found = true;
        else if (item < xList[midpoint])
            last = midpoint - 1;
        else
            first = midpoint + 1;
    }

    return midpoint;
}

void DopplerSpectralProcess::setProcessIndices(uint32_t prfIndex, uint32_t fftAvgNum, uint32_t cmpCrvIdx, bool isTDI)
{
    // TODO: FFT related params; update with CW params
    uint32_t fftOutRateAfeClks;
    if (isTDI)
        cmpCrvIdx = 35;
    // Startup parameter for PW artifact
    mBlankFrames = (512-PWBLANKFRAMES);
    mBlankGain = 0.0;
    mBlankGainStart = 24;
    mBlankGainInc = (float)(1.0/(PWBLANKFRAMES-mBlankGainStart));
    mBlankGainStart += mBlankFrames;

    // initialize FFT window coeffs
    for (int i = 0; i < MAX_PW_FFT_WINDOW_SIZE; i++)
    {
        mFFTWindowCoeffs[i] = 0.0f;
    }

    mNumDopplerSamplesPerFrame = getUpsReader()->getNumPwSamplesPerFrame(prfIndex, isTDI);

    // FFT related params from UPS
    if (!getUpsReader()->getPwFftSizeRate(mFFTSize, fftOutRateAfeClks))
    {
        LOGE("setProcessIndices - error in getPwFftSizeRate");
        return;
    }

    if (!getUpsReader()->getFftWindow(prfIndex, mNumSamplesFft, mFFTWindowCoeffs, isTDI))
    {
        LOGE("setProcessIndices - error in getFftWindow");
        return;
    }

    LOGD("DopplerSpectralProcess - prfIndex: %d, numDopplerSamplesPerFrame: %d, fftSize: %d, mNumSamplesFft: %d",
            prfIndex, mNumDopplerSamplesPerFrame, mFFTSize, mNumSamplesFft);

    mNumCompCoeffs = getUpsReader()->getDopplerCompressionCoeff(cmpCrvIdx, mCompressionCoeffs);
    LOGD("DopplerSpectralProcess - mNumCompCoeffs: %u", mNumCompCoeffs);

#ifdef DEBUG_FFT_WINDOW_COEFFS
    for (int i = 0; i < mNumSamplesFft; i++)
        LOGD("DopplerSpectralProcess - FFT Window Coeffs: %d %.10e", i, mFFTWindowCoeffs[i]);
#endif

#ifdef DEBUG_COMP_COEFFS
    for (int i = 0; i < mNumCompCoeffs; i++)
        LOGD("DopplerSpectralProcess - compression Coefficients: %d %.10e", i, mCompressionCoeffs[i]);
#endif

    // initialize FFT in and out buffers.  Also, average buffer
    for (int i = 0; i < mFFTSize; i++)
    {
        mFFTInBuffer[i*2] = 0.0f;
        mFFTInBuffer[i*2+1] = 0.0f;
        mFFTOutBuffer[i*2] = 0.0f;
        mFFTOutBuffer[i*2+1] = 0.0f;

        mFFTAvgBuffer[i] = 0.0f;
    }

    // FFT averaging
    mNumLinesToAvg = fftAvgNum;
    mLinesAvgScale = 1.0f / ((float) mNumLinesToAvg);
    mLineCnt = 0;

    // initialize resampling buffer
    mDopplerResamplingBuffer.setBufferParams(mNumDopplerSamplesPerFrame, getUpsReader()->getAfeClksPerPwSample(prfIndex, isTDI),
                                        mNumSamplesFft, fftOutRateAfeClks);

    // initialize pffft setup
    mPFFFTPtr = pffft_new_setup(mFFTSize, PFFFT_COMPLEX);

    return;
}


void DopplerSpectralProcess::setCwProcessIndices(uint32_t prfIndex, uint32_t fftAvgNum, uint32_t cmpCrvIdx)
{
    // For CW, CWClks are used.
    uint32_t fftOutRateCwClks;

    // Startup parameters for artifact blanking
    mBlankFrames = (512-CWBLANKFRAMES);
    mBlankGain = 0.0;
    mBlankGainStart = 128;
    mBlankGainInc = (float)(1.0/(CWBLANKFRAMES-mBlankGainStart));
    mBlankGainStart += mBlankFrames;

    // initialize FFT window coeffs
    for (int i = 0; i < MAX_PW_FFT_WINDOW_SIZE; i++)
    {
        mFFTWindowCoeffs[i] = 0.0f;
    }

    // input -> decimatedSamples Per Frame
    uint32_t decimationFactor;
    getUpsReader()->getCwDecFactorSamples(prfIndex, decimationFactor, mNumDopplerSamplesPerFrame);

    // FFT related params from UPS
    if (!getUpsReader()->getCwFftSizeRate(mFFTSize, fftOutRateCwClks))
    {
        LOGE("setPrfIndex - error in getCwFftSizeRate");
        return;
    }

    if (!getUpsReader()->getCwFftWindow(prfIndex, mNumSamplesFft, mFFTWindowCoeffs))
    {
        LOGE("setCwProcessIndices - error in getCwFftWindow");
        return;
    }

    LOGD("DopplerSpectralProcess (CW) - prfIndex: %d, decimationFactor: %d, numDopplerSamplesPerFrame (decim): %d, "
         "fftSize: %d, mNumSamplesFft: %d, fftOutRateCwClks: %d",
         prfIndex, decimationFactor, mNumDopplerSamplesPerFrame, mFFTSize, mNumSamplesFft, fftOutRateCwClks);

    mNumCompCoeffs = getUpsReader()->getDopplerCompressionCoeff(cmpCrvIdx, mCompressionCoeffs);
    LOGD("DopplerSpectralProcess - mNumCompCoeffs: %u", mNumCompCoeffs);

#ifdef DEBUG_FFT_WINDOW_COEFFS
    for (int i = 0; i < mNumSamplesFft; i++)
        LOGD("DopplerSpectralProcess - FFT Window Coeffs: %d %.10e", i, mFFTWindowCoeffs[i]);
#endif

#ifdef DEBUG_COMP_COEFFS
    for (int i = 0; i < mNumCompCoeffs; i++)
        LOGD("DopplerSpectralProcess - compression Coefficients: %d %.10e", i, mCompressionCoeffs[i]);
#endif

    // initialize FFT in and out buffers.  Also, average buffer
    for (int i = 0; i < mFFTSize; i++)
    {
        mFFTInBuffer[i*2] = 0.0f;
        mFFTInBuffer[i*2+1] = 0.0f;
        mFFTOutBuffer[i*2] = 0.0f;
        mFFTOutBuffer[i*2+1] = 0.0f;
        mFFTTempBuffer[i*2] = 0.0f;
        mFFTTempBuffer[i*2+1] = 0.0f;

        mFFTAvgBuffer[i] = 0.0f;
    }

    // FFT averaging
    mNumLinesToAvg = fftAvgNum;
    mLinesAvgScale = 1.0f / ((float) mNumLinesToAvg);
    mLineCnt = 0;

    // initialize resampling buffer
    mDopplerResamplingBuffer.setBufferParams(mNumDopplerSamplesPerFrame,
            decimationFactor * getUpsReader()->getCwClksPerCwSample(prfIndex),
            mNumSamplesFft, fftOutRateCwClks);

    // initialize pffft setup
    mPFFFTPtr = pffft_new_setup(mFFTSize, PFFFT_COMPLEX);

    return;
}

ThorStatus DopplerSpectralProcess::process(uint8_t* inputPtr, uint8_t* outputPtr)
{
    // input pointer - does not include header
    // Assumption: input is in 32-bit float I and Q. Alternating IQ
    float* inputFloatPtr = (float*) inputPtr;
    CineBuffer::CineFrameHeader* cineHeaderPtr =
            reinterpret_cast<CineBuffer::CineFrameHeader*>(outputPtr - sizeof(CineBuffer::CineFrameHeader));

    // output counter
    uint32_t output_cnt = 0;
    float tmpOutput;

    // put data into Resampling buffer
    mDopplerResamplingBuffer.putData(inputFloatPtr);

#ifdef FFT_SPECTRAL_FLOAT_OUT
    float* outputFloatPtr = (float*) outputPtr;
#endif

#ifdef FFT_UINT8_OUTPUT_DEBUG
    float maxOutputVal = 0.0f;
    float minOutputVal = 100000.0f;
#endif

    while(mDopplerResamplingBuffer.getData(mFFTInBuffer))
    {
        // apply window
        for (int i = 0; i < mNumSamplesFft; i++)
        {
            mFFTInBuffer[2*i] *= mFFTWindowCoeffs[i];
            mFFTInBuffer[2*i+1] *= mFFTWindowCoeffs[i];
        }

        // perform FFT with pffft
        pffft_transform_ordered(mPFFFTPtr, mFFTInBuffer, mFFTOutBuffer, mFFTTempBuffer, PFFFT_FORWARD);

        if (mBlankFrames > MAX_BLANKFRAMES) {
            // convert
            for (int i = 0; i < mFFTSize; i++) {
                // store the output to average buffer
                mFFTAvgBuffer[i] += (mFFTOutBuffer[i * 2] * mFFTOutBuffer[i * 2]
                        + mFFTOutBuffer[i * 2 + 1] * mFFTOutBuffer[i * 2 + 1]);
            }
        }
        else
        {
            for (int i = 0; i < mFFTSize; i++) {
                // store output to buffer after applying gain
                mFFTAvgBuffer[i] += ((mBlankGain * mBlankGain * mFFTOutBuffer[i * 2] * mFFTOutBuffer[i * 2]
                                     + mBlankGain * mBlankGain * mFFTOutBuffer[i * 2 + 1] * mFFTOutBuffer[i * 2 + 1]));
            }
            // Start ramping up the gain after mBlankGainStart frames
            if (mBlankFrames > mBlankGainStart){
                mBlankGain += mBlankGainInc;
            }
            mBlankFrames++;
         }

        mLineCnt++;

        if (mLineCnt >= mNumLinesToAvg)
        {
            for (int i = 0; i < mFFTSize; i++)
            {
                // load from Average Buffer
                tmpOutput = mFFTAvgBuffer[i] * mLinesAvgScale;

#ifdef FFT_SPECTRAL_FLOAT_OUT
                *outputFloatPtr++ = tmpOutput;
#else
                // compression Lookup
                uint32_t foundIndex = getCompressionIndex(mCompressionCoeffs, 0, mNumCompCoeffs-1, tmpOutput);

#ifdef FFT_UINT8_OUTPUT_DEBUG
                tmpOutput = foundIndex;

                if (maxOutputVal < tmpOutput)
                maxOutputVal = tmpOutput;

                if (minOutputVal > tmpOutput)
                    minOutputVal = tmpOutput;
#endif

#ifdef FFT_UINT8_TEST_PATTERN
                foundIndex = i;
#endif
                *outputPtr++ = (uint8_t) foundIndex;
#endif
                // Clear FFT AvgBuffer
                mFFTAvgBuffer[i] = 0.0f;
            }

            output_cnt++;
            mLineCnt = 0;
        }   // end if
    }

#ifdef FFT_UINT8_OUTPUT_DEBUG
    LOGD("Output Value in a frame: max(%f), min(%f)", maxOutputVal, minOutputVal);
#endif

    // update output Header
    cineHeaderPtr->numSamples = output_cnt;
    
    return THOR_OK;
}

void DopplerSpectralProcess::terminate() {
    // destroy pffft setup
    if (mPFFFTPtr != nullptr)
        pffft_destroy_setup(mPFFFTPtr);
}
