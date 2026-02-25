//
// Copyright 2017 EchoNous Inc.
//
//

#include <ImageProcessor.h>

//-----------------------------------------------------------------------------
ImageProcessor::ImageProcessor()
{
}

//-----------------------------------------------------------------------------
ImageProcessor::~ImageProcessor()
{
}

//-----------------------------------------------------------------------------
void ImageProcessor::

setProcessList(const std::vector<ImageProcess*>& processList)
{
    unsigned    curProcIndex = 0;
    unsigned    numProcs = 0;
    uint8_t*    imageAPtr = mImageA;
    uint8_t*    imageBPtr = mImageB;

    //
    // Populate the Process List
    //
    mProcessList.resize(processList.size());
    mProcessList.clear();
    for (curProcIndex = 0; curProcIndex < processList.size(); curProcIndex++)
    {
        if (processList[curProcIndex]->isEnabled())
        {
            ImageProcessNode newNode = { 
                                         processList[curProcIndex],
                                         nullptr,
                                         nullptr
                                       };
            mProcessList.push_back(newNode);
            numProcs++;
        }
    }

    // BUGBUG: It is assumed that at least one of the processes is enabled!

    // Define input and output pointers
    mProcessList[0].inputPtr = nullptr;
    for (curProcIndex = 1; curProcIndex < numProcs; curProcIndex++)
    {
        mProcessList[curProcIndex - 1].outputPtr = imageAPtr;
        mProcessList[curProcIndex].inputPtr = imageAPtr;

        std::swap(imageAPtr, imageBPtr);
    }
    mProcessList[numProcs - 1].outputPtr = nullptr;

    return;
}

//-----------------------------------------------------------------------------
ThorStatus ImageProcessor::processList(uint8_t* inputPtr, uint8_t* outputPtr)
{
    ThorStatus  retVal = THOR_OK;
    int         curProcIndex;
    int         numProcs = mProcessList.size();

    mProcessList[0].inputPtr = inputPtr;
    mProcessList[numProcs - 1].outputPtr = outputPtr;

    for (auto it = mProcessList.begin();
         it != mProcessList.end();
         it++)
    {
        (*it).processPtr->process((*it).inputPtr, (*it).outputPtr);
    }

    return(retVal); 
}

//-----------------------------------------------------------------------------
ThorStatus ImageProcessor::initProcesses()
{
    ThorStatus      retVal = THOR_OK;

    for (auto it = mProcessList.begin();
         it != mProcessList.end();
         it++)
    {
        if (IS_THOR_ERROR((*it).processPtr->init()))
        {
            retVal = TER_IE_IMAGE_PROCESSOR_INIT;
            break;
        }
    }

    return(retVal); 
}

//-----------------------------------------------------------------------------
void ImageProcessor::terminateProcesses()
{
    for (auto it = mProcessList.begin();
         it != mProcessList.end();
         it++)
    {
        (*it).processPtr->terminate();
    }
}

