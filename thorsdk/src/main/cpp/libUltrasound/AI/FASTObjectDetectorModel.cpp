#define LOG_TAG "FASTObjectDetectorModel"

//
// Copyright 2021 EchoNous Inc.
//
#include <FASTObjectDetectorModel.h>
#include <UltrasoundManager.h>
#include <MLOperations.h>
#include <algorithm>

const char *FAST_OBJ_DETECT_MODEL_PATHS[] = {
        "FASTExam/FASTmodel_20220126-132719-yolo-v22-both-mid-spacev3.dlc",
        "FASTExam/FASTmodel_20220126-132715-yolo-v22-both-mid-spacev3.dlc",
        "FASTExam/FASTmodel_20220126-132644-yolo-v22-both-mid-spacev3.dlc",
        "FASTExam/FASTmodel_20220126-132624-yolo-v22-both-mid-spacev3.dlc",
};

FASTAnchorInfo FAST_OBJ_DETECT_ANCHOR_INFOS[1] = {
        {8, 16, {15.0/16, 32.6/16, 48.2/16}, {15.0/16, 31.7/16, 49.3/16}},
};

FASTObjectDetectorParams FAST_OBJ_DETECT_PARAMS;

const char *FAST_OBJ_DETECT_VIEW_NAMES[FAST_OBJ_DETECT_NUM_VIEWS] = {
        "noise", "SUP", "RUQ", "LUQ", "AS", "IVC", "Aorta", "SUB", "SUB2", "A4C", "A2C", "PLAX", "PSAX", "Renal", "Lung"
};
const char *FAST_OBJ_DETECT_VIEW_DISPLAY_NAMES[FAST_OBJ_DETECT_NUM_VIEWS] = {
        "noise", "SUP", "RUQ", "LUQ", "AS", "IVC", "Aorta", "Cardiac", "Cardiac", "Cardiac", "Cardiac", "Cardiac", "Cardiac", "Renal", "Lung"
};

const char *FAST_OBJ_DETECT_LABEL_NAMES_WITH_VIEW[FAST_OBJ_DETECT_NUM_OBJECTS] = {
        "liver", "spleen", "kidney", "diaphragm", "gallbladder", "heart", "ivc", "aorta", "bladder", "uterus", "prostate", "effusion", "hepatorenal", "splenorenal", "pleural", "pericardium", "recto"
};

const char *FAST_OBJ_DETECT_LABEL_NAMES[FAST_OBJ_DETECT_NUM_OBJECTS] = {
        "liver", "spleen", "kidney", "diaphragm", "gallbladder", "heart", "ivc", "aorta", "bladder", "uterus", "prostate", "effusion", "hepatorenal", "splenorenal", "pleural", "pericardium", "recto"
};

const bool FAST_OBJ_DETECT_VIEW_LABEL_MASK[FAST_OBJ_DETECT_NUM_VIEWS][FAST_OBJ_DETECT_NUM_OBJECTS] =
{
//         liver, spleen, kidney, diaphragm, gallbladder, heart, ivc,   aorta, bladder, uterus, prostate, effusion, hepatorenal, splenorenal, pleural, pericardium, recto
/*noise*/ {false, false,  false,  false,     false,       false, false, false, false,   false,  false,    false,    false,       false,       false,   false,       false},
/*SUP*/   {false, false,  false,  false,     false,       false, false, false, true ,   true ,  false,    false,    false,       false,       false,   false,       false},
/*RUQ*/   {true , false,  true ,  true ,     true ,       false, true , false, false,   false,  false,    false,    true ,       false,       true ,   false,       false},
/*LUQ*/   {false, true ,  true ,  true ,     false,       false, false, false, false,   false,  false,    false,    false,       true ,       true ,   false,       false},
/*AS*/    {true , false,  false,  false,     false,       false, true , true , false,   false,  false,    false,    false,       false,       false,   false,       false},
/*IVC*/   {true , false,  false,  false,     false,       false, true , false, false,   false,  false,    false,    false,       false,       false,   false,       false},
/*Aorta*/ {true , false,  false,  false,     false,       false, false, true , false,   false,  false,    false,    false,       false,       false,   false,       false},
/*SUB*/   {true , false,  false,  false,     false,       true , false, false, false,   false,  false,    false,    false,       false,       false,   true ,       false},
/*SUB2*/  {true , false,  false,  false,     false,       true , false, false, false,   false,  false,    false,    false,       false,       false,   true ,       false},
/*A4C*/   {false, false,  false,  false,     false,       true , false, false, false,   false,  false,    false,    false,       false,       false,   true ,       false},
/*A2C*/   {false, false,  false,  false,     false,       true , false, false, false,   false,  false,    false,    false,       false,       false,   true ,       false},
/*PLAX*/  {false, false,  false,  false,     false,       true , false, false, false,   false,  false,    false,    false,       false,       false,   true ,       false},
/*PSAX*/  {false, false,  false,  false,     false,       true , false, false, false,   false,  false,    false,    false,       false,       false,   true ,       false},
/*Renal*/ {false, false,  true ,  false,     false,       false, false, false, false,   false,  false,    false,    false,       false,       false,   false,       false},
/*Lung*/  {false, false,  false,  false,     false,       false, false, false, false,   false,  false,    false,    false,       false,       false,   false,       false},
};

FASTObjectDetectorParams::FASTObjectDetectorParams()
{
    reset();
}
void FASTObjectDetectorParams::reset()
{
#define OBJ_INDEX(name) std::distance(                      \
        std::begin(FAST_OBJ_DETECT_LABEL_NAMES_WITH_VIEW),     \
        std::find(                                               \
            std::begin(FAST_OBJ_DETECT_LABEL_NAMES_WITH_VIEW), \
            std::end(FAST_OBJ_DETECT_LABEL_NAMES_WITH_VIEW),   \
            name))
    base_label_threshold = 0.0f;
    std::fill(std::begin(label_thresholds), std::end(label_thresholds), 0.0f);

    label_thresholds[OBJ_INDEX("liver")]       = 0.25f;
    label_thresholds[OBJ_INDEX("spleen")]      = 0.25f;
    label_thresholds[OBJ_INDEX("kidney")]      = 0.20f;
    label_thresholds[OBJ_INDEX("diaphragm")]   = 0.15f;
    label_thresholds[OBJ_INDEX("gallbladder")] = 0.30f;
    label_thresholds[OBJ_INDEX("heart")]       = 0.20f;
    label_thresholds[OBJ_INDEX("ivc")]         = 0.20f;
    label_thresholds[OBJ_INDEX("aorta")]       = 0.20f;
    label_thresholds[OBJ_INDEX("bladder")]     = 0.30f;
    label_thresholds[OBJ_INDEX("uterus")]      = 0.30f;
    label_thresholds[OBJ_INDEX("prostate")]    = 0.90f;
    label_thresholds[OBJ_INDEX("effusion")]    = 0.90f;
    label_thresholds[OBJ_INDEX("hepatorenal")] = 0.10f;
    label_thresholds[OBJ_INDEX("splenorenal")] = 0.10f;
    label_thresholds[OBJ_INDEX("pleural")]     = 0.10f;
    label_thresholds[OBJ_INDEX("pericardium")] = 0.10f;
    label_thresholds[OBJ_INDEX("recto")]       = 0.10f;


    view_threshold = 0.20f;
    label_buffer_frac = 0.0f;
}

ThorStatus FASTObjectDetectorModel::init(UltrasoundManager &manager, const char *path)
{
    ThorStatus status = THOR_OK;
    //status = loadModel(manager, path);
    if (IS_THOR_ERROR(status))
        return status;
    status = createBuffers();
    if (IS_THOR_ERROR(status))
        return status;

    return THOR_OK;
}

void FASTObjectDetectorModel::close()
{
    /*mModel = nullptr;

    mInputMap.clear();
    mOutputMap.clear();

    mInputBuffer = nullptr;
    mX0Buffer = nullptr;
    mX1Buffer = nullptr;
*/
    mX0.clear();
    mX1.clear();
}
ThorStatus FASTObjectDetectorModel::postProcess(std::vector<float>& view, std::vector<float>& yolo, std::vector<float>& outView, std::vector<FASTYoloPrediction>& outYolo)
{
    ThorStatus status = THOR_OK;
    if (IS_THOR_ERROR(status)) { return status; }
    std::copy(view.begin(), view.end(), mX0.begin());
    status = processClassLayer(outView);
    if (IS_THOR_ERROR(status)) { return status; }

    status = processYoloLayer(yolo, FAST_OBJ_DETECT_ANCHOR_INFOS[0]);
    if (IS_THOR_ERROR(status)) { return status; }

    status = postprocessYoloOutput(yolo, outYolo, FAST_OBJ_DETECT_ANCHOR_INFOS[0]);
    if (IS_THOR_ERROR(status)) { return status; }

    return status;
}
ThorStatus FASTObjectDetectorModel::annotate(float *postscan, std::vector<float> &view, std::vector<FASTYoloPrediction> &predictions)
{
    ThorStatus status;
    status = setInput(postscan);
    if (IS_THOR_ERROR(status)) { return status; }

    status = runModel();
    if (IS_THOR_ERROR(status)) { return status; }

    status = processClassLayer(view);
    if (IS_THOR_ERROR(status)) { return status; }

    status = processYoloLayer(mX1, FAST_OBJ_DETECT_ANCHOR_INFOS[0]);
    if (IS_THOR_ERROR(status)) { return status; }

    status = postprocessYoloOutput(mX1, predictions, FAST_OBJ_DETECT_ANCHOR_INFOS[0]);
    if (IS_THOR_ERROR(status)) { return status; }

    return THOR_OK;
}


ThorStatus FASTObjectDetectorModel::setInput(float *postscan)
{

    // Sanity check, have we created the input buffer descriptor?
    /*if (!mInputBuffer)
        return THOR_ERROR;

    if (mInputBuffer->setBufferAddress(postscan))
        return THOR_OK;
    else
        return THOR_ERROR;*/
}

ThorStatus FASTObjectDetectorModel::setOutputs(float *x0, float *x1)
{
    /*// Sanity check, have we created the output buffer descriptors?
    if (!mX0Buffer || !mX1Buffer)
        return THOR_ERROR;

    if (!mX0Buffer->setBufferAddress(x0))
        return THOR_ERROR;
    if (!mX1Buffer->setBufferAddress(x1))
        return THOR_ERROR;
*/
    return THOR_OK;
}


ThorStatus FASTObjectDetectorModel::runModel()
{
   /* // Check to ensure the input maps have the required size
    if (mInputMap.size() != 1 || mOutputMap.size() != 2)
        return THOR_ERROR;

    if (mModel->execute(mInputMap, mOutputMap))
        return THOR_OK;
    else
    {
        LOGE("SNPE error: %s", zdl::DlSystem::getLastErrorString());
        return THOR_ERROR;
    }*/
   return THOR_OK;
}

ThorStatus FASTObjectDetectorModel::loadModel(UltrasoundManager &manager, const char* path)
{
    std::vector<unsigned char> dlcData;
    ThorStatus result = manager.getFilesystem()->readAsset(path, dlcData);
    if (IS_THOR_ERROR(result)) {
        LOGE("Failed to open FAST Object Detect model");
        return result;
    }
}

ThorStatus FASTObjectDetectorModel::createBuffers()
{
    // Create output buffers
    mX0.resize(FAST_OBJ_DETECT_NUM_VIEWS);
    mX1.resize(8*8*FAST_OBJ_DETECT_NUM_ANCHORS*FAST_OBJ_DETECT_NUM_CHANNELS);
    return THOR_OK;
}

ThorStatus FASTObjectDetectorModel::processClassLayer(std::vector<float> &view)
{
    // softmax the view logits

    view.resize(FAST_OBJ_DETECT_NUM_VIEWS);
    softmax(mX0.begin(), mX0.end(), view.begin());

    return THOR_OK;
}

static size_t YoloIndex(const FASTAnchorInfo &info, int y, int x, int a, int ch)
{
    /// SNPE Layout
    return
            y*info.fsize*FAST_OBJ_DETECT_NUM_ANCHORS*FAST_OBJ_DETECT_NUM_CHANNELS +
            x*FAST_OBJ_DETECT_NUM_ANCHORS*FAST_OBJ_DETECT_NUM_CHANNELS +
            a*FAST_OBJ_DETECT_NUM_CHANNELS +
            ch;

    /// TFLITE Layout
    /*const int X_STRIDE = 1;
    const int Y_STRIDE = info.fsize;
    const int ELEMENT_STRIDE = info.fsize * info.fsize;
    const int A_STRIDE = ELEMENT_STRIDE * FAST_OBJ_DETECT_NUM_CHANNELS;
    int baseOffset = x * X_STRIDE + y * Y_STRIDE + a * A_STRIDE;
    return
            baseOffset + ch * ELEMENT_STRIDE;*/
}

static float sigmoid(float f)
{
    return 1/(1+exp(-f));
}

ThorStatus FASTObjectDetectorModel::processYoloLayer(std::vector<float> &layer, const FASTAnchorInfo &info)
{
    const int X_CHANNEL = 0;
    const int Y_CHANNEL = 1;
    const int W_CHANNEL = 2;
    const int H_CHANNEL = 3;

    // elementstride is
    // layer is fsize x fsize x num_anchors x num_channels
    for (int y=0; y < info.fsize; ++y)
        for (int x=0; x < info.fsize; ++x)
            for (int a=0; a < FAST_OBJ_DETECT_NUM_ANCHORS; ++a)
            {
                // Apply sigmoid
                layer[YoloIndex(info,y,x,a,X_CHANNEL)] = sigmoid(layer[YoloIndex(info,y,x,a,X_CHANNEL)]);
                layer[YoloIndex(info,y,x,a,Y_CHANNEL)] = sigmoid(layer[YoloIndex(info,y,x,a,Y_CHANNEL)]);
                for (int ch=4; ch < FAST_OBJ_DETECT_NUM_CHANNELS; ++ch)
                {
                    layer[YoloIndex(info,y,x,a,ch)] = sigmoid(layer[YoloIndex(info,y,x,a,ch)]);
                    //printf("class_conf = %f\n", layer[YoloIndex(info,y,x,a,ch)]);
                }

                layer[YoloIndex(info,y,x,a,X_CHANNEL)] += x;
                layer[YoloIndex(info,y,x,a,Y_CHANNEL)] += y;
                layer[YoloIndex(info,y,x,a,W_CHANNEL)] = exp(layer[YoloIndex(info,y,x,a,W_CHANNEL)]) * info.w_anchors[a];
                layer[YoloIndex(info,y,x,a,H_CHANNEL)] = exp(layer[YoloIndex(info,y,x,a,H_CHANNEL)]) * info.h_anchors[a];
                for (int ch=0; ch < 4; ++ch)
                    layer[YoloIndex(info,y,x,a,ch)] *= info.stride;
            }

    return THOR_OK;
}


static void NonMaxSuppression(std::vector<FASTYoloPrediction> detections_class[FAST_OBJ_DETECT_NUM_OBJECTS], std::vector<FASTYoloPrediction> &predictions)
{
    // detections_class is a series of lists, one for each object class
    // predictions is the output list, passed as a reference (I think I did this so that the
    //   memory is reused from frame to frame).

    // for each object class:
    for (int obj_class=0; obj_class< FAST_OBJ_DETECT_NUM_OBJECTS; ++obj_class)
    {
        // Get the list of boxes for that class
        auto &bboxes = detections_class[obj_class];
        if (bboxes.empty())
            continue;

        // Find (a pointer to) the max conf box in that list
        auto max_score_it = std::max_element(bboxes.begin(), bboxes.end(),
                                             [](FASTYoloPrediction p1, FASTYoloPrediction p2) { return p1.obj_conf*p1.class_conf < p2.obj_conf*p2.class_conf;});
        float conf = max_score_it->obj_conf * max_score_it->class_conf;
        // Add to output list if conf is > 0.01
        if (conf > 0.01f)
            predictions.push_back(*max_score_it);
    }
}

ThorStatus FASTObjectDetectorModel::postprocessYoloOutput(const std::vector<float> &layer1, std::vector<FASTYoloPrediction> &predictions, const FASTAnchorInfo &info)
{
    std::vector<FASTYoloPrediction> detections_class[FAST_OBJ_DETECT_NUM_OBJECTS];
    return postprocessYoloOutputExt(layer1, detections_class, predictions, info);
}

#ifdef min
#undef min
#endif
#ifdef max
#undef max
#endif


static float YoloIOU(const FASTYoloPrediction &a, const FASTYoloPrediction &b) {
    const float intersectionWidth = std::max(std::min(a.x+a.w/2.0f, b.x+b.w/2.0f) - std::max(a.x-a.w/2.0f, b.x-b.w/2.0f), 0.0f);
    const float intersectionHeight = std::max(std::min(a.y+a.h/2.0f, b.y+b.h/2.0f) - std::max(a.y-a.h/2.0f, b.y-b.h/2.0f), 0.0f);
    const float intersection = intersectionWidth*intersectionHeight;
    const float unionArea = a.w*a.h + b.w*b.h - intersection;

    if (unionArea <= 0.0f)
        return 10000000000.0f;
    return intersection / unionArea;
}

ThorStatus FASTObjectDetectorModel::postprocessYoloOutputExt(const std::vector<float> &layer1,
                                                             std::vector<FASTYoloPrediction> (&detections_by_label)[FAST_OBJ_DETECT_NUM_OBJECTS], std::vector<FASTYoloPrediction> &predictions, const FASTAnchorInfo &info)
{
    // Skipping conversion to 4 corner format, as we are only printing the centers...
    for (size_t i=0; i < FAST_OBJ_DETECT_NUM_OBJECTS; ++i)
        detections_by_label[i].clear();

    // layer is fsize x fsize x 3 x 54 vector
    const std::vector<float> *(layers[1]) = {&layer1};
    for (const auto *layer_ptr : layers)
    {
        const auto &layer = *layer_ptr;
        size_t N = layer.size()/FAST_OBJ_DETECT_NUM_CHANNELS;
        LOGD("[ FAST ] N formerly: %d", N);
        for(int i = 0; i < N; ++i) {
            auto class_pred_ptr = std::max_element(&layer[i * FAST_OBJ_DETECT_NUM_CHANNELS + 5],
                                                   &layer[(i + 1) *
                                                          FAST_OBJ_DETECT_NUM_CHANNELS]);
            int class_pred = static_cast<int>(class_pred_ptr -
                                              &layer[i * FAST_OBJ_DETECT_NUM_CHANNELS + 5]);
            float class_conf = *class_pred_ptr;

            float conf = layer[i * FAST_OBJ_DETECT_NUM_CHANNELS + 4] * class_conf;

            detections_by_label[class_pred].emplace_back(FASTYoloPrediction{
                    layer[i * FAST_OBJ_DETECT_NUM_CHANNELS + 0], // x
                    layer[i * FAST_OBJ_DETECT_NUM_CHANNELS + 1], // y
                    layer[i * FAST_OBJ_DETECT_NUM_CHANNELS + 2], // w
                    layer[i * FAST_OBJ_DETECT_NUM_CHANNELS + 3], // h
                    0, 0, 0, 0, // normalized coords (gets filled in later)
                    layer[i * FAST_OBJ_DETECT_NUM_CHANNELS + 4], // obj_conf
                    class_conf,
                    class_pred,
                    0.0f
            });
        }
    }

    NonMaxSuppression(detections_by_label, predictions);

    return THOR_OK;
}
