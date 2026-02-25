#define LOG_TAG "TDIAnnotator"
#include <ThorUtils.h>
#include <TDIAnnotator.h>
#include <MLOperations.h>
#include <UltrasoundManager.h>

const char *TDI_ANNOTATOR_MODEL_PATH = "2023_08_29_AutoLabel_TDI.dlc";
TDIAnchorInfo TDI_ANNOTATOR_ANCHOR_INFO = { 14, 224/14, {{16.0f, 32.0f}, {30.0f, 20.0f}, {33.0f, 58.0f}}};
TDIAnnotatorParams TDI_ANNOTATOR_PARAMS;
const bool TDI_ANNOTATOR_VIEW_LABEL_MASK[TDI_ANNOTATOR_CLASSIFIER_NUM_CLASSES][TDI_ANNOTATOR_LABELER_NUM_CLASSES] = {
        {false, false, false},
        {false, false, false},
        {false, false, false},
        {false, false, false},
        {true,  true,  true,},
        {true,  true,  true,},
        {true,  true,  true,},
        {false, false, false},
        {false, false, false},
        {false, false, false},
        {false, false, false},
        {false, false, false},
        {false, false, false},
        {false, false, false},
        {false, false, false},
        {false, false, false},
        {false, false, false},
        {false, false, false},
        {false, false, false},
        {false, false, false}

};

const char *TDI_ANNOTATOR_VIEW_NAMES[TDI_ANNOTATOR_CLASSIFIER_NUM_CLASSES] = {
        "A2C-OPTIMAL","A2C-SUBVIEW","A3C-OPTIMAL","A3C-SUBVIEW","A4C-OPTIMAL","A4C-SUBVIEW","A5C",
        "PLAX-OPTIMAL","PLAX-SUBVIEW","RVOT","RVIT","PSAX-AV","PSAX-MV","PSAX-PM","PSAX-AP",
        "SUBCOSTAL-4C-OPTIMAL","SUBCOSTAL-4C-SUBVIEW","SUBCOSTAL-IVC","SUPRASTERNAL","OTHER"
};
const char *TDI_ANNOTATOR_LABEL_NAMES_WITH_VIEW[TDI_ANNOTATOR_LABELER_NUM_CLASSES] = {"MVLA", "MVSA", "TVLA"};
const char *TDI_ANNOTATOR_LABEL_NAMES[TDI_ANNOTATOR_LABELER_NUM_CLASSES] = {"MVLA", "MVSA", "TVLA"};

ThorStatus TDIAnnotator::init(UltrasoundManager &manager)
{
    ThorStatus status;
    status = loadModel(manager);
    if (IS_THOR_ERROR(status))
        return status;
    status = createBuffers();
    if (IS_THOR_ERROR(status))
        return status;

    return THOR_OK;
}

void TDIAnnotator::close()
{

    mView.clear();
    mYolo.clear();
}

ThorStatus TDIAnnotator::annotate(float *postscan, std::vector<float> &view, std::vector<TDIYoloPrediction> &predictions)
{
    ThorStatus status;
    status = setInput(postscan);
    if (IS_THOR_ERROR(status)) { return status; }

    status = runModel();
    if (IS_THOR_ERROR(status)) { return status; }

    status = processClassLayer(view);
    if (IS_THOR_ERROR(status)) { return status; }

    status = processYoloLayer(mYolo, TDI_ANNOTATOR_ANCHOR_INFO, predictions);
    if (IS_THOR_ERROR(status)) { return status; }

    return THOR_OK;
}

ThorStatus TDIAnnotator::setInput(float *postscan)
{
    // Sanity check, have we created the input buffer descriptor?

    //if (mInputBuffer->setBufferAddress(postscan))
    return THOR_OK;
    //else
    //    return THOR_ERROR;
}

ThorStatus TDIAnnotator::setOutputs(float *view, float *yolo)
{
    return THOR_OK;
}

ThorStatus TDIAnnotator::runModel()
{
    return THOR_OK;
}

ThorStatus TDIAnnotator::loadModel(UltrasoundManager &manager)
{
    return THOR_OK;
}

ThorStatus TDIAnnotator::createBuffers()
{
    return THOR_OK;
}

ThorStatus TDIAnnotator::processClassLayer(std::vector<float> &view)
{
    // softmax the view logits
    view.resize(TDI_ANNOTATOR_CLASSIFIER_NUM_CLASSES);
    softmax(mView.begin(), mView.end(), view.begin());

    return THOR_OK;
}

static float sigmoid(float v) { return 1.0f/(1.0f+std::exp(-v)); }

ThorStatus TDIAnnotator::processYoloLayer(std::vector<float> &layer, const TDIAnchorInfo &info, std::vector<TDIYoloPrediction> &predictions)
{
    // For step 3: use predictions as a vector where predictions[i] = best conf prediction for label class i
    predictions.resize(TDI_ANNOTATOR_LABELER_NUM_CLASSES);
    for (TDIYoloPrediction &pred : predictions)
    {
        pred.conf = -1.0f;
    }

    const float *ptr = layer.data();
    const size_t yStride = info.fsize * TDI_ANNOTATOR_LABELER_NUM_ANCHORS * TDI_ANNOTATOR_LABELER_NUM_CHANNELS;
    const size_t xStride = TDI_ANNOTATOR_LABELER_NUM_ANCHORS * TDI_ANNOTATOR_LABELER_NUM_CHANNELS;
    const size_t aStride = TDI_ANNOTATOR_LABELER_NUM_CHANNELS;
    for (size_t y=0; y != info.fsize; ++y)
    {
        for (size_t x=0; x != info.fsize; ++x)
        {
            for (size_t a=0; a != TDI_ANNOTATOR_LABELER_NUM_ANCHORS; ++a)
            {
                const float *base = ptr + y*yStride + x*xStride + a*aStride;

                // Step 1: Apply sigmoid and transform x, y, w, h from local (grid cell) coords into global pixel coords
                float tx = base[0];
                float ty = base[1];
                float tw = base[2];
                float th = base[3];

                float bx = (x + sigmoid(tx) * 2.0f - 0.5f) * info.stride;
                float by = (y + sigmoid(ty) * 2.0f - 0.5f) * info.stride;
                float sw = sigmoid(tw);
                float bw = sw * sw * info.anchors[a][0];
                float sh = sigmoid(th);
                float bh = sh * sh * info.anchors[a][1];

                // Step 2: Find object and label confidence, multiply to get total confidence, and check vs threshold
                float objConf = sigmoid(base[4]);
                size_t maxLabelClass = 0;
                float maxLabelConf = -1.0f;
                for (size_t label=0; label != TDI_ANNOTATOR_LABELER_NUM_CLASSES; ++label)
                {
                    float labelConf = sigmoid(base[5+label]);
                    if (labelConf > maxLabelConf)
                    {
                        maxLabelConf = labelConf;
                        maxLabelClass = label;
                    }
                }
                float conf = objConf * maxLabelConf;
                if (conf > TDI_ANNOTATOR_PARAMS.label_thresholds[maxLabelClass])
                {
                    // Step 3 (Simple NMS): Only select label with highest conf per class
                    if (conf > predictions[maxLabelClass].conf)
                    {
                        predictions[maxLabelClass] = { bx, by, bw, bh, conf, (int)maxLabelClass };
                    }
                }
            }
        }
    }
    // Finally, erase any unpredicted labels (conf < 0)
    const auto unpredicted = [](const TDIYoloPrediction &p) { return p.conf < 0.0f; };
    predictions.erase(std::remove_if(predictions.begin(), predictions.end(), unpredicted), predictions.end());
    return THOR_OK;
}