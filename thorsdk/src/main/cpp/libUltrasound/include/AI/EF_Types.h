//
// Copyright 2019 EchoNous Inc.
//

// Data types used in EF workflow (refactor)

#pragma once

typedef uint32_t FrameId;

struct ClipPhaseData
{
    std::vector<float> phaseEstimate;
    std::vector<FrameId> edFrames;
    std::vector<FrameId> esFrames;
    FrameId ed;
    FrameId es;
};

struct ScanPhaseData
{
    ClipPhaseData a4c;
    ClipPhaseData a2c;
};

struct ClipSegmentationData
{
    cv::Mat mask;
    std::vector<cv::Point> contour;
    std::vector<cv::Point> controlPoints;
};

struct ScanSegmentationData
{
    ClipSegmentationData a4c;
    ClipSegmentationData a2c;
};

struct ClipStatistics
{
    float edv;
    float esv;
    float hr;
    float sv;
    float co;
    float ef;
};

struct ScanStatistics
{
    ClipStatistics a4c;
    ClipStatistics a2c;
    ClipStatistics biplane;
};

struct EFScanData
{
    ScanPhaseData phase;
    ScanSegmentationData segmentation;
    ScanStatistics stats;
};

struct EFState
{
    EFScanData ai;
    EFScanData user; 
};