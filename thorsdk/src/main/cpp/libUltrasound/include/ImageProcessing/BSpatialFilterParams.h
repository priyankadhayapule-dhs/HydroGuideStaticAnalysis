//
// Copyright 2021 EchoNous, Inc.
//

#pragma once

struct BSpatialFilterParams {
    int mode;
    float blackholeaxialthresh;
    float blackholelateralthresh;
    float blackholealpha;
    float noiseaxialthresh;
    float noiselateralthresh;
    float noisealpha;
    int kernelaxialsize;
    int kernellateralsize;
    float gaussianaxialsigma;
    float gaussianlateralsigma;
};