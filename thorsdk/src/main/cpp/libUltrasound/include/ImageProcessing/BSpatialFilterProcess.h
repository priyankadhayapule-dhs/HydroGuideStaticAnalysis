#pragma once

// An ImageProcess operation which applies an IIRFilter to every row of the input
#include <vector>
#include <opencv2/imgproc.hpp>
#include <BSpatialFilterParams.h>
#include "ImageProcess.h"

class BSpatialFilterProcess : public ImageProcess
{
public:
    BSpatialFilterProcess(const std::shared_ptr<UpsReader> &ups);

    static const char *name() { return "BSpatialFilterProcess"; }

    virtual ThorStatus init() override;
    virtual ThorStatus setProcessParams(ProcessParams &params) override;
    virtual ThorStatus process(uint8_t *inputPtr, uint8_t *outputPtr) override;
    virtual void terminate() override;
private:

    ThorStatus spatialFilter(uint8_t *inputPtr, uint8_t *outputPtr);
    ProcessParams mParams;
    BSpatialFilterParams mBSpatialFilterParams;
    std::vector<cv::Mat> mXs;
    std::vector<cv::Mat> mYs;
    cv::Mat mInputF;
    cv::Mat mForward;
    cv::Mat mBackward;
    cv::Mat mOutputF;
};
