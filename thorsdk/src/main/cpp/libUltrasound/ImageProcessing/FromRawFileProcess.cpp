#define LOG_TAG "FromRawFileProcess"
#include <ThorUtils.h>
#include <FromRawFileProcess.h>
#include <ThorRawReader.h>
#include <ThorFileProcessor.h>
#include <CineBuffer.h>
// For open()/close()
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

FromRawFileProcess::FromRawFileProcess(const std::shared_ptr<UpsReader> &ups, const char *path) :
    ImageProcess(ups),
    mPath(path),
    mFrameNum(0),
    mMessageReader(nullptr),
    mBMode()
{
}

ThorStatus FromRawFileProcess::init()
{
    mFrameNum = 0;
    
    // First try reading in old format:
    // old style format will read BMode frames directly into mRawFrames
    {
        ThorRawReader reader;
        ThorRawDataFile::Metadata metadata;
        ThorFileProcessor processor;
        ThorStatus status = reader.open(mPath.c_str(), metadata);
        
        
        if (!IS_THOR_ERROR(status)) {
            LOGD("Reading old style raw file %s", mPath.c_str());
            uint8_t *dataPtr = reader.getDataPtr();
            processor.processModeHeader(dataPtr, metadata.dataTypes, metadata.frameCount, reader.getVersion());
            
            size_t frameSize = MAX_B_FRAME_SIZE + sizeof(CineBuffer::CineFrameHeader);
            mRawFrames.resize(metadata.frameCount * frameSize);

            processor.getData(ThorRawDataFile::DataType::BMode, mRawFrames.data());
            
            if (IS_THOR_ERROR(status) && status != TER_MISC_NOT_IMPLEMENTED) {
                LOGE("[ THOR INPUT ] Unexpected error when reading thor file: 0x%08x", status);
                return status;
            }
            return THOR_OK;
        }
    }
    
    // Try opening file: if it exists then use it, otherwise do nothing
    kj::AutoCloseFd fd = kj::AutoCloseFd(::open(mPath.c_str(), O_RDONLY));
    if (fd == nullptr) {
        LOGD("[ THOR_INPUT ] Raw file %s not found, mock disabled", mPath.c_str());
        return THOR_OK;
    }
    try {
        capnp::ReaderOptions options;
        options.traversalLimitInWords =
                1024 * 1024 * 128; // expand traversal limit to allow up to 1GiB files
        mMessageReader = std::make_unique<capnp::StreamFdMessageReader>(std::move(fd), options);
        LOGD("Mocking DAU inputs from file %s", mPath.c_str());
        mBMode = mMessageReader->getRoot<echonous::capnp::ThorClip>().getBmode();
    } catch (std::exception &e) {
        LOGE("Exception while trying to read thor file in Capnp format: %s", e.what());
        terminate();
    }
    return THOR_OK;
}
ThorStatus FromRawFileProcess::setProcessParams(ProcessParams &params) {
    // Reset frameNum, as setting the processor params usually indicates resetting the DAU stream
    mFrameNum = 0;
    return THOR_OK;
}
ThorStatus FromRawFileProcess::process(uint8_t *inputPtr, uint8_t *outputPtr) {
    if (!mRawFrames.empty()) {
        size_t frameSize = MAX_B_FRAME_SIZE + sizeof(CineBuffer::CineFrameHeader);

        // We have read raw frames from an old style thor file, use that data
        std::copy_n(&mRawFrames[mFrameNum*frameSize + sizeof(CineBuffer::CineFrameHeader)], MAX_B_FRAME_SIZE, outputPtr);
        ++mFrameNum;
        if (mFrameNum*frameSize >= mRawFrames.size()) {
            mFrameNum = 0;
        }
        return THOR_OK;
    }
    if (mMessageReader == nullptr) {
        // Nothing to do, copy input to output
        std::copy_n(inputPtr, MAX_B_FRAME_SIZE, outputPtr);
        return THOR_OK;
    }
    try {
        const auto& frames = mBMode.getRawFrames();
        const auto& rawData = frames[mFrameNum].getFrame();
        std::copy(rawData.begin(), rawData.end(), outputPtr);
        ++mFrameNum;
        if (mFrameNum == frames.size()) {
            mFrameNum = 0;
        }
    } catch (std::exception &e)
    {
        LOGE("Exception while trying to read thor file in Capnp format: %s", e.what());
        terminate();
        return THOR_OK;
    }
    return THOR_OK;
}
void FromRawFileProcess::terminate() {
    mBMode = echonous::capnp::CineData::Reader();
    mMessageReader = nullptr;
    mRawFrames.clear();
}
