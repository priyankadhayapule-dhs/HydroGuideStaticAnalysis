//
// Copyright 2020 EchoNous Inc.
//
#define LOG_TAG "YoloBuffer"
#include <YoloBuffer.h>
#include <CardiacAnnotatorConfig.h>
#include <ThorUtils.h>
#include <MLOperations.h>

constexpr float YOLO_BUFFER_FRAC = 0.1f;
constexpr float VIEW_THRESH = 0.1f;

YoloBuffer::YoloBuffer(size_t length) :
	mYoloBuffer(length),
	mViewBuffer(length),
	mPredIndex(0),
	mSmoothPreds()
{}

void YoloBuffer::append(const std::vector<float> &view, const std::vector<YoloPrediction> &preds)
{
	auto &framePreds = mYoloBuffer[mPredIndex];
	auto &frameView = mViewBuffer[mPredIndex];
	++mPredIndex;
	if (mPredIndex == mYoloBuffer.size())
		mPredIndex = 0;

	// Copy view values
	if (view.size() != CARDIAC_ANNOTATOR_CLASSIFIER_NUM_CLASSES)
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
	if (*std::max_element(mViewFlat.begin(), mViewFlat.end()) > CARDIAC_ANNOTATOR_PARAMS.view_threshold)
	{
		// Copy yolo predictions into buffer
		for (const auto &pred : preds)
			framePreds[pred.class_pred] = pred;
	}

	update();
}

std::vector<YoloPrediction>& YoloBuffer::get()
{
	return mSmoothPreds;
}

std::pair<int, float> YoloBuffer::getView()
{
	return std::make_pair(mSmoothView, mSmoothViewProb);
}

void YoloBuffer::updateView()
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

void YoloBuffer::clear()
{
	// Reset buffer contents to default
	std::fill(mYoloBuffer.begin(), mYoloBuffer.end(), std::array<YoloPrediction, CARDIAC_ANNOTATOR_LABELER_NUM_CLASSES>());
	std::fill(mViewBuffer.begin(), mViewBuffer.end(), std::array<float, CARDIAC_ANNOTATOR_CLASSIFIER_NUM_CLASSES>());
	mPredIndex = 0;
	mSmoothPreds.clear();
}

void YoloBuffer::update()
{
	mSmoothView = argmax(mViewFlat.begin(), mViewFlat.end());
	mSmoothViewProb = mViewFlat[mSmoothView];
	size_t fracFrames = mYoloBuffer.size() * CARDIAC_ANNOTATOR_PARAMS.label_buffer_frac;
	mSmoothPreds.clear();
	for (int label=0; label < CARDIAC_ANNOTATOR_LABELER_NUM_CLASSES; ++label)
	{
		if (CARDIAC_ANNOTATOR_VIEW_LABEL_MASK[mSmoothView][label] == false)
			continue;

		YoloPrediction temp = {};
		size_t frames = 0;
		for (size_t frame=0; frame < mYoloBuffer.size(); ++frame)
		{
			YoloPrediction &pred = mYoloBuffer[frame][label];
			if (pred.obj_conf * pred.class_conf > 0)
			{
				// prediction is valid for this frame
				++frames;
				temp.x += pred.x;
				temp.y += pred.y;
				temp.w += pred.w;
				temp.h += pred.h;
				temp.normx += pred.normx;
				temp.normy += pred.normy;
				temp.normw += pred.normw;
				temp.normh += pred.normh;
				temp.obj_conf += pred.obj_conf * pred.class_conf;
			}
		}

		if (frames > fracFrames)
		{
			// detected enough over the buffer
			temp.x /= frames;
			temp.y /= frames;
			temp.w /= frames;
			temp.h /= frames;
			temp.normx /= frames;
			temp.normy /= frames;
			temp.normw /= frames;
			temp.normh /= frames;
			temp.obj_conf /= frames;
			temp.class_pred = label;
			mSmoothPreds.push_back(temp);
		}
	}
}
