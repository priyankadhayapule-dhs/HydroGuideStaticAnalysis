#define LOG_TAG "CardiacAnnotator"

//
// Copyright 2019 EchoNous Inc.
//
#include <CardiacAnnotator.h>
#include <UltrasoundManager.h>
#include <MLOperations.h>

const char *CARDIAC_ANNOTATOR_MODEL_PATH = "2022_06_10_AutoLabelV3-6.dlc";

AnchorInfo CARDIAC_ANNOTATOR_ANCHOR_INFOS[1] = {
    {14, 16, {16.0/16, 30.0/16, 33.0/16}, {32.0/16, 20.0/16, 58.0/16}},
};

AnnotatorParams CARDIAC_ANNOTATOR_PARAMS;

const char *CARDIAC_ANNOTATOR_VIEW_NAMES[CARDIAC_ANNOTATOR_CLASSIFIER_NUM_CLASSES] = {
        "A2C-OPTIMAL","A2C-SUBVIEW","A3C-OPTIMAL","A3C-SUBVIEW","A4C-OPTIMAL","A4C-SUBVIEW","A5C",
        "PLAX-OPTIMAL","PLAX-SUBVIEW","RVOT","RVIT","PSAX-AV","PSAX-MV","PSAX-PM","PSAX-AP",
        "SUBCOSTAL-4C-OPTIMAL","SUBCOSTAL-4C-SUBVIEW","SUBCOSTAL-IVC","SUPRASTERNAL","OTHER"
};

const char *CARDIAC_ANNOTATOR_LABEL_NAMES_WITH_VIEW[CARDIAC_ANNOTATOR_LABELER_NUM_CLASSES] = {
        "AO", "AL PAP", "AO ARCH", "AV", "DA", "IAS", "IVC", "IVS", "LA", "LIVER",
        "LV", "LVOT", "MPA", "MV", "PM PAP", "PV", "RA", "RV", "RVOT", "TV"
};

const char *CARDIAC_ANNOTATOR_LABEL_NAMES[CARDIAC_ANNOTATOR_LABELER_NUM_CLASSES] = {
        "AO", "AL PAP", "AO ARCH", "AV", "DA", "IAS", "IVC", "IVS", "LA", "LIVER",
        "LV", "LVOT", "MPA", "MV", "PM PAP", "PV", "RA", "RV", "RVOT", "TV"
};

const bool CARDIAC_ANNOTATOR_VIEW_LABEL_MASK[CARDIAC_ANNOTATOR_CLASSIFIER_NUM_CLASSES][CARDIAC_ANNOTATOR_LABELER_NUM_CLASSES] = 
{
        {false, false, false, false, false, false, false, false, true,  false, true,  false, false, true,  false, false, false, false, false, false},
        {false, false, false, false, false, false, false, false, true,  false, true,  false, false, true,  false, false, false, false, false, false},
        {true,  false, false, true,  false, false, false, false, true,  false, true,  true,  false, true,  false, false, false, false, false, false},
        {true,  false, false, true,  false, false, false, false, true,  false, true,  true,  false, true,  false, false, false, false, false, false},
        {false, false, false, false, false, true,  false, true,  true,  false, true,  false, false, true,  false, false, true,  true,  false, true},
        {true,  false, false, true,  false, true,  false, true,  true,  false, true,  true,  false, true,  false, false, true,  true,  false, true},
        {true,  false, false, true,  false, true,  false, true,  true,  false, true,  true,  false, true,  false, false, true,  true,  false, true},
        {true,  false, false, true,  false, false, false, true,  true,  false, true,  true,  false, true,  false, false, false, true,  false, false},
        {true,  false, false, true,  false, false, true,  true,  true,  false, true,  true,  true,  true,  false, true,  true,  true,  false, true},
        {false, false, false, false, false, false, false, true,  false, false, true,  false, true,  false, false, true,  false, false, true,  false},
        {false, false, false, false, false, false, true,  true,  false, false, true,  false, false, false, false, false, true,  true,  false, true},
        {false, false, false, true,  false, false, false, false, true,  false, false, false, true,  false, false, true,  true,  false, true,  true},
        {false, false, false, false, false, false, false, true,  false, false, true,  false, false, true,  false, false, false, true,  false, false},
        {false, true,  false, false, false, false, false, true,  false, false, true,  false, false, false, true,  false, false, true,  false, false},
        {false, false, false, false, false, false, false, true,  false, false, true,  false, false, false, false, false, false, true,  false, false},
        {false, false, false, false, false, true,  false, true,  true,  true,  true,  false, false, true,  false, false, true,  true,  false, true},
        {false, false, false, false, false, true,  false, true,  true,  true,  true,  false, false, true,  false, false, true,  true,  false, true},
        {false, false, false, false, false, false, true,  false, false, true,  false, false, false, false, false, false, false, false, false, false},
        {false, false, true,  false, true,  false, false, false, false, false, false, false, false, false, false, false, false, false, false, false},
        {false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false}
};

AnnotatorParams::AnnotatorParams()
{
    reset();
}
void AnnotatorParams::reset()
{
    #define VIEW_INDEX(name) std::distance(                      \
        std::begin(CARDIAC_ANNOTATOR_LABEL_NAMES_WITH_VIEW),     \
        std::find(                                               \
            std::begin(CARDIAC_ANNOTATOR_LABEL_NAMES_WITH_VIEW), \
            std::end(CARDIAC_ANNOTATOR_LABEL_NAMES_WITH_VIEW),   \
            name))
    base_label_threshold = 0.4f;
    std::fill(std::begin(label_thresholds), std::end(label_thresholds), 0.0f);

    label_thresholds[VIEW_INDEX("AO")] = 0.3f - base_label_threshold;
    label_thresholds[VIEW_INDEX("AV")] = 0.3f - base_label_threshold;
//    label_thresholds[VIEW_INDEX("LVOT")] = 0.2f - base_label_threshold;
    label_thresholds[VIEW_INDEX("MPA")] = 0.3f - base_label_threshold;
    label_thresholds[VIEW_INDEX("PV")] = 0.3f - base_label_threshold;

    view_threshold = 0.4f;
    label_buffer_frac = 0.4f;
}

ThorStatus CardiacAnnotator::init(UltrasoundManager &manager)
{
    ThorStatus status = THOR_OK;
    if (IS_THOR_ERROR(status))
        return status;
    status = createBuffers();
    if (IS_THOR_ERROR(status))
        return status;

    return THOR_OK;
}

void CardiacAnnotator::close()
{
    mX0.clear();
    mX1.clear();
}

ThorStatus CardiacAnnotator::createBuffers()
{
    // Create output buffers
    mX0.resize(CARDIAC_ANNOTATOR_CLASSIFIER_NUM_CLASSES);
    mX1.resize(14*14*CARDIAC_ANNOTATOR_LABELER_NUM_ANCHORS*CARDIAC_ANNOTATOR_LABELER_NUM_CHANNELS);

    return THOR_OK;
}

ThorStatus CardiacAnnotator::processClassLayer(std::vector<float> &view)
{
    // softmax the view logits

    view.resize(CARDIAC_ANNOTATOR_CLASSIFIER_NUM_CLASSES);
    softmax(mX0.begin(), mX0.end(), view.begin());

    return THOR_OK;
}

static size_t YoloIndex(const AnchorInfo &info, int y, int x, int a, int ch)
{
    return
        y*info.fsize*CARDIAC_ANNOTATOR_LABELER_NUM_ANCHORS*CARDIAC_ANNOTATOR_LABELER_NUM_CHANNELS +
        x*CARDIAC_ANNOTATOR_LABELER_NUM_ANCHORS*CARDIAC_ANNOTATOR_LABELER_NUM_CHANNELS +
        a*CARDIAC_ANNOTATOR_LABELER_NUM_CHANNELS +
        ch;
}

static float sigmoid(float f)
{
    return 1/(1+exp(-f));
}

ThorStatus CardiacAnnotator::processYoloLayer(std::vector<float> &layer, const AnchorInfo &info)
{
    // layer is fsize x fsize x num_anchors x num_channels
    for (int y=0; y < info.fsize; ++y)
        for (int x=0; x < info.fsize; ++x)
            for (int a=0; a < CARDIAC_ANNOTATOR_LABELER_NUM_ANCHORS; ++a)
            {
                // Apply sigmoid to all
                for (int ch=0; ch < CARDIAC_ANNOTATOR_LABELER_NUM_CHANNELS; ++ch)
                    layer[YoloIndex(info,y,x,a,ch)] = sigmoid(layer[YoloIndex(info,y,x,a,ch)]);

                // For x,y,w,h: multiply by 2
                for (int ch=0; ch < 4; ++ch)
                    layer[YoloIndex(info,y,x,a,ch)] *= 2;

                // For x, y: subtract by 0.5, add grid index, then multiply by stride
                layer[YoloIndex(info,y,x,a,0)] = (layer[YoloIndex(info,y,x,a,0)] - 0.5 + x) * info.stride;
                layer[YoloIndex(info,y,x,a,1)] = (layer[YoloIndex(info,y,x,a,1)] - 0.5 + y) * info.stride;

                // For w, h: square, then multiply by anchors
                layer[YoloIndex(info,y,x,a,2)] = (layer[YoloIndex(info,y,x,a,2)] * layer[YoloIndex(info,y,x,a,2)]) * info.w_anchors[a] * info.stride;
                layer[YoloIndex(info,y,x,a,3)] = (layer[YoloIndex(info,y,x,a,3)] * layer[YoloIndex(info,y,x,a,3)]) * info.h_anchors[a] * info.stride;
            }

    return THOR_OK;
}

static void NonMaxSuppression(std::vector<YoloPrediction> detections_class[CARDIAC_ANNOTATOR_LABELER_NUM_CLASSES], std::vector<YoloPrediction> &predictions)
{
    for (int obj_class=0; obj_class< CARDIAC_ANNOTATOR_LABELER_NUM_CLASSES; ++obj_class)
    {
        auto &bboxes = detections_class[obj_class];
        if (bboxes.empty())
            continue;
        auto max_score_it = std::max_element(bboxes.begin(), bboxes.end(),
            [](YoloPrediction p1, YoloPrediction p2) { return p1.obj_conf*p1.class_conf < p2.obj_conf*p2.class_conf;});
        predictions.push_back(*max_score_it);
    }
}

ThorStatus CardiacAnnotator::postprocessYoloOutput(const std::vector<float> &layer1, std::vector<YoloPrediction> &predictions)
{
    std::vector<YoloPrediction> detections_class[CARDIAC_ANNOTATOR_LABELER_NUM_CLASSES];
    return postprocessYoloOutputExt(layer1, detections_class, predictions);
}


ThorStatus CardiacAnnotator::postprocessYoloOutputExt(const std::vector<float> &layer1,
        std::vector<YoloPrediction> (&detections_by_label)[CARDIAC_ANNOTATOR_LABELER_NUM_CLASSES], std::vector<YoloPrediction> &predictions)
{
    // Skipping conversion to 4 corner format, as we are only printing the centers...
    for (size_t i=0; i < CARDIAC_ANNOTATOR_LABELER_NUM_CLASSES; ++i)
        detections_by_label[i].clear();

    // layer is fsize x fsize x 3 x 54 vector
    const std::vector<float> *(layers[1]) = {&layer1};
    for (const auto *layer_ptr : layers)
    {
        const auto &layer = *layer_ptr;
        size_t N = layer.size()/CARDIAC_ANNOTATOR_LABELER_NUM_CHANNELS;
        for (size_t i=0; i < N; ++i)
        {
            auto class_pred_ptr = std::max_element(&layer[i*CARDIAC_ANNOTATOR_LABELER_NUM_CHANNELS+5], &layer[(i+1)*CARDIAC_ANNOTATOR_LABELER_NUM_CHANNELS]);
            int class_pred = static_cast<int>(class_pred_ptr-&layer[i*CARDIAC_ANNOTATOR_LABELER_NUM_CHANNELS+5]);
            float class_conf = *class_pred_ptr;

            float conf = layer[i*CARDIAC_ANNOTATOR_LABELER_NUM_CHANNELS+4] * class_conf;
            if (conf < CARDIAC_ANNOTATOR_PARAMS.base_label_threshold + CARDIAC_ANNOTATOR_PARAMS.label_thresholds[class_pred])
                continue;

            detections_by_label[class_pred].emplace_back(YoloPrediction{
                layer[i*CARDIAC_ANNOTATOR_LABELER_NUM_CHANNELS+0], // x
                layer[i*CARDIAC_ANNOTATOR_LABELER_NUM_CHANNELS+1], // y
                layer[i*CARDIAC_ANNOTATOR_LABELER_NUM_CHANNELS+2], // w
                layer[i*CARDIAC_ANNOTATOR_LABELER_NUM_CHANNELS+3], // h
                0, 0, 0, 0, // normalized coords (gets filled in later)
                layer[i*CARDIAC_ANNOTATOR_LABELER_NUM_CHANNELS+4], // obj_conf
                class_conf,
                class_pred
            });
        }
    }

    NonMaxSuppression(detections_by_label, predictions);

    return THOR_OK;
}

