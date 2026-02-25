//
// Copyright 2017 EchoNous Inc.
//
//

#pragma once

#include <stdint.h>
#include <memory>
#include <vector>
#include <ThorError.h>
#include <ThorConstants.h>
#include <ImageProcess.h>

class ImageProcessor
{
public: // Methods
    ImageProcessor();
    ~ImageProcessor();

    void setProcessList(const std::vector<ImageProcess*>& processList);
    ThorStatus processList(uint8_t* inputPtr, uint8_t* outputPtr);

    ThorStatus  initProcesses();
    void        terminateProcesses();

private: // struct
    struct ImageProcessNode
    {
        ImageProcess*   processPtr;
        uint8_t*        inputPtr;
        uint8_t*        outputPtr;
    };

private: // Properties
    std::vector<ImageProcessNode>   mProcessList;

    // Intermediate buffers for processing
    uint8_t                         mImageA[MAX_B_FRAME_SIZE];
    uint8_t                         mImageB[MAX_B_FRAME_SIZE];
};
