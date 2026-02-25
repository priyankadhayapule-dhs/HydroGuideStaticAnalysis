#pragma once

#include <vector>
#include <opencv2/imgproc.hpp>
#include <ImageProcess.h>

class DopplerPeakMeanPreProcess : public ImageProcess
{
public:
	DopplerPeakMeanPreProcess(const std::shared_ptr<UpsReader> &ups);

	static const char *name() { return "DopplerPeakMeanPreProcess"; }

	virtual ThorStatus init() override;
	virtual ThorStatus setProcessParams(ProcessParams &params) override;
	virtual ThorStatus process(uint8_t *inputPtr, uint8_t *outputPtr) override;
	virtual void terminate() override;

	void setDataType(int dataType);

private:
	ProcessParams mParams;

	int 	      mDataType;
};
