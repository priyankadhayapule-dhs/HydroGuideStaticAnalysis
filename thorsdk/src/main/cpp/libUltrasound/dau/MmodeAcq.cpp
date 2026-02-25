//
// Copyright 2018 EchoNous Inc.
//

#define LOG_TAG "DauMmodeAcq"

#include <algorithm>
#include <ThorUtils.h>
#include <math.h>
#include <MmodeAcq.h>


MmodeAcq::MmodeAcq(const std::shared_ptr<UpsReader>& upsReader) :
    mUpsReaderSPtr(upsReader)
{
    LOGD("*** MmodeAcq - constructor");
}

MmodeAcq::~MmodeAcq()
{
    LOGD("*** MmodeAcq -destructor");
}

void MmodeAcq::getNearestBBeam(uint32_t imagingCaseId, float mLinePosition, uint32_t &beamNumber,
        uint32_t &multilinePosition, bool isLinearProbe)
{
    ScanConverterParams scp;
    float    mbeamPos;
    float    fracOff;
    int32_t  intPos;

    if (isLinearProbe)
        scp.probeShape = PROBE_SHAPE_LINEAR;
    else
        scp.probeShape = PROBE_SHAPE_PHASED_ARRAY;

    mUpsReaderSPtr->getScanConverterParams(imagingCaseId, scp, IMAGING_MODE_M);

    if (isLinearProbe)
        mbeamPos = ( (scp.startRayMm + scp.raySpacing * (scp.numRays - 1)) - mLinePosition ) / scp.raySpacing;
    else
        mbeamPos = (mLinePosition - scp.startRayRadian) / scp.raySpacing;

    intPos  = floor(mbeamPos);
    fracOff = mbeamPos - intPos;
    if (fracOff > 0.5)
    {
        intPos++;
    }

    if (intPos >= (scp.numRays - 1))
    {
        intPos = scp.numRays - 1;
    }
    if (intPos < 0)
    {
        intPos = 0;
    }

    beamNumber = ((uint32_t)intPos) >> 2;
    multilinePosition = ((uint32_t)intPos) & 3;
    LOGD("imgingCase = %d, mlinePosition = %f, beamNumber = %d, multiLinePosition = %d",
         imagingCaseId, mLinePosition, beamNumber, multilinePosition);
}

