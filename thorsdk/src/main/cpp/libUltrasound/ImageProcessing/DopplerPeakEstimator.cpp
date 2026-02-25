//
// Copyright 2021 EchoNous Inc.
//
//
#define LOG_TAG "DopplerPeakEstimator"
#include <DopplerPeakEstimator.h>

#define PEAK_THRESHOLD 0.7f             // peak threshold
#define ARRAY_LENGTH 128
#define LAST_INDEX_POS 127
#define CENTER_INDEX_POS 126
#define OFFSET_POS 125
#define TRACELINE_POS 124               // traceline position

#define PEAK_POS 121
#define MIN_SEARCH_RANGE 122
#define MAX_SEARCH_RANGE 123
#define WF_TYPE 120                     // waveform type single / dual (for MV VTI)
#define QUERY_X_COORD_POS 118
#define QUERY_Y_COORD_POS 119

#define PEAK_REJECT_THRESHOLD 1.5f      // ratio between median peak and real peak

#define CONTROL_POINT_SPAN 45
#define CONTROL_POINT_SPAN_SEC 0.08f
#define MAX_INT_POINTS 56

#define TWO_PEAK_WAVEFORM 101
#define QUERY_Y_COORD     201

DopplerPeakEstimator::DopplerPeakEstimator(CineBuffer* cineBuffer, AIManager* aiManager)
{
    mCineBuffer = cineBuffer;
    mAIManager = aiManager;

    // default value
    mImgWidth = 1520;
    mImgHeight = 659;
    mYShift = 0.0f;
    mYExpScale = 1.0f;

    mTraceInvert = false;

    //init mCompletionEvent
    mCompletionEvent.init(EventMode::AutoReset, false);
}

DopplerPeakEstimator::~DopplerPeakEstimator()
{
}

void DopplerPeakEstimator::setParams(ScrollModeRecordParams smParams, int imgWidth, int imgHeight,
                                     float yShift, float yExpScale, bool traceInvert)
{
    // Set Params
    mSMParams = smParams;
    mImgWidth = imgWidth;
    mImgHeight = imgHeight;
    mYShift = yShift;
    mYExpScale = yExpScale;
    mTraceInvert = traceInvert;
}

void DopplerPeakEstimator::completionCallback(void* userPtr, ThorStatus statusCode)
{
    ALOGD("VTI calculation complete with %d", statusCode);

    // set completion event flag
    ((DopplerPeakEstimator*) userPtr)->mCompletionEvent.set();

    return;
}

//-----------------------------------------------------------------------------
void DopplerPeakEstimator::estimatePeaks(float* mapMat, int arrayLength, int imgMode, uint32_t classIdInput)
{
    int         desired_peak_loc = 0;
    uint32_t    classId = classIdInput;

    desired_peak_loc = (int) (mapMat[PEAK_POS] * ((float) mSMParams.scrDataWidth) / mImgWidth);

    if (desired_peak_loc < 1)
        desired_peak_loc = (int) ( ((float) mSMParams.scrDataWidth) / mImgWidth * 0.5f);


    // TODO: UPDATE

    /* CLASS ID
    0: 'PW-MV': PW mode and Mitral valve
    1: 'PW-TV', PW mode and tricuspid valve
    2: 'CW-MV': CW mode and Mitral valve (Stenotic and Normal flows)
    3: 'CW-TV': CW mode and tricuspid valve (Stenotic and Normal flows)
    4: 'CWr-MV_TV': CW mode and Mitral/tricuspid valve for regurgitation (Backward flow)
    5: 'PW-other valves': PW mode and Aortic/Pulmonic valve/ LVOT
    6: 'CW -Other valves': CW mode and Aortic/Pulmonic valve/ LVOT (Stenotic and Normal flows)
    7: 'CWr - Other valves', CW mode and Aortic/Pulmonic valve/ LVOT for regurgitation (Backward flow)
    8: 'PW-TDI'
    */

    /* Temp definition (from UI)
   1010: general
   1000: MV VTI
   1001: AV VTI
   1002: LVOT
    */

    // class Id from WF_TYPE
    if (classIdInput == 100)
    {
        ALOGD("WF_TYPE: %d", (int) mapMat[WF_TYPE] );

        if ((int) mapMat[WF_TYPE] == 1000) {
            ALOGD("VTI Type: MV VTI ...");
            if (imgMode == IMAGING_MODE_PW)
                classId = 0;
            else
                classId = 2;
        }
        else {
            ALOGD("VTI Type: other ...");
            if (imgMode == IMAGING_MODE_PW)
                classId = 5;
            else
                classId = 6;
        }
    }

    // check - need to re-calculate
    if (!mAIManager->getVTICalculation()->compareCalState(classId)) {
        ALOGD("Calculating VTI using ML models ... ");
        // create Task - creating a task clears previously stored results
        auto task = mAIManager->getVTICalculation()->createVTICalculationTask(classId, this);
        mAIManager->getVTICalculation()->setCompletionCallback(&completionCallback);

        // push the task
        mAIManager->push(std::move(task));

        // wait 2 sec or callback resets the event
        mCompletionEvent.wait(2000);
    }

    //vtiPeaks
    auto vtiPeaks = mAIManager->getVTICalculation()->mVtiPeaks;

    float signAdj = 1.0f;
    if (mSMParams.isInvert)
        signAdj = -1.0f;


    uint32_t mapIdx;
    float xNormScale = mImgWidth / ((float) mSMParams.scrDataWidth);
    float yNormScale = signAdj * (mImgHeight / 256.0f);

    if (vtiPeaks.size() < 1)
    {
        // default control point
        mapIdx = 0;

        // Left
        mapMat[2*mapIdx] = 100.0f;
        mapMat[2*mapIdx+1] = 0.0f;
        mapIdx++;

        // center / mid
        mapMat[CENTER_INDEX_POS] = mapIdx;
        mapMat[2*mapIdx] = 200.0f;
        mapMat[2*mapIdx+1] = 0.0f;
        mapIdx++;

        // right
        mapMat[LAST_INDEX_POS] = mapIdx;
        mapMat[2*mapIdx] = 300.0f;
        mapMat[2*mapIdx+1] = 0.0f;
        return;
    }

    VtiPeak currentPeak;
    uint32_t currentIdx = 0;
    uint32_t minDiff = 1000000000;
    uint32_t currentDiff;
    VtiPeak selectPeak;

    for (auto it = vtiPeaks.begin(); it < vtiPeaks.end(); it++)
    {
        currentPeak = (*it);
        currentDiff = abs(currentPeak.peak.x - desired_peak_loc);

        if (minDiff > currentDiff)
        {
            minDiff = currentDiff;
            selectPeak = (*it);
        }

        currentIdx++;
    }

    mapIdx = 0;

    // baseL
    mapMat[2*mapIdx] = selectPeak.baseL.x * xNormScale;
    mapMat[2*mapIdx+1] = selectPeak.baseL.y * yNormScale;
    mapIdx++;


    for (int i = 0; i < selectPeak.numInterL; i++)
    {
        mapMat[2*mapIdx] = selectPeak.interL[i].x * xNormScale;
        mapMat[2*mapIdx+1] = selectPeak.interL[i].y * yNormScale;
        mapIdx++;
    }

    // Peak
    mapMat[CENTER_INDEX_POS] = mapIdx;
    mapMat[2*mapIdx] = selectPeak.peak.x * xNormScale;
    mapMat[2*mapIdx+1] = selectPeak.peak.y * yNormScale;
    mapIdx++;

    for (int i = 0; i < selectPeak.numInterR; i++)
    {
        mapMat[2*mapIdx] = selectPeak.interR[i].x * xNormScale;
        mapMat[2*mapIdx+1] = selectPeak.interR[i].y * yNormScale;
        mapIdx++;
    }

    // baseR
    mapMat[LAST_INDEX_POS] = mapIdx;
    mapMat[2*mapIdx] = selectPeak.baseR.x * xNormScale;
    mapMat[2*mapIdx+1] = selectPeak.baseR.y * yNormScale;
}

//-----------------------------------------------------------------------------
/*
 * Search priority
 * 1. Range first
 * 2. Desired Peak Location
 * 3. Offset
 */
void DopplerPeakEstimator::estimatePeaks(float* mapMat, int arrayLength, int imgMode)
{
    float*      inputPtrFloat;
    int         searchRange;
    float       samplingRate;
    float*      inData;
    float*      inDataMedian;
    float       signAdj = 1.0f;
    float       sampleMin = 1000.0f;
    float       sampleMax = -1000.0f;
    float       curSample;
    float       absTh;
    int         lastThSampleLoc = -1;
    int         minMidPeakSeparation = 0;
    int         offset = 0;
    bool        disableResearch = false;
    bool        medianInput = true;
    bool        twoPeakSearch = false;
    int         desired_peak_loc = 0;
    int         searchRangeMax = 0;
    int         searchRangeMin = 0;
    int         sampleMaxLoc = -1;
    int         medFilterSize;
    int         queryXCoord = 0;
    float       medianFilterTime = 0.10f;       // default
    float       rightMultiplier = 1.0f;
    bool        isLinearProbe = false;
    CineBuffer::CineFrameHeader* inputHeaderPtr;

    std::vector<std::pair<int,float>> topPts;
    std::vector<int> separationPts;

    if (arrayLength != (ARRAY_LENGTH)) {
        ALOGE("ArrayLength should be 128.");
        return;
    }

    // should be an even number - as each peak has two separation points
    offset = (int) mapMat[OFFSET_POS];

    if (offset < 0)
        offset = 0;

    desired_peak_loc = (int) (mapMat[PEAK_POS] * ((float) mSMParams.scrDataWidth) / mImgWidth);
    searchRangeMin = (int) (mapMat[MIN_SEARCH_RANGE] * ((float) mSMParams.scrDataWidth) / mImgWidth);
    searchRangeMax = (int) (mapMat[MAX_SEARCH_RANGE] * ((float) mSMParams.scrDataWidth) / mImgWidth);

    // add two-peak search - only when WF_TYPE is 101
    if ( ((int) (mapMat[WF_TYPE])) == TWO_PEAK_WAVEFORM )
    {
        twoPeakSearch = true;
    }

    // inputPtr;
    inputPtrFloat = (float*) mCineBuffer->getFrame(0, DAU_DATA_TYPE_DPMAX_SCR);

    // search range
    searchRange = mSMParams.frameWidth;
    if (mSMParams.isScrFrame)
    {
        searchRange = mSMParams.scrDataWidth;

        inputHeaderPtr =
                reinterpret_cast<CineBuffer::CineFrameHeader*>(((uint8_t*) inputPtrFloat) - sizeof(CineBuffer::CineFrameHeader));

        if (inputHeaderPtr->numSamples < searchRange)
            searchRange = inputHeaderPtr->numSamples;
    }

    // simple Y coordinate query
    if ( ((int) (mapMat[WF_TYPE])) == QUERY_Y_COORD )
    {
        queryXCoord = (int) ((mapMat[QUERY_X_COORD_POS] / ((float) mImgWidth)) * ((float) mSMParams.scrDataWidth));

        if (mTraceInvert)       // update input ptr to nPeak
            inputPtrFloat++;

        if (mSMParams.isInvert != mTraceInvert)
            signAdj = -1.0f;

        curSample = 0.0f;

        if (queryXCoord >= 0 && queryXCoord < searchRange)
            curSample = *(inputPtrFloat + 2 * queryXCoord);

        mapMat[QUERY_Y_COORD_POS] = -signAdj * abs(curSample) * (mImgHeight / 2.0f) * mYExpScale;;

        return;
    }

    samplingRate = mSMParams.scrollSpeed;
    minMidPeakSeparation = (int) (samplingRate * 0.055f);       // 0.055f

    // disable research when peak is too close to border
    if ((desired_peak_loc > 0) || (searchRangeMin > 0) || (searchRangeMax > 0))
    {
        disableResearch = true;
        offset = 0;                 // ignore offset
    }

    if ( (searchRangeMin > 0) || (searchRangeMax > 0) )
    {
        medianInput = false;
        twoPeakSearch = false;
    }

    // default -> select a peak near center
    if ( (searchRangeMin == 0) && (searchRangeMax == 0) && (desired_peak_loc == 0) )
    {
        desired_peak_loc = searchRange/2;
        disableResearch = true;
    }

    // value search Range Max
    if (searchRangeMax < 1)
        searchRangeMax = searchRange;
    else if (searchRangeMax > searchRange)
        searchRangeMax = searchRange;

    if (searchRangeMin >= searchRangeMax)
        searchRangeMin = searchRangeMax - 1;

    if (searchRangeMin < 0)
        searchRangeMin = 0;

    // set up input data
    inData = new float[searchRange + 1];
    inDataMedian = new float[searchRange + 1];

    if (mTraceInvert)       // update input ptr to nPeak
        inputPtrFloat++;

    if (mSMParams.isInvert != mTraceInvert)
    {
        signAdj = -1.0f;
    }

    for (int i = 0; i < searchRangeMin; i++)
        inData[i] = 0.0f;

    for (int i = searchRangeMin; i < searchRangeMax; i++)
    {
        // TODO: add filtering here, if necessary
        curSample = abs(*(inputPtrFloat + 2 * i));
        inData[i] = curSample;

        if (curSample < sampleMin)
            sampleMin = curSample;
    }

    if ( (mCineBuffer->getParams().probeType & DEV_PROBE_MASK) == PROBE_TYPE_LINEAR )
    {
        isLinearProbe = true;

        medianFilterTime = 0.02f;
        rightMultiplier = 4.0f;

        ALOGD("Procesisng for Linear Probe");
    }

    if (medianInput)
    {
        // median filter
        medFilterSize = (int) ceil(medianFilterTime * mSMParams.scrollSpeed);
        if ( ((medFilterSize >> 1) << 1) == medFilterSize )
            medFilterSize += 1;

        ALOGD("Median Filter Size: %d", medFilterSize);
        medianFilter(inData, inDataMedian, searchRangeMax, medFilterSize);
    }
    else
    {
        // copy data
        for (int i = searchRangeMin; i < searchRangeMax; i++)
            inDataMedian[i] = inData[i];
    }

    for (int i = searchRangeMin; i < searchRangeMax; i++)
    {
        curSample = inDataMedian[i];

        if (curSample > sampleMax) {
            sampleMax = curSample;
            sampleMaxLoc = i;
        }
    }

    // Threshold
    absTh = sampleMax * PEAK_THRESHOLD;

    // min abs th
    if (absTh < 0.05f/mYExpScale)
        absTh = 0.05f/mYExpScale;

    if (imgMode == IMAGING_MODE_CW && absTh > 0.50f)
        absTh = 0.50f;


    for (int i = searchRangeMin; i < searchRangeMax; i++)
    {
        if (inDataMedian[i] > absTh)
        {
            topPts.emplace_back(i, MIN(inData[i], inDataMedian[i]));

            if ( (i - lastThSampleLoc) > minMidPeakSeparation || (lastThSampleLoc == -1) )
            {
                if (lastThSampleLoc != - 1)
                    separationPts.push_back(lastThSampleLoc);

                separationPts.push_back(i);
            }

            // update lastThSampleLoc
            lastThSampleLoc = i;
        }
    }

    // if out of range.  ignore desired peak loc
    if (desired_peak_loc < searchRangeMin || desired_peak_loc > searchRangeMax)
        desired_peak_loc = -1;

    ALOGD("Search Range Min: %d, Max: %d, Desired Peak: %d", searchRangeMin, searchRangeMax, desired_peak_loc);
    ALOGD("sample - Max: %f, Min: %f, Threshold: %f, MaxLoc: %d ", sampleMax, sampleMin, absTh, sampleMaxLoc);

    keyCtrlPts* keyControlPts = new keyCtrlPts;
    keyControlPts->peakLoc = -1;
    keyControlPts->minLeftLoc = -1;
    keyControlPts->minRightLoc = -1;
    keyControlPts->offset = 0;

    bool crossed = findKeyControlPoints(inData, topPts, separationPts, searchRange, sampleMin, offset,
                                        searchRangeMin, searchRangeMax, desired_peak_loc,
                                        isLinearProbe, rightMultiplier, keyControlPts);

    if ( (separationPts.size() > 2) && ((keyControlPts->minLeftLoc < 4) || crossed) && !disableResearch)
    {
        // checking offset of the identified peak (crossed redline so not valid).
        offset = (int) keyControlPts->offset;

        if (offset == 0)
            offset += 2;
        else
            offset = 0;

        //trying to pick up 2nd peak
        findKeyControlPoints(inData, topPts, separationPts, searchRange, sampleMin, offset,
                             searchRangeMin, searchRangeMax, desired_peak_loc,
                             isLinearProbe, rightMultiplier, keyControlPts);
    }

    // Dual Peak Case, e.g., MV VTI
    if ( twoPeakSearch )
    {
        float avgPeakDist = 0.0f;
        std::vector<std::pair<int,float>> peakPts;
        float maxDiffDist = samplingRate * 2.0f;    // 2 sec (30 Hz)
        float minDiffDist = samplingRate * 0.25f;   // 240 Hz

        findPeakPoints(topPts, separationPts, peakPts, avgPeakDist);

        ALOGD("Processing Two peaks (MV VTI) - avg Peak Distance: %f", avgPeakDist);

        float minWidthThreshold = 0.7f * avgPeakDist;

        // proceed under these conditions
        if ( (avgPeakDist >= minDiffDist) && (avgPeakDist <= maxDiffDist)
             && ((float)(keyControlPts->minRightLoc - keyControlPts->minLeftLoc) < minWidthThreshold) ) {

            keyCtrlPts *keyControlPts2 = new keyCtrlPts;
            keyCtrlPts *keyControlPts3 = new keyCtrlPts;
            float maxPeakThreshold = 0.49f * avgPeakDist;           // near two peaks max separation
            float maxWidthThreshold = 1.2f * avgPeakDist;
            float minSepPeakThreshold = 0.8f * avgPeakDist;         // min peak separation for two separate peaks

            // searching right hand-side
            int searchRangeMinNew, searchRangeMaxNew;
            int offsetNew, desiredPeakLocNew;
            int minLeftLoc, minRightLoc;
            bool peak2Found = false;
            int minPeakDist = 100000;
            int minMaintainDuration = (int) mSMParams.scrollSpeed * 0.15f;

            // only proceed it detects at least two peaks
            if (peakPts.size() > 1)
            {
                for (int i = 0; i < peakPts.size(); i++)
                {
                    if (abs(keyControlPts->peakLoc - peakPts.at(i).first) < minPeakDist)
                    {
                        minPeakDist = abs(keyControlPts->peakLoc - peakPts.at(i).first);
                    }
                }

                if (minPeakDist < 0.15f * avgPeakDist)
                {
                    // close to identified peakLocation -> search right-hand side
                    searchRangeMinNew = keyControlPts->minRightLoc;
                    offsetNew = keyControlPts->offset + 2;
                    desiredPeakLocNew = -1;

                    findKeyControlPoints(inData, topPts, separationPts, searchRange, sampleMin, offsetNew,
                                         searchRangeMinNew, searchRangeMax, desiredPeakLocNew,
                                         isLinearProbe, rightMultiplier, keyControlPts2);

                    if ( ((keyControlPts2->peakLoc - keyControlPts->peakLoc) < 0.5f * avgPeakDist)
                         && (keyControlPts2->peakLoc > keyControlPts->peakLoc ) )
                    {
                        // peak - minor peak should be included the whole range
                        keyControlPts->minRightLoc = keyControlPts2->minRightLoc;
                    }
                    else
                    {
                        // need to search another peak between keyPts1 and keyPts2
                        searchRangeMinNew = keyControlPts->minRightLoc;
                        searchRangeMaxNew = keyControlPts2->minLeftLoc;
                        offsetNew = 0;
                        desiredPeakLocNew = -1;

                        // when identified peak is on the left
                        if (keyControlPts2->peakLoc <= keyControlPts->peakLoc)
                            searchRangeMaxNew = searchRangeMax-1;

                        findKeyControlPoints(inData, topPts, separationPts, searchRange, sampleMin, offsetNew,
                                             searchRangeMinNew, searchRangeMaxNew, desiredPeakLocNew,
                                             isLinearProbe, rightMultiplier, keyControlPts3);

                        // count the peak only when it's about 50% of the first peak
                        if (inData[keyControlPts3->peakLoc] >= inData[keyControlPts->peakLoc] * 0.5f)
                        {
                            findMinLocs(inData, inData[keyControlPts3->peakLoc], sampleMin,
                                        keyControlPts3->peakLoc,
                                        keyControlPts3->minLeftLoc, keyControlPts3->minRightLoc,
                                        minMaintainDuration,
                                        minLeftLoc, minRightLoc, true);

                            keyControlPts->minRightLoc = minRightLoc;
                        }
                    }
                }
                else
                {
                    // search left-hand side
                    // close to identified peakLocation -> search right-hand side
                    searchRangeMinNew = -1;
                    searchRangeMaxNew = keyControlPts->minLeftLoc;

                    if (searchRangeMaxNew > searchRangeMax)
                        searchRangeMaxNew = searchRangeMax - 1;

                    offsetNew = keyControlPts->offset - 2;
                    desiredPeakLocNew = -1;

                    findKeyControlPoints(inData, topPts, separationPts, searchRange, sampleMin, offsetNew,
                                         searchRangeMinNew, searchRangeMax, desiredPeakLocNew,
                                         isLinearProbe, rightMultiplier, keyControlPts2);

                    if ( abs(keyControlPts2->peakLoc - keyControlPts->peakLoc) < 0.5f * avgPeakDist )
                    {
                        // peak - minor peak should be included the whole range
                        keyControlPts->minLeftLoc = keyControlPts2->minLeftLoc;
                    }
                    else
                    {
                        searchRangeMinNew = keyControlPts2->minRightLoc;
                        searchRangeMaxNew = keyControlPts->minLeftLoc;
                        offsetNew = 0;
                        desiredPeakLocNew = -1;

                        findKeyControlPoints(inData, topPts, separationPts, searchRange, sampleMin, offsetNew,
                                             searchRangeMinNew, searchRangeMaxNew, desiredPeakLocNew,
                                             isLinearProbe, rightMultiplier, keyControlPts3);

                        // count the peak only when it's about 50% of the first peak
                        if (inData[keyControlPts3->peakLoc] >= inData[keyControlPts->peakLoc] * 0.5f)
                        {
                            findMinLocs(inData, inData[keyControlPts3->peakLoc], sampleMin,
                                        keyControlPts3->peakLoc,
                                        keyControlPts3->minLeftLoc, keyControlPts3->minRightLoc,
                                        minMaintainDuration,
                                        minLeftLoc, minRightLoc, true);

                            keyControlPts->minLeftLoc = minLeftLoc;
                        }
                    }
                }
            }   // end if - peakPts size

            delete keyControlPts2;
            delete keyControlPts3;
        }
    }  // end of two peaks

    // TODO: false positive rejection
    float tmpMat[ARRAY_LENGTH];
    for (int i = 0; i < (ARRAY_LENGTH); i++) {
        tmpMat[i] = 0.0f;
    }
    int numContolPoints = insertControlPoints(inData, tmpMat, keyControlPts, searchRange);

    // Apply default control points
    if ( numContolPoints < 3 )
    {
        getDefaultControlPoints(inData, tmpMat, searchRange);

        // update numControlPoints
        numContolPoints = ((int) tmpMat[LAST_INDEX_POS]) + 1;
    }

    // traceline Location
    tmpMat[TRACELINE_POS] = (float) mSMParams.traceIndex;

    ALOGD("OFFSET: %f, CENTER_INDEX: %f, LAST_INDEX: %f, TRACELINE: %f", tmpMat[OFFSET_POS],
          tmpMat[CENTER_INDEX_POS], tmpMat[LAST_INDEX_POS], tmpMat[TRACELINE_POS]);

    // boundary adjustment - force 0 values for boundaries
    tmpMat[1] = 0.0f;
    tmpMat[2*numContolPoints - 1] = 0.0f;

    // normalized for pixel coordinate.
    for (int i = 0; i < numContolPoints; i++)
    {
        mapMat[2*i] = tmpMat[2*i] / ((float) mSMParams.scrDataWidth) * mImgWidth;
        mapMat[2*i+1] = -signAdj * tmpMat[2*i+1] * (mImgHeight / 2.0f) * mYExpScale;
    }

    // copy fiducial control points
    mapMat[TRACELINE_POS] = tmpMat[TRACELINE_POS] / ((float) mSMParams.scrDataWidth) * mImgWidth;
    mapMat[OFFSET_POS] = tmpMat[OFFSET_POS];
    mapMat[CENTER_INDEX_POS] = tmpMat[CENTER_INDEX_POS];
    mapMat[LAST_INDEX_POS] = tmpMat[LAST_INDEX_POS];

    // clean up
    delete[] inData;
    delete[] inDataMedian;
    delete keyControlPts;
}

void DopplerPeakEstimator::findPeakPoints(std::vector<std::pair<int,float>> topPts, std::vector<int> separationPts,
                                          std::vector<std::pair<int,float>>& peakPts, float& avgDist)
{
    // this function extracts peaks and calculates an average interval
    // This is for detecting dual peaks.  So, the 2nd close peaks will be removed for the calculation

    avgDist = 0.0f;

    if (topPts.size() < 1)
        return;

    auto it = begin (topPts);
    int lastLocation = it->first;
    float peakVal = -1000.0f;
    int peakLoc = it->first;

    // extract peak points
    while (it != end (topPts))
    {
        if ( abs(lastLocation - it->first) > 1 )
        {
            peakPts.emplace_back(peakLoc, peakVal);
            peakVal = -1000.0f;
        }

        lastLocation = it->first;

        if (peakVal <= it->second)
        {
            peakVal = it->second;
            peakLoc = it->first;
        }

        ++it;
    }

    peakPts.emplace_back(peakLoc, peakVal);

    float sumDiff = 0.0f;
    float avgThreshold = 0.9f;

    // need at least 3 peaks
    if (peakPts.size() > 1)
    {
        // 1st pass to calculate average
        for (int i = 0; i < peakPts.size() - 1; i++)
        {
            sumDiff += abs(peakPts.at(i+1).first - peakPts.at(i).first);
        }

        avgDist = sumDiff / (peakPts.size() - 1);

        // remove elements, if distance is below the threshold
        for (int i = peakPts.size() - 1; i > 0; i--)
        {
            if ( abs(peakPts.at(i).first - peakPts.at(i-1).first) < (avgThreshold * avgDist) )
            {
                peakPts.erase(peakPts.begin() + i);
            }
        }

        if (peakPts.size() > 1)
        {
            sumDiff = 0.0f;

            for (int i = 0; i < peakPts.size() - 1; i++)
            {
                sumDiff += abs(peakPts.at(i+1).first - peakPts.at(i).first);
            }

            avgDist = sumDiff / (peakPts.size() - 1);
        }
    }

    return;
}

bool DopplerPeakEstimator::findKeyControlPoints(float* inData, std::vector<std::pair<int,float>> topPts,
                                                std::vector<int> separationPts, int searchRange, float sampleMin, int offset,
                                                int searchRangeMin, int searchRangeMax, int desiredPeakLoc,
                                                bool isLinearProbe, float rightMultiplier, keyCtrlPts* keyPts)
{
    int     peakLoc = -1;
    float   peakVal = -1000.0f;
    int     minLeftLoc = 0;
    int     minRightLoc = searchRange;
    int     minSearchRange = (int) mSMParams.scrollSpeed * 0.5f;            // tmp duration 0.5 sec.
    int     minMaintainDuration = (int) mSMParams.scrollSpeed * 0.15f;
    float   minVal;
    float   maxVal;
    int     minLoc;
    int     minDistance;
    int     searchStart, searchEnd;
    int     realOffset = 0;
    bool    crossTraceline = false;
    int     tracelineLoc = mSMParams.traceIndex;
    bool    predefinedRange = false;
    int     separationDist =  (int) mSMParams.scrollSpeed * 0.5f;

    if (desiredPeakLoc > 0 && searchRangeMin < 1 && searchRangeMax == searchRange &&
        separationPts.size() > 0)
    {
        minDistance = 1000000;
        minLoc = 0;

        for (int i = 0; i < separationPts.size(); i += 2)
        {
            if (abs(separationPts.at(i) - desiredPeakLoc) < minDistance)
            {
                minDistance = abs(separationPts.at(i) - desiredPeakLoc);
                minLoc = i;
            }
        }

        offset = minLoc;
        ALOGD("Identified offset value: %d, min distance: %d, separationPtsSize: %d", offset,
              minDistance, separationPts.size());
    }

    if (separationPts.size() > (2 + offset))
    {
        // at least two peaks - first peak
        auto it = begin (topPts);

        while (it->first <= separationPts.at(1 + offset))
        {
            if ( (it->second >= peakVal) && (it->first >= separationPts.at(0 + offset)) )
            {
                peakLoc = it->first;
                peakVal = it->second;
            }

            ++it;
        }

        realOffset = offset;

        // search range for minimum values
        minSearchRange = MIN(minSearchRange, separationPts.at(2 + offset) - separationPts.at(0 + offset));
        separationDist = separationPts.at(2 + offset) - separationPts.at(0 + offset);
    }
    else if ((offset != 0) && (separationPts.size() > offset))
    {
        // need to find peak with offsets
        auto it = begin (topPts);
        while (it != end (topPts))
        {
            if ((it->second >= peakVal) && (it->first >= separationPts.at(offset)))
            {
                peakLoc = it->first;
                peakVal = it->second;
            }

            ++it;
        }

        realOffset = offset;
    }
    else if (separationPts.size() > 0)
    {
        // at least one peaks
        for (auto it = begin (topPts); it != end (topPts); ++it)
        {
            if (it->second >= peakVal)
            {
                peakLoc = it->first;
                peakVal = it->second;
            }
        }

        realOffset = 0;
    }

    // when min and max of search range are defined.
    if (searchRangeMin > 0 && searchRange != searchRangeMax)
    {
        minLeftLoc = searchRangeMin;
        minRightLoc = searchRangeMax-1;
        predefinedRange = true;
    }
    else
    {
        if (sampleMin < 0.0f)
            sampleMin = 0.0f;

        searchStart = peakLoc - minSearchRange;

        if (searchStart < 0)
            searchStart = 0;

        if (isLinearProbe)
            searchEnd = peakLoc + separationDist;
        else
            searchEnd = peakLoc + minSearchRange;

        if (searchEnd > searchRange - 2)
            searchEnd = searchRange - 2;

        findMinLocs(inData, peakVal, sampleMin, peakLoc, searchStart, searchEnd, minMaintainDuration,
                    minLeftLoc, minRightLoc, rightMultiplier);

        // if identified peak is too narrow -> likely to be valve closure noise
        if ( (abs(minLeftLoc - minRightLoc) < 10 && abs(searchEnd - searchStart) > 50)  ||
             ( ((float) abs(minRightLoc - minLeftLoc))/ ((float) abs(searchEnd - searchStart)) < 0.12f
               && (abs(minLeftLoc - peakLoc) < 2 || abs(minRightLoc - peakLoc) < 2) ) )
        {
            int currentLoc = 0;
            float currentVal;
            float lastVal;
            int   lastLoc;
            int   minZeroLoc = 100000;
            int   maxZeroLoc = -10000;
            int   leftHalf;
            int   rightHalf;
            int   leftLimit = -1;
            int   rightLimit = -1;
            float leftHalfMin = 1000.0f;
            float rightHalfMin = 1000.0f;

            std::vector<std::pair<int,float>> leftTopPts;
            std::vector<std::pair<int,float>> rightTopPts;

            // zero crossing check and adjust searchStart and End
            for (int i = searchStart; i < searchEnd + 1; i++)
            {
                currentLoc = i;
                currentVal = inData[currentLoc];

                if (currentLoc > minRightLoc && currentLoc < searchEnd)
                {
                    if ( (currentVal < (sampleMin + 0.0001f)) && (currentLoc > maxZeroLoc) )
                        maxZeroLoc = currentLoc;
                }
                else if (currentLoc < minLeftLoc && currentLoc > searchStart)
                {
                    if ( (currentVal < (sampleMin + 0.0001f)) && (currentLoc < minZeroLoc) )
                        minZeroLoc = currentLoc;
                }
            }

            if (maxZeroLoc < searchEnd && maxZeroLoc > minRightLoc)
                searchEnd = maxZeroLoc;

            if (minZeroLoc < minLeftLoc && minZeroLoc > searchStart)
                searchStart = minZeroLoc;

            // half point
            leftHalf = searchStart + (minLeftLoc - searchStart)/2;
            rightHalf = minRightLoc + (searchEnd - minRightLoc)/2;

            ALOGD("SearchStart: %d, End: %d, leftHalf: %d, rightHalf: %d", searchStart, searchEnd, leftHalf, rightHalf);

            for (auto it = begin (topPts); it != end (topPts); ++it)
            {
                currentLoc = it->first;
                currentVal = it->second;

                if (currentLoc > minRightLoc && currentLoc < searchEnd)
                {
                    rightTopPts.emplace_back(currentLoc, currentVal);

                    if (currentLoc > rightHalf && currentVal < rightHalfMin)
                    {
                        rightHalfMin = currentVal;
                        rightLimit = currentLoc;
                    }
                }
                else if (currentLoc < minLeftLoc && currentLoc > searchStart)
                {
                    leftTopPts.emplace_back(currentLoc, currentVal);

                    if (currentLoc < leftHalf && currentVal < leftHalfMin)
                    {
                        leftHalfMin = currentVal;
                        leftLimit = currentLoc;
                    }
                }
            }

            if (rightTopPts.size() > leftTopPts.size())
            {
                searchStart = minRightLoc;
                searchEnd = searchEnd;

                if (rightLimit < 0)
                    rightLimit = searchEnd;

                lastVal = -1000.0f;

                for (auto it = rightTopPts.rbegin(); it != rightTopPts.rend(); ++it)
                {
                    currentLoc = it->first;
                    currentVal = it->second;

                    if (currentVal < lastVal && currentLoc < rightLimit)
                        break;

                    lastVal = currentVal;
                    lastLoc = currentLoc;
                }

                peakLoc = lastLoc;
                peakVal = lastVal;
            }
            else
            {
                searchStart = searchStart;
                searchEnd = minLeftLoc;

                // sort
                if (leftLimit < 0) {
                    leftLimit = searchStart;
                }

                lastVal = -1000.0f;

                for (auto it = begin (leftTopPts); it != end(leftTopPts); ++it)
                {
                    currentLoc = it->first;
                    currentVal = it->second;

                    if (currentVal < lastVal && currentLoc > leftLimit)
                        break;

                    lastVal = currentVal;
                    lastLoc = currentLoc;
                }

                peakLoc = lastLoc;
                peakVal = lastVal;
            }

            // potential error check
            if (peakLoc < searchStart || peakLoc > searchEnd)
                peakLoc = (searchStart + searchEnd) / 2;

            ALOGD("Too narrow Peak - Start: %d, searchEnd: %d, peakLoc: %d", searchStart, searchEnd, peakLoc);

            findMinLocs(inData, peakVal, sampleMin, peakLoc, searchStart, searchEnd, minMaintainDuration,
                        minLeftLoc, minRightLoc, rightMultiplier);
        }
    }

    maxVal = -10000.0f;

    // update PeakLoc when minLeft and Right are set
    for (int i = minLeftLoc; i < minRightLoc + 1; i++)
    {
        if (inData[i] > maxVal)
        {
            maxVal = inData[i];
            peakLoc = i;
        }
    }

    int halfPeakWidth = (int) ceil(0.02f * mSMParams.scrollSpeed);

    // Check the detected peak - ignore when it's on the border
    if ((peakLoc > halfPeakWidth) && (peakLoc < searchRange - halfPeakWidth) && !predefinedRange)
    {
        float deltaXX = 0.0f;

        for (int i = 1; i < (halfPeakWidth + 1); i++)
        {
            deltaXX += abs(inData[peakLoc - i] - inData[peakLoc]) + abs(inData[peakLoc + i] - inData[peakLoc]);
        }
        deltaXX = deltaXX / (halfPeakWidth * 2.0f);

        if (deltaXX >= 0.15f) {
            ALOGD("Attempting to update peak location.");

            int newPeakLoc = peakLoc;
            int newMinLeftLoc = minLeftLoc;

            if ((peakLoc - minLeftLoc) < (minRightLoc - peakLoc))
            {
                maxVal = -10000.0f;
                // right-side peak
                for (int i = peakLoc+halfPeakWidth; i < minRightLoc + 1; i++)
                {
                    if (inData[i] > maxVal)
                    {
                        maxVal = inData[i];
                        newPeakLoc = i;
                    }
                }

                minVal = 10000.0f;
                for (int i = peakLoc + 1; i < newPeakLoc + 1; i++)
                {
                    if (inData[i] < minVal)
                    {
                        minVal = inData[i];
                        newMinLeftLoc = i;
                    }
                }

                if ( (peakLoc < newMinLeftLoc) && (newMinLeftLoc < newPeakLoc) )
                {
                    minLeftLoc = newMinLeftLoc;
                    peakLoc = newPeakLoc;
                }
            }   // end if - ignore if not
        }
    }

    if ( abs(peakLoc - minLeftLoc) < 1 )
    {
        if (minLeftLoc > 0)
            minLeftLoc -= 1;
        else
            peakLoc += 1;
    }

    if ( abs(peakLoc - minRightLoc) < 1 )
    {
        if (minRightLoc < searchRange - 1)
            minRightLoc += 1;
        else
            peakLoc -=1;
    }

    // if peakLoc is not identified.
    if ( (minRightLoc > 0) && (minRightLoc > minLeftLoc + 1) && peakLoc < 2)
        peakLoc = (minLeftLoc + minRightLoc) / 2;

    // check whether identified peak/min locs cross the traceline
    if (peakLoc > tracelineLoc)
    {
        if (minLeftLoc < tracelineLoc) {
            minLeftLoc = tracelineLoc + 1;
            crossTraceline = true;
        }
    }
    else
    {
        if (minRightLoc > tracelineLoc) {
            minRightLoc = tracelineLoc - 1;
            crossTraceline = true;
        }
    }

    if (minRightLoc >= (searchRangeMax - 1))
        minRightLoc = searchRangeMax - 1;

    ALOGD("Estimated Locations: offset: %d, LeftMin: %d, Peak: %d, RightMin: %d",
          realOffset, minLeftLoc, peakLoc, minRightLoc);
    ALOGD("SearchRange: %d, Local Minima Search Range: %d", searchRange, minSearchRange);

    keyPts->offset = realOffset;
    keyPts->minLeftLoc = minLeftLoc;
    keyPts->minRightLoc = minRightLoc;
    keyPts->peakLoc = peakLoc;

    return crossTraceline;
}

void DopplerPeakEstimator::findMinLocs(float* inData, float peakVal, float sampleMin, int peakLoc, int searchStart,
                                       int searchEnd, int minMaintainDuration, int& minLeftLoc, int& minRightLoc, float rightMultiplier, bool disableDeltaThreshold)
{
    float minVal = 1000.0f;
    int currentIdx = peakLoc - 1;
    bool loopWhile = true;
    minLeftLoc = currentIdx;
    float preVal;
    float deltaThreshold = peakVal * 0.20f;
    bool  deltaRejection = false;

    // stage 1 - small value
    float leftBumpThresholdStage1 = peakVal * 0.02f;
    float leftBumpMaxValueStage1 = peakVal * 0.33f;

    // stage 2 - mid value
    float leftBumpThresholdStage2 = peakVal * 0.08f;
    float leftBumpMaxValueStage2 = peakVal * 0.50f;

    // right-hand side
    // stage 1 - small value
    float rightBumpThresholdStage1 = peakVal * 0.03f * rightMultiplier;
    float rightBumpMaxValueStage1 = peakVal * 0.16f;

    float rightBumpThresholdStage2 = peakVal * 0.05f * rightMultiplier;
    float rightBumpMaxValueStage2 = peakVal * 0.33f;

    // search left hand side
    while ( (currentIdx > searchStart) && ((sampleMin + 0.0001f) < minVal) && loopWhile)
    {
        if (inData[currentIdx] < minVal)
        {
            minVal = inData[currentIdx];
            minLeftLoc = currentIdx;
        }
        else if (inData[currentIdx] > peakVal * PEAK_REJECT_THRESHOLD)
        {
            loopWhile = false;
        }

        if (minVal < leftBumpMaxValueStage1)
        {
            if ((inData[currentIdx] - minVal) > leftBumpThresholdStage1) {
                loopWhile = false;
            }
        }
        else if (minVal < leftBumpMaxValueStage2)
        {
            if ((inData[currentIdx] - minVal) > leftBumpThresholdStage2) {
                loopWhile = false;
            }
        }

        if (abs(minLeftLoc - currentIdx) > minMaintainDuration)
        {
            loopWhile = false;
        }

        currentIdx--;
    }

    minVal = 1000.0f;
    currentIdx = peakLoc + 1;
    loopWhile = true;
    minRightLoc = currentIdx;

    // search right hand side
    while ( (currentIdx < searchEnd) && ((sampleMin + 0.0001f) < minVal) && loopWhile)
    {
        if (inData[currentIdx] < minVal)
        {
            minVal = inData[currentIdx];
            minRightLoc = currentIdx;
        }
        else if (inData[currentIdx] > peakVal * PEAK_REJECT_THRESHOLD) {
            loopWhile = false;
        }

        if (minVal < rightBumpMaxValueStage1)
        {
            if ((inData[currentIdx] - minVal) > rightBumpThresholdStage1) {
                loopWhile = false;
            }
        }
        else if (minVal < rightBumpMaxValueStage2)
        {
            if ((inData[currentIdx] - minVal) > rightBumpThresholdStage2) {
                loopWhile = false;
            }
        }

        if (abs(minRightLoc - currentIdx) > minMaintainDuration * rightMultiplier) {
            loopWhile = false;
        }

        // for click-noise rejection
        if ( ((inData[currentIdx] - minVal) > deltaThreshold * rightMultiplier) && !disableDeltaThreshold) {
            loopWhile = false;
            deltaRejection = true;
        }

        currentIdx++;
    }

    // when the peak is narrow and rejected by delta - re-search
    if ( (abs(minRightLoc - minLeftLoc) / ((float) (searchEnd - searchStart)) <= 0.15f) &&
         deltaRejection)
    {
        ALOGD("Right-hand side research");

        minVal = 1000.0f;
        currentIdx = peakLoc + 1;
        loopWhile = true;
        minRightLoc = currentIdx;

        // search right hand side
        while ( (currentIdx < searchEnd) && ((sampleMin + 0.0001f) < minVal) && loopWhile)
        {
            if (inData[currentIdx] < minVal)
            {
                minVal = inData[currentIdx];
                minRightLoc = currentIdx;
            }

            if (minVal < rightBumpMaxValueStage1)
            {
                if ((inData[currentIdx] - minVal) > rightBumpThresholdStage1) {
                    loopWhile = false;
                }
            }

            if (abs(minRightLoc - currentIdx) > minMaintainDuration) {
                loopWhile = false;
            }

            currentIdx++;
        }
    }
}

int DopplerPeakEstimator::insertControlPoints(float* inData, float* tmpMat, keyCtrlPts* keyPoints, int searchRange)
{
    // TODO: Minimum separation check
    int     tmpIdx;
    int     numLeftIns = 0;
    int     numRightIns = 0;
    int     peakLoc = keyPoints->peakLoc;
    int     minLeftLoc = keyPoints->minLeftLoc;
    int     minRightLoc = keyPoints->minRightLoc;
    int     realOffset = keyPoints->offset;
    int     totalIns;

    // not allowed condition
    if ((peakLoc < minLeftLoc) || (peakLoc > minRightLoc) || (minRightLoc < minLeftLoc) || (peakLoc <= 0))
        return 0;

    numLeftIns = (int) round((peakLoc - minLeftLoc) /((float) mSMParams.scrollSpeed) / CONTROL_POINT_SPAN_SEC);
    numRightIns = (int) round((minRightLoc - peakLoc) /((float) mSMParams.scrollSpeed) / CONTROL_POINT_SPAN_SEC);

    // MAX_INT_POINTS
    if ( (numLeftIns + numRightIns) > MAX_INT_POINTS )
    {
        ALOGD("Number of Ins Points - before adjustment: Left: %d, Right:%d", numLeftIns, numRightIns);
        totalIns = numLeftIns + numRightIns;

        numLeftIns = (int) floor( MAX_INT_POINTS / ((float) totalIns) * numLeftIns );
        numRightIns = (int) floor( MAX_INT_POINTS / ((float) totalIns) * numRightIns );
    }

    ALOGD("Number of Ins Points: Left: %d, Right: %d, Total Control Points: %d", numLeftIns, numRightIns, numLeftIns + numRightIns + 3);

    // peak index - mid index
    int     midIdx = numLeftIns + 1;
    int     rightIdx = midIdx + numRightIns + 1;

    tmpMat[CENTER_INDEX_POS] = (float) midIdx;
    tmpMat[LAST_INDEX_POS] = (float) rightIdx;
    tmpMat[OFFSET_POS] = (float) realOffset;

    // left index always 0
    tmpMat[0] = minLeftLoc;
    tmpMat[midIdx * 2] = peakLoc;
    tmpMat[rightIdx * 2] = minRightLoc;

    // left points
    tmpIdx = 1 * 2;
    for (int i = 1; i < (numLeftIns + 1); i ++)
    {
        tmpMat[tmpIdx] = minLeftLoc + i * ((peakLoc - minLeftLoc) / ((float) (numLeftIns + 1) ));
        tmpIdx += 2;
    }

    // right
    tmpIdx = (midIdx + 1) * 2;
    for (int i = 1; i < (numRightIns + 1); i++)
    {
        tmpMat[tmpIdx] = peakLoc + i * ((minRightLoc - peakLoc) /  ((float) (numRightIns + 1)));
        tmpIdx += 2;
    }

    for (int i = 0; i < (rightIdx + 1); i++)
    {
        tmpIdx = (int) tmpMat[2*i];
        if (tmpIdx < 0)
            tmpIdx = 0;

        if (tmpIdx > searchRange - 1)
            tmpIdx = searchRange - 1;

        tmpMat[2*i+1] = inData[tmpIdx];

        ALOGD("Control Points: %f, %f", tmpMat[2*i], tmpMat[2*i+1]);
    }

    return numLeftIns + numRightIns + 3;
}

void DopplerPeakEstimator::getDefaultControlPoints(float* inData, float* tmpMat, int searchRange)
{
    int minLeftLoc, peakLoc, minRightLoc;

    if (searchRange > 400)
    {
        minLeftLoc = 200;
        peakLoc = 300;
        minRightLoc = 400;
    }
    else
    {
        minRightLoc = searchRange;
        minLeftLoc = searchRange - 200;

        if (minLeftLoc < 0)
            minLeftLoc = 0;

        peakLoc = (int) ((minLeftLoc + minRightLoc) / 2);
    }

    ALOGD("Default Control Points - Left Loc: %d, Peak Loc: %d, Right Loc: %d", minLeftLoc, peakLoc, minRightLoc);

    int     tmpIdx;
    int     numLeftIns = 0;
    int     numRightIns = 0;

    numLeftIns = (int) round((peakLoc - minLeftLoc) * mImgWidth/((float) mSMParams.scrDataWidth) / CONTROL_POINT_SPAN);
    numRightIns = (int) round((minRightLoc - peakLoc) * mImgWidth/((float) mSMParams.scrDataWidth) / CONTROL_POINT_SPAN);

    // MAX_INT_POINTS
    if ( (numLeftIns + numRightIns) > MAX_INT_POINTS )
    {
        numLeftIns *= floor( MAX_INT_POINTS / ((float) numLeftIns + numRightIns) );
        numRightIns *= floor( MAX_INT_POINTS / ((float) numLeftIns + numRightIns) );
    }

    ALOGD("Number of Ins Points: Lef: %d, Right: %d, Total Control Points: %d", numLeftIns, numRightIns, numLeftIns + numRightIns + 3);

    // peak index - mid index
    int     midIdx = numLeftIns + 1;
    int     rightIdx = midIdx + numRightIns + 1;

    // left index always 0
    tmpMat[0] = minLeftLoc;
    tmpMat[midIdx * 2] = peakLoc;
    tmpMat[rightIdx * 2] = minRightLoc;

    tmpMat[CENTER_INDEX_POS] = (float) midIdx;
    tmpMat[LAST_INDEX_POS] = (float) rightIdx;
    tmpMat[OFFSET_POS] = 0.0f;

    // left points
    tmpIdx = 1 * 2;
    for (int i = 1; i < (numLeftIns + 1); i ++)
    {
        tmpMat[tmpIdx] = minLeftLoc + i * ((peakLoc - minLeftLoc) / ((float) (numLeftIns + 1) ));
        tmpIdx += 2;
    }

    // right
    tmpIdx = (midIdx + 1) * 2;
    for (int i = 1; i < (numRightIns + 1); i++)
    {
        tmpMat[tmpIdx] = peakLoc + i * ((minRightLoc - peakLoc) /  ((float) (numRightIns + 1)));
        tmpIdx += 2;
    }

    for (int i = 0; i < (rightIdx + 1); i++)
    {
        tmpIdx = (int) tmpMat[2*i];
        if (tmpIdx < 0)
            tmpIdx = 0;

        if (tmpIdx > searchRange - 1)
            tmpIdx = searchRange - 1;

        tmpMat[2*i+1] = inData[tmpIdx];

        ALOGD("Default Control Points: %f, %f", tmpMat[2*i], tmpMat[2*i+1]);
    }
}

// simple Median Filter, expects odd numbers for windowSize
void DopplerPeakEstimator::medianFilter(float* inData, float* outData, int arraySize, int windowSize)
{
    std::vector<float> window;
    int wh = windowSize/2;
    int ws = 0;

    for (int i = 0; i < arraySize; i++)
    {
        // clear
        window.clear();

        for (int j = i - wh; j <= i + wh; j++)
        {
            if (j >= 0 && j < arraySize)
                window.push_back(inData[j]);
        }

        std::sort(window.begin(), window.end());
        ws = window.size();
        outData[i] = window.at(ws/2);
    }
}

// simple Hole Fill Filter.
void DopplerPeakEstimator::holeFillFilter(float *inputPtrFloat, int arraySize, int threshold, int windowSize)
{
    float* tmpData = new float[arraySize + 1];

    // positive-side
    int wh = windowSize/2;
    int curVal;
    bool leftFound, rightFound;
    int curIndex;
    int leftVal, rightVal;

    // Positive-side
    for (int i = 0; i < arraySize; i++)
        tmpData[i] = *(inputPtrFloat + 2 * i);

    for (int i = wh; i < arraySize - wh; i++)
    {
        // reset flag and values
        leftVal = 0;
        rightVal = 0;
        leftFound = false;
        rightFound = false;

        // current index
        curIndex = i - 1;
        curVal = tmpData[i];

        while ((curIndex >= (i - wh)) && (leftFound == false))
        {
            if (abs(curVal - tmpData[curIndex]) > threshold)
            {

                leftVal = tmpData[curIndex];
                leftFound = true;
            }

            curIndex--;
        }

        curIndex = i + 1;

        while ((curIndex <= (i + wh)) && (rightFound == false))
        {
            if (abs(curVal - tmpData[curIndex]) > threshold)
            {
                rightVal = tmpData[curIndex];
                rightFound = true;
            }

            curIndex++;
        }

        if ( (leftVal - curVal) * (rightVal - curVal) > 0)
        {
            // found area to fill
            tmpData[i] = (leftVal + rightVal) / 2;
        }

    }

    for (int i = 0; i < arraySize; i++)
        *(inputPtrFloat + 2 * i) = tmpData[i];


    // Negative-side
    for (int i = 0; i < arraySize; i++)
        tmpData[i] = *(inputPtrFloat + 1 + 2 * i);

    for (int i = wh; i < arraySize - wh; i++)
    {
        // reset flag and values
        leftVal = 0;
        rightVal = 0;
        leftFound = false;
        rightFound = false;

        // current index
        curIndex = i - 1;
        curVal = tmpData[i];

        while ((curIndex >= (i - wh)) && (leftFound == false))
        {
            if (abs(curVal - tmpData[curIndex]) > threshold)
            {
                leftVal = tmpData[curIndex];
                leftFound = true;
            }

            curIndex--;
        }

        curIndex = i + 1;

        while ((curIndex <= (i + wh)) && (rightFound == false))
        {
            if (abs(curVal - tmpData[curIndex]) > threshold)
            {
                rightVal = tmpData[curIndex];
                rightFound = true;
            }

            curIndex++;
        }

        if ( (leftVal - curVal) * (rightVal - curVal) > 0)
        {
            // found area to fill
            tmpData[i] = (leftVal + rightVal) / 2;
        }

    }

    for (int i = 0; i < arraySize; i++)
        *(inputPtrFloat + 1 + 2 * i) = tmpData[i];

    // clean up
    delete[] tmpData;
}

