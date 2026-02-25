#pragma once

#include <vector>
#include <opencv2/imgproc.hpp>
#include <ImageProcess.h>
#include <CineBuffer.h>

class DopplerPeakMeanPostProcess : public ImageProcess
{
public:
	DopplerPeakMeanPostProcess(const std::shared_ptr<UpsReader> &ups);

	static const char *name() { return "DopplerPeakMeanPostProcess"; }

	virtual ThorStatus init() override;
	virtual ThorStatus setProcessParams(ProcessParams &params) override;
	virtual ThorStatus process(uint8_t *inputPtr, uint8_t *outputPtr) override;
	virtual void terminate() override;

	void setDataType(int dataType);
	void setProcessIndices(uint32_t , float relThreshold, float absThreshold);

private:
	ProcessParams mParams;

	int 	      	mDataType;
	uint32_t      	mNumFFT;
	float			mRelThreshold;
	float 			mAbsThreshold;
};
