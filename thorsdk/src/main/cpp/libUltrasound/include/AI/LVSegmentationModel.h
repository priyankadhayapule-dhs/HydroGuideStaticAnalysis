#pragma once

#include <ThorError.h>
#include <SNPE.h>

class LVSegmentationModel
{
public:

    // Initialize from dlc data
    ThorStatus init(void *dlc, size_t dlcSize);

    // Segment using the given input data
    // Input must be an interlaced window of 5 frames
    ThorStatus segment(void *postscan, void *outputProbMap);

private:

    ThorStatus createUserBuffers();

    std::unique_ptr<zdl::SNPE::SNPE> mModel;

    std::unique_ptr<zdl::DlSystem::IUserBuffer> mInputTensor;
    std::unique_ptr<zdl::DlSystem::IUserBuffer> mOutputTensor;

    zdl::DlSystem::UserBufferMap mInputMap;
    zdl::DlSystem::UserBufferMap mOutputMap;
};