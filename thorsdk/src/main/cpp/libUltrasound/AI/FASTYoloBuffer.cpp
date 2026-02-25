//
// Copyright 2020 EchoNous Inc.
//
#define LOG_TAG "FASTYoloBuffer"
#include <FASTYoloBuffer.h>
#include <FASTObjectDetectionConfig.h>
#include <ThorUtils.h>
#include <MLOperations.h>
#include <cmath>

constexpr float YOLO_BUFFER_FRAC = 0.1f;
constexpr float VIEW_THRESH = 0.1f;

FASTYoloBuffer::FASTYoloBuffer(size_t yololength, size_t viewlength) :
        mFASTYoloBuffer(yololength),
        mViewBuffer(viewlength),
        mPredIndexYolo(0),
        mPredIndexView(0),
        mSizeYolo(0),
        mSizeView(0),
        mSmoothPreds()
{}

void FASTYoloBuffer::append(const std::vector<float> &view, const std::vector<FASTYoloPrediction> &preds)
{
    auto &framePreds = mFASTYoloBuffer[mPredIndexYolo];
    ++mPredIndexYolo;
    if (mPredIndexYolo == mFASTYoloBuffer.size())
        mPredIndexYolo = 0;
    if (mSizeYolo != mFASTYoloBuffer.size())
        ++mSizeYolo;

    auto &frameView = mViewBuffer[mPredIndexView];
    ++mPredIndexView;
    if (mPredIndexView == mViewBuffer.size())
        mPredIndexView = 0;
    if (mSizeView != mViewBuffer.size())
        ++mSizeView;

    // Copy view values
    if (view.size() != FAST_OBJ_DETECT_NUM_VIEWS)
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
    //if (*std::max_element(mViewFlat.begin(), mViewFlat.end()) > FAST_OBJ_DETECT_PARAMS.view_threshold)
    {
        // Copy yolo predictions into buffer
        for (const auto &pred : preds)
            framePreds[pred.class_pred] = pred;
    }

    update();
}

std::vector<FASTYoloPrediction>& FASTYoloBuffer::get()
{
    return mSmoothPreds;
}

std::array<float, FAST_OBJ_DETECT_NUM_VIEWS>& FASTYoloBuffer::getViewscores()
{
    return mViewFlat;
}

std::pair<int, float> FASTYoloBuffer::getView()
{
    return std::make_pair(mSmoothView, mSmoothViewProb);
}

void FASTYoloBuffer::updateView()
{
    // Determine view first
    std::fill(mViewFlat.begin(), mViewFlat.end(), 0.0f);
    // Accumulate to calculate mean
    float N = static_cast<float>(mSizeView);
    for (size_t frame=0; frame < mSizeView; ++frame)
    {
        std::transform(mViewFlat.begin(), mViewFlat.end(),
                       mViewBuffer[frame].begin(), mViewFlat.begin(),
                       [=](float acc, float f) {
            float value = acc + f/N;
            if (!std::isfinite(value)) {
                LOGE("Non finite value in view calc, frame = %zu, mSizeView = %zu", frame, mSizeView);
            }
            if (value < 0.0f) {
                LOGE("Negative value for viewscore: %f, frame = %zu, mSizeView = %zu", value, frame, mSizeView);
            }
            return value;
        });
    }
}

void FASTYoloBuffer::clear()
{
    // Reset buffer contents to default
    std::fill(mFASTYoloBuffer.begin(), mFASTYoloBuffer.end(), std::array<FASTYoloPrediction, FAST_OBJ_DETECT_NUM_OBJECTS>());
    std::fill(mViewBuffer.begin(), mViewBuffer.end(), std::array<float, FAST_OBJ_DETECT_NUM_VIEWS>());
    mPredIndexYolo = 0;
    mSizeYolo = 0;
    mPredIndexView = 0;
    mSizeView = 0;
    mSmoothPreds.clear();
}

void FASTYoloBuffer::update()
{
    mSmoothView = argmax(mViewFlat.begin(), mViewFlat.end());
    mSmoothViewProb = mViewFlat[mSmoothView];
    size_t fracFrames = mFASTYoloBuffer.size() * FAST_OBJ_DETECT_PARAMS.label_buffer_frac; /* 7 */
    mSmoothPreds.clear();
    // For each object class
    for (int label=0; label < FAST_OBJ_DETECT_NUM_OBJECTS; ++label)
    {
        // temp will be the average of the predicted box for the current object class
        FASTYoloPrediction temp = {};
        // sum of coord values for valid boxes (since temp holds weighted sum)
        temp.coords[0] = 0.0f;
        temp.coords[1] = 0.0f;
        temp.coords[2] = 0.0f;
        temp.coords[3] = 0.0f;
        // Sum of object confidences
        float conf_sum = 0.0f;
        // Number of frames the object is present in
        size_t frames=0;
        // For each frame
        for (size_t frame=0; frame < mSizeYolo; ++frame)
        {
            FASTYoloPrediction &pred = mFASTYoloBuffer[frame][label];
            // check if prediction is valid for this frame
            if (pred.obj_conf * pred.class_conf > 0)
            {
                // accumulate weighed sum into temp, also increment frames and conf_sum
                float conf = pred.obj_conf * pred.class_conf;
                ++frames;
                conf_sum += conf;
                temp.x += pred.x*conf;
                temp.y += pred.y*conf;
                temp.w += pred.w*conf;
                temp.h += pred.h*conf;
                temp.normx += pred.normx*conf;
                temp.normy += pred.normy*conf;
                temp.normw += pred.normw*conf;
                temp.normh += pred.normh*conf;
                temp.obj_conf += conf;

                temp.coords[0] += pred.x-pred.w/2;
                temp.coords[1] += pred.y-pred.h/2;
                temp.coords[2] += pred.x+pred.w/2;
                temp.coords[3] += pred.y+pred.h/2;
            }
        }

        // Compute stddev in second pass over frames
        temp.sumerrors[0] = 0.0f;
        temp.sumerrors[1] = 0.0f;
        temp.sumerrors[2] = 0.0f;
        temp.sumerrors[3] = 0.0f;
        for (float &c : temp.coords)
            c /= frames;
        for (size_t frame=0; frame < mSizeYolo; ++frame)
        {
            FASTYoloPrediction &pred = mFASTYoloBuffer[frame][label];
            if (pred.obj_conf * pred.class_conf > 0)
            {
                float errors[] = {
                        pred.x-pred.w/2-temp.coords[0],
                        pred.y-pred.h/2-temp.coords[1],
                        pred.x+pred.w/2-temp.coords[2],
                        pred.y+pred.h/2-temp.coords[3]};
                temp.sumerrors[0] += errors[0]*errors[0];
                temp.sumerrors[1] += errors[1]*errors[1];
                temp.sumerrors[2] += errors[2]*errors[2];
                temp.sumerrors[3] += errors[3]*errors[3];
            }
        }

        // If we found enough frames, normalize temp to be weighted average and compute std_dev
        if (frames > fracFrames)
        {
            // detected enough over the buffer
            temp.x /= conf_sum;
            temp.y /= conf_sum;
            temp.w /= conf_sum;
            temp.h /= conf_sum;
            temp.normx /= conf_sum;
            temp.normy /= conf_sum;
            temp.normw /= conf_sum;
            temp.normh /= conf_sum;
            temp.obj_conf /= mFASTYoloBuffer.size();
            temp.class_pred = label;
            temp.std_dev = std::sqrt(temp.sumerrors[0]/frames);
            temp.std_dev += std::sqrt(temp.sumerrors[1]/frames);
            temp.std_dev += std::sqrt(temp.sumerrors[2]/frames);
            temp.std_dev += std::sqrt(temp.sumerrors[3]/frames);
            temp.frames = frames;

            // store in result
            mSmoothPreds.push_back(temp);
        }
    }
}