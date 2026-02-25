//
// Copyright 2019 EchoNous Inc.
//
#pragma once

#include <deque>
#include <memory>
#include <mutex>
#include <thread>
#include <vector>
#include <condition_variable>
#include <unordered_map>
#include <CineBuffer.h>
#include <ScanConverterCl.h>
#include <geometry.h>
#include <ImguiRenderer.h>
#include <OffScreenRenderSurface.h>
#include <BModeRenderer.h>
class UltrasoundManager;
class AIManager;
class EFWorkflow;
class FASTObjectDetectorTask;
class VTICalculation;
class AutoDepthGainPresetTask;
class CardiacSegmentation;
class BladderSegmentationTask;
struct ViewStatistics;
struct EFStatistics;
enum class CardiacView
{
	A2C,
	A4C,
	A5C,
	PLAX
};
// Generic AI task meant to be run async (in a single AI thread)
class AITask
{
public:
	AITask(AIManager *manager);
	virtual ~AITask() = default;
	virtual void run() = 0;

protected:
	// useful task functions
	CineBuffer* getCineBuffer();
	// The manager for this task
	AIManager *mManager;
};

class AITaskQueue
{
	// queue of task items
	std::deque<std::unique_ptr<AITask>> mTasks;
	std::mutex mMutex;
	std::condition_variable mTaskAdded, mTaskRemoved;
	// maximum length of queue
	size_t mMaxItems;
public:
	AITaskQueue();
	AITaskQueue(size_t maxItems);

	// push a task, and block if full
	void push(std::unique_ptr<AITask> task);

	// pop a task off the queue, blocking if no such task
	std::unique_ptr<AITask> pop();
};

class EFQualitySubviewTask;
class CardiacAnnotatorTask;
class AutoDepthGainPresetTask;

class AIManager : public CineBuffer::CineCallback
{
	AITaskQueue mQueue;
	std::thread mWorker;
	bool mShutdown;

	UltrasoundManager *mManager;
	CineBuffer *mCinebuffer;
	std::unique_ptr<EFWorkflow> mEFWorkflow;
	std::unique_ptr<EFQualitySubviewTask> mQualityTask;
	std::unique_ptr<CardiacAnnotatorTask> mAnnotatorTask;
	std::unique_ptr<AutoDepthGainPresetTask> mAutoDepthGainPresetTask;
	std::unique_ptr<FASTObjectDetectorTask> mFASTTask;
	std::unique_ptr<VTICalculation> mVTICalculation;
    std::unique_ptr<BladderSegmentationTask> mBladderSegmentationTask;

	float mCurrentFlipX, mCurrentFlipY;

public:
	AIManager(UltrasoundManager *manager);
	~AIManager();
	ThorStatus init(void *javaEnv, void *javaContext);
	void setLanguage(const char *lang);

	CineBuffer* getCineBuffer();
	ImguiRenderer& getImguiRenderer();

	void setFlip(float flipX, float flipY);

	// Shut down the AI Manager, terminating any background thread(s)
	void shutdown();

	// push a task to the manager, to be run async
	void push(std::unique_ptr<AITask> task);

	EFWorkflow* getEFWorkflow();
	VTICalculation* getVTICalculation();

	EFQualitySubviewTask* getEFQualityTask();
	CardiacAnnotatorTask* getCardiacAnnotatorTask();
	AutoDepthGainPresetTask* getAutoDepthGainPresetTask();
	FASTObjectDetectorTask* getFASTModule();
	BladderSegmentationTask* getBladderSegmentationTask();
    void setAutoPresetEnabled(bool isEnabled);
    /// Verification
	ThorStatus runEfVerificationOnCineBuffer(void* jniEnv, const char* inputFilea4c, const char* inputFileA2C, const char* outputFile);

	// TESTING
	/// Runs EF workflow on a pair of A4C/A2C files, returns <EFExam>
	/// Return format: { "a4c": <View>, "a2c": <View>, "stats": <Stats>}
	Json::Value RunEFWorkflow(const char *a4c, const char *a2c, UltrasoundManager *usm, void *jenv, const char* outpath);

	/// Runs EF Workflow for a given view, returns <View>
	/// Return format: {
	///     "clip": <clip name>,
	///     "imaging_depth": <depth>,
	///     "phase": <PhaseDetection>,
	///     "segmentation": <ClipSegmentation>
	/// }
	Json::Value RunEFView(CardiacView view, const char *clip, UltrasoundManager *usm, void *jenv, const char* outpath);

	/// Runs Phase detection on a clip, return <PhaseDetection>
	/// Return format: {
	///     "area_estimate": [<areas>],
	///     "T": <period>,
	///     "ed_frames_mapped": [<frame numbers>],
	///     "es_frames_mapped": [<frame numbers>],
	///     "estimated_efs": [<efs>],
	///     "uncertainties": [<uncertainties>],
	///     "selected_ed_frames": [<frame numbers>],
	///     "selected_es_frames": [<frame numbers>]
	/// }
	Json::Value RunPhaseDetection(CardiacView view, UltrasoundManager *usm, void *jenv, const char* outpath);


	/// Runs segmentation on a clip, returns <ClipSegmentation>
	/// Return format: {
	///     "<frame number>": <FrameSegmentation>
	/// }
	Json::Value RunClipSegmentation(CardiacView view, UltrasoundManager *usm, void *jenv, const char* outpath);

	/// Runs segmentation on a single frame, returns <FrameSegmentation>
	/// Return format: {
	///     "type": "ED" | "ES",
	///     "coords_x": [x coords],
	///     "coords_y": [y coords],
	///     "disks_left_x": [x coords],
	///     "disks_left_y": [y coords],
	///     "disks_right_x": [x coords],
	///     "disks_right_y": [y coords],
	///     "uncertainty": <uncertainty>,
	///     "legnth": <length in mm>
	/// }
	Json::Value RunFrameSegmentation(CardiacView view, uint32_t framenum, bool isEDFrame, UltrasoundManager *usm, void *jenv, const char* outpath);

	/// Computes final EF stats, returns <Stats>
	/// Return format: {
	///    "hr_biplane": [value],
	///    "ef_biplane": [value],
	///    "edv_biplane": [value],
	///    "esv_biplane": [value],
	///    "sv_biplane": [value],
	///    "co_biplane": [value],
	///    "hr_a4c": [value],
	///    "ef_a4c": [value],
	///    "edv_a4c": [value],
	///    "esv_a4c": [value],
	///    "sv_a4c": [value],
	///    "co_a4c": [value],
	///    "hr_a2c": [value],
	///    "ef_a2c": [value],
	///    "edv_a2c": [value],
	///    "esv_a2c": [value],
	///    "sv_a2c": [value],
	///    "co_a2c": [value]
	/// }
	Json::Value ComputeStats(UltrasoundManager *usm);

#define THOR_TRY(expr) ThorAssertOk(expr, #expr, __FILE__, __LINE__)
	void ThorAssertOk(ThorStatus result, const char *expr, const char *file, int line)
	{
		if (IS_THOR_ERROR(result))
		{
			fprintf(stderr, "Thor Error code 0x%08x in \"%s\" (%s:%d)\n", static_cast<uint32_t>(result), expr, file, line);
			fflush(stdout);
			std::abort();
		}
	}
#define AIM_IMG_WIDTH 224
#define AIM_IMG_HEIGHT 224
#define AIM_BUFFER_SIZE 5
#define AIM_FRAME_SIZE (AIM_IMG_WIDTH*AIM_IMG_HEIGHT)
    // temp buffer
    uint8_t             postSCBuffer[AIM_BUFFER_SIZE * AIM_IMG_WIDTH * AIM_IMG_HEIGHT];
    uint32_t            postSCBufferFrameNum[AIM_BUFFER_SIZE];
    // offscreen renderer
    std::mutex                      mSCMutex;
    OffScreenRenderSurface          mOffSCreenRenderSurface;
    BModeRenderer                   mBModeRenderer;
    bool                            mSCInit;
    bool                            mInit;
    void runScanConverter(uint32_t frameNum);
    void getScanConvertedFrame(uint32_t frameNum, uint32_t rows, uint32_t cols, uint8_t* dest);
    void setParams();
    virtual void setParams(CineBuffer::CineParams& params) override;
    void initSC();
};
