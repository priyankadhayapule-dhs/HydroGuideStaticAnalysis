//
// Copyright 2019 EchoNous, Inc.
//
#include <stdio.h>
#include <cstdint>
#include <algorithm>
#include <memory>
#include <fcntl.h>

#include <UpsReader.h>
#include <CineBuffer.h>
#include <DaProcess.h>
#include <SineGenerator.h>
#include <PciDauMemory.h>
#include <DauContext.h>


#define TEST_INPUT_MEM_SIZE 8096 //byte
#define TEST_NUM_FRAMES 1000


// Ecg Process
DaProcess*                 gDaProcessPtr;
CineBuffer                 gCineBuffer;

const char*                gUpsFileName;
std::shared_ptr<UpsReader> gUpsReaderSPtr;

uint32_t                   gDaSamplesPerFrame;
float                      gDaSamplesPerSecond;
uint32_t                   gImagingCaseId;

SineGenerator              *gSineOsc;

// Daum
std::shared_ptr<DauMemory> gDaumSPtr;
DauContext                 gDauContext;
uint32_t                   gChVal;


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
    // init cinebuffer - with dummy params for DaProcess
    CineBuffer::CineParams  cineParams;
    gImagingCaseId = 1000;
    cineParams.imagingCaseId = gImagingCaseId;
    uint32_t dtype = getDauDataTypes(IMAGING_MODE_B , true, false, true);
    cineParams.dauDataTypes = dtype;
    gCineBuffer.setParams(cineParams);
}

void initDaProcess()
{
    // initialization
    if (nullptr != gDaProcessPtr)
    {
        gDaProcessPtr->initializeProcessIndices();
    }
}

int setupDaProcess()
{
    // Daum
    gDaumSPtr = std::make_shared<PciDauMemory>();

    // manually adjust DauContext
    gDauContext.dauResourceFd = ::open("testmem", O_RDWR | O_NONBLOCK);

    if (!gDaumSPtr->map(&gDauContext))
    {
        printf("DauMemory::map() failed");
        return -1;
    }

    // set the channel value: needs to be between 1 ~ 3
    gChVal = 0x1;
    gDaumSPtr->write( &gChVal, SYS_TEST_ADDR, 1 );

    // verify the channel setting
    uint32_t readChVal;
    gDaumSPtr->read(SYS_TEST_ADDR, &readChVal, 1);
    printf("\nDa Testing Channel: %d\n", readChVal);

    // setting up necessary resources
    gDaProcessPtr = new DaProcess(gDaumSPtr, gUpsReaderSPtr);
    gDaProcessPtr->init();
    gDaProcessPtr->setCineBuffer(&gCineBuffer);

    // set parameters
    uint32_t afeClksPerDaSample, afeClksPerEcgSample;
    const UpsReader::Globals* glbPtr = gUpsReaderSPtr->getGlobalsPtr();
    float samplingFrequency = glbPtr->samplingFrequency;
    // TODO: update with sequenceBuilder if necessary
    gDaSamplesPerFrame = 448;

    gUpsReaderSPtr->getAfeClksPerDaEcgSample(0, afeClksPerDaSample, afeClksPerEcgSample);
    gDaSamplesPerSecond = samplingFrequency*1e6/afeClksPerDaSample;

    gDaProcessPtr->setProcessParams(gDaSamplesPerFrame, gUpsReaderSPtr->getImagingOrgan(gImagingCaseId));

    initDaProcess();

    // set dB gain - default gain 10 dB
    gDaProcessPtr->setGain(10.0);

    return 0;
}

void cleanUp()
{
    // clean up
    if (nullptr != gDaProcessPtr)
    {
        delete gDaProcessPtr;
        gDaProcessPtr = nullptr;
    }

    if (nullptr != gDaumSPtr)
    {
      gDaumSPtr = nullptr;
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

    float* inputFloat = new float[gDaSamplesPerFrame];
    for (int i = 0; i < gDaSamplesPerFrame; i++)
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

        // channel offset
        inputWordPtr += gChVal;

        for (int i = 0; i < gDaSamplesPerFrame; i++)
        {
            fval = inputFloat[i];

            fval_int = ((fval / 2.0) + 0.5) * 1024.0;
            uval = (uint32_t) (fval_int);

            uval = (uval & 0x3FF);

            *inputWordPtr = uval;
            inputWordPtr += 4;
        }

        gDaProcessPtr->process((uint8_t*) inputPtr, cnt);
        cnt++;
    }

    delete[] inputFloat;
}

// test case II
void prepareInputCaseII(uint32_t* inputPtr)
{
    // input word pointer
    uint32_t *inputWordPtr;

    // data
    float fval;
    float fval_int;
    uint32_t uval;
    int cnt = 0;

    float* inputFloat = new float[gDaSamplesPerFrame];
    for (int i = 0; i < gDaSamplesPerFrame; i++)
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

        // channel offset
        inputWordPtr += gChVal;

        for (int i = 0; i < gDaSamplesPerFrame; i++)
        {
            fval = inputFloat[i];

            if ((cnt == 30) & (i == 0))
                fval = 0.5;

            fval_int = ((fval / 2.0) + 0.5) * 1024.0;
            uval = (uint32_t) (fval_int);

            uval = (uval & 0x3FF);

            *inputWordPtr = uval;
            inputWordPtr += 4;
        }

        gDaProcessPtr->process((uint8_t*) inputPtr, cnt);
        cnt++;
    }

    delete[] inputFloat;
}


// test case III - sine signal
void prepareInputCaseIII(uint32_t* inputPtr, float frequency)
{
    // Sine Wave test.  100 Hz
    gSineOsc = new SineGenerator();
    gSineOsc->setup(frequency, ceil(gDaSamplesPerSecond), 0.25);

    // offset data test for the IIR filter
    // input word pointer
    uint32_t *inputWordPtr;

    // data
    float fval;
    float fval_int;
    uint32_t uval;
    int cnt = 0;

    float* inputFloat = new float[gDaSamplesPerFrame];
    for (int i = 0; i < gDaSamplesPerFrame; i++)
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

        // channel offset
        inputWordPtr += gChVal;

        // update frame data
        gSineOsc->render(inputFloat, 1, gDaSamplesPerFrame);

        for (int i = 0; i < gDaSamplesPerFrame; i++)
        {
            fval = inputFloat[i];

            fval_int = ((fval / 2.0) + 0.5) * 1024.0;
            uval = (uint32_t) (fval_int);

            uval = (uval & 0x3FF);

            *inputWordPtr = uval;
            inputWordPtr += 4;
        }

        gDaProcessPtr->process((uint8_t*) inputPtr, cnt);
        cnt++;
    }

    delete[] inputFloat;
}


bool verifyTestCaseI(float* outputFloat)
{
    // return value: false -> fail
    //               true -> pass
    bool retVal = false;

    // expected. after about 50000 values should be close to zero (less than 0.00001)
    int overCnt = 0;
    float thVal = 0.00001f;

    for (int i = 50000; i < TEST_NUM_FRAMES * gDaSamplesPerFrame; i++)
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

    // expected. zeros until index 13440.
    // max index -> 437, min index -> 443
    // recommend to plot the data to see the pattern
    float thVal = 0.00001f;
    int minIdx = 0;
    int maxIdx = 0;
    float minVal = 1000.0f;
    float maxVal = -1000.0f;
    int nonZeroCnt = 0;

    for (int i = 0; i < gDaSamplesPerFrame * TEST_NUM_FRAMES; i++)
    {
        if ((outputFloat[i] > thVal) && (i < gDaSamplesPerFrame * 30))
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

    if ((nonZeroCnt > 0) || (maxIdx != 13619) || (minIdx != 13635))
    {
        printf("\nTest Verification Failed: Test Case II\n");
        printf("nonZero: %d, maxIdx: %d, minIdx: %d", nonZeroCnt, maxIdx, minIdx);
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
    int startIdx = 1800;
    int spanSize = 128;
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

    freq = 1.0f/((maxIdx2 - maxIdx1)/gDaSamplesPerSecond);

    if (abs(100.0f - freq)/100.0f > 0.01f)
    {
        printf("\nTest Verification Failed: Test Case III\n");
        printf("Estimated frequency: %f Hz\n", freq);
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

    // expected. after about 1000 values should be close to zero (less than 0.01)
    // as those need to be filtered out
    int overCnt = 0;
    float thVal = 0.005f;

    for (int i = 1000; i < TEST_NUM_FRAMES * gDaSamplesPerFrame; i++)
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
        outputFloatPtr = (float*)gCineBuffer.getFrame(i, DAU_DATA_TYPE_DA, CineBuffer::FrameContents::DataOnly);

        for (int j = 0; j < gDaSamplesPerFrame; j++)
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

    for (int i = 0; i < TEST_NUM_FRAMES * gDaSamplesPerFrame; i++)
    {
        fprintf(outfile, "%f\n", output[i]);
    }

    fclose(outfile);

    return 0;
}


// create test memory file for memory mapping
void testMemFile()
{
    FILE* testout;
    int   testSize = 4096 * 4096;

    uchar* out = (uchar*) malloc(testSize);
    memset(out, 0, testSize);

    testout = fopen("testmem", "w");
    fwrite(out , sizeof(uchar), testSize, testout);

    fclose(testout);
}

void printUsage()
{
    printf("Placeholder for Usage: \n");
}

int main(int argc, char * argv[])
{
    /* ---------------------------------------------------------------
     * Automated test for Da Process
     *
     * Da Process: 1) high-pass IIR, then 2) band-pass 360-tap FIR (organ: heart)
     *      Sampling Rate: 12715.66, IIR cutoff: 4 Hz, FIR cufoff: 50 Hz ~ 600 Hz
     *
     * Test Case I: DC offset test
     *      - input: DC (0.5), expected output: peak near beginning then close to 0
     *
     * Test Case II: impulse test
     *      - input: impulse all zero except 13441 data (0.5),
     *      - expected output: close to the shape of FIR filter
     *
     * Test Case III: sine wave of freq. 100 Hz.
     *      - input: 100 Hz sine wave
     *      - expected output: close to input
     *
     * Test Case IV: sine wave of freq. 800 Hz.
     *      - input: 800 Hz sine wave
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

    // setup DaProcess
    if (setupDaProcess() < 0)
    {
        printf("SetupDaProcess failed.\n");
        return (-1);
    }

    // Local variables
    // input pointers
    uint32_t* inputPtr = (uint32_t*) malloc (TEST_INPUT_MEM_SIZE);

    //output pointer
    float* outputFloat = new float[gDaSamplesPerFrame*TEST_NUM_FRAMES];

    printf("\n-----------------------------------\n");
    printf("starting Test Case I - dc offset test.\n");

    prepareInputCaseI(inputPtr);
    prepareOutput(outputFloat);
    verifyTestCaseI(outputFloat);
    outputToFile("daProcessTest_Case1.txt", outputFloat);


    printf("\n-----------------------------------\n");
    printf("starting Test Case II - impulse.\n");

    initDaProcess();

    prepareInputCaseII(inputPtr);
    prepareOutput(outputFloat);
    verifyTestCaseII(outputFloat);
    outputToFile("daProcessTest_Case2.txt", outputFloat);


    printf("\n-----------------------------------\n");
    printf("starting Test Case III - Sine 100 Hz.\n");

    initDaProcess();

    prepareInputCaseIII(inputPtr, 100.0f);
    prepareOutput(outputFloat);
    verifyTestCaseIII(outputFloat);
    outputToFile("daProcessTest_Case3.txt", outputFloat);


    printf("\n-----------------------------------\n");
    printf("starting Test Case IV - Sine 800 Hz.\n");

    initDaProcess();

    prepareInputCaseIII(inputPtr, 800.0f);
    prepareOutput(outputFloat);
    verifyTestCaseIV(outputFloat);
    outputToFile("daProcessTest_Case4.txt", outputFloat);

    printf("\n-----------------------------------\n");
    printf("\nAll Test Cases Complete.\n");

    //clean up
    cleanUp();

    delete[] inputPtr;
    delete[] outputFloat;

    return (0);
}
