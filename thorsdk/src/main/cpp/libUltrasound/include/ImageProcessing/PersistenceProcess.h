#pragma once

#include <opencv2/core.hpp>
#include "ImageProcess.h"

class PersistenceProcess : public ImageProcess
{
public:
    PersistenceProcess(const std::shared_ptr<UpsReader> &upsReader);

    static const char *name() { return "PersistenceProcess"; }


    ThorStatus init() override;
    ThorStatus setProcessParams(ProcessParams &params) override;
    ThorStatus process(uint8_t *inputPtr, uint8_t *outputPtr) override;
    void terminate() override;
    void setResetFlag();

private:

	bool mReset; // should we reset the stored persistence frame (used when changing params)
    cv::Mat mLUT;
    ProcessParams mParams;
    cv::Mat mPersist;
    cv::Mat mDifference;
    cv::Mat mAlpha;
};