//
// Copyright 2019 EchoNous Inc.
//
// Implementation of EFWorkflow (native side)

#define LOG_TAG "EFWorkflowNative"


#include <ThorUtils.h>
#include <EFWorkflow.h>
#include <CatmullRom.h>
#include <EFNativeJNI.h>

const size_t MultiCycleFrames::MAX_CYCLES;
const float MAX_UNCERTAINTY_A4C_ED = 0.1131f;
const float MAX_UNCERTAINTY_A4C_ES = 0.226f;
const float MAX_UNCERTAINTY_A2C_ED = 0.183f;
const float MAX_UNCERTAINTY_A2C_ES = 0.305f;
const float BIPLANE_LENGTH_MAX_DIFFERENCE_ED = 0.10f;
const float BIPLANE_LENGTH_MAX_DIFFERENCE_ES = 0.10f;

struct AttachJNITask: public AITask {
    EFNativeJNI *mJNI;
    AttachJNITask(AIManager *manager, EFNativeJNI *jni) : AITask(manager), mJNI(jni) {}
    virtual void run() {
        mJNI->attachToThread();
    }
};

ThorStatus EFWorkflow::init(AIManager *manager, void *javaEnv, void *javaContext) {
	mManager = manager;
	mEFNativeJNI = new EFNativeJNI(static_cast<JNIEnv*>(javaEnv), static_cast<jobject>(javaContext));

    // Send the JNI structure to the worker thread (WHICH NOW MUST BE SINGLETON)
    // Task immediately attaches it to the current thread
    // Worker thread is AIManagerWorker, from AIManager.cpp
    manager->push(std::make_unique<AttachJNITask>(manager, mEFNativeJNI));
	return THOR_OK;
}

EFWorkflow::EFWorkflow()
{
	mManager = nullptr;
    mFlipX = -1.0f;
	reset();
}

void EFWorkflow::resetEdit()
{
	mStats.hasEdited = false;
	mUsera2c.frames = {-1,-1};
	mUsera2c.segmentations.clear();
	mUsera4c.frames = {-1,-1};
	mUsera4c.segmentations.clear();

	mStats.editedA4cStats = {0};
	mStats.editedA2cStats = {0};
	mStats.editedBiplaneStats = {0};
}

void EFWorkflow::reset()
{
	LOGD("EFworkflow::reset");
	resetEdit();

	std::fill(std::begin(mAIa2c.frames.edFrames), std::end(mAIa2c.frames.edFrames), -1);
	std::fill(std::begin(mAIa2c.frames.esFrames), std::end(mAIa2c.frames.esFrames), -1);
	mAIa2c.frames.numCycles = 0;
	mAIa2c.segmentations.clear();

	std::fill(std::begin(mAIa4c.frames.edFrames), std::end(mAIa4c.frames.edFrames), -1);
	std::fill(std::begin(mAIa4c.frames.esFrames), std::end(mAIa4c.frames.esFrames), -1);
	mAIa4c.frames.numCycles = 0;
	mAIa4c.segmentations.clear();

	mStats.heartRate = 0.0f;
	mStats.originalA4cStats = {0};
	mStats.originalA2cStats = {0};
	mStats.originalBiplaneStats = {0};
}

void EFWorkflow::setFrameIDCallback(void (*cb)(void*, EFWorkflow*, CardiacView))
{
	mFrameIDCallback = cb;
}
void EFWorkflow::setSegmentationCallback(void (*cb)(void*, EFWorkflow*, CardiacView, int))
{
	mSegmentationCallback = cb;
}
void EFWorkflow::setStatisticsCallback(void (*cb)(void*, EFWorkflow*))
{
	mStatisticsCallback = cb;
}
void EFWorkflow::setErrorCallback(void (*cb)(void*, EFWorkflow*, ThorStatus))
{
	mErrorCallback = cb;
}
void EFWorkflow::setAutocaptureStartCallback(void (*cb)(void *, EFWorkflow *))
{
    mAutocaptureStartCallback = cb;
}
void EFWorkflow::setAutocaptureFailCallback(void (*cb)(void *, EFWorkflow *))
{
	mAutocaptureFailCallback = cb;
}
void EFWorkflow::setAutocaptureSucceedCallback(void (*cb)(void *, EFWorkflow *))
{
	mAutocaptureSucceedCallback = cb;
}
void EFWorkflow::setAutocaptureProgressCallback(void (*cb)(void *, EFWorkflow*, float))
{
    mAutocaptureProgressCallback = cb;
}

void EFWorkflow::setSmartCaptureProgressCallback(void (*cb)(void *, EFWorkflow*, float))
{
	mSmartCaptureProgressCallback = cb;
}

void EFWorkflow::setAIFrames(CardiacView view, const MultiCycleFrames& frames)
{
	LOGD("%s(%s,{%d,%d})", __func__, view == CardiacView::A2C ? "A2C" : "A4C", frames.edFrames[0], frames.esFrames[0]);
	if (view == CardiacView::A2C)
		mAIa2c.frames = frames;
	else
		mAIa4c.frames = frames;
}

const MultiCycleFrames& EFWorkflow::getAIFrames(CardiacView view)
{
	LOGD("%s(%s)", __func__, view == CardiacView::A2C ? "A2C" : "A4C");
	if (view == CardiacView::A2C)
		return mAIa2c.frames;
	else
		return mAIa4c.frames;
}

void EFWorkflow::setAISegmentation(CardiacView view, int frame, CardiacSegmentation segmentation)
{
	LOGD("%s(%s,%d)", __func__, view == CardiacView::A2C ? "A2C" : "A4C", frame);
	if (view == CardiacView::A2C)
		mAIa2c.segmentations[frame] = segmentation;
	else
		mAIa4c.segmentations[frame] = segmentation;
}

CardiacSegmentation EFWorkflow::getAISegmentation(CardiacView view, int frame)
{
	LOGD("%s(%s, %d)", __func__, view == CardiacView::A2C ? "A2C" : "A4C", frame);
	if (view == CardiacView::A2C)
		return mAIa2c.segmentations[frame];
	else
		return mAIa4c.segmentations[frame];
}

	// Get/set user override results
void EFWorkflow::setUserFrames(CardiacView view, CardiacFrames frames)
{
	LOGD("%s(%s, {%d,%d}", __func__, view == CardiacView::A2C ? "A2C" : "A4C", frames.edFrame, frames.esFrame);
	mStats.hasEdited = true;
	if (view == CardiacView::A2C)
		mUsera2c.frames = frames;
	else
		mUsera4c.frames = frames;
}
CardiacFrames EFWorkflow::getUserFrames(CardiacView view)
{
	LOGD("%s(%s)", __func__, view == CardiacView::A2C ? "A2C" : "A4C");
	if (view == CardiacView::A2C)
		return mUsera2c.frames;
	else
		return mUsera4c.frames;
}

void EFWorkflow::setUserSegmentation(CardiacView view, int frame, CardiacSegmentation segmentation)
{
	LOGD("%s(%s,%d)", __func__, view == CardiacView::A2C ? "A2C" : "A4C", frame);
	mStats.hasEdited = true;
	CardiacSegmentation *s;
	if (view == CardiacView::A2C)
		s = &mUsera2c.segmentations[frame];
	else
		s = &mUsera4c.segmentations[frame];

	*s = std::move(segmentation);
}

CardiacSegmentation EFWorkflow::getUserSegmentation(CardiacView view, int frame)
{
	LOGD("%s(%s,%d)", __func__, view == CardiacView::A2C ? "A2C" : "A4C", frame);
	if (view == CardiacView::A2C)
		return mUsera2c.segmentations[frame];
	else
		return mUsera4c.segmentations[frame];
}

EFStatistics EFWorkflow::getStatistics()
{
	LOGD("%s", __func__);
	return mStats;
}

// Find a line intersection point (one line given in point/normal format, the other given as two points)
static vec2 LineIntersect(vec2 p, vec2 n, vec2 l0, vec2 l1)
{
	float d0 = dot(n, p-l0); // distance from l0 to line
	float d1 = dot(n, p-l1); // distance from l1 to line
	float r = d1/(d1-d0);
	return r*l0 + (1-r)*l1;
}

static const int NUM_DISKS = 20;

void EFWorkflow::computeDisks(CardiacSegmentation &segmentation)
{
	if (segmentation.coords.empty())
	{
		LOGW("Asked to compute disk intersections on an empty segmentation spline");
		return;
	}

	// add initial point and terminal point before and after base
	//  these extra points are a linear extension of the 2 points before/after them
	std::vector<vec2> points = segmentation.coords;
	vec2 pre = points[0]+(points[0]-points[1]);
	vec2 post = points.back()+(points.back()-points[points.size()-2]);
	points.insert(points.begin(), pre);
	points.push_back(post);

	auto spline = CatmullRom(std::begin(points), std::end(points));
	// base points
	vec2 b0 = points[1];
	vec2 b1 = points[points.size()-2];
	// apex point (expecting size() to be odd, so integer rounding gets what we want)
	vec2 apex = points[points.size()/2];

	// halfway between the bases (aka start of apex line)
	vec2 base = 0.5f*(b0+b1);
	// line from base to apex
	vec2 line = apex-base;
	float len = length(line);
	vec2 n = line/len;

	segmentation.base = base;
	segmentation.apex = apex;

	// starting guesses for left and right intersections
	float leftT = 0.0f;
	float rightT = 1.0f;

	// left and right intersection points
	vec2 leftI{0,0}, rightI{0,0};

	auto &result = segmentation.disks;
	result.clear();

	for (int i=0; i < NUM_DISKS; ++i)
	{
		// base of the disk line
		vec2 diskP = base + line*((i+0.5f)/NUM_DISKS);
		// find two intersection points
		if (CatmullRom::INTERSECT != spline.intersect(diskP, n, leftT, leftI))
		{
			// find intersection with base line
			leftT = clamp(leftT, 0.0f, 1.0f);
			leftI = LineIntersect(diskP, n, b0, b1);
		}
		if (CatmullRom::INTERSECT != spline.intersect(diskP, n, rightT, rightI))
		{
			// find intersection with base line
			rightT = clamp(rightT, 0.0f, 1.0f);
			rightI = LineIntersect(diskP, n, b0, b1);
		}

		result.emplace_back(leftI, rightI);
	}
}

float EFWorkflow::volumeSinglePlane(CardiacSegmentation &a4c, ThorStatus *error)
{
	// compute disks if needed
	if (a4c.disks.empty())
	{
		computeDisks(a4c);
		if (a4c.disks.empty()) {
            if (error) {
                *error = TER_AI_SEGMENTATION;
            }
            return -1.0f; // if still empty, then we have no data
        }
	}

	// height of each disk
	a4c.length = distance(a4c.base, a4c.apex);
	float h = a4c.length / NUM_DISKS;
	float volume = 0.0f;
	for (const auto &disk : a4c.disks)
	{
		// assume each disk is a perfect cylinder
		float r = 0.5f*distance(disk.first, disk.second);
		float dv = 3.14159265f * r * r * h;
		volume += dv;
	}

	volume *= 0.001f; // convert cubic millimeters to mL

	return volume;
}

float EFWorkflow::volumeBiplane(CardiacSegmentation &a4c, CardiacSegmentation &a2c, bool isED, ThorStatus *error)
{
	// compute disks if needed
	if (a4c.disks.empty())
	{
		computeDisks(a4c);
		// if still empty, then we have no data
        if (a4c.disks.empty()) {
            if (error) {
                *error = TER_AI_SEGMENTATION;
            }
            return -1.0f;
        }
	}
	if (a2c.disks.empty())
	{
		computeDisks(a2c);
		// if still empty, then we have no data
        if (a2c.disks.empty()) {
            if (error) {
                *error = TER_AI_SEGMENTATION;
            }
            return -1.0f;
        }
	}

	// length is the maximum of the two views
	a4c.length = distance(a4c.base, a4c.apex);
	a2c.length = distance(a2c.base, a2c.apex);
    float L = std::max(a4c.length, a2c.length);

	float threshold = isED ? BIPLANE_LENGTH_MAX_DIFFERENCE_ED : BIPLANE_LENGTH_MAX_DIFFERENCE_ES;
	if (std::abs(a4c.length-a2c.length) / L > threshold) {
        // NOTE: Continue with computation, but also set error
        if (error) {
            *error = a4c.length < a2c.length ? TER_AI_A4C_FORESHORTENED : TER_AI_A2C_FORESHORTENED;
        }
    }
	float h = L / NUM_DISKS;
	float volume = 0.0f;
	for (int i=0; i < NUM_DISKS; ++i)
	{
		float r1 = 0.5f * distance(a4c.disks[i].first, a4c.disks[i].second);
		float r2 = 0.5f * distance(a2c.disks[i].first, a2c.disks[i].second);
		volume += 3.14159265f * r1 * r2 * h;
	}
	volume *= 0.001f; // convert cubic millimeters to mL
	return volume;
}

void EFWorkflow::setHeartRate(float hr)
{
	LOGD("Setting HR to %0.2f bpm", hr);
	mStats.heartRate = hr;
}

void EFWorkflow::setFlip(float flipX, float flipY)
{
	mFlipX = flipX;
}

float EFWorkflow::getFlipX() const
{
	return mFlipX;
}

ViewStatistics EFWorkflow::computeViewStats(int numCycles, CycleSegmentation *segs, float hr, CardiacView view, ThorStatus *error)
{
	ViewStatistics result = {0};
	float uncertaintyThresholdED;
	float uncertaintyThresholdES;
	if (view == CardiacView::A4C) {
	    uncertaintyThresholdED = MAX_UNCERTAINTY_A4C_ED;
	    uncertaintyThresholdES = MAX_UNCERTAINTY_A4C_ES;
	} else {
        uncertaintyThresholdED = MAX_UNCERTAINTY_A2C_ED;
        uncertaintyThresholdES = MAX_UNCERTAINTY_A2C_ES;
	}

	// Create list of (cycle index, uncertainty) where uncertainty is under threshold
	std::vector<std::pair<int, float>> uncertainties;
	for (int i=0; i != numCycles; ++i) {
	    float uncertaintyED = segs[i].ed->uncertainty;
	    float uncertaintyES = segs[i].es->uncertainty;
		float uncertainty = (uncertaintyED + uncertaintyES) / 2.0f;
		LOGD("Single plane cycle %d has uncertainty %f (ED %f, ES %f)", i, uncertainty, uncertaintyED, uncertaintyES);
		if (uncertaintyED <= uncertaintyThresholdED && uncertaintyES <= uncertaintyThresholdES)
			uncertainties.emplace_back(i, uncertainty);
	}
	// Sort to get lowest three uncertainties (or fewer if fewer pass threshold)
	auto by_uncertainty = [](const std::pair<int, float>& a, const std::pair<int, float>& b) { return a.second < b.second; };
	std::sort(uncertainties.begin(), uncertainties.end(), by_uncertainty);
	int validCycles = std::min<int>(uncertainties.size(), 3);

	if (validCycles == 0) {
        if (error) {
            *error = TER_AI_LV_UNCERTAINTY_ERROR;
        }
		// proceed with just the lowest uncertainty cycle
		float minUncertainty = 10000.0f;
		int minUncertaintyCycle = -1;
		for (int i=0; i != numCycles; ++i) {
			   float uncertaintyED = segs[i].ed->uncertainty;
			   float uncertaintyES = segs[i].es->uncertainty;
			   float uncertainty = (uncertaintyED + uncertaintyES) / 2.0f;
			   LOGD("Single plane cycle %d has uncertainty %f (ED %f, ES %f)", i, uncertainty, uncertaintyED, uncertaintyES);
			   if (minUncertaintyCycle == -1 || uncertainty < minUncertainty) {
					   minUncertainty = uncertainty;
					   minUncertaintyCycle = i;
			   }
		}
		if (minUncertaintyCycle == -1) {
			   LOGE("Asked to computeViewStats with no cycles? Logic error somewhere.");
			   return {-1,-1,-1,-1,-1,-1};
		}

		validCycles = 1;
		uncertainties.emplace_back(minUncertaintyCycle, minUncertainty);
	}

    for (int i=0; i != validCycles; ++i)
    {
    	int cycle = uncertainties[i].first;
    	LOGD("Single plane: computing volume of cycle %d (uncertainty %f)", cycle, uncertainties[i].second);
        float edv = volumeSinglePlane(*segs[cycle].ed, error);
        float esv = volumeSinglePlane(*segs[cycle].es, error);

        result.edVolume += edv;
        result.esVolume += esv;
        result.uncertainty += uncertainties[i].second;
		result.selectedA4CEDFrames[i] = segs[cycle].edFrame;
		result.selectedA4CESFrames[i] = segs[cycle].esFrame;
		result.selectedA2CEDFrames[i] = segs[cycle].edFrame;
		result.selectedA2CESFrames[i] = segs[cycle].esFrame;
    }

    float denom = (float)validCycles;
    result.edVolume /= denom;
    result.esVolume /= denom;
    result.uncertainty /= denom;
    result.numCycles = validCycles;

	result.SV = result.edVolume - result.esVolume;
	result.EF = result.SV / result.edVolume * 100.0f;
	result.CO = result.SV * hr / 1000.0f; // change from mL / sec to L / sec

	return result;
}

ViewStatistics EFWorkflow::computeViewStatsBiplane(int numCyclesA4C, int numCyclesA2C, CycleSegmentation *a4c,
        CycleSegmentation *a2c, float hr, ThorStatus *error)
{
	ViewStatistics result = {0};

	// Create lists of (cycle index, uncertainty) where uncertainty is under threshold
	std::vector<std::pair<int, float>> a4c_uncertainties;
	for (int i=0; i != numCyclesA4C; ++i) {
        float uncertaintyED = a4c[i].ed->uncertainty;
        float uncertaintyES = a4c[i].es->uncertainty;
        float uncertainty = (uncertaintyED + uncertaintyES) / 2.0f;
        LOGD("Biplane A4C cycle %d has uncertainty %f (ED %f, ES %f)", i, uncertainty, uncertaintyED, uncertaintyES);
		if (uncertaintyED <= MAX_UNCERTAINTY_A4C_ED && uncertaintyES <= MAX_UNCERTAINTY_A4C_ES)
			a4c_uncertainties.emplace_back(i, uncertainty);
	}
	std::vector<std::pair<int, float>> a2c_uncertainties;
	for (int i=0; i != numCyclesA2C; ++i) {
        float uncertaintyED = a2c[i].ed->uncertainty;
        float uncertaintyES = a2c[i].es->uncertainty;
        float uncertainty = (uncertaintyED + uncertaintyES) / 2.0f;
        LOGD("Biplane A2C cycle %d has uncertainty %f (ED %f, ES %f)", i, uncertainty, uncertaintyED, uncertaintyES);
        if (uncertaintyED <= MAX_UNCERTAINTY_A2C_ED && uncertaintyES <= MAX_UNCERTAINTY_A2C_ES)
            a2c_uncertainties.emplace_back(i, uncertainty);
    }


	// Sort to get lowest three uncertainties (or fewer if fewer pass threshold)
	auto by_uncertainty = [](const std::pair<int, float>& a, const std::pair<int, float>& b) { return a.second < b.second; };
	std::sort(a4c_uncertainties.begin(), a4c_uncertainties.end(), by_uncertainty);
	std::sort(a2c_uncertainties.begin(), a2c_uncertainties.end(), by_uncertainty);

	auto validCycles = std::min(3ul, std::min(a2c_uncertainties.size(), a4c_uncertainties.size()));

	if (validCycles == 0) {
		if (error) {
			*error = TER_AI_LV_UNCERTAINTY_ERROR;
		}
		auto pushMinUncertainty = [](const CycleSegmentation *cycles, int numCycles, std::vector<std::pair<int, float>> &uncertainties) {
			float minUncertainty = 100000.0f;
			int minUncertaintyCycle = -1;
			for (int i=0; i != numCycles; ++i) {
				float uncertaintyED = cycles[i].ed->uncertainty;
				float uncertaintyES = cycles[i].es->uncertainty;
				float uncertainty = (uncertaintyED + uncertaintyES) / 2.0f;
				if (uncertainty < minUncertainty || minUncertaintyCycle == -1) {
					minUncertainty = uncertainty;
					minUncertaintyCycle = i;
				}
			}
			if (minUncertaintyCycle == -1) {
				LOGE("numCycles is 0? Logic error in computeViewStatsBiplane");
			} else {
				uncertainties.emplace_back(minUncertaintyCycle, minUncertainty);
			}
		};

        // proceed with just the lowest uncertainty cycle for both A4C and A2C
		if (a4c_uncertainties.empty()) {
			pushMinUncertainty(a4c, numCyclesA4C, a4c_uncertainties);
		}
		if (a2c_uncertainties.empty()) {
			pushMinUncertainty(a2c, numCyclesA2C, a2c_uncertainties);
		}
		if (a4c_uncertainties.empty() || a2c_uncertainties.empty()) {
			// Logic error, we should always have at least one a4c and one a2c cycle (already logged above)
			return {-1,-1,-1,-1,-1,-1};
		}
		validCycles = 1;
	}

	int droppedCycles = 0;
	for (int i=0; i != validCycles; ++i)
	{
		int a4c_cycle = a4c_uncertainties[i].first;
		int a2c_cycle = a2c_uncertainties[i].first;
		LOGD("Computing volume cycle %d, A4C ED: %d ES: %d, A2C ED: %d ES: %d",
			 i, a4c[a4c_cycle].edFrame, a4c[a4c_cycle].esFrame,
			 a2c[a2c_cycle].edFrame, a2c[a2c_cycle].esFrame);
		ThorStatus cycleError = THOR_OK;
		float edv = volumeBiplane(*a4c[a4c_cycle].ed, *a2c[a2c_cycle].ed, true, &cycleError);
		float esv = volumeBiplane(*a4c[a4c_cycle].es, *a2c[a2c_cycle].es, false, &cycleError);
		if (i > 0 && cycleError != THOR_OK) {
			// Drop cycle if not the first (first is shown to user, should not be dropped)
			LOGD("Dropping cycle %d due to volume not computed (length mismatch?)", i);
			++droppedCycles;
			continue;
		} else if (i == 0 && cycleError != THOR_OK) {
			LOGD("Error 0x%08x on first cycle, going to send to app", cycleError);
			if (error) {
				*error = cycleError;
			}
		}

		float uncertainty = (a4c_uncertainties[i-droppedCycles].second + a2c_uncertainties[i-droppedCycles].second) / 2.0f;
		result.edVolume += edv;
		result.esVolume += esv;
		result.uncertainty += uncertainty;
		result.selectedA4CEDFrames[i-droppedCycles] = a4c[a4c_cycle].edFrame;
		result.selectedA4CESFrames[i-droppedCycles] = a4c[a4c_cycle].esFrame;
		result.selectedA2CEDFrames[i-droppedCycles] = a2c[a2c_cycle].edFrame;
		result.selectedA2CESFrames[i-droppedCycles] = a2c[a2c_cycle].esFrame;

		LOGD("Biplane volume from A4C cycle %d (u: %f), A2C cycle %d (u: %f)",
				a4c_cycle, a4c_uncertainties[i-droppedCycles].second,
				a2c_cycle, a2c_uncertainties[i-droppedCycles].second);
	}

	if (validCycles == droppedCycles)
    {
	    LOGD("All cycles dropped from biplane volume computation, returning error");
        return {-1,-1,-1,-1,-1,-1};
    }

	float denom = (float)(validCycles-droppedCycles);
	result.edVolume /= denom;
	result.esVolume /= denom;
	result.uncertainty /= denom;
	result.numCycles = validCycles-droppedCycles;

	result.SV = result.edVolume - result.esVolume;
	result.EF = result.SV / result.edVolume * 100.0f;
	result.CO = result.SV * hr / 1000.0f; // change from mL / sec to L / sec

	return result;
}

ThorStatus EFWorkflow::computeStats()
{
	LOGD("EFWorkflow::computeStats");
    ThorStatus status = THOR_OK;

	// check if we are doing biplane or single plane method
	bool biplane = mAIa2c.frames.numCycles > 0;

	// original calculation: uses ai frames and ai segmentation
	std::vector<CycleSegmentation> ai_a4c_segs;
	for (int cycle=0; cycle != mAIa4c.frames.numCycles; ++cycle) {
		int edFrame = mAIa4c.frames.edFrames[cycle];
		int esFrame = mAIa4c.frames.esFrames[cycle];
		CardiacSegmentation *ed_seg = &mAIa4c.segmentations[edFrame];
		CardiacSegmentation *es_seg = &mAIa4c.segmentations[esFrame];
		ai_a4c_segs.push_back({edFrame, esFrame, ed_seg, es_seg});
	}
	mStats.originalA4cStats = computeViewStats(mAIa4c.frames.numCycles, ai_a4c_segs.data(), mStats.heartRate, CardiacView::A4C, &status);

	if (biplane)
	{
		std::vector<CycleSegmentation> ai_a2c_segs;
		for (int cycle=0; cycle != mAIa2c.frames.numCycles; ++cycle) {
			int edFrame = mAIa2c.frames.edFrames[cycle];
			int esFrame = mAIa2c.frames.esFrames[cycle];
			CardiacSegmentation *ed_seg = &mAIa2c.segmentations[edFrame];
			CardiacSegmentation *es_seg = &mAIa2c.segmentations[esFrame];
			ai_a2c_segs.push_back({edFrame, esFrame, ed_seg, es_seg});
		}

		mStats.originalA2cStats = computeViewStats(mAIa2c.frames.numCycles, ai_a2c_segs.data(), mStats.heartRate, CardiacView::A2C, &status);
		mStats.originalBiplaneStats = computeViewStatsBiplane(mAIa4c.frames.numCycles, mAIa2c.frames.numCycles, ai_a4c_segs.data(), ai_a2c_segs.data(), mStats.heartRate, &status);
	}

	// exit if no user edits have been made
	if (!mStats.hasEdited) {
        // Check AI foreshortened status
        if (status == TER_AI_A4C_FORESHORTENED) {
            mStats.foreshortenedStatus = ForeshortenedStatus::A4C_Foreshortened;
        } else if (status == TER_AI_A2C_FORESHORTENED) {
            mStats.foreshortenedStatus = ForeshortenedStatus::A2C_Foreshortened;
        } else {
            mStats.foreshortenedStatus = ForeshortenedStatus::None;
        }
        return status;
    }

    // If the user is making edits, clear any pending uncertainty/foreshortened errors
    // User edit should never raise an uncertainty error, and the foreshortened error we want to re-check
    // using the frames/segmentations as edited by the user.
    if (status == TER_AI_LV_UNCERTAINTY_ERROR || status == TER_AI_A4C_FORESHORTENED || status == TER_AI_A2C_FORESHORTENED) {
        status = THOR_OK;
    }

	// User override calculcation
	// if user has not selected frames, use AI frames
	CardiacFrames editA4CFrames = mUsera4c.frames.esFrame == -1 ? CardiacFrames{mAIa4c.frames.esFrames[0], mAIa4c.frames.edFrames[0]} : mUsera4c.frames;
	CardiacFrames editA2CFrames = mUsera2c.frames.esFrame == -1 ? CardiacFrames{mAIa2c.frames.esFrames[0], mAIa2c.frames.edFrames[0]} : mUsera2c.frames;

	// then, if user has overridden the segmentation for a given frame, use that segmentation
	// otherwise, use AI segmentation

	// Try to find segmentation in user section, or fall back to AI segmentation
	auto it = mUsera4c.segmentations.find(editA4CFrames.esFrame);
	CardiacSegmentation &edit_a4c_es = (it == mUsera4c.segmentations.end()) ? mAIa4c.segmentations[editA4CFrames.esFrame] : it->second;
	it = mUsera4c.segmentations.find(editA4CFrames.edFrame);
	CardiacSegmentation &edit_a4c_ed = (it == mUsera4c.segmentations.end()) ? mAIa4c.segmentations[editA4CFrames.edFrame] : it->second;
	CycleSegmentation edit_a4c = {editA4CFrames.edFrame, editA4CFrames.esFrame, &edit_a4c_ed, &edit_a4c_es};

	mStats.editedA4cStats = computeViewStats(1, &edit_a4c, mStats.heartRate, CardiacView::A4C, &status);

	if (biplane)
	{
		it = mUsera2c.segmentations.find(editA2CFrames.esFrame);
		CardiacSegmentation &edit_a2c_es = (it == mUsera2c.segmentations.end()) ? mAIa2c.segmentations[editA2CFrames.esFrame] : it->second;
		it = mUsera2c.segmentations.find(editA2CFrames.edFrame);
		CardiacSegmentation &edit_a2c_ed = (it == mUsera2c.segmentations.end()) ? mAIa2c.segmentations[editA2CFrames.edFrame] : it->second;
        CycleSegmentation edit_a2c = {editA2CFrames.edFrame, editA2CFrames.esFrame, &edit_a2c_ed, &edit_a2c_es};
		mStats.editedA2cStats = computeViewStats(1, &edit_a2c, mStats.heartRate, CardiacView::A2C, &status);
		mStats.editedBiplaneStats = computeViewStatsBiplane(1, 1, &edit_a4c, &edit_a2c, mStats.heartRate, &status);
	}

    // Check User edit foreshortened status
    if (status == TER_AI_A4C_FORESHORTENED) {
        mStats.foreshortenedStatus = ForeshortenedStatus::A4C_Foreshortened;
    } else if (status == TER_AI_A2C_FORESHORTENED) {
        mStats.foreshortenedStatus = ForeshortenedStatus::A2C_Foreshortened;
    } else {
        mStats.foreshortenedStatus = ForeshortenedStatus::None;
    }

	// If the user is making edits, we no longer want to see uncertainty or length mismatch errors,
	// regardless of whether they come from the AI info or the user edits.
	if (status == TER_AI_LV_UNCERTAINTY_ERROR || status == TER_AI_A4C_FORESHORTENED || status == TER_AI_A2C_FORESHORTENED) {
		status = THOR_OK;
	}

	return status;
}

void EFWorkflow::notifyFrameIDComplete(void *user, CardiacView view)
{
	LOGD("%s(view=%s)", __func__, view == CardiacView::A2C? "A2C" : "A4C");
	if (mFrameIDCallback)
		mFrameIDCallback(user, this, view);
	else
		LOGE("No Frame ID callback set!");
}

void EFWorkflow::notifySegmentationComplete(void *user, CardiacView view, int frame)
{
	LOGD("%s(view=%s, frame=%d)", __func__, view == CardiacView::A2C? "A2C" : "A4C", frame);
	if (mSegmentationCallback)
		mSegmentationCallback(user, this, view, frame);
	else
		LOGE("No segmentation callback set!");
}

void EFWorkflow::notifyStatisticsComplete(void *user)
{
	LOGD("%s()", __func__);
	if (mStatisticsCallback)
		mStatisticsCallback(user, this);
	else
		LOGE("No statistics callback set!");
}

void EFWorkflow::notifyError(void *user, ThorStatus error)
{
	LOGE("Error in EF Workflow: %08x", error);
	if (mErrorCallback)
		mErrorCallback(user, this, error);
	else
	{
		LOGE("No error callback set");
	}
}
void EFWorkflow::notifyAutocaptureStart(void *user)
{
    LOGD("%s()", __func__);
    if (mAutocaptureStartCallback)
        mAutocaptureStartCallback(user, this);
    else
    {
        LOGE("No autocapture start callback set");
    }
}
void EFWorkflow::notifyAutocaptureFail(void *user)
{
	LOGD("%s()", __func__);
	if (mAutocaptureFailCallback)
		mAutocaptureFailCallback(user, this);
	else
	{
		LOGE("No autocapture fail callback set");
	}
}

void EFWorkflow::notifyAutocaptureSucceed(void *user)
{
	LOGD("%s()", __func__);
	if (mAutocaptureSucceedCallback)
		mAutocaptureSucceedCallback(user, this);
	else
	{
		LOGE("No autocapture succeed callback set");
	}
}
void EFWorkflow::notifyAutocaptureProgress(void *user, float progress)
{
	if (mAutocaptureProgressCallback)
		mAutocaptureProgressCallback(user, this, progress);
	else {
		LOGE("No autocapture progress callback set");
	}
}

void EFWorkflow::notifySmartCaptureProgress(void *user, float progress)
{
	if (mSmartCaptureProgressCallback)
		mSmartCaptureProgressCallback(user, this, progress);
	else {
		LOGE("No Smart Capture progress callback set");
	}
}

EFNativeJNI* EFWorkflow::getNativeJNI() {
    return mEFNativeJNI;
}