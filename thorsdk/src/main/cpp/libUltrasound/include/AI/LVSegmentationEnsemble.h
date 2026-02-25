#pragma once

#include <ThorError.h>
#include <EF_Types.h>
#include <LVSegmentationModel.h>


class LVSegmentationEnsemble
{
public:
    enum class Specialization {
        ED,
        ES,
        CombinedEDES
    };


    ThorStatus init(Specialization specialization);

    ThorStatus setCineBuffer(CineBuffer *cinebuffer);

    ThorStatus segment(FrameId frame, cv::Mat mask);

private:
    LVSegmentationModel mModels[3];
};