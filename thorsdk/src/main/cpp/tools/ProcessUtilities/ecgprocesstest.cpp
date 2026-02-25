//
// Copyright 2018 EchoNous, Inc.
//
#include <stdio.h>
#include <cstdint>
#include <algorithm>
#include <memory>

#include <UpsReader.h>
#include <CineBuffer.h>
#include <EcgProcess.h>
#include <SineGenerator.h>


#define TEST_INPUT_MEM_SIZE 2048 //byte
#define TEST_NUM_FRAMES 1000


// Ecg Process
EcgProcess*                gEcgProcessPtr;
CineBuffer                 gCineBuffer;

const char*                gUpsFileName;
std::shared_ptr<UpsReader> gUpsReaderSPtr;

uint32_t                   gEcgSamplesPerFrame;
float                      gEcgSamplesPerSecond;
uint32_t                   gEcgADCMax;

SineGenerator              *gSineOsc;


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
    gCineBuffer.setParams(cineParams);
}

void initEcgProcess()
{
    // initialization
    if (nullptr != gEcgProcessPtr)
    {
        gEcgProcessPtr->initializeProcessIndices();
    }
}

void setupEcgProcess()
{
    // setting up necessary resources
    gEcgProcessPtr = new EcgProcess(gUpsReaderSPtr);
    gEcgProcessPtr->init();
    gEcgProcessPtr->setCineBuffer(&gCineBuffer);

    // set parameters
    uint32_t afeClksPerDaSample, afeClksPerEcgSample;
    const UpsReader::Globals* glbPtr = gUpsReaderSPtr->getGlobalsPtr();
    float samplingFrequency = glbPtr->samplingFrequency;
    // TODO: update with sequenceBuilder if necessary
    //gEcgSamplesPerFrame = mSequenceBuilderPtr->getNumEcgSamplesPerFrame();
    gEcgSamplesPerFrame = 14;

    gUpsReaderSPtr->getAfeClksPerDaEcgSample(0, afeClksPerDaSample, afeClksPerEcgSample);
    gUpsReaderSPtr->getEcgADCMax(0, gEcgADCMax);
    gEcgProcessPtr->setADCMax(gEcgADCMax);
    gEcgSamplesPerSecond = samplingFrequency*1e6/afeClksPerEcgSample;

    gEcgProcessPtr->setProcessParams(gEcgSamplesPerFrame, gEcgSamplesPerSecond);

    initEcgProcess();

    // set dB gain - default gain 1 dB
    gEcgProcessPtr->setGain(1.0);
}

void cleanUp()
{
    // clean up
    if (nullptr != gEcgProcessPtr)
    {
        delete gEcgProcessPtr;
        gEcgProcessPtr = nullptr;
    }

    // UPS Reader should be closed at the very end
    gUpsReaderSPtr->close();
}

// test case I
void prepareInputCaseI(uint32_t* inputPtr)
{
    // offset data test for the IIR filter

    // input word pointer
    uint32_t *inputWordPtr;

    // data
    float fval;
    float fval_int;
    uint32_t uval;
    int cnt = 0;

    float* inputFloat = new float[gEcgSamplesPerFrame];
    for (int i = 0; i < gEcgSamplesPerFrame; i++)
    {
        inputFloat[i] = 0.5f;
    }

    // test frames
    while (cnt < TEST_NUM_FRAMES)
    {
        // prepare inputPtr
        inputWordPtr = (uint32_t*)inputPtr;

        // skip the header (size 16)
        inputWordPtr += 4; // skip past header

        for (int i = 0; i < gEcgSamplesPerFrame; i++)
        {
            fval = inputFloat[i];

            fval_int = fval / 750.0 * 3.5 / 4.8 + 0.5;
            uval = (uint32_t) (fval_int * gEcgADCMax);

            // status code
            uval = (uval | 0x26000000);

            *inputWordPtr = uval;
            inputWordPtr += 4;
        }

        gEcgProcessPtr->process((uint8_t*) inputPtr, cnt);
        cnt++;
    }

    delete[] inputFloat;
}

void prepareInputCaseII(uint32_t* inputPtr)
{
    // input word pointer
    uint32_t *inputWordPtr;

    // data
    float fval;
    float fval_int;
    uint32_t uval;
    int cnt = 0;

    float* inputFloat = new float[gEcgSamplesPerFrame];
    for (int i = 0; i < gEcgSamplesPerFrame; i++)
    {
        inputFloat[i] = 0.0f;
    }

    // test frames
    while (cnt < TEST_NUM_FRAMES)
    {
        // prepare inputPtr
        inputWordPtr = (uint32_t*)inputPtr;

        // skip the header (size 16)
        inputWordPtr += 4; // skip past header

        for (int i = 0; i < gEcgSamplesPerFrame; i++)
        {
            //printf("sample number %d\n", i);

            fval = inputFloat[i];

            if ((cnt == 30) & (i == 0))
                fval = 0.5;

            fval_int = fval / 750.0 * 3.5 / 4.8 + 0.5;
            uval = (uint32_t) (fval_int * gEcgADCMax);

            // status code
            uval = (uval | 0x26000000);

            *inputWordPtr = uval;
            inputWordPtr += 4;
        }

        gEcgProcessPtr->process((uint8_t*) inputPtr, cnt);
        cnt++;
    }

    delete[] inputFloat;
}

// test case III
void prepareInputCaseIII(uint32_t* inputPtr)
{
    // Sine Wave test.  25 Hz
    gSineOsc = new SineGenerator();
    gSineOsc->setup(25.0, ceil(gEcgSamplesPerSecond), 0.25);

    // input word pointer
    uint32_t *inputWordPtr;

    // data
    float fval;
    float fval_int;
    uint32_t uval;
    int cnt = 0;

    float* inputFloat = new float[gEcgSamplesPerFrame];
    for (int i = 0; i < gEcgSamplesPerFrame; i++)
    {
        inputFloat[i] = 0.0f;
    }

    // test frames
    while (cnt < TEST_NUM_FRAMES)
    {
        // prepare inputPtr
        inputWordPtr = (uint32_t*)inputPtr;

        // skip the header (size 16)
        inputWordPtr += 4; // skip past header

        // update frame data
        gSineOsc->render(inputFloat, 1, gEcgSamplesPerFrame);

        for (int i = 0; i < gEcgSamplesPerFrame; i++)
        {
            fval = inputFloat[i];

            fval_int = fval / 750.0 * 3.5 / 4.8 + 0.5;
            uval = (uint32_t) (fval_int * gEcgADCMax);

            // status code
            uval = (uval | 0x26000000);

            *inputWordPtr = uval;
            inputWordPtr += 4;
        }

        gEcgProcessPtr->process((uint8_t*) inputPtr, cnt);
        cnt++;
    }

    delete[] inputFloat;
    delete gSineOsc;
}

// test case IV
void prepareInputCaseIV(uint32_t* inputPtr)
{
    // Sine Wave test.  75 Hz
    gSineOsc = new SineGenerator();
    gSineOsc->setup(75.0, ceil(gEcgSamplesPerSecond), 0.25);

    // input word pointer
    uint32_t *inputWordPtr;

    // data
    float fval;
    float fval_int;
    uint32_t uval;
    int cnt = 0;

    float* inputFloat = new float[gEcgSamplesPerFrame];
    for (int i = 0; i < gEcgSamplesPerFrame; i++)
    {
        inputFloat[i] = 0.0f;
    }

    // test frames
    while (cnt < TEST_NUM_FRAMES)
    {
        // prepare inputPtr
        inputWordPtr = (uint32_t*)inputPtr;

        // skip the header (size 16)
        inputWordPtr += 4; // skip past header

        // update frame data
        gSineOsc->render(inputFloat, 1, gEcgSamplesPerFrame);

        for (int i = 0; i < gEcgSamplesPerFrame; i++)
        {
            fval = inputFloat[i];

            fval_int = fval / 750.0 * 3.5 / 4.8 + 0.5;
            uval = (uint32_t) (fval_int * gEcgADCMax);

            // status code
            uval = (uval | 0x26000000);

            *inputWordPtr = uval;
            inputWordPtr += 4;
        }

        gEcgProcessPtr->process((uint8_t*) inputPtr, cnt);
        cnt++;
    }

    delete[] inputFloat;
    delete gSineOsc;
}

bool verifyTestCaseI(float* outputFloat)
{
    // return value: false -> fail
    //               true -> pass
    bool retVal = false;

    // expected. after about 3000 values should be close to zero (less than 0.00001)
    int overCnt = 0;
    float thVal = 0.00001f;

    for (int i = 3000; i < TEST_NUM_FRAMES * gEcgSamplesPerFrame; i++)
    {
        if (abs(outputFloat[i]) > thVal)
            overCnt++;
    }

    if (overCnt > 0)
    {
        printf("\nTest Verification Failed: Test Case I\n");
    }
    else
    {
        printf("\nTest Verification Success: Test Case I\n");
        retVal = true;
    }

    return retVal;
}

bool verifyTestCaseII(float* outputFloat)
{
    // return value: false -> fail
    //               true -> pass
    bool retVal = false;

    // expected. zeros until index 420.
    // max index -> 437, min index -> 443
    // recommend to plot the data to see the pattern
    float thVal = 0.00001f;
    int minIdx = 0;
    int maxIdx = 0;
    float minVal = 1000.0f;
    float maxVal = -1000.0f;
    int nonZeroCnt = 0;

    for (int i = 0; i < gEcgSamplesPerFrame * TEST_NUM_FRAMES; i++)
    {
        if ((outputFloat[i] > thVal) && (i < gEcgSamplesPerFrame * 30))
            nonZeroCnt++;

        if (outputFloat[i] > maxVal)
        {
            maxVal = outputFloat[i];
            maxIdx = i;
        }

        if (outputFloat[i] < minVal)
        {
            minVal = outputFloat[i];
            minIdx = i;
        }
    }

    if ((nonZeroCnt > 0) || (maxIdx != 437) || (minIdx != 443))
    {
        printf("\nTest Verification Failed: Test Case II\n");
    }
    else
    {
        printf("\nTest Verification Success: Test Case II\n");
        retVal = true;
    }

    return retVal;
}

bool verifyTestCaseIII(float* outputFloat)
{
    // return value: false -> fail
    //               true -> pass
    bool retVal = false;

    // peak to peak pixel count
    // pass: error within 1% of estimated frequency
    int closeZeroIdx = 0;
    int maxIdx1 = 0;
    int maxIdx2 = 0;
    float maxVal = -1000.0f;
    float minVal = 1000.0f;
    int startIdx = 1890;
    int spanSize = 17;
    float freq = 0.0f;

    // close zero
    for (int i = startIdx; i < startIdx + spanSize; i++)
    {
        if (abs(outputFloat[i]) < minVal)
        {
            minVal = abs(outputFloat[i]);
            closeZeroIdx = i;
        }
    }

    // max1
    for (int i = closeZeroIdx; i < closeZeroIdx + spanSize; i++)
    {
        if (outputFloat[i] > maxVal)
        {
            maxVal = outputFloat[i];
            maxIdx1 = i;
        }
    }

    // reset maxVal
    maxVal = -1000.0f;

    // max2
    for (int i = closeZeroIdx + spanSize; i < closeZeroIdx + 2 * spanSize; i++)
    {
        if (outputFloat[i] > maxVal)
        {
            maxVal = outputFloat[i];
            maxIdx2 = i;
        }
    }

    freq = 1.0f/((maxIdx2 - maxIdx1)/gEcgSamplesPerSecond);

    if (abs(25.0f - freq)/25.0f > 0.01f)
    {
        printf("\nTest Verification Failed: Test Case III\n");
    }
    else
    {
        printf("\nTest Verification Success: Test Case III\n");
        printf("Estimated frequency: %f Hz\n", freq);
        retVal = true;
    }

    return retVal;
}




bool verifyTestCaseIV(float* outputFloat)
{
    // return value: false -> fail
    //               true -> pass
    bool retVal = false;

    // expected. after about 100 values should be close to zero (less than 0.001)
    // as those need to be filtered out
    int overCnt = 0;
    float thVal = 0.001f;

    for (int i = 500; i < TEST_NUM_FRAMES * gEcgSamplesPerFrame; i++)
    {
        if (abs(outputFloat[i]) > thVal)
            overCnt++;
    }

    if (overCnt > 0)
    {
        printf("\nTest Verification Failed: Test Case IV\n");
    }
    else
    {
        printf("\nTest Verification Success: Test Case IV\n");
        retVal = true;
    }

    return retVal;
}

// output preparation
void prepareOutput(float* outputFloat)
{
    // output pointer
    float* outputFloatPtr;

    // sample count
    int smpCnt = 0;

    for (int i = 0; i < TEST_NUM_FRAMES; i++)
    {
        outputFloatPtr = (float*)gCineBuffer.getFrame(i, DAU_DATA_TYPE_ECG, CineBuffer::FrameContents::DataOnly);

        for (int j = 0; j < gEcgSamplesPerFrame; j++)
        {
            outputFloat[smpCnt] = outputFloatPtr[j];
            smpCnt++;
        }
    }
}

// make output txt file
int outputToFile(const char* outfileName, float* output)
{
    // output file can be read via Python Numpy
    // data = np.loadtxt('filename.txt')

    FILE* outfile;
    outfile = fopen(outfileName, "w");

    for (int i = 0; i < TEST_NUM_FRAMES * gEcgSamplesPerFrame; i++)
    {
        fprintf(outfile, "%f\n", output[i]);
    }

    fclose(outfile);

    return 0;
}


void printUsage()
{
    printf("Placeholder for Usage: \n");
}

int main(int argc, char * argv[])
{
    /* ---------------------------------------------------------------
     * Automated test for Ecg Process
     *
     * Ecp Process: 1) high-pass IIR, then 2) low-pass 36-tap FIR
     *      Sampling Rate: 397.36, IIR cutoff: 0.5 Hz, FIR cufoff: 52.5 Hz
     *
     * Test Case I: DC offset test
     *      - input: DC (0.5), expected output: peak near beginning then close to 0
     *
     * Test Case II: impulse test
     *      - input: impulse all zero except 421 data (0.5),
     *      - expected output: close to the shape of FIR filter
     *
     * Test Case III: sine wave of freq. 25 Hz.
     *      - input: 25 Hz sine wave
     *      - expected output: close to input
     *
     * Test Case IV: sine wave of freq. 75 Hz.
     *      - input: 75 Hz sine wave
     *      - expected output: close to 0
     *
     */

    // setup Ups Reader
    if (setupUpsReader() < 0)
    {
        printf("There is an error in setting up UpsReader.\n");
        return (-1);
    }

    // setup CineBuffer with Dummy Params
    setupCineBufferWithDummyParams();

    // setup EcgProcess
    setupEcgProcess();

    // Local variables
    // input pointers
    uint32_t* inputPtr = (uint32_t*) malloc (TEST_INPUT_MEM_SIZE);

    //output pointer
    float* outputFloat = new float[gEcgSamplesPerFrame*TEST_NUM_FRAMES];

    printf("\n-----------------------------------\n");
    printf("starting Test Case I - dc offset test.\n");

    prepareInputCaseI(inputPtr);
    prepareOutput(outputFloat);
    verifyTestCaseI(outputFloat);
    outputToFile("ecgProcessTest_Case1.txt", outputFloat);


    printf("\n-----------------------------------\n");
    printf("starting Test Case II - impulse.\n");

    initEcgProcess();

    prepareInputCaseII(inputPtr);
    prepareOutput(outputFloat);
    verifyTestCaseII(outputFloat);
    outputToFile("ecgProcessTest_Case2.txt", outputFloat);


    printf("\n-----------------------------------\n");
    printf("starting Test Case III - sine 25 Hz.\n");

    initEcgProcess();

    prepareInputCaseIII(inputPtr);
    prepareOutput(outputFloat);
    verifyTestCaseIII(outputFloat);
    outputToFile("ecgProcessTest_Case3.txt", outputFloat);


    printf("\n-----------------------------------\n");
    printf("starting Test Case IV - sine 75 Hz.\n");

    initEcgProcess();

    prepareInputCaseIV(inputPtr);
    prepareOutput(outputFloat);
    verifyTestCaseIV(outputFloat);
    outputToFile("ecgProcessTest_Case4.txt", outputFloat);

    printf("\n-----------------------------------\n");
    printf("\nAll Test Cases - test completed.\n");

    //clean up
    cleanUp();

    delete[] inputPtr;
    delete[] outputFloat;

    return (0);
}
