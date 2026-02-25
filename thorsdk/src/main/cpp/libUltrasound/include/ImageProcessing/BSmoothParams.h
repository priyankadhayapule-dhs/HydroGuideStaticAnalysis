//
// Copyright 2021 EchoNous, Inc.
//

#pragma once
#include <cstdint>
#include <vector>

struct BSmoothParams {
    uint32_t mode;
    uint32_t numLPFtaps;
    std::vector<float> aCoeffs;
    std::vector<float> bCoeffs;
};