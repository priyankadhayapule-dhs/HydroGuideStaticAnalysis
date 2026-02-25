//
// Copyright 2019 EchoNous Inc.
//
// Implementation of EFWorkflow (native side)

#pragma once
#include <unordered_map>
#include <vector>
#include <AIManager.h>
#include <geometry.h>

/*enum class CardiacView
{
	A2C,
	A4C,
	A5C,
	PLAX
};*/

struct CardiacFrames
{
	int esFrame;
	int edFrame;
};

struct MultiCycleFrames
{
	static const size_t MAX_CYCLES = 10;
	int esFrames[MAX_CYCLES];
	int edFrames[MAX_CYCLES];
	int numCycles;
};

struct CardiacSegmentation
{
	std::vector<vec2> coords;
	// apex line goes from base to apex
	vec2 base, apex;
	float length;
	std::vector<std::pair<vec2,vec2>> disks;
	float uncertainty;
};

struct CycleSegmentation {
    int edFrame;
    int esFrame;
    CardiacSegmentation *ed;
    CardiacSegmentation *es;
};

enum class ForeshortenedStatus {
    None,
    A4C_Foreshortened,
    A2C_Foreshortened
};

struct ViewStatistics
{
	float edVolume;
	float esVolume;
	float SV;
	float EF;
	float CO;
	float uncertainty;
	int numCycles;
	int selectedA4CEDFrames[3];
    int selectedA4CESFrames[3];
    int selectedA2CEDFrames[3];
    int selectedA2CESFrames[3];
};

struct EFStatistics
{
	float heartRate; // Not able to be edited, so has no original/edited version

	ViewStatistics originalA4cStats;
	ViewStatistics originalA2cStats;
	ViewStatistics originalBiplaneStats;

	bool hasEdited;

	ViewStatistics editedA4cStats;
	ViewStatistics editedA2cStats;
	ViewStatistics editedBiplaneStats;

    ForeshortenedStatus foreshortenedStatus;
};

struct EFNativeJNI;

class EFWorkflow
{
	// private member data
	AIManager *mManager;

	struct {
        MultiCycleFrames frames;
		std::unordered_map<int, CardiacSegmentation> segmentations;
	} mAIa2c, mAIa4c;

	struct {
		CardiacFrames frames;
		std::unordered_map<int, CardiacSegmentation> segmentations;
	} mUsera2c, mUsera4c;
	EFStatistics mStats;

	// Quality and subview history
	std::vector<int> mA4CQuality;
	std::vector<int> mA2CQuality;
	std::vector<int> mSubview;

	void (*mFrameIDCallback)(void*, EFWorkflow*, CardiacView);
	void (*mSegmentationCallback)(void*, EFWorkflow*, CardiacView, int);
	void (*mStatisticsCallback)(void*, EFWorkflow*);
	void (*mErrorCallback)(void*, EFWorkflow*, ThorStatus);
	void (*mAutocaptureStartCallback)(void*, EFWorkflow*);
	void (*mAutocaptureFailCallback)(void*, EFWorkflow*);
	void (*mAutocaptureSucceedCallback)(void*, EFWorkflow*);
	void (*mAutocaptureProgressCallback)(void*, EFWorkflow*, float);
	void (*mSmartCaptureProgressCallback)(void*, EFWorkflow*, float);

	float mFlipX;

    EFNativeJNI* mEFNativeJNI;

public:
	ThorStatus init(AIManager *manager, void *javaEnv, void *javaContext);

	// Reset all user edited values, reverting to only the AI results
	void resetEdit();

	// reset all internal state, including AI results
	void reset();

	// set listener callbacks
	void setFrameIDCallback(void (*cb)(void*, EFWorkflow*, CardiacView));
	void setSegmentationCallback(void (*cb)(void*,EFWorkflow*, CardiacView, int));
	void setStatisticsCallback(void (*cb)(void*,EFWorkflow*));
	void setErrorCallback(void (*cb)(void*, EFWorkflow*, ThorStatus));

	void setAutocaptureStartCallback(void (*cb)(void*, EFWorkflow*));
	void setAutocaptureFailCallback(void (*cb)(void*, EFWorkflow*));
	void setAutocaptureSucceedCallback(void (*cb)(void*, EFWorkflow*));
	void setAutocaptureProgressCallback(void (*cb)(void*, EFWorkflow*, float));

	void setSmartCaptureProgressCallback(void (*cb)(void*, EFWorkflow*, float));

	// Set heart rate (to be used when entering edit screen)
	void setHeartRate(float hr);
	void setFlip(float flipX, float flipY);
	float getFlipX() const;

	//void beginQualityMeasure(CardiacView view);
	//void endQualityMeasure();

	std::unique_ptr<AITask> createFrameIDTask(void*,CardiacView view);
	std::unique_ptr<AITask> createSegmentationTask(void*,CardiacView view, int frame);

	std::unique_ptr<AITask> createEFCalculationTask(void*);

	// Set AI determined frames (to be used when reloading information from PIMS)
	void setAIFrames(CardiacView view, const MultiCycleFrames& frames);
	// Get AI determined ED/ES frames
    const MultiCycleFrames& getAIFrames(CardiacView view);
	// Set AU determined segmentation for a given view/frame
	//  (to be used when reloading information from PIMS)
	void setAISegmentation(CardiacView view, int frame, CardiacSegmentation segmentation);
	// Get AI determined segmentation for a given view/frame
	CardiacSegmentation getAISegmentation(CardiacView view, int frame);

	// Get/set user override results
	void setUserFrames(CardiacView view, CardiacFrames frames);
	CardiacFrames getUserFrames(CardiacView view);
	void setUserSegmentation(CardiacView view, int frame, CardiacSegmentation segmentation);
	CardiacSegmentation getUserSegmentation(CardiacView view, int frame);

	EFStatistics getStatistics();

	// Functions for tasks (not for use by client code)
	ViewStatistics computeViewStats(int numCycles, CycleSegmentation *segs, float hr, CardiacView view, ThorStatus *error);
	ViewStatistics computeViewStatsBiplane(int numA4CCycles, int numA2CCycles, CycleSegmentation *a4c,
                                           CycleSegmentation *a2c, float hr, ThorStatus *error);
	ThorStatus computeStats();
	void notifyFrameIDComplete(void *user, CardiacView view);
	void notifySegmentationComplete(void *user, CardiacView view, int frame);
	void notifyStatisticsComplete(void *user);
	void notifyError(void *user, ThorStatus error);

	void notifyAutocaptureStart(void *user);
	void notifyAutocaptureFail(void *user);
	void notifyAutocaptureSucceed(void *user);
	void notifyAutocaptureProgress(void *user, float progress);

	void notifySmartCaptureProgress(void *user, float progress);

    EFNativeJNI* getNativeJNI();

	friend class FrameIDTask;
	friend class SegmentationTask;
	friend class EFCalculationTask;
public:
	// private functions
	EFWorkflow();
	~EFWorkflow() = default;

	// compute/recompute the disks for a given segmentation
	// after this function, the segmentation normal, d, length, and disks contain valid data
	void computeDisks(CardiacSegmentation &segmentation);
	float volumeSinglePlane(CardiacSegmentation &a4c, ThorStatus *error);
	float volumeBiplane(CardiacSegmentation &a4c, CardiacSegmentation &a2c, bool isED, ThorStatus *error);
};
