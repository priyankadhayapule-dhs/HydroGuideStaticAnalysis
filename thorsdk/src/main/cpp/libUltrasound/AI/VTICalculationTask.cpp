//
// Copyright 2023 EchoNous Inc.
//
// Implementation of VTI Trace Task

#define LOG_TAG "VTICalculationTask"

#include <chrono>
#include <thread>
#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>
#include <AIManager.h>
#include <VTICalculationTask.h>
#include <jni.h>

#define HALF_IMAGE_WIDTH 650
#define PW_IMG_HEIGHT 256
#define CW_IMG_HEIGHT 512
#define INPUT_IMG_SIZE 256

#define CTRL_POINT_SPAN_SEC 0.08f
#define SCALE_FACTOR (650.0f/256.0f)       // Half img / input/output img

#define MIN_PEAK_WIDTH 16                  // in terms of pixels

// TODO: UPDATE
float class_Uncertainty_threshold[] = {0.6, 0.6, 0.6, 0.6, 0.6, 0.6, 0.6, 0.6, 0.6};

//class_map = {0: 'PW-MV', 1: 'PW-TV', 2: 'CW-MV', 3: 'CW-TV', 4: 'CWr-MV_TV', 5: 'PW-other valves', 6: 'CW -Other valves',
//              7: 'CWr - Other valves', 8: 'PW-TDI'}
/////////////////


///// test
#include <ostream>
#include <fstream>
#include <string>
#include <sstream>
std::string gCurrentClipName;
void SaveProbMap(cv::Mat image, const char* name, unsigned int frameNum)
{
    std::stringstream path;
    path << "/data/local/thortests/EF_Verification_RAW/";
    //path << gCurrentClipName;
    //path << ((view == CardiacView::A4C) ? "_a4c_" : "_a2c_");
    path << "_frame" << frameNum << "_";
    path << name << ".raw";
    LOGD("Saving probmap in path %s", path.str().c_str());
    std::ofstream output(path.str(), std::ios::binary);

    // Warning: only works if the image is contiguous
    output.write((const char*)image.data, image.total() * image.elemSize());
}

//////


static void flatten(cv::InputArray input, cv::OutputArray output)
{
    cv::Mat probmap = input.getMat();
    cv::Size size = probmap.size();

    output.create(size, CV_8UC1);
    cv::Mat seg = output.getMat();

    for (int y=0; y < size.height; ++y)
        for (int x=0; x < size.width; ++x)
        {
            float ch0 = probmap.at<float[2]>(y,x)[0];
            float ch1 = probmap.at<float[2]>(y,x)[1];

            if (ch1 > ch0)
                seg.at<uint8_t>(y,x) = 1u;
            else
                seg.at<uint8_t>(y,x) = 0u;

        }
}

VTICalculationTask::VTICalculationTask(AIManager *manager, VTICalculation* vtiCalculation, uint32_t classId) :
    AITask(manager),
    mVTICal(vtiCalculation),
    mClassId(classId)
{
}

void VTICalculationTask::run()
{
    ThorStatus err;

    // run Ensemble
    runEnsemble(mClassId, mVTICal->getNativeJNI());

    // notify
    mVTICal->notifyComplete(THOR_OK);
}

// VTI run Ensemble
ThorStatus VTICalculationTask::runEnsemble(uint32_t classId, VtiNativeJni* jni)
{
    ThorStatus status = THOR_OK;

    auto *cb = getCineBuffer();
    auto params = cb->getParams();

    int inputFrameType, inputImgHeight;
    float baselineShiftFracton = 0.0f;

    if (params.imagingMode == IMAGING_MODE_CW) {
        inputFrameType = TEX_FRAME_TYPE_CW;
        inputImgHeight = CW_IMG_HEIGHT;
        params.upsReader->getCwBaselineShiftFraction(params.dopplerBaselineShiftIndex, baselineShiftFracton);
    }
    else
    {
        inputFrameType = TEX_FRAME_TYPE_PW;
        inputImgHeight = PW_IMG_HEIGHT;
        params.upsReader->getPwBaselineShiftFraction(params.dopplerBaselineShiftIndex, baselineShiftFracton);
    }
    // org input ptr
    uint8_t* processInPtr = cb->getFrame(inputFrameType, DAU_DATA_TYPE_TEX);
    uint8_t* processInPtr_patch2 = processInPtr + (inputImgHeight * HALF_IMAGE_WIDTH);

    // process first half (650) & 2nd half (650)
    cv::Mat input_8u_patch1(HALF_IMAGE_WIDTH, inputImgHeight, CV_8U, processInPtr);
    cv::Mat input_8u_patch2(HALF_IMAGE_WIDTH, inputImgHeight, CV_8U, processInPtr_patch2);

    // input shift (pre-processing)
    cv::Mat input_8u_shift_patch1(HALF_IMAGE_WIDTH, INPUT_IMG_SIZE, CV_8U);
    cv::Mat input_8u_shift_patch2(HALF_IMAGE_WIDTH, INPUT_IMG_SIZE, CV_8U);

    // shift
    uint32_t shiftVal, baselineLocation;

    if (params.imagingMode == IMAGING_MODE_CW) {
        shiftVal = (uint32_t) ((0.5f + baselineShiftFracton * 2.0f) * 256.0f);

        input_8u_patch1(cv::Rect(0, 0, PW_IMG_HEIGHT - shiftVal, HALF_IMAGE_WIDTH)).copyTo(input_8u_shift_patch1(cv::Rect(shiftVal, 0, PW_IMG_HEIGHT - shiftVal, HALF_IMAGE_WIDTH )));
        input_8u_patch1(cv::Rect(CW_IMG_HEIGHT - shiftVal, 0, shiftVal, HALF_IMAGE_WIDTH)).copyTo(input_8u_shift_patch1(cv::Rect(0, 0, shiftVal, HALF_IMAGE_WIDTH)));

        input_8u_patch2(cv::Rect(0, 0, PW_IMG_HEIGHT - shiftVal, HALF_IMAGE_WIDTH)).copyTo(input_8u_shift_patch2(cv::Rect(shiftVal, 0, PW_IMG_HEIGHT - shiftVal, HALF_IMAGE_WIDTH )));
        input_8u_patch2(cv::Rect(CW_IMG_HEIGHT - shiftVal, 0, shiftVal, HALF_IMAGE_WIDTH)).copyTo(input_8u_shift_patch2(cv::Rect(0, 0, shiftVal, HALF_IMAGE_WIDTH)));
    }
    else
    {
        shiftVal = (uint32_t) ((0.5f + baselineShiftFracton) * 256.0f);

        input_8u_patch1(cv::Rect(0, 0, PW_IMG_HEIGHT - shiftVal, HALF_IMAGE_WIDTH)).copyTo(input_8u_shift_patch1(cv::Rect(shiftVal, 0, PW_IMG_HEIGHT - shiftVal, HALF_IMAGE_WIDTH )));
        input_8u_patch1(cv::Rect(PW_IMG_HEIGHT - shiftVal, 0, shiftVal, HALF_IMAGE_WIDTH)).copyTo(input_8u_shift_patch1(cv::Rect(0, 0, shiftVal, HALF_IMAGE_WIDTH)));

        input_8u_patch2(cv::Rect(0, 0, PW_IMG_HEIGHT - shiftVal, HALF_IMAGE_WIDTH)).copyTo(input_8u_shift_patch2(cv::Rect(shiftVal, 0, PW_IMG_HEIGHT - shiftVal, HALF_IMAGE_WIDTH )));
        input_8u_patch2(cv::Rect(PW_IMG_HEIGHT - shiftVal, 0, shiftVal, HALF_IMAGE_WIDTH)).copyTo(input_8u_shift_patch2(cv::Rect(0, 0, shiftVal, HALF_IMAGE_WIDTH)));
    }

    // baseline location
    baselineLocation = 256 - shiftVal;
    if (baselineLocation < 0)
        baselineLocation = 0;

    ALOGD("BaselineShiftFraction: %f, shiftVal: %d, baselineLocation: %d", baselineShiftFracton, shiftVal, baselineLocation);

    // input and output size of ML modules
    cv::Size ioSize = {INPUT_IMG_SIZE, INPUT_IMG_SIZE};
    cv::Size concatSize = {INPUT_IMG_SIZE * 2, INPUT_IMG_SIZE};

    // input resize & rorate (transpose)
    cv::Mat input_8u_resize_patch1(ioSize, CV_8UC1);
    cv::Mat input_8u_resize_t_patch1(ioSize, CV_8UC1);
    cv::Mat input_8f_resize_t_patch1(ioSize, CV_32FC1);

    cv::Mat input_8u_resize_patch2(ioSize, CV_8UC1);
    cv::Mat input_8u_resize_t_patch2(ioSize, CV_8UC1);
    cv::Mat input_8f_resize_t_patch2(ioSize, CV_32FC1);

    // resize
    cv::resize(input_8u_shift_patch1, input_8u_resize_patch1, ioSize, cv::INTER_LINEAR);
    cv::resize(input_8u_shift_patch2, input_8u_resize_patch2, ioSize, cv::INTER_LINEAR);

    // rotate
    cv::rotate(input_8u_resize_patch1, input_8u_resize_t_patch1, ROTATE_90_COUNTERCLOCKWISE);
    input_8u_resize_t_patch1.convertTo(input_8f_resize_t_patch1, CV_32FC1);

    cv::rotate(input_8u_resize_patch2, input_8u_resize_t_patch2, ROTATE_90_COUNTERCLOCKWISE);
    input_8u_resize_t_patch2.convertTo(input_8f_resize_t_patch2, CV_32FC1);


    // prob map output has the same size as input
    cv::Mat probMap1_patch1(ioSize, CV_32FC2);
    cv::Mat probMap2_patch1(ioSize, CV_32FC2);
    cv::Mat probMap3_patch1(ioSize, CV_32FC2);

    cv::Mat probMap1_patch2(ioSize, CV_32FC2);
    cv::Mat probMap2_patch2(ioSize, CV_32FC2);
    cv::Mat probMap3_patch2(ioSize, CV_32FC2);

    // concat size
    cv::Mat probMap1, probMap2, probMap3;

    // struct in
    float* structIn = new float[9];

    for (int i = 0; i < 9; i++)
        structIn[i] = 0.0f;

    // classId check
    if (classId < 0)
        classId = 0;

    if (classId > 8)
        classId = 8;

    structIn[classId] = 1.0f;
    ///////////////////////////////////

    float *outputs1_patch1[] = {probMap1_patch1.ptr<float>()};
    float *outputs2_patch1[] = {probMap2_patch1.ptr<float>()};
    float *outputs3_patch1[] = {probMap3_patch1.ptr<float>()};

    float *outputs1_patch2[] = {probMap1_patch2.ptr<float>()};
    float *outputs2_patch2[] = {probMap2_patch2.ptr<float>()};
    float *outputs3_patch2[] = {probMap3_patch2.ptr<float>()};

    // output -> 256 x 256 x 2
    // for patch 1
    memcpy(jni->imageBufferNative.data(), input_8f_resize_t_patch1.ptr<float>(),INPUT_IMG_SIZE * INPUT_IMG_SIZE * sizeof(float));
    memcpy(jni->kernelBufferNative.data(), structIn, sizeof(float) * 9);
    // process model 1 into probMap1
    //LOGD(" [ VTITASK ] Model Call (Struct): %.02f", structIn[0]);
    jni->jenv->CallVoidMethod(jni->instance, jni->process_38, jni->imageBuffer, jni->kernelBuffer, jni->outputBuffer);
    memcpy(outputs1_patch1[0], jni->outputBufferNative.data(), sizeof(float) * jni->outputBufferNative.size());

    // process model 2 into probMap2
    LOGD(" [ VTITASK ] Model2 Call (Struct): %.02f", structIn[0]);
    jni->jenv->CallVoidMethod(jni->instance, jni->process_36, jni->imageBuffer, jni->kernelBuffer, jni->outputBuffer);
    memcpy(outputs2_patch1[0], jni->outputBufferNative.data(), sizeof(float) * jni->outputBufferNative.size());

    // process model 3 into probMap3
    jni->jenv->CallVoidMethod(jni->instance, jni->process_50, jni->imageBuffer, jni->kernelBuffer, jni->outputBuffer);
    memcpy(outputs3_patch1[0], jni->outputBufferNative.data(), sizeof(float) * jni->outputBufferNative.size());

    // for patch 2
    memcpy(jni->imageBufferNative.data(), input_8f_resize_t_patch2.ptr<float>(),INPUT_IMG_SIZE * INPUT_IMG_SIZE * sizeof(float));
    jni->jenv->CallVoidMethod(jni->instance, jni->process_38, jni->imageBuffer, jni->kernelBuffer, jni->outputBuffer2nd);
    memcpy(outputs1_patch2[0], jni->outputBuffer2ndNative.data(), sizeof(float) * jni->outputBuffer2ndNative.size());

    jni->jenv->CallVoidMethod(jni->instance, jni->process_36, jni->imageBuffer, jni->kernelBuffer, jni->outputBuffer2nd);
    memcpy(outputs2_patch2[0], jni->outputBuffer2ndNative.data(), sizeof(float) * jni->outputBuffer2ndNative.size());
    jni->jenv->CallVoidMethod(jni->instance, jni->process_50, jni->imageBuffer, jni->kernelBuffer, jni->outputBuffer2nd);
    memcpy(outputs3_patch2[0], jni->outputBuffer2ndNative.data(), sizeof(float) * jni->outputBuffer2ndNative.size());

    // concatenate outputs 256 x 512 x 2
    cv::hconcat(probMap1_patch1, probMap1_patch2, probMap1);
    cv::hconcat(probMap2_patch1, probMap2_patch2, probMap2);
    cv::hconcat(probMap3_patch1, probMap3_patch2, probMap3);

    // model outputs (0 or 1) prob map
    cv::Mat y_pred1, y_pred2, y_pred3;

    // masking operation
    flatten(probMap1, y_pred1);
    flatten(probMap2, y_pred2);
    flatten(probMap3, y_pred3);

    // output Mats
    cv::Mat meanProbMap(concatSize, CV_32FC1);
    cv::Mat stdMap(concatSize, CV_32FC1);       // sigma

    // Y_Pred and con
    cv::Mat y_pred_ax(concatSize, CV_8UC1);
    cv::Mat y_pred_ax_con(concatSize, CV_8UC1);

    float mVal, stdVal;
    // std - sqrt ( E(X^2) - E(x)^2 )
    for (int h = 0; h < y_pred1.rows; h++) {
        for (int w = 0; w < y_pred1.cols; w++) {
            mVal = ((float) (y_pred1.at<uint8_t>(h, w) + y_pred2.at<uint8_t>(h, w) + y_pred3.at<uint8_t>(h, w)) ) / 3.0f;
            meanProbMap.at<float>(h, w) = mVal;

            stdVal = sqrt(mVal - (mVal * mVal));
            stdMap.at<float>(h, w) = stdVal;

            y_pred_ax.at<uint8_t>(h, w) = round(mVal);
            y_pred_ax_con.at<uint8_t>(h, w) =
                    round(mVal + 1.96 * stdVal) - round(mVal - 1.96 * stdVal);
        }
    }


    // Python
    // nb_components, output, stats, centroids = cv2.connectedComponentsWithStats(np.uint8(thresh))
    // nb_components -> int
    // output -> image with numbers
    // stats -> array (CV_32S) (int) min x, y, width, height, area
    // centroids CV_64F
    cv::Mat labels;     //output
    cv::Mat stats;
    cv::Mat centroids;
    int nb_components = cv::connectedComponentsWithStats(y_pred_ax, labels, stats, centroids, 8, CV_16U);

    ALOGD("nb_components: %d", nb_components);

    // TODO: case for nb_components = 0 or too big
    if (nb_components < 1)
        nb_components = 1;

    // Process connected components
    uint32_t component_uncertainty[nb_components];
    uint32_t component_mask[nb_components];

    for (int i = 0; i < nb_components; i++){
        component_uncertainty[i] = 0;
        component_mask[i] = 0;
    }

    uint16_t label_idx;
    float y_pred_val, y_pred_con_val;

    for (int h = 0; h < y_pred1.rows; h++) {
        for (int w = 0; w < y_pred1.cols; w++) {
            label_idx = labels.at<uint16_t>(h, w);
            y_pred_val = (uint32_t) y_pred_ax.at<uint8_t>(h, w);
            y_pred_con_val = (uint32_t) y_pred_ax_con.at<uint8_t>(h, w);

            component_uncertainty[label_idx] += y_pred_con_val;
            component_mask[label_idx] += y_pred_val;
        }
    }

    uint32_t max_area_idx = 0;
    uint32_t max_area = 0;
    float uncertainty;
    std::vector<uint32_t> good_cc_idx;

    for (uint32_t i = 0; i < nb_components; i++) {
        // only when component_mask > 0
        if (component_mask[i] > 0) {
            uncertainty = ((float) component_uncertainty[i]) / ((float) component_mask[i]);

            if (uncertainty < class_Uncertainty_threshold[classId]) {
                good_cc_idx.push_back(i);

                if (max_area < component_mask[i]) {
                    max_area = component_mask[i];
                    max_area_idx = i;
                }
            }
        }
    }

    // find VtiPeaks
    std::vector<uint32_t>::iterator it;
    uint32_t currentIdx;
    uint32_t peakYLoc, peakXLoc;
    uint32_t peakXMin, peakXMax;
    uint32_t ccYMin, ccYMax;
    int increment = 1;

    // inter points multiplier
    float intPtsMul = 1.0f;
    if (classId == 0 || classId == 2 || classId == 4)
        intPtsMul = 1.8f;

    uint32_t traceLineLoc = round(params.traceIndex * INPUT_IMG_SIZE / ((float) HALF_IMAGE_WIDTH));

    for (it = good_cc_idx.begin(); it < good_cc_idx.end(); it++)
    {
        currentIdx = (*it);

        VtiPeak vtiPeak;

        peakXMin = stats.at<int>(currentIdx, 0);
        peakXMax = peakXMin + stats.at<int>(currentIdx, 2) - 1;

        if (abs(peakXMax - peakXMin) < MIN_PEAK_WIDTH)
            continue;

        // find Peak Location
        if (centroids.at<double>(currentIdx, 1) - baselineLocation > 0)
        {
            // processing from bottom (256 -> 0)
            peakYLoc = stats.at<int>(currentIdx, 1) + stats.at<int>(currentIdx, 3) - 1;
            peakXLoc = findPeakLocation(labels, currentIdx, peakYLoc);

            increment = -1;

            ALOGD("PeakLoc - currentIdx: %d, Peak X: %d, Y: %d", currentIdx, peakXLoc, peakYLoc);
        }
        else
        {
            // processing from top (0 -> 256)
            peakYLoc = stats.at<int>(currentIdx, 1);
            peakXLoc = findPeakLocation(labels, currentIdx, peakYLoc);

            increment = 1;

            ALOGD("PeakLoc - currentIdx: %d, Peak X: %d, Y: %d", currentIdx, peakXLoc, peakYLoc);
        }

        // peak Location
        vtiPeak.peak.x = peakXLoc * SCALE_FACTOR;
        vtiPeak.peak.y = peakYLoc - baselineLocation;

        // when traceline breaks the shape
        if ( (peakXMin < traceLineLoc) && (peakXMax > traceLineLoc))
        {
            if (peakXLoc > traceLineLoc)
                peakXMin = traceLineLoc;
            else
                peakXMax = traceLineLoc;
        }

        // base points LR
        vtiPeak.baseL.x = peakXMin * SCALE_FACTOR;
        vtiPeak.baseL.y = 0;

        vtiPeak.baseR.x = peakXMax * SCALE_FACTOR;
        vtiPeak.baseR.y = 0;

        ALOGD("currentIdx: %d, PeakXMin: %d, PeakXMax: %d", currentIdx, peakXMin, peakXMax);

        ccYMin = stats.at<int>(currentIdx, 1);
        ccYMax = ccYMin + stats.at<int>(currentIdx, 3) - 1;

        // inter points
        int numLeftIns = 0;
        int numRightIns = 0;

        numLeftIns = (int) round(SCALE_FACTOR * (peakXLoc - peakXMin) /((float) params.scrollSpeed) / CTRL_POINT_SPAN_SEC * intPtsMul);
        numRightIns = (int) round(SCALE_FACTOR * (peakXMax - peakXLoc) /((float) params.scrollSpeed) / CTRL_POINT_SPAN_SEC * intPtsMul);

        if (numLeftIns > MAX_INTER_PTS)
            numLeftIns = MAX_INTER_PTS;

        if (numRightIns > MAX_INTER_PTS)
            numRightIns = MAX_INTER_PTS;

        // num InterPts
        vtiPeak.numInterL = numLeftIns;
        vtiPeak.numInterR = numRightIns;

        uint32_t xLoc, yLoc;
        // number of points on Left
        for (int i = 0; i < numLeftIns; i++)
        {
            xLoc = peakXMin + (i + 1) * ((peakXLoc - peakXMin) / ((float) (numLeftIns + 1) ));

            // find yLoc
            yLoc = findYLocation(labels, currentIdx, xLoc, ccYMin, ccYMax, increment);

            vtiPeak.interL[i].x = xLoc * SCALE_FACTOR;
            vtiPeak.interL[i].y = yLoc - baselineLocation;
        }
        // number of points on Right
        for (int i = 0; i < numRightIns; i++)
        {
            xLoc = peakXLoc + (i + 1) * ((peakXMax - peakXLoc) / ((float) (numRightIns + 1) ));

            // find yLoc
            yLoc = findYLocation(labels, currentIdx, xLoc, ccYMin, ccYMax, increment);

            vtiPeak.interR[i].x = xLoc * SCALE_FACTOR;
            vtiPeak.interR[i].y = yLoc - baselineLocation;
        }

        mVTICal->mVtiPeaks.push_back(vtiPeak);
    }

    // set calculation state
    // header information
    uint8_t* texPtr = cb->getFrame(inputFrameType, DAU_DATA_TYPE_TEX, CineBuffer::FrameContents::IncludeHeader);
    CineBuffer::CineFrameHeader* cineHeaderPtr = reinterpret_cast<CineBuffer::CineFrameHeader*>(texPtr);

    mVTICal->setCalState(params.imagingMode, cineHeaderPtr->frameNum, cineHeaderPtr->numSamples, params.dopplerBaselineShiftIndex,
                         params.dopplerBaselineInvert, cineHeaderPtr->timeStamp, classId);

    // clean up
    delete[] structIn;

    // return status
    return status;
}

// Find PeakLocation
uint32_t VTICalculationTask::findPeakLocation(cv::InputArray input, uint32_t ccIdx, uint32_t yLocation)
{
    uint32_t loc = 0;
    cv::Mat lvMap = input.getMat();

    uint32_t h = yLocation;

    for (int w = 0; w < lvMap.cols; w++) {
        if (lvMap.at<uint16_t>(h, w) == ccIdx)
        {
            loc = w;
            break;
        }
    }

    return loc;
}

// Find Y Location from X
uint32_t VTICalculationTask::findYLocation(cv::InputArray input, uint32_t ccIdx, uint32_t xLocation, uint32_t yMin, uint32_t yMax, int increment)
{
    uint32_t loc = 0;
    cv::Mat lvMap = input.getMat();

    uint32_t w = xLocation;
    uint32_t h;
    bool loop = true;

    // increment should be 1 or -1
    if (increment > 0)
    {
        h = yMin;
        while ((h <= yMax) && loop)
        {
            if (lvMap.at<uint16_t>(h, w) == ccIdx)
            {
                loc = h;
                loop = false;
            }

            h += 1;
        }
    }
    else
    {
        h = yMax;
        while ((h >= yMin) && loop)
        {
            if (lvMap.at<uint16_t>(h, w) == ccIdx)
            {
                loc = h;
                loop = false;
            }

            h -= 1;
        }
    }

    return loc;
}