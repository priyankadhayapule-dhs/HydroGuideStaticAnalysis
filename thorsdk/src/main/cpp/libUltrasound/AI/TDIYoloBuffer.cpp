//
// Copyright 2020 EchoNous Inc.
//
#define LOG_TAG "TDIYoloBuffer"
#include <TDIYoloBuffer.h>
#include <TDIAnnotatorConfig.h>
#include <ThorUtils.h>
#include <MLOperations.h>
constexpr float YOLO_BUFFER_FRAC = 0.4f;
constexpr float VIEW_THRESH = 0.4f;
TDIYoloBuffer::TDIYoloBuffer(size_t length) :
        mYoloBuffer(length),
        mViewBuffer(length),
        mPredIndex(0),
        mSmoothPreds()
{}

void TDIYoloBuffer::append(const std::vector<float> &view, const std::vector<TDIYoloPrediction> &preds)
{
    auto &framePreds = mYoloBuffer[mPredIndex];
    auto &frameView = mViewBuffer[mPredIndex];
    ++mPredIndex;
    if (mPredIndex == mYoloBuffer.size())
        mPredIndex = 0;
    // Copy view values
    if (view.size() != TDI_ANNOTATOR_CLASSIFIER_NUM_CLASSES)
    {
        LOGE("Incorrect size for autolabel view buffer: %lu\n", (unsigned long)view.size());
        mSmoothPreds.clear();
        return;
    }
    std::copy(view.begin(), view.end(), frameView.begin());
    updateView();
    // zero out old predictions
    for (auto &labelPred : framePreds)
        labelPred = {};
    // Only add predictions if view confidence is high enough
    if (*std::max_element(mViewFlat.begin(), mViewFlat.end()) > TDI_ANNOTATOR_PARAMS.view_threshold)
    {
        // Copy yolo predictions into buffer
        for (const auto &pred : preds)
            framePreds[pred.label] = pred;
    }
    update();
}

std::vector<TDIYoloPrediction>& TDIYoloBuffer::get()
{
    return mSmoothPreds;
}

std::pair<int, float> TDIYoloBuffer::getView()
{
    return std::make_pair(mSmoothView, mSmoothViewProb);
}

void TDIYoloBuffer::updateView()
{
    // Determine view first
    std::fill(mViewFlat.begin(), mViewFlat.end(), 0.0f);
    // Accumulate to calculate mean
    float N = mViewBuffer.size();
    for (size_t frame=0; frame < mViewBuffer.size(); ++frame)
    {
        std::transform(mViewFlat.begin(), mViewFlat.end(),
                       mViewBuffer[frame].begin(), mViewFlat.begin(),
                       [=](float acc, float f) { return acc + f/N;});
    }
}

void TDIYoloBuffer::clear()
{
    // Reset buffer contents to default
    std::fill(mYoloBuffer.begin(), mYoloBuffer.end(), std::array<TDIYoloPrediction, TDI_ANNOTATOR_LABELER_NUM_CLASSES>());
    std::fill(mViewBuffer.begin(), mViewBuffer.end(), std::array<float, TDI_ANNOTATOR_CLASSIFIER_NUM_CLASSES>());
    mPredIndex = 0;
    mSmoothPreds.clear();
}

void TDIYoloBuffer::update()
{
    mSmoothView = argmax(mViewFlat.begin(), mViewFlat.end());
    mSmoothViewProb = mViewFlat[mSmoothView];
    size_t fracFrames = mYoloBuffer.size() * TDI_ANNOTATOR_PARAMS.label_buffer_frac;
    mSmoothPreds.clear();
    for (int label=0; label < TDI_ANNOTATOR_LABELER_NUM_CLASSES; ++label)
    {
        if (TDI_ANNOTATOR_VIEW_LABEL_MASK[mSmoothView][label] == false)
            continue;
        TDIYoloPrediction temp = {};
        size_t frames = 0;
        for (size_t frame=0; frame < mYoloBuffer.size(); ++frame)
        {
            TDIYoloPrediction &pred = mYoloBuffer[frame][label];
            if (pred.conf > 0)
            {
                // prediction is valid for this frame
                ++frames;
                temp.x += pred.x;
                temp.y += pred.y;
                temp.w += pred.w;
                temp.h += pred.h;
                temp.conf += pred.conf;
            }
        }
        if (frames > fracFrames)
        {
            // detected enough over the buffer
            temp.x /= frames;
            temp.y /= frames;
            temp.w /= frames;
            temp.h /= frames;
            temp.conf /= frames;
            mSmoothPreds.push_back(temp);
        }
    }
}