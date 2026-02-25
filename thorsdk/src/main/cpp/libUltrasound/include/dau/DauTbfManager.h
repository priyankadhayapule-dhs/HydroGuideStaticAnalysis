//
// Copyright 2017 EchoNous Inc.
//
//
#pragma once
#include <cstdint>
#include <memory>
#include <DauLutManager.h>
#include <ThorConstants.h>
#include <DauMemory.h>
#include <ColorAcq.h>
#include <ColorAcqLinear.h>

class DauTbfManager
{
public:
    DauTbfManager(const std::shared_ptr<DauMemory>& daum);
    ~DauTbfManager();
    bool init(DauLutManager& lutm);
    void reinit();
    bool ASICinit();
    bool setImagingCase(DauLutManager& lutm, uint32_t imagingCase, int imagingMode=IMAGING_MODE_B);
    bool setColorImagingCase(DauLutManager& lutm, uint32_t imagingCase, ColorAcq *colorAcqPtr, uint32_t wallFilterType);
    bool setColorImagingCaseLinear(DauLutManager& lutm, uint32_t imagingCase, ColorAcqLinear *colorAcqPtr, uint32_t wallFilterType);
    bool setPwImagingCase(DauLutManager& lutm, uint32_t imagingCase, DopplerAcq *dopplerAcqPtr);
    bool setPwImagingCaseLinear(DauLutManager& lutm, uint32_t imagingCase, DopplerAcqLinear *dopplerAcqPtr);
    bool setCwImagingCase(DauLutManager& lutm, uint32_t imagingCase, CWAcq *cwAcqPtr);
    bool start();
    bool stop();

private:
    DauTbfManager();
    uint32_t AsicTestCount;

    void fillTestPattern();

private:
    std::shared_ptr<DauMemory>          mDaumSPtr;
};



