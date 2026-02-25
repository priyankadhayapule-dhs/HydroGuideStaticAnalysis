//
// Copyright 2020 EchoNous Inc.
//
#define LOG_TAG "EFQualitySubviewConfig"
#include <EFQualitySubviewConfig.h>
#include <ThorUtils.h>
#include <algorithm>

EFQualitySubviewConfig::EFQualitySubviewConfig()
{
    reset();
}

void EFQualitySubviewConfig::loadFrom(const char *path)
{

}

void EFQualitySubviewConfig::saveTo(const char *path)
{

}

void EFQualitySubviewConfig::reset()
{
    LOGD("IQSubviewConfig::reset()");
    a4cQualityCapTau = 0.25f;
    a2cQualityCapTau = 0.25f;

    a4cQualityTau[0] = 0.0f;
    a4cQualityTau[1] = 0.0f;
    a4cQualityTau[2] = 0.35f;
    a4cQualityTau[3] = 0.25f;
    a4cQualityTau[4] = 0.15f;
    
    a2cQualityTau[0] = 0.0f;
    a2cQualityTau[1] = 0.0f;
    a2cQualityTau[2] = 0.3f;
    a2cQualityTau[3] = 0.25f;
    a2cQualityTau[4] = 0.20f;

    subviewTau[0] =  0.0f;
    subviewTau[1] =  0.0f;
    subviewTau[2] =  0.0f;
    subviewTau[3] =  0.0f;
    subviewTau[4] =  0.0f;
    subviewTau[5] =  0.0f;
    subviewTau[6] =  0.0f;
    subviewTau[7] =  0.3f;
    subviewTau[8] =  0.0f;
    subviewTau[9] =  0.0f;
    subviewTau[10] = 0.0f;
    subviewTau[11] = 0.3f;
    subviewTau[12] = 0.0f;
    subviewTau[13] = 0.0f;
    subviewTau[14] = 0.0f;
    subviewTau[15] = 0.0f;
    subviewTau[16] = 0.0f;
    subviewTau[17] = 0.0f;
    subviewTau[18] = 0.0f;
    subviewTau[19] = 0.0f;
}