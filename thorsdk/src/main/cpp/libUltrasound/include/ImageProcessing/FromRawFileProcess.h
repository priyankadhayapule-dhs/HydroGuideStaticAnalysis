#pragma once
#ifndef FROM_RAW_FILE_PROCESS_H
#define FROM_RAW_FILE_PROCESS_H
#include <memory>
#include <ImageProcess.h>
#include <capnp/serialize.h>
#include <ThorCapnpTypes.capnp.h>

class FromRawFileProcess : public ImageProcess
{
public:
    FromRawFileProcess(const std::shared_ptr<UpsReader> &ups, const char *path);

    static const char *name() { return "FromRawFileProcess"; }
    virtual ThorStatus init() override;
    virtual ThorStatus setProcessParams(ProcessParams &params) override;
    virtual ThorStatus process(uint8_t *inputPtr, uint8_t *outputPtr) override;
    virtual void terminate() override;
private:
    std::string mPath;
    uint32_t mFrameNum;
    std::vector<uint8_t> mRawFrames;
    std::unique_ptr<capnp::StreamFdMessageReader> mMessageReader;
    echonous::capnp::CineData::Reader mBMode;
};
#endif
