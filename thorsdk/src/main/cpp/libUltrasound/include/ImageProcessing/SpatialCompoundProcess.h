#pragma once

// An ImageProcess operation which applies an IIRFilter to every row of the input
#include <vector>
#include <opencv2/imgproc.hpp>
#include <BEdgeFilterParams.h>
#include <ImageProcess.h>
#include <CineBuffer.h>

class SpatialCompoundProcess : public ImageProcess
{
public:
	SpatialCompoundProcess(const std::shared_ptr<UpsReader> &ups);

	static const char *name() { return "SpatialCompoundProcess"; }

	virtual ThorStatus init() override;
	virtual ThorStatus setProcessParams(ProcessParams &params) override;
	virtual ThorStatus process(uint8_t *inputPtr, uint8_t *outputPtr) override;
	virtual void terminate() override;
private:

	ProcessParams mParams;
	bool 	  	  mBypassCompounding;
	BEdgeFilterParams mBEdgeFilterParams;
    cv::Mat mPosImg;        // positive angle
    cv::Mat mNegImg;        // negative angle
    cv::Mat mCtrImg;
    cv::Mat mPosImg2;       // positive angle
    cv::Mat mNegImg2;       // negative angle
    cv::Mat mCtrImg2;
    cv::Mat mInOutImg;		// input/output Img
    cv::Mat mMaxImg;

	cv::Mat mWeightCtr;		// ctr weighting img
	cv::Mat mWeightPos;		// pos weighting img
	cv::Mat mWeightNeg;		// neg weighting img

    Point2f mSrcTri[3];     // src triangle
    Point2f mPosTri[3];     // forward triangle
    Point2f mNegTri[3];     // backward triangle

    cv::Mat mPosMatrix;
    cv::Mat mNegMatrix;
};
