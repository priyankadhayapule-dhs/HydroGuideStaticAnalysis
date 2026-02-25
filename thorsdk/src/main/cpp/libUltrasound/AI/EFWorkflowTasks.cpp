//
// Copyright 2019 EchoNous Inc.
//
// Implementation of EFWorkflow Tasks (native side)

#define LOG_TAG "EFWorkflowTasks"

#include <chrono>
#include <thread>
#include <numeric>
#include <sstream>
#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>
#include <ThorUtils.h>
#include <EFWorkflow.h>
#include <EFWorkflowTasks.h>
#include <AIManager.h>
#include <UltrasoundManager.h>
#include <ImageUtils.h>
#include <ScanConverterHelper.h>
#include <PhaseDetection.h>
#include <LVSegmentation.h>
#include <AI/EFWorkflow.h>
#include <EFNativeJNI.h>



// Functions implemented in EFWorkflowJNI, takes userdata pointer which must be weak reference to
// EFWorkflow java instance
int GetEFForcedError(void *user);
void ClearEFForcedError(void *user);

// TODO: is there a way to access the AppConstants contents from here??
enum EFForcedError {
    EF_NO_ERROR = 0,
    EF_A4C_Phase = 1,
    EF_A4C_LvNotFound = 2,
    EF_A4C_MultipleLv = 3,
    EF_A4C_LvTooSmall = 4,
    EF_A4C_Uncertainty = 5,
    EF_A4C_Foreshortened = 6,
    EF_A2C_Phase = 7,
    EF_A2C_LvNotFound = 8,
    EF_A2C_MultipleLv = 9,
    EF_A2C_LvTooSmall = 10,
    EF_A2C_Uncertainty = 11,
    EF_A2C_Foreshortened = 12
};

static EFForcedError GetEFForcedErrorEnum(void *user) {
    return static_cast<EFForcedError>(GetEFForcedError(user));
}

static bool IsSegmentationError(EFForcedError error) {
    switch (error) {
        case EF_NO_ERROR: return false;
        case EF_A4C_Phase: return false;
        case EF_A4C_LvNotFound: return true;
        case EF_A4C_MultipleLv: return true;
        case EF_A4C_LvTooSmall: return true;
        case EF_A4C_Uncertainty: return true;
        case EF_A4C_Foreshortened: return false;
        case EF_A2C_Phase: return false;
        case EF_A2C_LvNotFound: return true;
        case EF_A2C_MultipleLv: return true;
        case EF_A2C_LvTooSmall: return true;
        case EF_A2C_Uncertainty: return true;
        case EF_A2C_Foreshortened: return false;
    }
}

static bool ErrorAppliesToView(EFForcedError error, CardiacView view) {
    switch (error) {
        case EF_NO_ERROR: return false;
        case EF_A4C_Phase:
        case EF_A4C_LvNotFound:
        case EF_A4C_MultipleLv:
        case EF_A4C_LvTooSmall:
        case EF_A4C_Uncertainty:
        case EF_A4C_Foreshortened:
            return view == CardiacView::A4C;
        case EF_A2C_Phase:
        case EF_A2C_LvNotFound:
        case EF_A2C_MultipleLv:
        case EF_A2C_LvTooSmall:
        case EF_A2C_Uncertainty:
        case EF_A2C_Foreshortened:
            return view == CardiacView::A2C;
    }
}

static ThorStatus GetThorStatusError(EFForcedError error) {

    switch (error) {

        case EF_NO_ERROR: return THOR_OK;

        case EF_A4C_Phase: return TER_AI_FRAME_ID;

        case EF_A4C_LvNotFound: return TER_AI_LV_NOT_FOUND;

        case EF_A4C_MultipleLv: return TER_AI_MULTIPLE_LV;

        case EF_A4C_LvTooSmall: return TER_AI_LV_TOO_SMALL;

        case EF_A4C_Uncertainty: return TER_AI_LV_UNCERTAINTY_ERROR;

        case EF_A4C_Foreshortened: return TER_AI_A4C_FORESHORTENED;

        case EF_A2C_Phase: return TER_AI_FRAME_ID;

        case EF_A2C_LvNotFound: return TER_AI_LV_NOT_FOUND;

        case EF_A2C_MultipleLv: return TER_AI_MULTIPLE_LV;

        case EF_A2C_LvTooSmall: return TER_AI_LV_TOO_SMALL;

        case EF_A2C_Uncertainty: return TER_AI_LV_UNCERTAINTY_ERROR;

        case EF_A2C_Foreshortened: return TER_AI_A2C_FORESHORTENED;

    }

}

FrameIDTask::FrameIDTask(AIManager *manager, void *user, EFWorkflow *workflow, CardiacView view) :
    AITask(manager),
    mUser(user), mWorkflow(workflow), mView(view), mDebugOutPath(nullptr) {}

// For testing purposes
const std::vector<float>& FrameIDTask::getPhaseAreas() const {
    return mPhase;
}
int FrameIDTask::getPeriod() const { return mPeriod; }
const std::vector<int>& FrameIDTask::getEDFramesMapped() const { return mEDFramesMapped; }
const std::vector<int>& FrameIDTask::getESFramesMapped() const { return mESFramesMapped; }
const std::vector<float>& FrameIDTask::getEstimatedEFs() const { return mEstimatedEF; }
const std::vector<float>& FrameIDTask::getEDUncertainties() const { return mEDUncertainties; }
const std::vector<float>& FrameIDTask::getESUncertainties() const { return mESUncertainties; }
const std::vector<float>& FrameIDTask::getEDLengths() const { return mEDLengths; }
const std::vector<float>& FrameIDTask::getESLengths() const { return mESLengths; }
const MultiCycleFrames & FrameIDTask::getFrames() const { return mFrames; }


void FrameIDTask::run()
{
    ThorStatus err = THOR_OK;
    auto *cb = getCineBuffer();


    // Find model width and height
    ivec3 size = {128,128,0};

    std::vector<float> frameBuffer;
    std::vector<cv::Mat> frames;

    err = echonous::phase::ScanConvert(cb, cv::Size(size.x, size.y), frameBuffer, mWorkflow->getFlipX(), &frames);
    if (IS_THOR_ERROR(err))
    {
        LOGE("Scan conversion failed");
        mWorkflow->notifyError(mUser, err);
        return;
    }

    mPhase.clear();
    err = echonous::phase::RunModel( frames, mPhase, mWorkflow->getNativeJNI());
    if (IS_THOR_ERROR(err))
    {
        LOGE("Failed to run model");
        mWorkflow->notifyError(mUser, err);
        return;
    }


    MultiCycleFrames resultFrames;
    {
        std::vector<float> normalized_phase;
        echonous::phase::normalize(mPhase.begin(), mPhase.end(),
                                   std::back_inserter(normalized_phase));

        typedef std::vector<float>::iterator iterator;
        std::vector<iterator> ed_frames_raw, es_frames_raw;

        // Determine window size as half the period
        size_t T =
                echonous::phase::period(normalized_phase.begin(), normalized_phase.end()) + 1;
        if (T == 0)
            T = 20;

        es_frames_raw.push_back(normalized_phase.begin());

        // Find local minima/maxima
        echonous::phase::local_minmax(normalized_phase.begin(), normalized_phase.end(), T,
                                      std::back_inserter(ed_frames_raw),
                                      std::back_inserter(es_frames_raw));

        // BUG(20200225) Make extra safety check here
        if (ed_frames_raw.empty()) {
            LOGE("No ED frames found from local_minmax");
            mWorkflow->notifyError(mUser, TER_AI_FRAME_ID);
            return;
        }

        // for each es frame, find the corresponding ed frame (or estimate via period)
        std::vector<iterator> es_frames_mapped;
        std::vector<iterator> ed_frames_mapped;
        auto cur_ed_frame = ed_frames_raw.begin(); // BUG(20200225) always valid becuase of empty check above
        auto next_ed_frame =
                cur_ed_frame + 1;       // BUG(20200225) may be equal to ed_frames_raw.end()
        for (size_t i = 1; i != es_frames_raw.size(); ++i) {
            iterator cycle_start = es_frames_raw[i - 1];
            iterator cycle_end = es_frames_raw[i];

            // advance ed frame iterators
            // next points one past the current cycle, so cur points to the last ed frame in the cycle
            // BUG(20200225) Dereference here is OK because we have just checked against end
            while (next_ed_frame != ed_frames_raw.end() && *next_ed_frame < cycle_end) {
                ++cur_ed_frame;
                ++next_ed_frame;
            }

            // BUG(20200225) Dereference is OK because cur_ed_frame is always valid (loop invariant)
            if (cycle_start < *cur_ed_frame && *cur_ed_frame <= cycle_end) {
                ed_frames_mapped.push_back(*cur_ed_frame);
                es_frames_mapped.push_back(cycle_end);
            } else {
                // estimate using previous ed frame (if any) if the value would be in the cycle
                if (!ed_frames_mapped.empty() && ed_frames_mapped.back() + T < cycle_end) {
                    ed_frames_mapped.push_back(ed_frames_mapped.back() + T);
                    es_frames_mapped.push_back(cycle_end);
                }
            }
        }

        if (ed_frames_mapped.empty()) {
            LOGE("Failed to identify ed/es frames");
            mWorkflow->notifyError(mUser, TER_AI_FRAME_ID);
            return;
        }

        // select the pair of ed/es frames which are closest to the mean "ef"
        std::vector<std::pair<float, unsigned int>> estimated_ef;
        float mean_ef = 0.0f;
        for (unsigned i = 0; i < ed_frames_mapped.size(); ++i) {
            float ef = (mPhase[ed_frames_mapped[i] - normalized_phase.begin()] -
                    mPhase[es_frames_mapped[i] - normalized_phase.begin()]) /
                    mPhase[ed_frames_mapped[i] - normalized_phase.begin()];
            estimated_ef.emplace_back(ef, i);
            mean_ef += ef;
        }
        mean_ef /= estimated_ef.size();

        // Sort by estimated "ef" (comparing of areas)
        auto compare = [=](std::pair<float, unsigned int> a, std::pair<float, unsigned int> b) {
            return std::abs(a.first - mean_ef) < std::abs(b.first - mean_ef);
        };
        std::sort(estimated_ef.begin(), estimated_ef.end(), compare);
        unsigned int numCycles = min(estimated_ef.size(), MultiCycleFrames::MAX_CYCLES);
        for (unsigned int i = 0; i != numCycles; ++i) {
            int cycle = estimated_ef[i].second;
            resultFrames.edFrames[i] = ed_frames_mapped[cycle] - normalized_phase.begin();
            resultFrames.esFrames[i] = es_frames_mapped[cycle] - normalized_phase.begin();
        }
        resultFrames.numCycles = numCycles;
    }

    err = doSegmentationAndUncertaintyCheck(&resultFrames);
    // Allow an uncertainty error here, and do NOT report it to the app
    // The app will soon call beginSegmentation, and THAT is when we should return th
    // uncertainty error.
    //
    // Note that, when we have an uncertainty error, we now force the system to have a single
    // valid cycle (pair of ED/ES frame numbers), so setAIFrames works as intended.
    if (IS_THOR_ERROR(err) && err != TER_AI_LV_UNCERTAINTY_ERROR)
    {
        LOGE("Error in doSegmentationAndUncertaintyCheck: 0x%08x", err);
        mWorkflow->notifyError(mUser, err);
        return;
    }

    // Potentially force error
    {
        EFForcedError forcedError = GetEFForcedErrorEnum(mUser);
        if (forcedError == EF_A4C_Phase && mView == CardiacView::A4C) {
            LOGD("Forcing A4C phase detection error");
            ClearEFForcedError(mUser);
            mWorkflow->notifyError(mUser, TER_AI_FRAME_ID);
            return;
        } else if (forcedError == EF_A2C_Phase && mView == CardiacView::A2C) {
            LOGD("Forcing A2C phase detection error");
            ClearEFForcedError(mUser);
            mWorkflow->notifyError(mUser, TER_AI_FRAME_ID);
            return;
        }
    }

    mWorkflow->setAIFrames(mView, resultFrames);
    mWorkflow->notifyFrameIDComplete(mUser, mView);
}


std::ostream& operator<<(std::ostream& stream, const MultiCycleFrames& frames)
{
    stream << "[";
    for (int cycle=0; cycle != frames.numCycles; ++cycle) {
        if (cycle != 0)
            stream << ", ";
        stream << "[" << frames.edFrames[cycle] << ", " << frames.esFrames[cycle] << "]";
    }
    return stream << "]";
}

ThorStatus FrameIDTask::runImmediate(FILE* rawOut, void *jenv, const char* debugOut)
{
    ThorStatus err = THOR_OK;
    setDebugOutPath(debugOut);
    auto *cb = getCineBuffer();

    // Find model width and height
    ivec3 size = ivec3(128,128,1);

    std::vector<float> frameBuffer;
    std::vector<cv::Mat> frames;
    size.x = 128;
    size.y = 128;

    err = echonous::phase::ScanConvert(cb, cv::Size(size.x, size.y), frameBuffer, mWorkflow->getFlipX(), &frames);
    if (IS_THOR_ERROR(err))
    {
        LOGE("Scan conversion failed");
        return err;
    }

    mPhase.clear();
    err = echonous::phase::RunModel( frames, mPhase, mWorkflow->getNativeJNI(), jenv);

    if (IS_THOR_ERROR(err))
    {
        LOGE("Failed to run model");
        return err;
    }


    {
        std::vector<float> normalized_phase;
        echonous::phase::normalize(mPhase.begin(), mPhase.end(),
                                   std::back_inserter(normalized_phase));

        typedef std::vector<float>::iterator iterator;
        std::vector<iterator> ed_frames_raw, es_frames_raw;

        // Determine window size as half the period
        size_t T =
                echonous::phase::period(normalized_phase.begin(), normalized_phase.end()) + 1;
        if (T == 0)
            T = 20;
        mPeriod = static_cast<int>(T);

        es_frames_raw.push_back(normalized_phase.begin());

        // Find local minima/maxima
        echonous::phase::local_minmax(normalized_phase.begin(), normalized_phase.end(), T,
                                      std::back_inserter(ed_frames_raw),
                                      std::back_inserter(es_frames_raw));

        // BUG(20200225) Make extra safety check here
        if (ed_frames_raw.empty()) {
            LOGE("No ED frames found from local_minmax");
            return TER_AI_FRAME_ID;
        }

        // for each es frame, find the corresponding ed frame (or estimate via period)
        std::vector<iterator> es_frames_mapped;
        std::vector<iterator> ed_frames_mapped;
        auto cur_ed_frame = ed_frames_raw.begin(); // BUG(20200225) always valid becuase of empty check above
        auto next_ed_frame =
                cur_ed_frame + 1;       // BUG(20200225) may be equal to ed_frames_raw.end()
        for (size_t i = 1; i != es_frames_raw.size(); ++i) {
            iterator cycle_start = es_frames_raw[i - 1];
            iterator cycle_end = es_frames_raw[i];

            // advance ed frame iterators
            // next points one past the current cycle, so cur points to the last ed frame in the cycle
            // BUG(20200225) Dereference here is OK because we have just checked against end
            while (next_ed_frame != ed_frames_raw.end() && *next_ed_frame < cycle_end) {
                ++cur_ed_frame;
                ++next_ed_frame;
            }

            // BUG(20200225) Dereference is OK because cur_ed_frame is always valid (loop invariant)
            if (cycle_start < *cur_ed_frame && *cur_ed_frame <= cycle_end) {
                ed_frames_mapped.push_back(*cur_ed_frame);
                es_frames_mapped.push_back(cycle_end);
            } else {
                // estimate using previous ed frame (if any) if the value would be in the cycle
                if (!ed_frames_mapped.empty() && ed_frames_mapped.back() + T < cycle_end) {
                    ed_frames_mapped.push_back(ed_frames_mapped.back() + T);
                    es_frames_mapped.push_back(cycle_end);
                }
            }
        }

        if (ed_frames_mapped.empty()) {
            LOGE("Failed to identify ed/es frames");
            return TER_AI_FRAME_ID;
        }
        for (iterator it : ed_frames_mapped) {
            mEDFramesMapped.push_back(it-normalized_phase.begin());
        }
        for (iterator it : es_frames_mapped) {
            mESFramesMapped.push_back(it-normalized_phase.begin());
        }

        // select the pair of ed/es frames which are closest to the mean "ef"
        std::vector<std::pair<float, unsigned int>> estimated_ef;
        float mean_ef = 0.0f;
        for (unsigned i = 0; i < ed_frames_mapped.size(); ++i) {
            float ef = (mPhase[ed_frames_mapped[i] - normalized_phase.begin()] -
                        mPhase[es_frames_mapped[i] - normalized_phase.begin()]) /
                       mPhase[ed_frames_mapped[i] - normalized_phase.begin()];
            estimated_ef.emplace_back(ef, i);
            mEstimatedEF.push_back(ef);
            mean_ef += ef;
            int frame = ed_frames_mapped[i] - normalized_phase.begin();
            LOGD("[ EF VALIDATION ] ED Frame: %d Estimated EF: %.03f, Mean EF: %.03f", frame, ef, mean_ef / estimated_ef.size());
        }
        mean_ef /= estimated_ef.size();

        // Sort by estimated "ef" (comparing of areas)
        auto compare = [=](std::pair<float, unsigned int> a, std::pair<float, unsigned int> b) {
            return std::abs(a.first - mean_ef) < std::abs(b.first - mean_ef);
        };
        std::sort(estimated_ef.begin(), estimated_ef.end(), compare);
        unsigned int numCycles = min(estimated_ef.size(), MultiCycleFrames::MAX_CYCLES);
        for (unsigned int i = 0; i != numCycles; ++i) {
            int cycle = estimated_ef[i].second;
            mFrames.edFrames[i] = ed_frames_mapped[cycle] - normalized_phase.begin();
            mFrames.esFrames[i] = es_frames_mapped[cycle] - normalized_phase.begin();
        }
        mFrames.numCycles = numCycles;
    }

    err = doSegmentationAndUncertaintyCheck(&mFrames, jenv);
    if (err != THOR_OK)
    {
        LOGE("Error in doSegmentationAndUncertaintyCheck");
        return err;
    }

    auto &viewData = mView == CardiacView::A2C ? mWorkflow->mAIa2c : mWorkflow->mAIa4c;
    for (int edframe : mEDFramesMapped) {
        LOGD("Fetching uncertainty for ED frame %d", edframe);
        auto iter = viewData.segmentations.find(edframe);
        if (iter != viewData.segmentations.end()) {
            mWorkflow->volumeSinglePlane(iter->second, &err);
            mEDUncertainties.push_back(iter->second.uncertainty);
            mEDLengths.push_back(iter->second.length);
        } else {
            mEDUncertainties.push_back(-1.0f);
            mEDLengths.push_back(-1.0f);
        }
    }
    for (int esframe : mESFramesMapped) {
        LOGD("Fetching uncertainty for ES frame %d", esframe);
        auto iter = viewData.segmentations.find(esframe);
        if (iter != viewData.segmentations.end()) {
            mWorkflow->volumeSinglePlane(iter->second, &err);
            mESUncertainties.push_back(iter->second.uncertainty);
            mESLengths.push_back(iter->second.length);
        } else {
            mESUncertainties.push_back(-1.0f);
            mESLengths.push_back(-1.0f);
        }
    }

    return THOR_OK;
}
ThorStatus FrameIDTask::doSegmentationAndUncertaintyCheck(MultiCycleFrames *frames)
{
    std::stringstream sout;
    sout << *frames;
    LOGD("Phase detection: running segmentation. Input cycles are: %s", sout.str().c_str());


    auto &viewData = mView == CardiacView::A2C ? mWorkflow->mAIa2c : mWorkflow->mAIa4c;

    CycleSegmentation segs[MultiCycleFrames::MAX_CYCLES];
    for (int cycle = 0; cycle != frames->numCycles; ++cycle)
    {
        auto& cycleSeg = segs[cycle];
        cycleSeg.edFrame = frames->edFrames[cycle];
        cycleSeg.esFrame = frames->esFrames[cycle];
        auto err = segment(cycleSeg.edFrame, viewData.segmentations, true);
        if (err != THOR_OK) {
            LOGE("Error segmenting ED frame %d", cycleSeg.edFrame);
            return err;
        }
        err = segment(cycleSeg.esFrame, viewData.segmentations, false);
        if (err != THOR_OK) {
            LOGE("Error segmenting ES frame %d", cycleSeg.esFrame);
            return err;
        }
    }

    for (int cycle = 0; cycle != frames->numCycles; ++cycle)
    {
        auto& cycleSeg = segs[cycle];
        // DANGER! These are pointers into a std::unordered_map
        // Any alteration to the map will invalidate these pointers
        // That is why this is a separate loop from above:
        // the above loop adds to the viewData.segmentations map, which would invalidate previous pointers
        cycleSeg.ed = &viewData.segmentations[cycleSeg.edFrame];
        cycleSeg.es = &viewData.segmentations[cycleSeg.esFrame];
    }

    ThorStatus err = THOR_OK;
    auto stats = mWorkflow->computeViewStats(frames->numCycles, segs, 0.0f, mView, &err);
    if (IS_THOR_ERROR(err) && err != TER_AI_LV_UNCERTAINTY_ERROR)
    {
        LOGE("Unknown/unexpected error from computeViewStats: 0x%08x", err);
        return err;
    }
    // In this case, we might have an uncertainty error, but we proceed to store the frames anyway

    // Set the input MultiCycleFrames to reflect the result
    const auto& selectedEDFrames = mView == CardiacView::A4C ? stats.selectedA4CEDFrames : stats.selectedA2CEDFrames;
    const auto& selectedESFrames = mView == CardiacView::A4C ? stats.selectedA4CESFrames : stats.selectedA2CESFrames;
    frames->numCycles = stats.numCycles;
    for (int cycle = 0; cycle != stats.numCycles; ++cycle) {
        frames->edFrames[cycle] = selectedEDFrames[cycle];
        frames->esFrames[cycle] = selectedESFrames[cycle];
    }

    sout.str(std::string());
    sout << *frames;
    LOGD("Phase detection: done with segmentation. Final frames are: %s", sout.str().c_str());
    return err; // Can now be an uncertainty error
}

ThorStatus FrameIDTask::doSegmentationAndUncertaintyCheck(MultiCycleFrames *frames, void *jenv)
{
    std::stringstream sout;
    sout << *frames;
    LOGD("Phase detection: running segmentation. Input cycles are: %s", sout.str().c_str());


    auto &viewData = mView == CardiacView::A2C ? mWorkflow->mAIa2c : mWorkflow->mAIa4c;

    CycleSegmentation segs[MultiCycleFrames::MAX_CYCLES];
    for (int cycle = 0; cycle != frames->numCycles; ++cycle)
    {
        auto& cycleSeg = segs[cycle];
        cycleSeg.edFrame = frames->edFrames[cycle];
        cycleSeg.esFrame = frames->esFrames[cycle];
        auto err = segment(cycleSeg.edFrame, viewData.segmentations, true, jenv);
        if (err != THOR_OK) {
            LOGE("Error segmenting ED frame %d", cycleSeg.edFrame);
            return err;
        }
        err = segment(cycleSeg.esFrame, viewData.segmentations, false, jenv);
        if (err != THOR_OK) {
            LOGE("Error segmenting ES frame %d", cycleSeg.esFrame);
            return err;
        }
    }

    for (int cycle = 0; cycle != frames->numCycles; ++cycle)
    {
        auto& cycleSeg = segs[cycle];
        // DANGER! These are pointers into a std::unordered_map
        // Any alteration to the map will invalidate these pointers
        // That is why this is a separate loop from above:
        // the above loop adds to the viewData.segmentations map, which would invalidate previous pointers
        cycleSeg.ed = &viewData.segmentations[cycleSeg.edFrame];
        cycleSeg.es = &viewData.segmentations[cycleSeg.esFrame];
    }

    ThorStatus err = THOR_OK;
    auto stats = mWorkflow->computeViewStats(frames->numCycles, segs, 0.0f, mView, &err);
    if (IS_THOR_ERROR(err) && err != TER_AI_LV_UNCERTAINTY_ERROR)
    {
        LOGE("Unknown/unexpected error from computeViewStats: 0x%08x", err);
        return err;
    }
    // In this case, we might have an uncertainty error, but we proceed to store the frames anyway

    // Set the input MultiCycleFrames to reflect the result
    const auto& selectedEDFrames = mView == CardiacView::A4C ? stats.selectedA4CEDFrames : stats.selectedA2CEDFrames;
    const auto& selectedESFrames = mView == CardiacView::A4C ? stats.selectedA4CESFrames : stats.selectedA2CESFrames;
    frames->numCycles = stats.numCycles;
    for (int cycle = 0; cycle != stats.numCycles; ++cycle) {
        frames->edFrames[cycle] = selectedEDFrames[cycle];
        frames->esFrames[cycle] = selectedESFrames[cycle];
    }

    sout.str(std::string());
    sout << *frames;
    LOGD("Phase detection: done with segmentation. Final frames are: %s", sout.str().c_str());
    return err; // Can now be an uncertainty error
}

ThorStatus FrameIDTask::segment(uint32_t frameNum, std::unordered_map<int, CardiacSegmentation> &segs, bool isEDFrame)
{
    ThorStatus err;
    CineBuffer *cb = getCineBuffer();

    if (segs.find(frameNum) != segs.end())
    {
        LOGD("Using existing segmentation for view %s frame %d", mView == CardiacView::A4C ? "A4C":"A2C", frameNum);
        return THOR_OK;
    }

    // Find model width and height
    ivec3 size = ivec3(128, 128, 1);

    // Scan converted frames
    err = echonous::lv::ScanConvertWindow(cb, cv::Size(size.x, size.y), frameNum, mWorkflow->getFlipX(), mWorkflow->getNativeJNI()->segmentationImageBufferNative);
    if (IS_THOR_ERROR(err)) { return err; }

    // Run Ensemble
    cv::Mat mask;
    float uncertainty;
    err = echonous::lv::RunEnsemble(mWorkflow->getNativeJNI(), mask, &uncertainty,
                                    isEDFrame, frameNum, mView);
    if (IS_THOR_ERROR(err)) { return err; }

    // Find largest component
    cv::Mat lv;
    float area;
    err = echonous::lv::LargestComponent(mask, lv, &area);
    if (IS_THOR_ERROR(err)) { return err; }

    std::vector<cv::Point> denseContour;
    err = echonous::lv::DenseContour(lv, denseContour);
    if (IS_THOR_ERROR(err)) { return err; }

    std::vector<int> splineIndices;
    err = echonous::lv::ShapeDetect(denseContour, splineIndices);
    if (IS_THOR_ERROR(err)) { return err; }

    std::vector<float> arcLength;
    err = echonous::lv::PartialArcLength(denseContour, arcLength);
    if (IS_THOR_ERROR(err)) { return err; }

    err = echonous::lv::PlaceSidePoints(denseContour, arcLength, splineIndices);
    if (IS_THOR_ERROR(err)) { return err; }


    ScanConverterParams params = cb->getParams().converterParams;
    float depth = params.startSampleMm + (params.sampleSpacingMm * (params.numSamples - 1));
    CardiacSegmentation seg;
    err = echonous::lv::ConvertToPhysical(denseContour, splineIndices, -params.startRayRadian, depth, mWorkflow->getFlipX(), cv::Size(size.x, size.y), seg);
    if (IS_THOR_ERROR(err)) { return err; }
    seg.uncertainty = uncertainty;

    segs[static_cast<int>(frameNum)] = std::move(seg);

    return THOR_OK;
}

ThorStatus FrameIDTask::segment(uint32_t frameNum, std::unordered_map<int, CardiacSegmentation> &segs, bool isEDFrame, void *jenv)
{
    ThorStatus err;
    CineBuffer *cb = getCineBuffer();

    if (segs.find(frameNum) != segs.end())
    {
        LOGD("Using existing segmentation for view %s frame %d", mView == CardiacView::A4C ? "A4C":"A2C", frameNum);
        return THOR_OK;
    }

    // Find model width and height
    ivec3 size = ivec3(128, 128, 1);

    // Scan converted frames
    err = echonous::lv::ScanConvertWindow(cb, cv::Size(size.x, size.y), frameNum, mWorkflow->getFlipX(), mWorkflow->getNativeJNI()->segmentationImageBufferNative);
    if (IS_THOR_ERROR(err)) { return err; }

    // Run Ensemble
    cv::Mat mask;
    float uncertainty;
    err = echonous::lv::RunEnsemble(mWorkflow->getNativeJNI(), mask, &uncertainty,
                                    isEDFrame, frameNum, mView, jenv, mDebugOutPath);
    if (IS_THOR_ERROR(err)) { return err; }

    // Find largest component
    cv::Mat lv;
    float area;
    err = echonous::lv::LargestComponent(mask, lv, &area);
    if (IS_THOR_ERROR(err)) { return err; }

    std::vector<cv::Point> denseContour;
    err = echonous::lv::DenseContour(lv, denseContour);
    if (IS_THOR_ERROR(err)) { return err; }

    std::vector<int> splineIndices;
    err = echonous::lv::ShapeDetect(denseContour, splineIndices);
    if (IS_THOR_ERROR(err)) { return err; }

    std::vector<float> arcLength;
    err = echonous::lv::PartialArcLength(denseContour, arcLength);
    if (IS_THOR_ERROR(err)) { return err; }

    err = echonous::lv::PlaceSidePoints(denseContour, arcLength, splineIndices);
    if (IS_THOR_ERROR(err)) { return err; }


    ScanConverterParams params = cb->getParams().converterParams;
    float depth = params.startSampleMm + (params.sampleSpacingMm * (params.numSamples - 1));
    CardiacSegmentation seg;
    err = echonous::lv::ConvertToPhysical(denseContour, splineIndices, -params.startRayRadian, depth, mWorkflow->getFlipX(), cv::Size(size.x, size.y), seg);
    if (IS_THOR_ERROR(err)) { return err; }
    seg.uncertainty = uncertainty;

    segs[static_cast<int>(frameNum)] = std::move(seg);

    return THOR_OK;
}

void FrameIDTask::setDebugOutPath(const char *debugOut) { mDebugOutPath = debugOut;}

SegmentationTask::SegmentationTask(AIManager *manager, void *user, EFWorkflow *workflow, CardiacView view, int frame, bool isEDFrame, bool isUserEdit) :
    AITask(manager),
    mUser(user), mWorkflow(workflow), mView(view), mFrame(frame), mIsEDFrame(isEDFrame), mIsUserEdit(isUserEdit), mDebugOutPath(nullptr) {}

ThorStatus SegmentationTask::segment(uint32_t frameNum, std::unordered_map<int, CardiacSegmentation> &segs)
{
    ThorStatus err;
    CineBuffer *cb = getCineBuffer();

    if (segs.find(frameNum) != segs.end())
    {
        LOGD("Using existing segmentation for view %s frame %d", mView == CardiacView::A4C ? "A4C":"A2C", frameNum);
        return THOR_OK;
    }

    // Find model width and height
    ivec3 size = ivec3(128, 128, 1);

    // Scan converted frames
    cv::Mat frameWindow;
    err = echonous::lv::ScanConvertWindow(cb, cv::Size(size.x, size.y), frameNum, mWorkflow->getFlipX(), mWorkflow->getNativeJNI()->segmentationImageBufferNative);
    if (IS_THOR_ERROR(err)) { return err; }

    // Run Ensemble
    cv::Mat mask;
    float uncertainty;
    err = echonous::lv::RunEnsemble(mWorkflow->getNativeJNI(), mask, &uncertainty,
            mIsEDFrame, frameNum, mView);
    if (IS_THOR_ERROR(err)) { return err; }

    // Find largest component
    cv::Mat lv;
    float area;
    err = echonous::lv::LargestComponent(mask, lv, &area);
    if (IS_THOR_ERROR(err)) { return err; }

    std::vector<cv::Point> denseContour;
    err = echonous::lv::DenseContour(lv, denseContour);
    if (IS_THOR_ERROR(err)) { return err; }

    std::vector<int> splineIndices;
    err = echonous::lv::ShapeDetect(denseContour, splineIndices);
    if (IS_THOR_ERROR(err)) { return err; }

    std::vector<float> arcLength;
    err = echonous::lv::PartialArcLength(denseContour, arcLength);
    if (IS_THOR_ERROR(err)) { return err; }

    err = echonous::lv::PlaceSidePoints(denseContour, arcLength, splineIndices);
    if (IS_THOR_ERROR(err)) { return err; }


    ScanConverterParams params = cb->getParams().converterParams;
    float depth = params.startSampleMm + (params.sampleSpacingMm * (params.numSamples - 1));
    CardiacSegmentation seg;
    err = echonous::lv::ConvertToPhysical(denseContour, splineIndices, -params.startRayRadian, depth, mWorkflow->getFlipX(), cv::Size(size.x, size.y), seg);
    if (IS_THOR_ERROR(err)) { return err; }
    seg.uncertainty = uncertainty;

    segs[static_cast<int>(frameNum)] = std::move(seg);

    return THOR_OK;
}

ThorStatus SegmentationTask::segment(uint32_t frameNum, std::unordered_map<int, CardiacSegmentation> &segs, void *jenv)
{
    ThorStatus err;
    CineBuffer *cb = getCineBuffer();
    if (segs.find(frameNum) != segs.end())
    {
        LOGD("Using existing segmentation for view %s frame %d", mView == CardiacView::A4C ? "A4C":"A2C", frameNum);
        return THOR_OK;
    }

    // Find model width and height
    ivec3 size = ivec3(128, 128, 1);

    // Scan converted frames
    cv::Mat frameWindow;
    err = echonous::lv::ScanConvertWindow(cb, cv::Size(size.x, size.y), frameNum, mWorkflow->getFlipX(), mWorkflow->getNativeJNI()->segmentationImageBufferNative);
    if (IS_THOR_ERROR(err)) { return err; }

    // Run Ensemble
    cv::Mat mask;
    float uncertainty;
    err = echonous::lv::RunEnsemble(mWorkflow->getNativeJNI(), mask, &uncertainty,
                                    mIsEDFrame, frameNum, mView, jenv, mDebugOutPath);
    if (IS_THOR_ERROR(err)) { return err; }

    // Find largest component
    cv::Mat lv;
    float area;
    err = echonous::lv::LargestComponent(mask, lv, &area);
    if (IS_THOR_ERROR(err)) { return err; }

    std::vector<cv::Point> denseContour;
    err = echonous::lv::DenseContour(lv, denseContour);
    if (IS_THOR_ERROR(err)) { return err; }

    std::vector<int> splineIndices;
    err = echonous::lv::ShapeDetect(denseContour, splineIndices);
    if (IS_THOR_ERROR(err)) { return err; }

    std::vector<float> arcLength;
    err = echonous::lv::PartialArcLength(denseContour, arcLength);
    if (IS_THOR_ERROR(err)) { return err; }

    err = echonous::lv::PlaceSidePoints(denseContour, arcLength, splineIndices);
    if (IS_THOR_ERROR(err)) { return err; }


    ScanConverterParams params = cb->getParams().converterParams;
    float depth = params.startSampleMm + (params.sampleSpacingMm * (params.numSamples - 1));
    CardiacSegmentation seg;
    err = echonous::lv::ConvertToPhysical(denseContour, splineIndices, -params.startRayRadian, depth, mWorkflow->getFlipX(), cv::Size(size.x, size.y), seg);
    if (IS_THOR_ERROR(err)) { return err; }
    seg.uncertainty = uncertainty;

    segs[static_cast<int>(frameNum)] = std::move(seg);

    return THOR_OK;
}

void SegmentationTask::run()
{
    ThorStatus err;
    std::shared_ptr<AIModel> model1, model2, model3;

    auto *cb = getCineBuffer();

    // need to pull heart rate when we are looking at a4c
    if (mView == CardiacView::A4C && !mIsEDFrame)
    {
        auto frameNum = mFrame;
        float heartRate = -1.0f;

        if (cb->getParams().dauDataTypes & 1<<DAU_DATA_TYPE_ECG) {
            while (frameNum >= 0 && heartRate < 0.0f) {
                auto *header = reinterpret_cast<CineBuffer::CineFrameHeader *>(
                        cb->getFrame(frameNum, DAU_DATA_TYPE_ECG,
                                     CineBuffer::FrameContents::IncludeHeader));

                if (header)
                    heartRate = header->heartRate;

                frameNum -= 10;
            }
        }

        mWorkflow->setHeartRate(heartRate);
    }

    auto &viewData = mView == CardiacView::A2C ? mWorkflow->mAIa2c : mWorkflow->mAIa4c;
    const auto& userViewData = mView == CardiacView::A2C ? mWorkflow->mUsera2c : mWorkflow->mUsera4c;
    const int* edFrames = nullptr;
    const int* esFrames = nullptr;
    const int* frames = nullptr;
    int numCycles = 0;
    if (mIsUserEdit) {
        edFrames = &userViewData.frames.edFrame;
        esFrames = &userViewData.frames.esFrame;
        numCycles = 1;
    } else {
        edFrames = viewData.frames.edFrames;
        esFrames = viewData.frames.esFrames;
        numCycles = viewData.frames.numCycles;
    }
    frames = mIsEDFrame ? edFrames : esFrames;

    CycleSegmentation segs[MultiCycleFrames::MAX_CYCLES];
    for (int cycle=0; cycle != numCycles; ++cycle) {
        auto& cycleSeg = segs[cycle];
        cycleSeg.edFrame = edFrames[cycle];
        cycleSeg.esFrame = esFrames[cycle];
        int frame = frames[cycle];
        LOGD("Segmenting frame %d", frame);
        err = segment(frame, viewData.segmentations);
        if (IS_THOR_ERROR(err)) { mWorkflow->notifyError(mUser, err); return; }
    }

    bool hasAllSegmentations = true;
    for (int cycle = 0; cycle != numCycles; ++cycle)
    {
        auto& cycleSeg = segs[cycle];
        // DANGER! These are pointers into a std::unordered_map
        // Any alteration to the map will invalidate these pointers
        // That is why this is a separate loop from above:
        // the above loop adds to the viewData.segmentations map, which would invalidate previous pointers
        auto it = viewData.segmentations.find(cycleSeg.edFrame);
        if (it != viewData.segmentations.end()) {
            cycleSeg.ed = &(it->second);
        } else {
            hasAllSegmentations = false;
        }
        it = viewData.segmentations.find(cycleSeg.esFrame);
        if (it != viewData.segmentations.end()) {
            cycleSeg.es = &(it->second);
        } else {
            hasAllSegmentations = false;
        }
    }

    // Check for uncertainty error here, rather than during EF calculation
    if (hasAllSegmentations)
    {
        auto stats = mWorkflow->computeViewStats(numCycles, segs, 0.0f, mView, &err);
        UNUSED(stats);
        if (err == TER_AI_LV_UNCERTAINTY_ERROR && !mIsUserEdit)
        {
            mWorkflow->notifyError(mUser, err);
            return;
        }
    }

    // Potentially force error
    {
        // Note: making use of missing key is default 0, which means no error here
        EFForcedError forcedError = GetEFForcedErrorEnum(mUser);
        if (ErrorAppliesToView(forcedError, mView) && IsSegmentationError(forcedError)) {
            ThorStatus forced = GetThorStatusError(forcedError);
            LOGD("Forcing error 0x%08x", forced);
            ClearEFForcedError(mUser);
            mWorkflow->notifyError(mUser, forced);
            return;
        }
    }

    mWorkflow->notifySegmentationComplete(mUser, mView, mFrame);
}

ThorStatus SegmentationTask::runImmediate(void *jenv, const char* debugOut)
{
    ThorStatus err;
    setDebugOutPath(debugOut);
    auto *cb = getCineBuffer();

    // need to pull heart rate when we are looking at a4c
    if (mView == CardiacView::A4C && !mIsEDFrame)
    {
        auto frameNum = mFrame;
        float heartRate = -1.0f;

        if (cb->getParams().dauDataTypes & 1<<DAU_DATA_TYPE_ECG) {
            while (frameNum >= 0 && heartRate < 0.0f) {
                auto *header = reinterpret_cast<CineBuffer::CineFrameHeader *>(
                        cb->getFrame(frameNum, DAU_DATA_TYPE_ECG,
                                     CineBuffer::FrameContents::IncludeHeader));

                if (header)
                    heartRate = header->heartRate;

                frameNum -= 10;
            }
        }

        mWorkflow->setHeartRate(heartRate);
    }

    auto &viewData = mView == CardiacView::A2C ? mWorkflow->mAIa2c : mWorkflow->mAIa4c;
    const auto& userViewData = mView == CardiacView::A2C ? mWorkflow->mUsera2c : mWorkflow->mUsera4c;
    const int* frames = nullptr;
    int numCycles = 0;
    if (mIsUserEdit) {
        frames = mIsEDFrame ? &userViewData.frames.edFrame : &userViewData.frames.esFrame;
        numCycles = 1;
    } else {
        frames = mIsEDFrame ? viewData.frames.edFrames : viewData.frames.esFrames;
        numCycles = viewData.frames.numCycles;
    }

    for (int cycle=0; cycle != numCycles; ++cycle) {
        int frame = frames[cycle];
        LOGD("Segmenting frame %d", frame);
        err = segment(frame, viewData.segmentations, jenv);
        if (IS_THOR_ERROR(err)) { mWorkflow->notifyError(mUser, err); return err; }
    }
    
    return err;
}
void SegmentationTask::setDebugOutPath(const char *debugOut) {
    mDebugOutPath = debugOut;
}
EFCalculationTask::EFCalculationTask(AIManager *manager, void *user, EFWorkflow *workflow) :
    AITask(manager),
    mUser(user), mWorkflow(workflow) {}

void EFCalculationTask::run()
{
    // TODO change this so that computeStats can return an error/warning (about mismatched lengths...)
    ThorStatus status = mWorkflow->computeStats();
    if (status != THOR_OK && status != TER_AI_LV_UNCERTAINTY_ERROR) {
        // Never send uncertainty errors from EFCalculation, they should now only ever come from segmentation
        mWorkflow->notifyError(mUser, status);
        return;
    }
    // notify that the task is complete
    auto stats = mWorkflow->getStatistics();
    if (stats.originalA4cStats.edVolume < 0 || stats.originalA2cStats.edVolume < 0 || stats.originalBiplaneStats.edVolume < 0)
    {
        LOGE("No volume computed for some view, uncertainty too high?");
        mWorkflow->notifyError(mUser, TER_AI_SEGMENTATION);
        return;
    }

    // Potentially force error
    {
        // Note: making use of missing key is default 0, which means no error here
        EFForcedError forcedError = GetEFForcedErrorEnum(mUser);
        if (forcedError == EF_A4C_Foreshortened && stats.originalA2cStats.numCycles > 0) {
            LOGD("Forcing A4C foreshortened error");
            ClearEFForcedError(mUser);
            mWorkflow->notifyError(mUser, TER_AI_A4C_FORESHORTENED);
            return;
        } else if (forcedError == EF_A2C_Foreshortened && stats.originalA2cStats.numCycles > 0) {
            LOGD("Forcing A2C foreshortened error");
            ClearEFForcedError(mUser);
            mWorkflow->notifyError(mUser, TER_AI_A2C_FORESHORTENED);
            return;
        }
    }

    mWorkflow->notifyStatisticsComplete(mUser);
}

std::unique_ptr<AITask> EFWorkflow::createFrameIDTask(void *user, CardiacView view)
{
    return std::unique_ptr<AITask>(new FrameIDTask(mManager, user, this, view));
}

std::unique_ptr<AITask> EFWorkflow::createSegmentationTask(void *user, CardiacView view, int frame)
{
    // determine if frame is ED or ES
    bool isEDFrame = true;
    if ( (view == CardiacView::A2C && mAIa2c.frames.esFrames[0] == frame) ||
         (view == CardiacView::A2C && mUsera2c.frames.esFrame == frame) ||
         (view == CardiacView::A4C && mAIa4c.frames.esFrames[0] == frame) ||
         (view == CardiacView::A4C && mUsera4c.frames.esFrame == frame) )
        isEDFrame = false;

    // determine if frame is a user edit frame or not
    bool isUserEdit = false;
    if (view == CardiacView::A2C) {
        if (isEDFrame)
            isUserEdit = frame == mUsera2c.frames.edFrame;
        else
            isUserEdit = frame == mUsera2c.frames.esFrame;
    } else {
        if (isEDFrame)
            isUserEdit = frame == mUsera4c.frames.edFrame;
        else
            isUserEdit = frame == mUsera4c.frames.esFrame;
    }

    return std::unique_ptr<AITask>(new SegmentationTask(mManager, user, this, view, frame, isEDFrame, isUserEdit));
}

std::unique_ptr<AITask> EFWorkflow::createEFCalculationTask(void *user)
{
    computeStats();
    return std::unique_ptr<AITask>(new EFCalculationTask(mManager, user, this));
}

