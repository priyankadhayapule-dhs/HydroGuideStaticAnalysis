#define LOG_TAG "PhaseDetection"
#include <PhaseDetection.h>
#include <CineBuffer.h>
#include <ScanConverterCl.h>
#include <EFNativeJNI.h>
namespace echonous
{
namespace phase
{

static ScanConverterCL *gScanConverter = nullptr;

ThorStatus ScanConvert(CineBuffer *cine,
                       cv::Size size,
                       std::vector<float> &buffer,
                       float flipX,
                       std::vector<cv::Mat> *frames)
{
    if (nullptr == gScanConverter) {
        gScanConverter = new ScanConverterCL(ScanConverterCL::OutputType::FLOAT_UNSIGNED_SCALE_255);
    }
    gScanConverter->setConversionParameters(
        size.width,
        size.height,
        flipX,
        1.0f,
        ScanConverterCL::ScaleMode::NON_UNIFORM_SCALE_TO_FIT,
        cine->getParams().converterParams);

    // find CineBuffer bounds
    auto startFrame = cine->getMinFrameNum();
    auto endFrame = cine->getMaxFrameNum()+1; // one past the end frame
    auto numFrames = endFrame-startFrame;

    // Resize output buffer (and frames vector)
    buffer.resize(size.width * size.height * numFrames);
    if (frames)
        frames->resize(numFrames);

    // Run scan converter and optionally create cv::Mat instances
    for (int n=0; n < numFrames; ++n)
    {
        uint8_t *raw = cine->getFrame(n+startFrame, DAU_DATA_TYPE_B);
        auto err = gScanConverter->runCLTex(raw, &buffer[n*size.width*size.height]);
        if (err != THOR_OK)
            RETURN_ERROR(err, "Failed to scan convert frame %d", n);

        if (frames)
            (*frames)[n] = cv::Mat(size.width, size.height, CV_32FC1, &buffer[n*size.width*size.height]);
    }

    return THOR_OK;
}

ThorStatus RunModel(const std::vector<cv::Mat> &frames,
                    std::vector<float> &output, EFNativeJNI* jni)
{
    output.resize(frames.size());
    for (unsigned int i=0; i < frames.size(); ++i)
    {
        const float *input_i = frames[i].ptr<float>();
        memcpy(jni->phaseDetectionImageBufferNative.data(), input_i, sizeof(float)*128*128);
        jfloat outputJni = jni->jenv->CallFloatMethod(jni->instance, jni->runPhaseDetection, jni->phaseDetectionImageBuffer);
        //float *output_i = &output[i];
        auto err = THOR_OK;
        output[i] = outputJni;
        if (IS_THOR_ERROR(err))
        {
            LOGE("Error running model: %08x", err);
            return err;
        }
    }
    return THOR_OK;
}

    ThorStatus RunModel(const std::vector<cv::Mat> &frames,
                        std::vector<float> &output, EFNativeJNI* jni, void *jenv)
    {
        output.resize(frames.size());
        JNIEnv* jnienv = static_cast<JNIEnv*>(jenv);
        for (unsigned int i=0; i < frames.size(); ++i)
        {
            const float *input_i = frames[i].ptr<float>();
            memcpy(jni->phaseDetectionImageBufferNative.data(), input_i, sizeof(float)*128*128);
            jfloat outputJni = jnienv->CallFloatMethod(jni->instance, jni->runPhaseDetection, jni->phaseDetectionImageBuffer);
            float *output_i = &output[i];
            auto err = THOR_OK;
            output[i] = outputJni;
            if (IS_THOR_ERROR(err))
            {
                LOGE("Error running model: %08x", err);
                return err;
            }
        }
        return THOR_OK;
    }

ThorStatus MinMax(const std::vector<float> &input, int window, std::vector<int> &output)
{
    output.clear();
    output.resize(input.size());
    int mid = window/2;

    auto begin = input.begin();

    // first and last window/2 values should be 0
    for (int i=0; i < mid; ++i)
    {
        output[i] = 0;
        output[output.size()-i-1] = 0;
    }

    for (int i=0; i < output.size()-window; ++i)
    {
        auto windowStart = begin+i;
        auto minmax = std::minmax_element(windowStart, windowStart+window);
        output[minmax.first-begin] -= 1;
        output[minmax.second-begin] += 1;
    }
    return THOR_OK;
}



}
}