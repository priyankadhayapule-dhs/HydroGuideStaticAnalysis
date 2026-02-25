#include <LVSegmentationModel.h>
#include <ThorUtils.h>

// All lv segmentation models have the same structure, so these may be hard coded here
static const char *OUTPUT_LAYER = "prelu_12";
//static const char *OUTPUT_TENSOR = "169";

static const size_t INPUT_SIZE = 128*128*5*sizeof(float);

static const size_t INPUT_STRIDES[] = {
    128*128*5*sizeof(float),
    128*5*sizeof(float),
    5*sizeof(float),
    1*sizeof(float)
};

static const size_t OUTPUT_SIZE = 128*128*4*sizeof(float);
static const size_t OUTPUT_STRIDES[] = {
    128*128*4*sizeof(float),
    128*4*sizeof(float),
    4*sizeof(float),
    1*sizeof(float)
};


ThorStatus LVSegmentationModel::init(void *dlc, size_t dlcSize)
{
    ThorStatus status;

    auto container = zdl::DlContainer::IDlContainer::open(static_cast<const uint8_t*>(dlc), dlcSize);
    if (!container)
    {
        LOGE("Failed to open DLC container data");
        return TER_AI_MODEL_LOAD; 
    }

    zdl::SNPE::SNPEBuilder builder(container.get());
    zdl::DlSystem::StringList sl(1);

    sl.append(OUTPUT_LAYER);

    mModel = builder.setOutputLayers(sl)
        .setRuntimeProcessor(GetSNPERuntime())
        .setCPUFallbackMode(true)
        .setDebugMode(false)
        .setPerformanceProfile(zdl::DlSystem::PerformanceProfile_t::HIGH_PERFORMANCE)
        .setUseUserSuppliedBuffers(true)
        .build();

    if (!mModel)
        RETURN_ERROR(TER_AI_MODEL_LOAD, "Failed to build SNPE instance");

    status = createUserBuffers();

    return status;
}

ThorStatus LVSegmentationModel::createUserBuffers()
{
    auto &ubf = zdl::SNPE::SNPEFactory::getUserBufferFactory();
    zdl::DlSystem::UserBufferEncodingFloat userBufferEncodingFloat;

    auto inputNames = mModel->getInputTensorNames();
    if (!inputNames)
        RETURN_ERROR(TER_AI_MODEL_LOAD, "Failed to get input tensor names");

    auto &inputStringList = *inputNames;
    if (inputStringList.size() != 1)
        RETURN_ERROR(TER_AI_MODEL_LOAD, "Unexpected size of input tensor list (expected 1, got %lu)", inputStringList.size());

    const char *inputName = inputStringList.at(0);
    auto inputStrides = zdl::DlSystem::TensorShape(INPUT_STRIDES, 4);
    mInputTensor = ubf.createUserBuffer(nullptr, INPUT_SIZE, inputStrides, &userBufferEncodingFloat);
    mInputMap.add(inputName, mInputTensor.get());


    auto outputNames = mModel->getOutputTensorNames();
    if (!outputNames)
        RETURN_ERROR(TER_AI_MODEL_LOAD, "Failed to get output tensor names");

    auto &outputStringList = *outputNames;
    if (outputStringList.size() != 1)
        RETURN_ERROR(TER_AI_MODEL_LOAD, "Unexpected size of output tensor list (expected 1, got %lu)", outputStringList.size());

    const char *outputName = outputStringList.at(0);
    auto outputStrides = zdl::DlSystem::TensorShape(OUTPUT_STRIDES, 4);
    mOutputTensor = ubf.createUserBuffer(nullptr, OUTPUT_SIZE, outputStrides, &userBufferEncodingFloat);
    mOutputMap.add(outputName, mOutputTensor.get());

    return THOR_OK;
}

ThorStatus LVSegmentationModel::segment(void *postscan, void *outputProbMap)
{
    if (!mInputTensor->setBufferAddress(postscan))
    {
        LOGE("Failed to set input buffer address to %p", postscan);
        return THOR_ERROR;
    }

    if (!mOutputTensor->setBufferAddress(outputProbMap))
    {
        LOGE("Failed to set output buffer address to %p", outputProbMap);
        return THOR_ERROR;
    }

    if (!mModel->execute(mInputMap, mOutputMap))
    {
        LOGE("Failed to execute model");
        return THOR_ERROR;
    }

    return THOR_OK;
}