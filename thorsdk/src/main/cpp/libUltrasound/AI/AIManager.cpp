//
// Copyright 2019 EchoNous Inc.
//
#define ENABLE_ELAPSED_TIME
#define LOG_TAG "AIManager"

#include <ElapsedTime.h>
#include <UltrasoundManager.h>
#include <EFQualitySubviewTask.h>
#include <CardiacAnnotatorTask.h>
#include <AnatomyClassifierTask.h>
#include <FASTObjectDetectorTask.h>
#include <AutoPreset/AutoDepthGainPresetTask.h>
#include <CineViewer.h>
#include <VTICalculation.h>
#include <ostream>
#include <iostream>
#include <fstream>
#include <json/json.h>
#include <EFWorkflow.h>
#include <EFWorkflowTasks.h>
#include <Bladder/BladderSegmentationTask.h>
#include <AIManager.h>
AITask::AITask(AIManager *manager) : mManager(manager) {}

CineBuffer* AITask::getCineBuffer()
{
	return mManager->getCineBuffer();
}

static const size_t TASK_QUEUE_DEFAULT_MAX_ITEMS = 16;
AITaskQueue::AITaskQueue() : AITaskQueue(TASK_QUEUE_DEFAULT_MAX_ITEMS) {}
AITaskQueue::AITaskQueue(size_t maxItems) : mMaxItems(maxItems) {}

void AITaskQueue::push(std::unique_ptr<AITask> task)
{
	std::unique_lock<std::mutex> lock{mMutex};
	while (mTasks.size() >= mMaxItems)
		mTaskRemoved.wait(lock);

	mTasks.emplace_back(std::move(task));
	mTaskAdded.notify_one();
}

std::unique_ptr<AITask> AITaskQueue::pop()
{
	std::unique_lock<std::mutex> lock{mMutex};
	while (mTasks.empty())
		mTaskAdded.wait(lock);

	std::unique_ptr<AITask> task = std::move(mTasks.front());
	mTasks.pop_front();

	mTaskRemoved.notify_one();
	return task;
}

static void AIManagerWorker(AITaskQueue &queue)
{
	std::unique_ptr<AITask> task = queue.pop();
	while (task)
	{
		task->run();
		task = queue.pop();
	}
}

AIManager::AIManager(UltrasoundManager *manager) :
	mWorker(AIManagerWorker, std::ref(mQueue)),
	mShutdown(false),
	mManager(manager),
	mCinebuffer(&manager->getCineBuffer()),
    mEFWorkflow(std::unique_ptr<EFWorkflow>(new EFWorkflow())),
	mQualityTask(std::unique_ptr<EFQualitySubviewTask>(new EFQualitySubviewTask(*this))),
	mAnnotatorTask(std::unique_ptr<CardiacAnnotatorTask>(new CardiacAnnotatorTask(manager))),
	//mAnatomyTask(std::unique_ptr<AnatomyClassifierTask>(new AnatomyClassifierTask(*this, manager->getAutoControlManager())))
	mFASTTask(std::unique_ptr<FASTObjectDetectorTask>(new FASTObjectDetectorTask(manager))),
	mVTICalculation(std::unique_ptr<VTICalculation>(new VTICalculation())),
	mBladderSegmentationTask(std::unique_ptr<BladderSegmentationTask>(new BladderSegmentationTask(manager))),
    mAutoDepthGainPresetTask(std::unique_ptr<AutoDepthGainPresetTask>(new AutoDepthGainPresetTask(manager, *this, manager->getAutoControlManager()))),
	mCurrentFlipX(0.0f),
	mCurrentFlipY(0.0f),
    mSCInit(false)
{
	manager->getCineViewer().getImguiRenderer().addDrawable(mQualityTask.get());
	manager->getCineViewer().getImguiRenderer().addDrawable(mAnnotatorTask.get());
	manager->getCineViewer().getImguiRenderer().addDrawable(mFASTTask.get());
    manager->getCineViewer().getImguiRenderer().addDrawable(mBladderSegmentationTask.get());
}

AIManager::~AIManager() { shutdown(); }

ThorStatus AIManager::init(void *javaEnv, void *javaContext)
{
	LOGD("AI Manager Init");
	ELAPSED_FUNC();
	ThorStatus status;
	ELAPSED_BEGIN("mEFWorkflow->init(this)");
	status = mEFWorkflow->init(this, javaEnv, javaContext);
	ELAPSED_END();
	if (IS_THOR_ERROR(status)) {
		LOGE("Error initializing mEFWorkflow: %08x", status);
		return status;
	}

	ELAPSED_BEGIN("mVTICalculation->init(this)");
	status = mVTICalculation->init(this, javaEnv, javaContext);
	ELAPSED_END();
	if (IS_THOR_ERROR(status)) {
		LOGE("Error initializing mVTICalculation: %08x", status);
		return status;
	}

	ELAPSED_BEGIN("mQualityTask->init(mManager)");
	status = mQualityTask->init(mManager);
	ELAPSED_END();
	if (IS_THOR_ERROR(status)) {
		LOGE("Error initializing mQualityTask: %08x", status);
		return status;
	}
	ELAPSED_BEGIN("mQualityTask->start()");
	mQualityTask->start(javaEnv, javaContext);
	ELAPSED_END();

	ELAPSED_BEGIN("mAnnotatorTask->init()");
	status = mAnnotatorTask->init();
	ELAPSED_END();
	if (IS_THOR_ERROR(status)) {
		LOGE("Error initializing mAnnotatorTask: %08x", status);
		return status;
	}
	ELAPSED_BEGIN("mAnnotatorTask->start()");
	mAnnotatorTask->start(javaEnv, javaContext);
	ELAPSED_END();

	ELAPSED_BEGIN("mFASTTask->init()");
	status = mFASTTask->init();
	ELAPSED_END();
	if (IS_THOR_ERROR(status)) {
		LOGE("Error initializing mFASTTask: %08x", status);
		return status;
	}
	ELAPSED_BEGIN("mFASTTask->start()");
	mFASTTask->start(javaEnv, javaContext);
	ELAPSED_END();

	status = mAutoDepthGainPresetTask->init(javaEnv, javaContext);
	if (IS_THOR_ERROR(status)) {
		LOGE("Error initializing mAutoDepthGainPresetTask: %08x", status);
		return status;
	}
	mAutoDepthGainPresetTask->start();

    status = mBladderSegmentationTask->init();
    if (IS_THOR_ERROR(status)) {
        LOGE("Error initializing mBladderSegmentationTask: %08x", status);
        return status;
    }
    mBladderSegmentationTask->start(javaEnv, javaContext);
    setFlip(-1.0f, 1.0f);
    mCinebuffer->registerCallback(this);
    return THOR_OK;
}

void AIManager::setLanguage(const char *lang)
{
	mQualityTask->setLanguage(lang);
	mAnnotatorTask->setLanguage(lang);
	mFASTTask->setLanguage(lang);
}

CineBuffer* AIManager::getCineBuffer()
{
	return mCinebuffer;
}

ImguiRenderer& AIManager::getImguiRenderer()
{
	return mManager->getCineViewer().getImguiRenderer();
}

void AIManager::setFlip(float flipX, float flipY) {
	auto fltequal = [](float a, float b) { return std::abs(a-b) < 0.0001f; };
	if (fltequal(flipX, mCurrentFlipX) && fltequal(flipY, mCurrentFlipY)) {
		return;
	}
	mCurrentFlipX = flipX;
	mCurrentFlipY = flipY;

	mAnnotatorTask->setFlip(flipX, flipY);
	mQualityTask->setFlip(flipX, flipY);
	mFASTTask->setFlip(flipX, flipY);
	mEFWorkflow->setFlip(flipX, flipY);
	mAutoDepthGainPresetTask->setFlip(flipX, flipY);
    if (mSCInit)
    {
        // update flipX - adjust aspect ratio
        ScanConverterParams scParams = mCinebuffer->getParams().converterParams;
        float adjustedFlipX;
        if (abs(scParams.startRayRadian) > 0.01f ) {
            float scaleX = (AIM_IMG_WIDTH*0.5f)/(AIM_IMG_HEIGHT*sin(-scParams.startRayRadian));

            if (scParams.probeShape == PROBE_SHAPE_PHASED_ARRAY_FLATTOP)
                scaleX = scaleX * FLATTOP_ASPECT_ADJUST_FACTOR;

            adjustedFlipX = mCurrentFlipX * scaleX;
        } else {
            // LEXSA/linear probe, do not do any scale adjustment
            adjustedFlipX = mCurrentFlipX;
        }


        mBModeRenderer.setFlip(adjustedFlipX, mCurrentFlipY);
    }
}

void AIManager::shutdown()
{
	LOGD("AIManager shutting down");
	if (!mShutdown)
	{
		// Send a nullptr task to signal the worker to terminate
		// only one worker thread, so only one nullptr is needed
		mQueue.push(nullptr);
		mWorker.join();
		mShutdown = true;
	}
    // clean up OffScreenRenderSurface
    mOffSCreenRenderSurface.close();
    mOffSCreenRenderSurface.clearRenderList();
    mSCInit = false;
	LOGD("AIManager shutdown complete");
}

void AIManager::push(std::unique_ptr<AITask> task)
{
	mQueue.push(std::move(task));
}

EFQualitySubviewTask* AIManager::getEFQualityTask()
{
	return mQualityTask.get();
}

CardiacAnnotatorTask* AIManager::getCardiacAnnotatorTask()
{
	return mAnnotatorTask.get();
}

AutoDepthGainPresetTask* AIManager::getAutoDepthGainPresetTask()
{
	return mAutoDepthGainPresetTask.get();
}

FASTObjectDetectorTask* AIManager::getFASTModule()
{
	return mFASTTask.get();
}

EFWorkflow *AIManager::getEFWorkflow() {
    return mEFWorkflow.get();
}

VTICalculation* AIManager::getVTICalculation()
{
	return mVTICalculation.get();
}

BladderSegmentationTask* AIManager::getBladderSegmentationTask()
{
    return mBladderSegmentationTask.get();
}


void AIManager::setAutoPresetEnabled(bool isEnabled){
    mManager->getAutoControlManager().setAutoPresetEnabled(isEnabled);
    bool threadActive = mManager->getAutoControlManager().getActive();
    if(threadActive)
    {
        mAutoDepthGainPresetTask->start();
    }
    else
    {
        mAutoDepthGainPresetTask->stop();
    }
}
void EFErrorCallback(void *ptr, EFWorkflow *ef, ThorStatus status) {
    *reinterpret_cast<ThorStatus*>(ptr) = status;
}

/// Checks the return value of an expression. Context is optional, added to an error if present
/// If status is THOR_OK: returns Json null
/// Else, returns an object {"error": <error value>, "context": <context>}
Json::Value CheckStatus(ThorStatus status, const char *context=nullptr) {
    if (IS_THOR_ERROR(status)) {
        Json::Value error;
        error["error"] = static_cast<int>(status);
        if (context) {
            error["context"] = context;
        }
        return error;
    } else {
        return Json::Value();
    }
}

/// Finds the scan depth given scan converter params
float ScanDepth(const ScanConverterParams& params) {
	return params.numSamples * params.sampleSpacingMm + params.startSampleMm;
}

Json::Value ToJson(const std::vector<float> &values) {
    Json::Value json;
    for (float f : values) {
        json.append(f);
    }
    return json;
}
Json::Value ToJson(const std::vector<int> &values) {
    Json::Value json;
    for (float f : values) {
        json.append(f);
    }
    return json;
}

std::ostream& operator<<(std::ostream& out, const CardiacFrames &frames) {
	out << "[" << frames.edFrame << ", " << frames.esFrame << "]";
	return out;
}

ThorStatus AIManager::runEfVerificationOnCineBuffer(void* jniEnv, const char* inputFilea4c, const char* inputFileA2C, const char* outputFile)
{
    LOGD("[ EF VERIFY ] Running Verification on %s and %s", inputFilea4c, inputFileA2C);
	auto outputJson = RunEFWorkflow(inputFilea4c, inputFileA2C, mManager, jniEnv, outputFile);
	getEFWorkflow()->setFlip(-1.0f, 1.0f);
    std::string outPath = outputFile;
    outPath.push_back('/');
    outPath.append("ef_verification.json");
	std::ofstream outstream(outPath);
	outstream << outputJson;
	return THOR_OK;
}

/// Runs EF workflow on a pair of A4C/A2C files, returns <EFExam>
/// Return format: { "a4c": <View>, "a2c": <View>, "stats": <Stats>}
Json::Value AIManager::RunEFWorkflow(const char *a4c, const char *a2c, UltrasoundManager *usm, void *jenv, const char* outpath) {
	// reset current exam
	auto *ef = usm->getAIManager().getEFWorkflow();
	ef->reset();

	Json::Value efexam;
	efexam["a4c"] = RunEFView(CardiacView::A4C, a4c, usm, jenv, outpath);
	efexam["a2c"] = RunEFView(CardiacView::A2C, a2c, usm, jenv, outpath);
	efexam["stats"] = ComputeStats(usm);
	return efexam;
}

/// Runs segmentation on a single frame, returns <FrameSegmentation>
/// Return format: {
///     "coords_x": [x coords],
///     "coords_y": [y coords],
///     "disks_left_x": [x coords],
///     "disks_left_y": [y coords],
///     "disks_right_x": [x coords],
///     "disks_right_y": [y coords],
///     "uncertainty": <uncertainty>
/// }
Json::Value AIManager::RunFrameSegmentation(CardiacView view, uint32_t framenum, bool isEDFrame, UltrasoundManager *usm, void *jenv, const char* outpath) {
    auto &aiManager = usm->getAIManager();
    auto *ef = usm->getAIManager().getEFWorkflow();
    SegmentationTask task(&aiManager, nullptr, ef, view, framenum, isEDFrame, false);
    auto status = CheckStatus(task.runImmediate(jenv, outpath), "SegmentationTask::run");

    const CardiacSegmentation &seg = ef->getAISegmentation(view, static_cast<int>(framenum));
    Json::Value frameseg;
    Json::Value &xcoords = frameseg["coords_x"];
    Json::Value &ycoords = frameseg["coords_y"];
    for (vec2 coord : seg.coords) {
        xcoords.append(coord.x);
        ycoords.append(coord.y);
    }

    Json::Value &disks_left_x = frameseg["disks_left_x"];
    Json::Value &disks_left_y = frameseg["disks_left_y"];
    Json::Value &disks_right_x = frameseg["disks_right_x"];
    Json::Value &disks_right_y = frameseg["disks_right_y"];
    for (std::pair<vec2, vec2> disk : seg.disks) {
        disks_left_x.append(disk.first.x);
        disks_left_y.append(disk.first.y);
        disks_right_x.append(disk.second.x);
        disks_right_y.append(disk.second.y);
    }
    frameseg["uncertainty"] = seg.uncertainty;
    frameseg["length"] = seg.length;
    return frameseg;
}

/// Runs segmentation on a clip, returns <ClipSegmentation>
/// Return format: {
///     "<frame number>": <FrameSegmentation>
/// }
Json::Value AIManager::RunClipSegmentation(CardiacView view, UltrasoundManager *usm, void *jenv, const char* outpath) {
	auto *ef = usm->getAIManager().getEFWorkflow();
	const MultiCycleFrames &frames = ef->getAIFrames(view);
	LOGD("Running clip segmentation, frames.numCycles = %d", frames.numCycles);

	Json::Value clipseg;
	Json::Value &edsegs = clipseg["ed"];
	Json::Value &essegs = clipseg["es"];
	for (int i=0; i != frames.numCycles; ++i) {
		edsegs.append(RunFrameSegmentation(view, frames.edFrames[i], true, usm, jenv, outpath));
		essegs.append(RunFrameSegmentation(view, frames.esFrames[i], false, usm, jenv, outpath));
	}
	return clipseg;
}

Json::Value AIManager::RunPhaseDetection(CardiacView view, UltrasoundManager *usm, void *jenv, const char* outpath) {
	auto &aiManager = usm->getAIManager();
	auto *ef = usm->getAIManager().getEFWorkflow();
	FrameIDTask task(&aiManager, nullptr, ef, view);
	//task.clipName = mCurrentClipName;
	FILE* rawOut = fopen("test", "wb");
	auto status = CheckStatus(task.runImmediate(rawOut, jenv, outpath), "FrameIDTask::run");
	if(rawOut != nullptr)
		fclose(rawOut);
	if (status) { return status; }
	Json::Value phasedetection;
	LOGD("Filling field: area_estimate"); phasedetection["area_estimate"] = ToJson(task.getPhaseAreas());
	LOGD("Filling field: T"); phasedetection["T"] = task.getPeriod();
	LOGD("Filling field: ed_frames_mapped"); phasedetection["ed_frames_mapped"] = ToJson(task.getEDFramesMapped());
	LOGD("Filling field: es_frames_mapped"); phasedetection["es_frames_mapped"] = ToJson(task.getESFramesMapped());
	LOGD("Filling field: estimated_efs"); phasedetection["estimated_efs"] = ToJson(task.getEstimatedEFs());
	LOGD("Filling field: ed_uncertainties"); phasedetection["ed_uncertainties"] = ToJson(task.getEDUncertainties());
	LOGD("Filling field: es_uncertainties"); phasedetection["es_uncertainties"] = ToJson(task.getESUncertainties());
	LOGD("Filling field: ed_lengths"); phasedetection["ed_lengths"] = ToJson(task.getEDLengths());
	LOGD("Filling field: es_lengths"); phasedetection["es_lengths"] = ToJson(task.getESLengths());
	const MultiCycleFrames &frames = task.getFrames();
	LOGD("frames = length %d values {(%d,%d), (%d,%d), (%d,%d), (%d, %d), ...}",
		 frames.numCycles,
		 frames.edFrames[0], frames.esFrames[0],
		 frames.edFrames[1], frames.esFrames[1],
		 frames.edFrames[2], frames.esFrames[2],
		 frames.edFrames[3], frames.esFrames[3]);
	Json::Value &edframes = phasedetection["selected_ed_frames"];
	Json::Value &esframes = phasedetection["selected_es_frames"];
	for (int i=0; i != frames.numCycles; ++i) {
		edframes.append(frames.edFrames[i]);
		esframes.append(frames.esFrames[i]);
	}
	LOGD("Setting ai frames on ef = %p", ef);
	ef->setAIFrames(view, frames);
	return phasedetection;
}

/// Runs EF Workflow for a given view, returns <View>
/// Return format: {
///     "clip": <clip name>,
///     "imaging_depth": <depth>,
///     "phase": <PhaseDetection>,
///     "segmentation": <ClipSegmentation>
/// }
Json::Value AIManager::RunEFView(CardiacView view, const char *clip, UltrasoundManager *usm, void *jenv, const char* outpath) {;
	if(view == CardiacView::A2C) // assume that we want to open the clip
	{
		auto status = CheckStatus(usm->getCinePlayer().openRawFile(clip), "Cineplayer::openRawFile");
		if (status) { return status; }
	}

	float depth = ScanDepth(usm->getCineBuffer().getParams().converterParams);

	Json::Value efview;
	efview["clip"] = clip;
	efview["imaging_depth"] = depth;
	efview["phase"] = RunPhaseDetection(view, usm, jenv, outpath);
	efview["segmentation"] = RunClipSegmentation(view, usm, jenv, outpath);
	return efview;
}

/// Computes final EF stats, returns <Stats>
/// Return format: {
///    "hr_biplane": 0.000000,
///    "ef_biplane": 54.604160,
///    "edv_biplane": 64.933273,
///    "esv_biplane": 29.477007,
///    "sv_biplane": 35.456268,
///    "co_biplane": 0.000000,
///    "hr_a4c": 0.000000,
///    "ef_a4c": 52.339203,
///    "edv_a4c": 50.964821,
///    "esv_a4c": 24.290241,
///    "sv_a4c": 26.674580,
///    "co_a4c": 0.000000,
///    "hr_a2c": 0.000000,
///    "ef_a2c": 57.009811,
///    "edv_a2c": 83.245880,
///    "esv_a2c": 35.787560,
///    "sv_a2c": 47.458321,
///    "co_a2c": 0.000000
/// }
Json::Value AIManager::ComputeStats(UltrasoundManager *usm) {
	auto *ef = usm->getAIManager().getEFWorkflow();
	ThorStatus statsOutput = ef->computeStats();
	EFStatistics stats = ef->getStatistics();

	Json::Value jstats;
    std::string error = "THOR_OK";
    // Modify biplane stats if we received uncertainty or foreshortening errors
    if(statsOutput == TER_AI_LV_UNCERTAINTY_ERROR || statsOutput == TER_AI_A4C_FORESHORTENED || statsOutput == TER_AI_A2C_FORESHORTENED)
    {
        stats.originalBiplaneStats.EF = -1.f;
        stats.originalBiplaneStats.edVolume = -1.f;
        stats.originalBiplaneStats.esVolume = -1.f;
        stats.originalBiplaneStats.CO = -1.f;
        stats.originalBiplaneStats.SV = -1.f;
        if(statsOutput == TER_AI_LV_UNCERTAINTY_ERROR)
        {
            error = "TER_AI_LV_UNCERTAINTY_ERROR";
        }
        else if(statsOutput == TER_AI_A4C_FORESHORTENED){
            error = "TER_AI_A4C_FORESHORTENED";
        }
        else {
            error = "TER_AI_A2C_FORESHORTENED";
        }

    }
	jstats["hr_biplane"]  = stats.heartRate;
	jstats["ef_biplane"]  = stats.originalBiplaneStats.EF;
	jstats["edv_biplane"] = stats.originalBiplaneStats.edVolume;
	jstats["esv_biplane"] = stats.originalBiplaneStats.esVolume;
	jstats["sv_biplane"]  = stats.originalBiplaneStats.SV;
	jstats["co_biplane"]  = stats.originalBiplaneStats.CO;
	jstats["hr_a4c"]  = stats.heartRate;
	jstats["ef_a4c"]  = stats.originalA4cStats.EF;
	jstats["edv_a4c"] = stats.originalA4cStats.edVolume;
	jstats["esv_a4c"] = stats.originalA4cStats.esVolume;
	jstats["sv_a4c"]  = stats.originalA4cStats.SV;
	jstats["co_a4c"]  = stats.originalA4cStats.CO;
	jstats["hr_a2c"]  = stats.heartRate;
	jstats["ef_a2c"]  = stats.originalA2cStats.EF;
	jstats["edv_a2c"] = stats.originalA2cStats.edVolume;
	jstats["esv_a2c"] = stats.originalA2cStats.esVolume;
	jstats["sv_a2c"]  = stats.originalA2cStats.SV;
	jstats["co_a2c"]  = stats.originalA2cStats.CO;

	return jstats;
}

void AIManager::runScanConverter(uint32_t frameNum)
{
    std::unique_lock<std::mutex> lock(mSCMutex);
    uint32_t frameIdx = frameNum%AIM_BUFFER_SIZE;
    uint8_t* outPtr = &postSCBuffer[frameIdx * AIM_FRAME_SIZE];
    // set Frame
    mBModeRenderer.setFrame(mCinebuffer->getFrame(frameNum, DAU_DATA_TYPE_B));

    // run converter

    mOffSCreenRenderSurface.takeScreenShotToGrayPtrUD(outPtr);

    // update store frameNum
    postSCBufferFrameNum[frameIdx] = frameNum;
}

void AIManager::getScanConvertedFrame(uint32_t frameNum, uint32_t rows, uint32_t cols, uint8_t* dest)
{
    // Todo: ensure setParams is called in the correct time/location?
    if (!mSCInit) {
        setParams();
    }
    uint32_t frameIdx = frameNum%AIM_BUFFER_SIZE;
    if ((postSCBufferFrameNum[frameIdx] != frameNum) && mSCInit) {
        runScanConverter(frameNum);
    }
    // Create cv::Mat referencing data in postSCBuffer
    cv::Mat original(AIM_IMG_HEIGHT, AIM_IMG_WIDTH, CV_8UC1,
                     &postSCBuffer[frameIdx*AIM_FRAME_SIZE]);

    // Create cv::Mat where image data is backed by dest
    cv::Mat destMat(rows, cols, CV_8UC1, dest);
    cv::resize(original, destMat, cv::Size(rows, cols));
}
void AIManager::setParams(CineBuffer::CineParams& params){
    mSCInit = false;
}
void AIManager::setParams()
{
    if (!mSCInit && !mOffSCreenRenderSurface.isOpen()) {
        initSC();
    }
    else
    {
        mOffSCreenRenderSurface.close();
        mOffSCreenRenderSurface.clearRenderList();
        initSC();
    }

    // TODO: re-init when SC - numSamples, Rays change

    ScanConverterParams scParams = mCinebuffer->getParams().converterParams;

    // set params
    mBModeRenderer.setParams(scParams);

}

void AIManager::initSC()
{
    // prepare OffScreen Scan Converter

    // TODO update
    float r = 1.0f;
    float g = 1.0f;
    float b = 1.0f;

    ScanConverterParams scParams = mCinebuffer->getParams().converterParams;

    mBModeRenderer.setParams(scParams);
    mBModeRenderer.setTintAdjustment(r, g, b);
    mOffSCreenRenderSurface.addRenderer(&mBModeRenderer, 0.0f, 0.0f, 100.0f, 100.0f);

    mOffSCreenRenderSurface.open(AIM_IMG_WIDTH, AIM_IMG_HEIGHT);

    // update flipX - adjust aspect ratio
    float adjustedFlipX;
    if (abs(scParams.startRayRadian) > 0.01f ) {
        float scaleX = (AIM_IMG_WIDTH*0.5f)/(AIM_IMG_HEIGHT*sin(-scParams.startRayRadian));

        if (scParams.probeShape == PROBE_SHAPE_PHASED_ARRAY_FLATTOP)
            scaleX = scaleX * FLATTOP_ASPECT_ADJUST_FACTOR;

        adjustedFlipX = mCurrentFlipX * scaleX;
    } else {
        // LEXSA/linear probe, do not do any scale adjustment
        adjustedFlipX = mCurrentFlipX;
    }


    mBModeRenderer.setFlip(adjustedFlipX, mCurrentFlipY);
    mSCInit = true;
}