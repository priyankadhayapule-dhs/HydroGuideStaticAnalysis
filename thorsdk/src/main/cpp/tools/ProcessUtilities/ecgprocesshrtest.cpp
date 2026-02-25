//
// Copyright 2018 EchoNous, Inc.
//
#include <stdio.h>
#include <stdlib.h>
#include <cstdint>
#include <algorithm>
#include <memory>
#include <map>

#include <UpsReader.h>
#include <CineBuffer.h>
#include <EcgProcess.h>
#include <SineGenerator.h>

#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>


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

    // set dB gain - default gain 10 dB
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

// is regular file
int is_regular_file(const char *path)
{
    struct stat path_stat;
    stat(path, &path_stat);
    return S_ISREG(path_stat.st_mode);
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

// read estimated heart rate
float getEstimatedHR(int frameNum)
{
    uint8_t *outputPtr = gCineBuffer.getFrame(frameNum, DAU_DATA_TYPE_ECG, CineBuffer::FrameContents::IncludeHeader);
    CineBuffer::CineFrameHeader* cineHeaderPtr = reinterpret_cast<CineBuffer::CineFrameHeader*>(outputPtr);

    return cineHeaderPtr->heartRate;
}

void printHRTrend(int maxFrameNum)
{
    for (int i = 0; i < maxFrameNum; i++)
    {
        uint8_t *outputPtr = gCineBuffer.getFrame(i, DAU_DATA_TYPE_ECG, CineBuffer::FrameContents::IncludeHeader);
        CineBuffer::CineFrameHeader* cineHeaderPtr = reinterpret_cast<CineBuffer::CineFrameHeader*>(outputPtr);
        float currentHR = cineHeaderPtr->heartRate;

        printf("FrameNum: %d, estimatedHR: %f\n", i, currentHR);
    }

    return;
}


// Input File Processing
void processInputFile(char* inputFile, uint32_t* inputPtr)
{
    // process Input File
    FILE* infile;
    infile = fopen(inputFile, "rb");

    // input word pointer
    uint32_t *inputWordPtr;
    // data
    float fval;
    float fval_int;
    uint32_t uval;

    float readVal;
    int readCnt = 0;
    int frameCnt = 0;

    float max_val = -10000.0;

    // temp input location
    float* inputFloat = new float[gEcgSamplesPerFrame];

    initEcgProcess();

    while((fscanf(infile,"%f",&readVal) == 1) && (frameCnt < TEST_NUM_FRAMES))
    {
        //printf("%.15f \n", readVal);
        inputFloat[readCnt] = readVal;
        readCnt++;

        if (readCnt >= gEcgSamplesPerFrame)
        {
            // prepare inputPtr
            inputWordPtr = (uint32_t*)inputPtr;

            // skip the header (size 16)
            inputWordPtr += 4; // skip past header

            for (int i = 0; i < gEcgSamplesPerFrame; i++)
            {
                fval = inputFloat[i];

                if (fval > max_val)
                    max_val = fval;

                fval_int = fval / 750.0 * 3.5 / 4.8 + 0.5;
                uval = (uint32_t) (fval_int * gEcgADCMax);

                // status code
                uval = (uval | 0x26000000);

                *inputWordPtr = uval;
                inputWordPtr += 4;
            }

            gEcgProcessPtr->process((uint8_t*) inputPtr, frameCnt);
            frameCnt++;

            // reset Counter;
            readCnt = 0;
        }
    }

    fclose(infile);

    return;
}

// Processing Directory
int processInputDir(char* inputDir, uint32_t* inputPtr)
{
    char file_str[512];
    char* pch;
    char int_num[4];
    char frac_num[3];
    int int_part;
    int frac_part;
    float hr;
    float estimatedHR;

    int err_cnt = 0;
    int match_cnt = 0;

    std::map<int, int> result;

    struct dirent *de;  // Pointer for directory entry

    // opendir() returns a pointer of DIR type.
    DIR *dr = opendir(inputDir);

    if (dr == NULL)  // opendir returns NULL if couldn't open directory
    {
        printf("Could not open current directory" );
        return 0;
    }

    // for readdir()
     while ((de = readdir(dr)) != NULL)
     {
        //memset(file_str, 0, sizeof(file_str));
        strcpy(file_str, inputDir);
        strcat(file_str, "/");
        strcat(file_str, de->d_name);

        pch = strstr(file_str, ".dat");

        if (pch != NULL)
        {
            memcpy(int_num, pch-6, 3);
            memcpy(frac_num, pch-2, 2);
            int_num[3] = '\0';
            frac_num[2] = '\0';

            int_part = atoi(int_num);
            frac_part = atoi(frac_num);

            hr = ((float) int_part) + ((float) frac_part) * 0.01f;
            hr = round(hr);

            if (is_regular_file(file_str))
            {
                processInputFile(file_str, inputPtr);
                estimatedHR = getEstimatedHR(265);

                if (estimatedHR < 0.0)
                    estimatedHR = getEstimatedHR(200);

                if (estimatedHR > 0.0)
                {
                    // if bigger than threshold, use try ealier measurement
                    if (abs(hr - estimatedHR) > 3)
                    {
                        estimatedHR = getEstimatedHR(200);
                    }

                    if (abs(hr - estimatedHR) <= 3)
                    {
                        match_cnt++;
                        result[abs(hr - estimatedHR)]++;

                        if (abs(hr - estimatedHR) == 4)
                        {
                            estimatedHR = getEstimatedHR(200);
                            printf("%s, reference: %f, estimated: %f, error: %f\n", file_str, hr, estimatedHR, abs(hr - estimatedHR));
                        }

                    }
                    else
                    {
                        err_cnt++;
                        printf("\n%s, reference: %f, estimated: %f, error: %f\n", file_str, hr, estimatedHR, abs(hr - estimatedHR));
                    }
                }
            }
        }
     }

     closedir(dr);

     char pass_fail_str[5];

     if (err_cnt == 0)
        strcpy(pass_fail_str, "PASS");
    else
        strcpy(pass_fail_str, "FAIL");

     printf("\n========== HR estimate process test result ==========\n");

     printf("match_cnt: %d, err_cnt: %d\n", match_cnt, err_cnt);

     for (auto itr = result.begin(); itr != result.end(); ++itr)
     {
        printf("Measurement error: %d, %d\n", itr->first, itr->second);
     }

     printf("Test result: %s\n", pass_fail_str);

     printf("=====================================================\n");

     return 0;
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
     * Automated test for Ecg HR Estimation
     * ECG data in a plain text format should be placed under "MLII_out"
     *     directory.
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
    printf("starting ECG Heart Rate Estimation Module Test.\n");

    processInputDir("MLII_out", inputPtr);

    printf("\nECG Heart Rate Estimation Module Test - test complete.\n");

    //clean up
    cleanUp();

    delete[] inputPtr;
    delete[] outputFloat;

    return (0);
}
