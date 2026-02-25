//
// Copyright 2019 EchoNous Inc.
//

// Image processing operations not in OpenCV
#pragma once

#include <cstdio>
#include <opencv2/core.hpp>

// Write an input to a stream in either PGM or PPM format (uncompressed)
//  The image must be either 1 or 3 channel, and either CV_8U or CV_32F depth
//  For a 32F image, expected values (non-normalized) are between 0 and 1.
void WriteImg(const char *path, cv::InputArray _input, bool normalize=false);

// Finds the indices of the maximum value in _input across all channels
// Input is a WxHxN image, output will be a WxHx1 image of type CV_8U
void ChannelMax(cv::InputArray _input, cv::OutputArray _output);

// Finds all connected components in _src using cv::connectedComponents,
//  then selects the largest component and masks out all others
void LargestComponent(cv::InputArray _src, cv::OutputArray _dst);