//
//  BladderWorkflow.h
//  EchoNous
//
//  Created by Echonous Inc. on 5/22/24.
//

#pragma once

#include <vector>
#include "opencv2/core/types.hpp"

enum BVType {
    SAGITTAL = 0,
    TRANSVERSE = 1
};

struct BVPoint
{
    float x;
    float y;
};

struct BVCaliper
{
    BVPoint startPoint;
    BVPoint endPoint;
};

struct BVSegmentation
{
    BVCaliper majorCaliper;
    BVCaliper minorCaliper;
    std::vector<cv::Point2f> contour;
};

class BladderWorkflow {
public:
    
public:
    BladderWorkflow();
    ~BladderWorkflow() = default;
};
