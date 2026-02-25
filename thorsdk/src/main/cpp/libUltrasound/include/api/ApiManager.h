//
// Copyright 2019 EchoNous Inc.
//
//
#pragma once

#include <stdint.h>
#include <memory>
#include "ApiCalculator.h"
#include "UpsReader.h"
#include "DauSequenceBuilder.h"
#include "ColorAcq.h"
#include "ColorAcqLinear.h"
#include "DopplerAcq.h"
#include "DopplerAcqLinear.h"
#include "CWAcq.h"
#include "ThorError.h"

class ApiManager
{
public:


public:
    ApiManager(const std::shared_ptr<UpsReader>& upsReader, uint32_t probeType);
    ApiManager() {};
    ~ApiManager() {};
    void init( const std::shared_ptr<UpsReader>& upsReader) { mUpsReaderSPtr = upsReader; };
    ThorStatus initApiTable();
    ThorStatus estimateIndices( uint32_t imagingCase, uint32_t imagingMode, bool isLinear, DauSequenceBuilder* seqBuilderPtr, ColorAcq* colorAcqPtr, ColorAcqLinear* colorAcqLinearPtr,
            DopplerAcq* dopplerAcqPtr, DopplerAcqLinear* dopplerAcqLinearPtr, CWAcq* cwAcqPtr, float indices[4], float &txVoltage, bool isTDI);
    ThorStatus estimateIndicesPW( uint32_t imagingCase, uint32_t imagingMode, bool isLinear, DauSequenceBuilder* seqBuilderPtr, ColorAcq* colorAcqPtr, ColorAcqLinear* colorAcqLinearPtr,
                                DopplerAcq* dopplerAcqPtr, DopplerAcqLinear* dopplerAcqLinearPtr, CWAcq* cwAcqPtr, float indices[4], float &txVoltage, bool isTDI, uint32_t organId, float gateRange);
    ThorStatus setupInputVector(uint32_t imagingCase, uint32_t imagingMode, bool isLinear, DauSequenceBuilder* seqBuilderPtr, ColorAcq* colorAcqPtr, ColorAcqLinear* colorAcqLinearPtr,
            DopplerAcq* dopplerAcqPtr, DopplerAcqLinear* dopplerAcqLinearPtr, CWAcq* cwAcqPtr, bool isTDI, uint32_t organId=0, float gateRange=0.0);
    void logApiTable();
    void logInputOutputVectors();
    void test();
    static const int MAX_API_TABLE_SIZE = 500;
private:
    int Invoke (API_INVECTOR_p_t, API_TABLE_p_t, API_OUTVECTOR_p_t);
    void setupApiTableHeader();
    ThorStatus initUtpIdTable();
    ThorStatus findApiTableIndices(uint32_t numUtps, int32_t utpIds[3], int32_t apiTableIndices[3]);

    API_TABLE_t                 mApiTable;
    API_UTPID_TABLE_t           mUtpIdTable[UpsReader::MAX_UTPS_IN_UPS];
    ApiCalculator               m_calc;
    uint32_t                    mNumUtps;
    API_INVECTOR_t              mInputVector;
    API_OUTVECTOR_t             mOutputVector;
    std::shared_ptr<UpsReader>  mUpsReaderSPtr;
    uint32_t                    mProbeType;
    char                        mUtpString[10000];
    uint32_t                    mStartUTPID;
    float                       mDefaultApex;
};

