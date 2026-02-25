//
// Copyright 2020 EchoNous, Inc.
//
#include <stdio.h>
#include <cstdint>
#include <algorithm>
#include <memory>

#include "cnpy.h"

#include <UpsReader.h>
#include <CineBuffer.h>
#include <EcgProcess.h>
#include <SineGenerator.h>

#include <PwLowPassDecimationFilterProcess.h>
#include <DopplerWallFilterProcess.h>
#include <DopplerSpectralProcess.h>
#include <DopplerHilbertProcess.h>
#include <DopplerAudioUpsampleProcess.h>

#define TEST_INPUT_MEM_SIZE 819200 * 2   //byte
#define TEST_NUM_FRAMES 1000

//#define FLOAT_TEST

//#define PRINT_ERRORS

const char*                gUpsFileName;
std::shared_ptr<UpsReader> gUpsReaderSPtr;

CineBuffer                 gCineBuffer;

int setupUpsReader()
{
    // gUpsFileName
    gUpsFileName = "../../../../../../app/src/main/assets/thor.db";

    // Ups Reader
    gUpsReaderSPtr = std::make_shared<UpsReader>();

    if (!gUpsReaderSPtr->open(gUpsFileName))
    {
        printf("ERROR: couldn't open %s\n", gUpsFileName);
        return (-1);
    }

    gUpsReaderSPtr->loadGlobals();
    gUpsReaderSPtr->loadTransducerInfo();

    return 0;
}


void setupCineBufferWithDummyParams()
{
    // init cinebuffer - with dummy params
    CineBuffer::CineParams  cineParams;
    cineParams.imagingCaseId = 1000;
    uint32_t dtype = getDauDataTypes(IMAGING_MODE_B , false, true, true);
    cineParams.dauDataTypes = dtype;

    // imaging mode to PW
    cineParams.imagingMode = IMAGING_MODE_PW;
    // set params
    gCineBuffer.setParams(cineParams);
}

// read Input pw IQ data from npy in 64-bit complex format
void readInputNpyData(std::string inFile, uint32_t* inputPtr, float scale, uint32_t readSize, uint32_t numFrames)
{
    cnpy::NpyArray pwInArr = cnpy::npy_load(inFile);
    float* pwIn_arr = pwInArr.data<float>();

    printf("pwIn data - NumVal: %d\n", pwInArr.num_vals);
    printf("pwIn data - word size: %d\n", pwInArr.word_size);
    printf("readSize: %d, numFrames: %d\n", readSize, numFrames);

    for (int j = 0; j < numFrames; j++)
    {
        // skip past header (header =  4 words)
        inputPtr += 4;

        // packing the data into 32-bit uint32_t
        // perFrame then Gate Samples order
        for (int i = 0; i < readSize; i++) {
            float cur_I, cur_Q;
            int ival_I, ival_Q;
            uint32_t uval_I, uval_Q;
            uint32_t uval;

            cur_I = pwIn_arr[j * 2 * readSize + i * 2];
            cur_Q = pwIn_arr[j * 2 * readSize + i * 2 + 1];

            ival_I = round(cur_I * scale);
            ival_Q = round(cur_Q * scale);

            if (ival_I < 0)
                uval_I = (uint32_t)(65536 + ival_I);
            else
                uval_I = (uint32_t) ival_I;

            if (ival_Q < 0)
                uval_Q = (uint32_t)(65536 + ival_Q);
            else
                uval_Q = (uint32_t) ival_Q;

            uval = (uval_I & 0x0000ffff) | ((uval_Q & 0x0000ffff) << 16);

            *inputPtr = uval;

            // jump 4 words (quad).
            inputPtr += 4;
        }
    }

    printf("Reading Input File %s has been completed.\n", inFile.c_str());
}


template <typename T>
void fftBaselineShift(T* output, T* input, uint32_t shiftAmount, uint32_t arraySize)
{
    uint32_t cur_index = shiftAmount;

    for (uint32_t i = 0; i < arraySize; i++)
    {
        output[cur_index] = input[i];
        cur_index++;

        if (cur_index >= arraySize)
        {
            cur_index -= arraySize;
        }
    }
}

template <typename T>
void getOutputFromCine(T* output, uint32_t dataCntPerFrame, uint32_t numFrames, uint32_t dataType = DAU_DATA_TYPE_PW)
{
    T* curFrameData;

    for (int fi = 0; fi < numFrames; fi++)
    {
        curFrameData = (T*) gCineBuffer.getFrame(fi, dataType, CineBuffer::FrameContents::DataOnly);

        for (int dc = 0; dc < dataCntPerFrame; dc++)
        {
            *output++ = curFrameData[dc];
        }
    }
}

template <typename T>
void getFftOutputFromCine(T* output, uint32_t fftSize, uint32_t numFrames)
{
    T* curFrameData;
    uint8_t* curHeaderPtr;
    CineBuffer::CineFrameHeader* cineHeaderPtr;

    for (int fi = 0; fi < numFrames; fi++)
    {
        curFrameData = (T*) gCineBuffer.getFrame(fi, DAU_DATA_TYPE_PW, CineBuffer::FrameContents::DataOnly);
        curHeaderPtr = gCineBuffer.getFrame(fi, DAU_DATA_TYPE_PW, CineBuffer::FrameContents::IncludeHeader);

        // header Ptr;
        cineHeaderPtr = reinterpret_cast<CineBuffer::CineFrameHeader*>(curHeaderPtr);

        for (int dc = 0; dc < cineHeaderPtr->numSamples * fftSize; dc++)
        {
            *output++ = curFrameData[dc];
        }
    }
}



void printUsage()
{
    printf("Placeholder for Usage: \n");
}

int main(int argc, char * argv[])
{
    /* ---------------------------------------------------------------
     * Automated test for Pw Process
     *
     * This function follows steps taken in PW Test Python script, testPW.py by JJ.
     * Each step of Python PW Process generates npy file.
     *
     * So, this testing function reads npy files for comparing outputs of C modules.
     *
     *
     */

    // setup Ups Reader
    if (setupUpsReader() < 0)
    {
        printf("There is an error in setting up UpsReader.\n");
        return (-1);
    }

    // test case input
    // TODO: update with args
    uint32_t    prfIndex = 14;
    uint32_t    gateIndex = 5;
    uint32_t    wallFilterIndex = 1;

    uint32_t    numPwSamplesPerFrame;
    uint32_t    numGateSamples;
    uint32_t    numWallFilterTap;
    uint32_t    numFrames;

    float* dummyPtr = (float*) malloc(256*sizeof(float));

    numPwSamplesPerFrame = gUpsReaderSPtr->getNumPwSamplesPerFrame(prfIndex);
    numGateSamples = gUpsReaderSPtr->getGateSizeSamples(gateIndex);
    numWallFilterTap = gUpsReaderSPtr->getPwWallFilterCoefficients(wallFilterIndex, dummyPtr);

    printf("\nNumPwSamplesPerFrame: %d, numGateSamples: %d\n", numPwSamplesPerFrame, numGateSamples);

    numFrames = 10;

    // setup CineBuffer
    setupCineBufferWithDummyParams();

    ////////////////////////////////////////////////////////////////////
    // STEP 1: read input file
    ////////////////////////////////////////////////////////////////////
    // input file -> samples row then gate range.
    // g(0) [s(0), s(1), s(2), .... s(numSamples-1)]
    // g(1) ....
    // g(R-1)
    // then to the next frame

    // input pointer (emulates FIFO)
    uint32_t* inputPtr = (uint32_t*) malloc (TEST_INPUT_MEM_SIZE);

    std::string inputNpyFile = "pwIn.npy";
    float scale = 1024.0f;

    printf("\nReading input npy data\n");
    readInputNpyData(inputNpyFile, inputPtr, scale, numPwSamplesPerFrame*numGateSamples, numFrames);

    ////////////////////////////////////////////////////////////////////
    // STEP 2: PwLowPassDecimFilter
    ////////////////////////////////////////////////////////////////////
    // input: 32-bit packed IQ with 4 words stride + 4 words header
    // output: float I & Q (64-bit per a sample)
    // output pointer
    uint32_t* pwSumOutPtr = (uint32_t*) malloc (numPwSamplesPerFrame * sizeof(float) * 2 * numFrames);

    PwLowPassDecimationFilterProcess pwLPDecim = PwLowPassDecimationFilterProcess(gUpsReaderSPtr);
    pwLPDecim.init();
    pwLPDecim.setProcessIndices(gateIndex, prfIndex);

    // multi-frame processing (store to Cine)
    for (int i = 0; i < numFrames; i++)
    {
        uint32_t* curFramePtr = inputPtr + (4 + 4 * numPwSamplesPerFrame*numGateSamples) * i;

        pwLPDecim.process(((uint8_t*) curFramePtr),
                gCineBuffer.getFrame(i, DAU_DATA_TYPE_PW, CineBuffer::FrameContents::DataOnly));
    }

    // output float pointer
    float* pwSumOutPtrFloat = (float*) pwSumOutPtr;
    getOutputFromCine<float>(pwSumOutPtrFloat, numPwSamplesPerFrame*2, numFrames);  // output: IQ data

    cnpy::NpyArray postSumIQArr = cnpy::npy_load("pwSumSampleSpectralOut.npy");
    float* postSumIQ_arr = postSumIQArr.data<float>();

    float err_sum = 0.0f;
    float I_err, Q_err;
    float adj_scale = 16.64f;
    float rms_err;

    for (int i = 0; i < numPwSamplesPerFrame * numFrames; i++)
    {
        I_err = pwSumOutPtrFloat[2*i]*adj_scale - postSumIQ_arr[2*i];
        Q_err = pwSumOutPtrFloat[2*i+1]*adj_scale - postSumIQ_arr[2*i+1];

#ifdef PRINT_ERRORS
        if (abs(I_err) > 1.0f)
            printf("PwSumSampleOut - I error: %d, err: %f, %f, ref: %f\n", i, I_err,
                   pwSumOutPtrFloat[2*i]*adj_scale, postSumIQ_arr[2*i]);

        if (abs(Q_err) > 1.0f)
            printf("PwSumSampleOut - Q error: %d, err: %f, %f, ref: %f\n", i, Q_err,
                   pwSumOutPtrFloat[2*i+1]*adj_scale, postSumIQ_arr[2*i+1]);
#endif

        err_sum += I_err * I_err;
        err_sum += Q_err * Q_err;
    }

    rms_err = sqrt(err_sum/(2*numPwSamplesPerFrame*numFrames));

    printf("\n\nPwSumSampleout - Rms Error: %f\n\n", rms_err);



    ////////////////////////////////////////////////////////////////////
    // STEP 3: PwWallFilter
    ////////////////////////////////////////////////////////////////////
    //
    //
    //  input: float I & Q (64-bit per a sample)
    //  output: float I & Q
    //
    uint8_t* pwWFOutPtr = (uint8_t*) malloc(numPwSamplesPerFrame * sizeof(float) * 2 * numFrames);
    DopplerWallFilterProcess pwWFProcess = DopplerWallFilterProcess(gUpsReaderSPtr);
    pwWFProcess.init();
    pwWFProcess.setProcessIndices(wallFilterIndex, prfIndex);
    //pwWFProcess.process((uint8_t*) postSumIQ_arr, pwWFOutPtr);

    // multi-frame processing (store to Cine)
    for (int i = 0; i < numFrames; i++)
    {
        // input I&Q
        float* curFramePtr = postSumIQ_arr + 2 * numPwSamplesPerFrame * i;

        pwWFProcess.process(((uint8_t*) curFramePtr),
                          gCineBuffer.getFrame(i, DAU_DATA_TYPE_PW, CineBuffer::FrameContents::DataOnly));
    }

    float* pwWFOutPtrFloat = (float*) pwWFOutPtr;
    getOutputFromCine<float>(pwWFOutPtrFloat, numPwSamplesPerFrame*2, numFrames);  // output: IQ data

    cnpy::NpyArray pwWFOutArr = cnpy::npy_load("pwwallFilterSpectralOut.npy");
    float* pwWFOut_arr = pwWFOutArr.data<float>();

    printf("\npwWFOutArr data - NumVal: %d\n", pwWFOutArr.num_vals);

    err_sum = 0.0f;
    int WFOutShiftAdj = 0;    //numWallFilterTap;
    int adj_index;

    for (int i = 0; i < pwWFOutArr.num_vals; i++)
    {
        adj_index = WFOutShiftAdj + i;

        I_err = pwWFOutPtrFloat[2*adj_index] - pwWFOut_arr[2*i];
        Q_err = pwWFOutPtrFloat[2*adj_index+1] - pwWFOut_arr[2*i+1];

#ifdef PRINT_ERRORS
        if (abs(I_err) > 1.0f)
            printf("PwWallFilter - I error: %d, err: %f, %f, ref: %f\n", i, I_err,
                   pwWFOutPtrFloat[2*adj_index], pwWFOut_arr[2*i]);

        if (abs(Q_err) > 1.0f)
            printf("PwWallFilter - Q error: %d, err: %f, %f, ref: %f\n", i, Q_err,
                   pwWFOutPtrFloat[2*adj_index+1], pwWFOut_arr[2*i+1]);
#endif

        err_sum += I_err * I_err;
        err_sum += Q_err * Q_err;
    }

    rms_err = sqrt(err_sum/(2*pwWFOutArr.num_vals));

    printf("\nPwWallFilter - Rms Error: %f\n", rms_err);


    ////////////////////////////////////////////////////////////////////
    // STEP 4: FFTOut
    ////////////////////////////////////////////////////////////////////
    //
    //
    uint8_t* pwFftOutPtr = (uint8_t*) malloc (TEST_INPUT_MEM_SIZE);
    CineBuffer::CineFrameHeader* cineHeaderPtr;
    uint32_t pwSpectralCompressionCurveIndex = 34;

    DopplerSpectralProcess pwSpecProc = DopplerSpectralProcess(gUpsReaderSPtr);
    pwSpecProc.init();
    pwSpecProc.setProcessIndices(prfIndex, 1, pwSpectralCompressionCurveIndex);

    // multi-frame processing
    for (int i = 0; i < numFrames; i++)
    {
        float* curFramePtr = pwWFOutPtrFloat + 2 * numPwSamplesPerFrame * i;

        pwSpecProc.process((uint8_t*) curFramePtr,
                gCineBuffer.getFrame(i, DAU_DATA_TYPE_PW, CineBuffer::FrameContents::DataOnly));
    }

    int samplesCnt[10];
    int samplesCntSum = 0;
    for (int i = 0; i < numFrames; i++)
    {
        uint8_t* tmpPtr = gCineBuffer.getFrame(i, DAU_DATA_TYPE_PW, CineBuffer::FrameContents::IncludeHeader);
        cineHeaderPtr = reinterpret_cast<CineBuffer::CineFrameHeader*>(tmpPtr);
        samplesCnt[i] = cineHeaderPtr->numSamples;
        samplesCntSum += samplesCnt[i];

        //printf("Samples Counter (header) - frameNo: %d, samples: %d\n", i, samplesCnt[i]);
    }

    printf("\nSample Counter Sum for %d frames: %d\n", numFrames, samplesCntSum);

    // FFT outline x multi-line x multi-frame
    cnpy::NpyArray pwFftOut = cnpy::npy_load("pwFftOut.npy");
    float* pwFftOut_arr = pwFftOut.data<float>();

    int fftSize = 256;
    float cur_err = 0.0f;
    int line_shift = 0;
    err_sum = 0.0f;

    printf("\npwFftOutArr data - NumVal: %d, SamplesCount: %d\n", pwFftOut.num_vals, pwFftOut.num_vals/fftSize);

#ifndef FLOAT_TEST
    // get FFT data from Cine
    getFftOutputFromCine<uint8_t>(pwFftOutPtr, fftSize, numFrames);  // output: IQ data

    uint8_t* shiftedOut = new uint8_t[fftSize];

    for (int fn = 0; fn < numFrames; fn++)
    {
        for (int j = 0; j < samplesCnt[fn]; j++) {
            //line_shift = cur_shift + j * fftSize;
            line_shift += fftSize;

            // FFT Baseline shift
            fftBaselineShift<uint8_t>(shiftedOut, &pwFftOutPtr[line_shift], 128,
                                      (uint32_t) fftSize);

            for (int i = 0; i < fftSize; i++) {
                cur_err = abs(pwFftOut_arr[line_shift + i] - ((float) shiftedOut[i]) / 2.0f);

#ifdef PRINT_ERRORS
                if (cur_err > 1.0f)
                    printf("loc: %d, FFTOut: %f, Ref: %f\n", i, shiftedOut[i],
                           pwFftOut_arr[line_shift + i]);
#endif

                err_sum += cur_err;
            }
        }
    }

    rms_err = sqrt(err_sum/(fftSize * cineHeaderPtr->numSamples * numFrames));
    printf("\nFFTSpectralOut - Rms Error: %f\n", rms_err);

#else
    // Test for float out pf Spectral Processing
    float* pwFftOutFloatPtr = (float*) pwFftOutPtr;
    float* shiftedOut = new float[fftSize];

    // get FFT data from Cine
    getFftOutputFromCine<float>(pwFftOutFloatPtr, fftSize, numFrames);  // output: IQ data

    for (int fn = 0; fn < numFrames; fn++)
    {

        for (int j = 0; j < samplesCnt[fn]; j++) {
            line_shift += fftSize;
            // FFT Baseline shift
            fftBaselineShift<float>(shiftedOut, &pwFftOutFloatPtr[line_shift], 128,
                                    (uint32_t) fftSize);

            for (int i = 0; i < fftSize; i++) {
                cur_err = abs(pwFftOut_arr[line_shift + i] - shiftedOut[i]);

                if (cur_err > 1.0f)
                    printf("loc - j: %d, i: %d, FFTOut: %f, Ref: %f\n", j, i, shiftedOut[i],
                           pwFftOut_arr[line_shift + i]);

                err_sum += cur_err;
            }
        }
    }

    rms_err = sqrt(err_sum/(fftSize * cineHeaderPtr->numSamples * numFrames));
    printf("\nFFTSpectralOut - Rms Error: %f\n", rms_err);

#endif


    ////////////////////////////////////////////////////////////////////
    // PW Audio Process - Stereo Separation Process
    ////////////////////////////////////////////////////////////////////
    //
    uint32_t hilbertFilterIndex = 0;

    ////////////////////////////////////////////////////////////////////
    // STEP 5: PwAudio - PwHilbertFilter
    ////////////////////////////////////////////////////////////////////
    //
    //  using values from the npy file from the PwWallFilter (STEP 3)
    //
    //  input: float I & Q (64-bit per a sample)
    //  output: float HilbertFwd & HilbertRev (pre Upsample and LPF)
    //

    uint8_t* pwHilbertOutPtr = (uint8_t*) malloc (TEST_INPUT_MEM_SIZE);
    float* pwHilbertOutFloat = (float*) pwHilbertOutPtr;

    DopplerHilbertProcess pwHBProcess = DopplerHilbertProcess(gUpsReaderSPtr);
    pwHBProcess.init();
    pwHBProcess.setProcessIndices(prfIndex, hilbertFilterIndex);

    // multi-frame processing (store to Cine - PW AUDIO)
    for (int i = 0; i < numFrames; i++)
    {
        // input I&Q - post Wall Filter
        float* curFramePtr = pwWFOut_arr + 2 * numPwSamplesPerFrame * i;

        pwHBProcess.process(((uint8_t*) curFramePtr),
                            gCineBuffer.getFrame(i, DAU_DATA_TYPE_PW_ADO, CineBuffer::FrameContents::DataOnly));
    }

    getOutputFromCine<float>(pwHilbertOutFloat, numPwSamplesPerFrame*2, numFrames, DAU_DATA_TYPE_PW_ADO);  // output: Fwd/Rev data

    // reference output
    cnpy::NpyArray pwHilbertFwd = cnpy::npy_load("pwHilbertFwd.npy");
    float* pwHilbertFwd_arr = pwHilbertFwd.data<float>();

    cnpy::NpyArray pwHilbertRev = cnpy::npy_load("pwHilbertRev.npy");
    float* pwHilbertRev_arr = pwHilbertRev.data<float>();

    err_sum = 0.0f;
    float cur_err_fwd = 0.0f;
    float cur_err_rev = 0.0f;

    for (int i = 0; i < pwWFOutArr.num_vals; i++)
    {
        cur_err_fwd = abs(pwHilbertOutFloat[2*i] - pwHilbertFwd_arr[i]);
        cur_err_rev = abs(pwHilbertOutFloat[2*i+1] - pwHilbertRev_arr[i]);

#ifdef PRINT_ERRORS
        if ((cur_err_fwd > 0.1f) || (cur_err_rev > 0.1f))
            printf("Loc: %d, PwHilbert - Fwd: %f, Rev: %f -- Ref - Fwd: %f, Rev: %f\n",
                   pwHilbertOutFloat[2*i], pwHilbertOutFloat[2*i+1], pwHilbertFwd_arr[i], pwHilbertRev_arr[i]);
#endif

        err_sum += cur_err_fwd + cur_err_rev;
    }

    rms_err = sqrt(err_sum/(numPwSamplesPerFrame * numFrames * 2));     // Fwd and Rev
    printf("\nPwHilbertFilterOut - Rms Error: %f\n", rms_err);

    ////////////////////////////////////////////////////////////////////
    // STEP 6: PwAudio - PwUpsampleProcess
    ////////////////////////////////////////////////////////////////////
    //
    //  input: float Fwd & Rev audio Samples Interleaved (STEP 5 output)
    //  output: float Fwd & Rev (pre Upsample and LPF)
    //
    uint8_t* pwHilbertUpLprOutPtr = (uint8_t*) malloc (TEST_INPUT_MEM_SIZE);
    float* pwHilbertUpLpfOutFloat = (float*) pwHilbertUpLprOutPtr;

    DopplerAudioUpsampleProcess pwUpLpfProcess = DopplerAudioUpsampleProcess(gUpsReaderSPtr);
    pwUpLpfProcess.init();
    pwUpLpfProcess.setProcessingIndices(prfIndex, hilbertFilterIndex);
    uint32_t UpSampleRatio = 2;

    // multi-frame processing (store to Cine - PW AUDIO)
    for (int i = 0; i < numFrames; i++)
    {
        // input Fwd & Rev Interleaved
        float* curFramePtr = pwHilbertOutFloat + 2 * numPwSamplesPerFrame * i;

        pwUpLpfProcess.process(((uint8_t*) curFramePtr),
                            gCineBuffer.getFrame(i, DAU_DATA_TYPE_PW_ADO, CineBuffer::FrameContents::DataOnly));
    }

    // output: Fwd/Rev data up sampled and LPFiltered
    getOutputFromCine<float>(pwHilbertUpLpfOutFloat, numPwSamplesPerFrame*2*UpSampleRatio, numFrames, DAU_DATA_TYPE_PW_ADO);

    // reference output
    cnpy::NpyArray pwHilbertUpLpfFwd = cnpy::npy_load("pwHilbertUpLpfFwd.npy");
    float* pwHilbertUpLpfFwd_arr = pwHilbertUpLpfFwd.data<float>();

    cnpy::NpyArray pwHilbertUpLpfRev = cnpy::npy_load("pwHilbertUpLpfRev.npy");
    float* pwHilbertUpLpfRev_arr = pwHilbertUpLpfRev.data<float>();

    // err_sum = 0;
    err_sum = 0;

    for (int i = 0; i < numPwSamplesPerFrame * numFrames * UpSampleRatio; i++)
    {
        cur_err_fwd = abs(pwHilbertUpLpfOutFloat[2*i] - pwHilbertUpLpfFwd_arr[i]);
        cur_err_rev = abs(pwHilbertUpLpfOutFloat[2*i+1] - pwHilbertUpLpfRev_arr[i]);

#ifdef PRINT_ERRORS
        if ((cur_err_fwd > 0.1f) || (cur_err_rev > 0.1f))
            printf("Loc: %d, PwHilbertUpLpf - Fwd: %f, Rev: %f -- Ref - Fwd: %f, Rev: %f\n",
                   pwHilbertUpLpfOutFloat[2*i], pwHilbertUpLpfOutFloat[2*i+1],
                   pwHilbertUpLpfFwd_arr[i], pwHilbertUpLpfRev_arr[i]);
#endif

        err_sum += cur_err_fwd + cur_err_rev;
    }

    rms_err = sqrt(err_sum/(numPwSamplesPerFrame * numFrames * 2));     // Fwd and Rev
    printf("\nPwHilbertUpsampleLpfOut - Rms Error: %f\n", rms_err);

    delete[] shiftedOut;

    free(dummyPtr);
    free(inputPtr);
    free(pwSumOutPtr);
    free(pwWFOutPtr);
    free(pwFftOutPtr);
    free(pwHilbertOutPtr);
    free(pwHilbertUpLprOutPtr);

    return (0);
}
