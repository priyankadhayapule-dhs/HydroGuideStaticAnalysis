//
//  PWAnnotator.cpp
//  EchoNous
//
//  Created by Tim Crossley on 9/19/23.
//
#define LOG_TAG "AutoDopplerPWAnnotator"

#include "ThorUtils.h"
#include "AutoDopplerPWAnnotator.h"
#include "MLOperations.h"
#include "CardiacAnnotatorTask.h"

PWAnchorInfo PW_ANNOTATOR_ANCHOR_INFO = { 14, 224/14, {{16.0f, 32.0f}, {30.0f, 20.0f}, {33.0f, 58.0f}}};
PWAnnotatorParams PW_ANNOTATOR_PARAMS;
const bool PW_ANNOTATOR_VIEW_LABEL_MASK[PW_ANNOTATOR_CLASSIFIER_NUM_CLASSES][PW_ANNOTATOR_LABELER_NUM_CLASSES] = {
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
const char *PW_ANNOTATOR_VIEW_NAMES[PW_ANNOTATOR_CLASSIFIER_NUM_CLASSES] = {
    "A2C-OPTIMAL","A2C-SUBVIEW","A3C-OPTIMAL","A3C-SUBVIEW","A4C-OPTIMAL","A4C-SUBVIEW","A5C",
    "PLAX-OPTIMAL","PLAX-SUBVIEW","RVOT","RVIT","PSAX-AV","PSAX-MV","PSAX-PM","PSAX-AP",
    "SUBCOSTAL-4C-OPTIMAL","SUBCOSTAL-4C-SUBVIEW","SUBCOSTAL-IVC","SUPRASTERNAL","OTHER"
};
const char *PW_ANNOTATOR_LABEL_NAMES_WITH_VIEW[PW_ANNOTATOR_LABELER_NUM_CLASSES] = {
    "AO", "AL PAP", "AO ARCH", "AV", "DA", "IAS", "IVC", "IVS", "LA", "LIVER",
    "LV", "LVOT", "MPA", "MV", "PM PAP", "PV", "RA", "RV", "RVOT", "TV"
};
const char *PW_ANNOTATOR_LABEL_NAMES[PW_ANNOTATOR_LABELER_NUM_CLASSES] = {
    "AO", "AL PAP", "AO ARCH", "AV", "DA", "IAS", "IVC", "IVS", "LA", "LIVER",
    "LV", "LVOT", "MPA", "MV", "PM PAP", "PV", "RA", "RV", "RVOT", "TV"
};

ThorStatus AutoDopplerPWAnnotator::init()
{
    mModel = nullptr; // TODO:: init JNI
    LOGD("PWModel: %p", mModel);
    return THOR_OK;
}
void AutoDopplerPWAnnotator::close()
{
    mModel = NULL;
}

ThorStatus AutoDopplerPWAnnotator::annotate(const unsigned char *postscan, std::vector<float> &view, std::vector<PWYoloPrediction> &predictions)
{
    /*@autoreleasepool {
        CVPixelBufferRef input;
        CVPixelBufferCreateWithBytes(
                                     NULL,
                                     PW_ANNOTATOR_INPUT_WIDTH,
                                     PW_ANNOTATOR_INPUT_HEIGHT,
                                     kCVPixelFormatType_OneComponent8,
                                     (void*)postscan,
                                     PW_ANNOTATOR_INPUT_WIDTH,
                                     NULL, NULL, NULL, &input);
        
        NSError *error = NULL;
        Autolabel_PWOutput *output = [mModel predictionFromInput:input error:&error];
        CVPixelBufferRelease(input);
        if (output == nil)
        {
            LOGE("PW model output is nil!");
            NSLog(@"PW model output is nil, error: %@", error);
            return TER_AI_MODEL_RUN;
        }
        
        processClassLayer(output.view_out, view);
        processYoloLayer(output.yolo_out, PW_ANNOTATOR_ANCHOR_INFO, predictions);
    }*/
    
    return THOR_OK;
}
ThorStatus AutoDopplerPWAnnotator::annotate(const unsigned char *postscan, std::vector<float> &view, std::vector<PWYoloPrediction> &predictions, PWVerificationResult &result)
{
    /*@autoreleasepool {
        CVPixelBufferRef input;
        CVPixelBufferCreateWithBytes(
                                     NULL,
                                     PW_ANNOTATOR_INPUT_WIDTH,
                                     PW_ANNOTATOR_INPUT_HEIGHT,
                                     kCVPixelFormatType_OneComponent8,
                                     (void*)postscan,
                                     PW_ANNOTATOR_INPUT_WIDTH,
                                     NULL, NULL, NULL, &input);
        
        NSError *error = NULL;
        Autolabel_PWOutput *output = [mModel predictionFromInput:input error:&error];
        CVPixelBufferRelease(input);
        if (output == nil)
        {
            LOGE("PW model output is nil!");
            NSLog(@"PW model output is nil, error: %@", error);
            return TER_AI_MODEL_RUN;
        }
        
        [output.view_out getBytesWithHandler:^(const void *bytes, NSInteger size) {
             const float *ptr = reinterpret_cast<const float*>(bytes);
             const size_t stride = output.view_out.strides[1].unsignedLongValue;
             
             for (size_t i=0; i != output.view_out.count; ++i) {
                result.viewRaw.push_back(ptr[i*stride]);
             }
         }];
                
        processClassLayer(output.view_out, view);
        processYoloLayer(output.yolo_out, PW_ANNOTATOR_ANCHOR_INFO, predictions, result);
        */
        
        // Append predicted view and labels to json
        Json::Value frame = Json::Value(Json::objectValue);
        Json::Value viewPred = Json::Value(Json::arrayValue);
        for (float f : view) { viewPred.append(f); }
        frame["viewPred"] = viewPred;
        for (const PWYoloPrediction &pred : predictions) {
            Json::Value jsonPred;
            jsonPred["class_pred"] = pred.label;
            jsonPred["conf"] = pred.conf;
            jsonPred["x"] = pred.x;
            jsonPred["y"] = pred.y;
            jsonPred["w"] = pred.w;
            jsonPred["h"] = pred.h;
            frame["predictions"].append(jsonPred);
        }
        result.json["frames"].append(frame);
   // }
    
    return THOR_OK;
}

void AutoDopplerPWAnnotator::processClassLayer(std::vector<float> viewIn, std::vector<float> &viewOut)
{
    // Apply softmax to view
    viewOut.resize(viewIn.size());
    /*[viewIn getBytesWithHandler:^(const void *bytes, NSInteger size) {
        const float *ptr = reinterpret_cast<const float*>(bytes);
        const size_t stride = viewIn.strides[1].unsignedLongValue;
        
        for (size_t i=0; i != viewOut.size(); ++i) {
            viewOut[i] = ptr[i*stride];
        }
    }];*/
    softmax(viewOut.begin(), viewOut.end());
}

static float sigmoid(float v) { return 1.0f/(1.0f+std::exp(-v)); }

void AutoDopplerPWAnnotator::processYoloLayer(std::vector<float> yoloIn, const PWAnchorInfo &info, std::vector<PWYoloPrediction> &predictions)
{
    // For step 3: use predictions as a vector where predictions[i] = best conf prediction for label class i
    predictions.resize(PW_ANNOTATOR_LABELER_NUM_CLASSES);
    for (PWYoloPrediction &pred : predictions)
    {
        pred.conf = -1.0f;
    }

    const float *ptr = (const float*)yoloIn.data();
    const size_t yStride = 5; // yoloIn.strides[2].unsignedLongValue;
    const size_t xStride = 5; // yoloIn.strides[3].unsignedLongValue;
    const size_t boxStride = 5; // yoloIn.strides[1].unsignedLongValue;
    const size_t aStride = boxStride * (5+PW_ANNOTATOR_LABELER_NUM_CLASSES);
    for (size_t y=0; y != info.fsize; ++y)
    {
        for (size_t x=0; x != info.fsize; ++x)
        {
            for (size_t a=0; a != PW_ANNOTATOR_LABELER_NUM_ANCHORS; ++a)
            {
                const float *base = ptr + y*yStride + x*xStride + a*aStride;
                
                // Step 1: Apply sigmoid and transform x, y, w, h from local (grid cell) coords into global pixel coords
                float tx = base[0 * boxStride];
                float ty = base[1 * boxStride];
                float tw = base[2 * boxStride];
                float th = base[3 * boxStride];
                
                float bx = (x + sigmoid(tx) * 2.0f - 0.5f) * info.stride;
                float by = (y + sigmoid(ty) * 2.0f - 0.5f) * info.stride;
                float sw = sigmoid(tw);
                float bw = 4 * sw * sw * info.anchors[a][0];
                float sh = sigmoid(th);
                float bh = 4 * sh * sh * info.anchors[a][1];
                
                // Step 2: Find object and label confidence, multiply to get total confidence, and check vs threshold
                float objConf = sigmoid(base[4 * boxStride]);
                size_t maxLabelClass = 0;
                float maxLabelConf = -1.0f;
                for (size_t label=0; label != PW_ANNOTATOR_LABELER_NUM_CLASSES; ++label)
                {
                    float labelConf = sigmoid(base[(5+label) * boxStride]);
                    if (labelConf > maxLabelConf)
                    {
                        maxLabelConf = labelConf;
                        maxLabelClass = label;
                    }
                }
                float conf = objConf * maxLabelConf;
                if (conf > PW_ANNOTATOR_PARAMS.label_thresholds[maxLabelClass])
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
    const auto unpredicted = [](const PWYoloPrediction &p) { return p.conf < 0.0f; };
    predictions.erase(std::remove_if(predictions.begin(), predictions.end(), unpredicted), predictions.end());
}

void AutoDopplerPWAnnotator::processYoloLayer(std::vector<float> yoloIn, const PWAnchorInfo &info, std::vector<PWYoloPrediction> &predictions, PWVerificationResult &result)
{
    // For step 3: use predictions as a vector where predictions[i] = best conf prediction for label class i
    predictions.resize(PW_ANNOTATOR_LABELER_NUM_CLASSES);
    for (PWYoloPrediction &pred : predictions)
    {
        pred.conf = -1.0f;
    }
    
    auto index = [](size_t y, size_t x, size_t a, size_t ch) {
        return a*(5+PW_ANNOTATOR_LABELER_NUM_CLASSES)*14*14 + ch * 14 + y * 14 + x;
    };

    const float *ptr = (const float*)yoloIn.data();
    const size_t yStride = 1;//yoloIn.strides[2].unsignedLongValue;
    const size_t xStride = 5;//yoloIn.strides[3].unsignedLongValue;
    const size_t boxStride = 6;//yoloIn.strides[1].unsignedLongValue;
    const size_t aStride = boxStride * (5+PW_ANNOTATOR_LABELER_NUM_CLASSES);
    
    const size_t yoloSize = yoloIn.size();
    result.yoloRaw.resize(result.yoloRaw.size() + yoloSize);
    result.yoloProc.resize(result.yoloProc.size() + yoloSize);
    float *yoloRawBase = result.yoloRaw.data() + result.yoloRaw.size() - yoloSize;
    float *yoloProcBase = result.yoloProc.data() + result.yoloProc.size() - yoloSize;
    for (size_t a=0; a != PW_ANNOTATOR_LABELER_NUM_ANCHORS; ++a)
    {
        for (size_t x=0; x != info.fsize; ++x)
        {
            for (size_t y=0; y != info.fsize; ++y)
            {
                const float *base = ptr + y*yStride + x*xStride + a*aStride;
                
                // Step 1: Apply sigmoid and transform x, y, w, h from local (grid cell) coords into global pixel coords
                float tx = base[0 * boxStride];
                float ty = base[1 * boxStride];
                float tw = base[2 * boxStride];
                float th = base[3 * boxStride];
                
                float bx = (x + sigmoid(tx) * 2.0f - 0.5f) * info.stride;
                float by = (y + sigmoid(ty) * 2.0f - 0.5f) * info.stride;
                float sw = sigmoid(tw);
                float bw = 4 * sw * sw * info.anchors[a][0];
                float sh = sigmoid(th);
                float bh = 4 * sh * sh * info.anchors[a][1];
                
                // save out x, y, w, h
                yoloRawBase[index(y, x, a, 0)] = tx;
                yoloRawBase[index(y, x, a, 1)] = ty;
                yoloRawBase[index(y, x, a, 2)] = tw;
                yoloRawBase[index(y, x, a, 3)] = th;
                
                yoloProcBase[index(y, x, a, 0)] = bx;
                yoloProcBase[index(y, x, a, 1)] = by;
                yoloProcBase[index(y, x, a, 2)] = bw;
                yoloProcBase[index(y, x, a, 3)] = bh;
                
                // Step 2: Find object and label confidence, multiply to get total confidence, and check vs threshold
                float objConf = sigmoid(base[4 * boxStride]);
                yoloRawBase[index(y,x,a,4)] = base[4 * boxStride];
                yoloProcBase[index(y, x, a, 4)] = objConf;
                size_t maxLabelClass = 0;
                float maxLabelConf = -1.0f;
                for (size_t label=0; label != PW_ANNOTATOR_LABELER_NUM_CLASSES; ++label)
                {
                    float labelConf = sigmoid(base[(5+label) * boxStride]);
                    yoloRawBase[index(y, x, a, 5+label)] = base[(5+label) * boxStride];
                    yoloProcBase[index(y, x, a, 5+label)] = labelConf;
                    if (labelConf > maxLabelConf)
                    {
                        maxLabelConf = labelConf;
                        maxLabelClass = label;
                    }
                }
                float conf = objConf * maxLabelConf;
                if (conf > PW_ANNOTATOR_PARAMS.base_label_threshold + PW_ANNOTATOR_PARAMS.label_thresholds[maxLabelClass])
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
    const auto unpredicted = [](const PWYoloPrediction &p) { return p.conf < 0.0f; };
    predictions.erase(std::remove_if(predictions.begin(), predictions.end(), unpredicted), predictions.end());
}
