#pragma once

// An ImageProcess operation which applies an IIRFilter to every row of the input
#include <vector>
#include <opencv2/imgproc.hpp>
#include <BSmoothParams.h>

#include "ImageProcess.h"

class BSmoothProcess : public ImageProcess
{
public:
	BSmoothProcess(const std::shared_ptr<UpsReader> &ups);

	static const char *name() { return "BSmoothProcess"; }

	virtual ThorStatus init() override;
	virtual ThorStatus setProcessParams(ProcessParams &params) override;
	virtual ThorStatus process(uint8_t *inputPtr, uint8_t *outputPtr) override;
	virtual void terminate() override;
private:

	ProcessParams mParams;
	BSmoothParams mBSmoothParams;
	std::vector<cv::Mat> mXs;
	std::vector<cv::Mat> mYs;
	cv::Mat mInputF;
	cv::Mat mForward;
	cv::Mat mBackward;
	cv::Mat mBlended;
};