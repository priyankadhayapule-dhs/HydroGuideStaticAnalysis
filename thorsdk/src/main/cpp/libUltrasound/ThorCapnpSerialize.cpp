//
// Copyright 2020 EchoNous Inc.
//

#include <ThorCapnpSerialize.h>
#include <capnp/serialize.h>
#include <capnp/message.h>
#include <BitfieldMacros.h>

// For open()/close()
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

ThorStatus WriteThorCapnpFile(const char *path, CineBuffer &cinebuffer, uint32_t startFrame, uint32_t endFrame)
{
    int fd = ::open(path, O_RDWR | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);
    if (-1 == fd)
    {
    	LOGE("%s: Failed to open file for writing: %s, errno = %d", __func__, path, errno);
    	return TER_MISC_CREATE_FILE_FAILED;
    }

	capnp::MallocMessageBuilder message;
	auto thorClipBuilder = message.initRoot<echonous::capnp::ThorClip>();
	BuildThorClip(thorClipBuilder, cinebuffer, startFrame, endFrame);
	cinebuffer.onSave(startFrame, endFrame, thorClipBuilder);

	writeMessageToFd(fd, message);
	::close(fd);
	return THOR_OK;
}
ThorStatus ReadThorCapnpFile(const char *path, CineBuffer &cinebuffer, const char *dbPath)
{
	ThorStatus retVal = THOR_OK;
	int fd = ::open(path, O_RDONLY);
    if (-1 == fd)
    {
    	LOGE("%s: Failed to open file for reading: %s, errno = %d", __func__, path, errno);
    	return TER_MISC_OPEN_FILE_FAILED;
    }

    // If a file is completely invalid (e.g. an empty file) capnp will throw an exception
    // We could recompile capnp to instead call into a user supplied callback, this seems simpler
    // given that we already need to have exceptions enabled for SNPE
	try {
		capnp::ReaderOptions options;
		options.traversalLimitInWords =
				1024 * 1024 * 128; // expand traversal limit to allow up to 1GiB files
		capnp::StreamFdMessageReader message(fd, options);
		auto thorClipReader = message.getRoot<echonous::capnp::ThorClip>();
		retVal = ReadThorClip(thorClipReader, cinebuffer, dbPath);
		cinebuffer.onLoad(thorClipReader);
	} catch (std::exception &e)
	{
		LOGE("Exception while trying to read thor file in Capnp format: %s", e.what());
		if (fd != -1) { ::close(fd); }
		return TER_MISC_INVALID_FILE;
	}

	::close(fd);
	return retVal;
}

// This function is used only for M/PW/CW modes to update params and Peak/Mean data in ExamEdit
ThorStatus UpdateCineParamsThorCapnpFile(const char *path, CineBuffer &cinebuffer)
{
	LOGI("%s :  File path is %s ",__func__,path);
	int fd = ::open(path, O_RDONLY);
    if (-1 == fd)
    {
        LOGE("%s: Failed to open file for reading: %s, errno = %d", __func__, path, errno);
        return TER_MISC_OPEN_FILE_FAILED;
    }

    // If a file is completely invalid (e.g. an empty file) capnp will throw an exception
    // We could recompile capnp to instead call into a user supplied callback, this seems simpler
    // given that we already need to have exceptions enabled for SNPE
    try {
        capnp::ReaderOptions options;
        options.traversalLimitInWords = 1024 * 1024 * 128;
        capnp::StreamFdMessageReader message(fd, options);
        auto thorClipReader = message.getRoot<echonous::capnp::ThorClip>();

        capnp::MallocMessageBuilder messageBuilder;
        messageBuilder.setRoot(std::move(thorClipReader));		// deep copy
        auto thorClipBuilder = messageBuilder.getRoot<echonous::capnp::ThorClip>();

        // close read only fd
        ::close(fd);

        // reopen for writing
		fd = ::open(path, O_RDWR | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);
		if (-1 == fd)
		{
			LOGE("%s: Failed to open file for writing: %s, errno = %d", __func__, path, errno);
			return TER_MISC_CREATE_FILE_FAILED;
		}

		// update CineParams
		BuildCineParams(thorClipBuilder.initCineParams(), cinebuffer.getParams());

		// Peak/Mean Data Update
		if (cinebuffer.getParams().dopplerMeanDraw || cinebuffer.getParams().dopplerPeakDraw)
		{
			if (!thorClipBuilder.hasDopplerpeakmean())
				BuildScrollingData(thorClipBuilder.initDopplerpeakmean(), cinebuffer, cinebuffer.getMinFrameNum(),
						cinebuffer.getMaxFrameNum(), DAU_DATA_TYPE_DOP_PM);
		}

		writeMessageToFd(fd, messageBuilder);

    } catch (std::exception &e)
    {
        LOGE("Exception while trying to read thor file in Capnp format: %s", e.what());
        if (fd != -1) { ::close(fd); }
        return TER_MISC_INVALID_FILE;
    }

    ::close(fd);
    return THOR_OK;
}

//----------------------------------------------------------
// Writing/Building
// 

// Note that none of these functions return an error code.
// That is because the only failure case is an out of memory error, which
// is not really recoverable...
void BuildThorClip(echonous::capnp::ThorClip::Builder thorClipBuilder, CineBuffer &cinebuffer, uint32_t startFrame, uint32_t endFrame)
{
	// Ignore timestamp for now...
	thorClipBuilder.setNumFrames(endFrame-startFrame+1);
	auto cineParams = cinebuffer.getParams();
	BuildCineParams(thorClipBuilder.initCineParams(), cineParams);


    if (BF_GET(cineParams.dauDataTypes, DAU_DATA_TYPE_B, 1))
        BuildCineData(thorClipBuilder.initBmode(), cinebuffer, startFrame, endFrame, DAU_DATA_TYPE_B, MAX_B_FRAME_SIZE);

    if (IMAGING_MODE_COLOR == cineParams.imagingMode)
        BuildCineData(thorClipBuilder.initCmode(), cinebuffer, startFrame, endFrame, DAU_DATA_TYPE_COLOR, MAX_COLOR_FRAME_SIZE);

    if (IMAGING_MODE_M == cineParams.imagingMode)
        BuildScrollingData(thorClipBuilder.initMmode(), cinebuffer, startFrame, endFrame, DAU_DATA_TYPE_M);

	if (IMAGING_MODE_PW == cineParams.imagingMode) {
		BuildScrollingData(thorClipBuilder.initPwmode(), cinebuffer, startFrame, endFrame, DAU_DATA_TYPE_PW);

		// audio data, only when multi-frame
		if (startFrame != endFrame)
			BuildScrollingData(thorClipBuilder.initPwaudio(), cinebuffer, startFrame, endFrame, DAU_DATA_TYPE_PW_ADO);
	}

	if (IMAGING_MODE_CW == cineParams.imagingMode) {
		BuildScrollingData(thorClipBuilder.initCwmode(), cinebuffer, startFrame, endFrame, DAU_DATA_TYPE_CW);

		// audio data, only when multi-frame
		if (startFrame != endFrame)
			BuildScrollingData(thorClipBuilder.initCwaudio(), cinebuffer, startFrame, endFrame, DAU_DATA_TYPE_CW_ADO);
	}

	// Transition frame's peak mean data are also stored here
	if (cineParams.dopplerPeakDraw || cineParams.dopplerMeanDraw)
		BuildScrollingData(thorClipBuilder.initDopplerpeakmean(), cinebuffer, startFrame, endFrame, DAU_DATA_TYPE_DOP_PM);

	if (cineParams.isTransitionRenderingReady && (cineParams.renderedTransitionImgMode >= 0) )
	{
		BuildTransitionFrames(thorClipBuilder.initTransitionframe(), cinebuffer);
	}

    if (BF_GET(cineParams.dauDataTypes, DAU_DATA_TYPE_DA, 1))
        BuildScrollingData(thorClipBuilder.initDa(), cinebuffer, startFrame, endFrame, DAU_DATA_TYPE_DA);

    if (BF_GET(cineParams.dauDataTypes, DAU_DATA_TYPE_ECG, 1))
        BuildScrollingData(thorClipBuilder.initEcg(), cinebuffer, startFrame, endFrame, DAU_DATA_TYPE_ECG);
}
void BuildCineParams(echonous::capnp::CineParams::Builder cineParamsBuilder, const CineBuffer::CineParams &cineParams)
{
	BuildScanConverterParams(cineParamsBuilder.initBmodeParams(), cineParams.converterParams);
	BuildScanConverterParams(cineParamsBuilder.initCmodeParams(), cineParams.colorConverterParams);

	UpsReader::VersionInfo upsVersion = {0,0,0};
	cineParams.upsReader->getVersionInfo(upsVersion);

	cineParamsBuilder.setImagingCaseId(cineParams.imagingCaseId);
	cineParamsBuilder.setProbeType(cineParams.probeType);
	cineParamsBuilder.setOrganId(cineParams.organId);
	cineParamsBuilder.setDauDataTypes(cineParams.dauDataTypes);
	cineParamsBuilder.setImagingMode(cineParams.imagingMode);
	cineParamsBuilder.setFrameInterval(cineParams.frameInterval);
	cineParamsBuilder.setFrameRate(cineParams.frameRate);
	cineParamsBuilder.setUpsVersionMajor(upsVersion.releaseNumMajor);
	cineParamsBuilder.setUpsVersionMinor(upsVersion.releaseNumMinor);
	cineParamsBuilder.setScrollSpeed(cineParams.scrollSpeed);
	cineParamsBuilder.setTargetFrameRate(cineParams.targetFrameRate);
	cineParamsBuilder.setMlinesPerFrame(cineParams.mlinesPerFrame);
	cineParamsBuilder.setStillTimeShift(cineParams.stillTimeShift);
	cineParamsBuilder.setStillMLineTime(cineParams.stillMLineTime);

	cineParamsBuilder.setColorMapIndex(cineParams.colorMapIndex);
	cineParamsBuilder.setIsColorMapInverted(cineParams.isColorMapInverted);

	cineParamsBuilder.setEcgSamplesPerFrame(cineParams.ecgSamplesPerFrame);
	cineParamsBuilder.setEcgSamplesPerSecond(cineParams.ecgSamplesPerSecond);
	cineParamsBuilder.setDaEcgScrollSpeedIndex(cineParams.daEcgScrollSpeedIndex);

	cineParamsBuilder.setDaSamplesPerFrame(cineParams.daSamplesPerFrame);
	cineParamsBuilder.setDaSamplesPerSecond(cineParams.daSamplesPerSecond);

	cineParamsBuilder.setEcgSingleFrameWidth(cineParams.ecgSingleFrameWidth);
	cineParamsBuilder.setEcgTraceIndex(cineParams.ecgTraceIndex);

	cineParamsBuilder.setDaSingleFrameWidth(cineParams.daSingleFrameWidth);
	cineParamsBuilder.setDaTraceIndex(cineParams.daTraceIndex);

	cineParamsBuilder.setIsEcgExternal(cineParams.isEcgExternal);
	cineParamsBuilder.setEcgLeadNumExternal(cineParams.ecgLeadNumExternal);

	cineParamsBuilder.setMSingleFrameWidth(cineParams.mSingleFrameWidth);

	// Doppler Params
	cineParamsBuilder.setPwPrfIndex(cineParams.pwPrfIndex);
    cineParamsBuilder.setPwGateIndex(cineParams.pwGateIndex);

    cineParamsBuilder.setCwPrfIndex(cineParams.cwPrfIndex);
    cineParamsBuilder.setCwDecimationFactor(cineParams.cwDecimationFactor);

    cineParamsBuilder.setDopplerBaselineShiftIndex(cineParams.dopplerBaselineShiftIndex);
    cineParamsBuilder.setDopplerBaselineInvert(cineParams.dopplerBaselineInvert);
    cineParamsBuilder.setDopplerSamplesPerFrame(cineParams.dopplerSamplesPerFrame);
    cineParamsBuilder.setDopplerFFTSize(cineParams.dopplerFFTSize);
    cineParamsBuilder.setDopplerNumLinesPerFrame(cineParams.dopplerNumLinesPerFrame);
    cineParamsBuilder.setDopplerNumLinesPerFrameFloat(cineParams.dopplerNumLinesPerFrame);
    cineParamsBuilder.setDopplerSweepSpeedIndex(cineParams.dopplerSweepSpeedIndex);
    cineParamsBuilder.setDopplerFFTNumLinesToAvg(cineParams.dopplerFFTNumLinesToAvg);
    cineParamsBuilder.setDopplerAudioUpsampleRatio(cineParams.dopplerAudioUpsampleRatio);
    cineParamsBuilder.setDopplerVelocityScale(cineParams.dopplerVelocityScale);

    cineParamsBuilder.setColorBThreshold(cineParams.colorBThreshold);
    cineParamsBuilder.setColorVelThreshold(cineParams.colorVelThreshold);
    cineParamsBuilder.setIsTransitionRenderingReady(cineParams.isTransitionRenderingReady);
    cineParamsBuilder.setTransitionImagingMode(cineParams.transitionImagingMode);
    cineParamsBuilder.setTransitionAltImagingMode(cineParams.transitionAltImagingMode);
    cineParamsBuilder.setRenderedTransitionImgMode(cineParams.renderedTransitionImgMode);

    // zoomPan Params
    BuildZoomPanParams(cineParamsBuilder.initZoomPanParams(), cineParams.zoomPanParams);
    // traceindex
    cineParamsBuilder.setTraceIndex(cineParams.traceIndex);
    // doppler Wall Filter gain
    cineParamsBuilder.setDopplerWFGain(cineParams.dopplerWFGain);
    // draw Peak/Mean lines
    cineParamsBuilder.setDopplerPeakDraw(cineParams.dopplerPeakDraw);
    cineParamsBuilder.setDopplerMeanDraw(cineParams.dopplerMeanDraw);
    // doppler Peak Max
    cineParamsBuilder.setDopplerPeakMax(cineParams.dopplerPeakMax);
    // IsPWTDI
    cineParamsBuilder.setIsPWTDI(cineParams.isPWTDI);
    // M-mode Sweep Speed
    cineParamsBuilder.setMModeSweepSpeedIndex(cineParams.mModeSweepSpeedIndex);
    // Color Mode - CPD / CVD
    cineParamsBuilder.setColorDopplerMode(cineParams.colorDopplerMode);
}
void BuildScanConverterParams(echonous::capnp::ScanConverterParams::Builder scanConverterParamsBuilder, const ScanConverterParams &scanConverterParams)
{
	scanConverterParamsBuilder.setNumSampleMesh(scanConverterParams.numSampleMesh);
    scanConverterParamsBuilder.setNumRayMesh(scanConverterParams.numRayMesh);
    scanConverterParamsBuilder.setNumSamples(scanConverterParams.numSamples);
    scanConverterParamsBuilder.setNumRays(scanConverterParams.numRays);
    scanConverterParamsBuilder.setStartSampleMm(scanConverterParams.startSampleMm);
    scanConverterParamsBuilder.setSampleSpacingMm(scanConverterParams.sampleSpacingMm);
    scanConverterParamsBuilder.setRaySpacing(scanConverterParams.raySpacing);
    scanConverterParamsBuilder.setStartRayRadian(scanConverterParams.startRayRadian);
	scanConverterParamsBuilder.setProbeShape(scanConverterParams.probeShape);
	scanConverterParamsBuilder.setStartRayMm(scanConverterParams.startRayMm);
	scanConverterParamsBuilder.setSteeringAngleRad(scanConverterParams.steeringAngleRad);
}
void BuildZoomPanParams(echonous::capnp::ZoomPanParams::Builder zoomPanParamsBuilder, const float* zoomPanParams)
{
    zoomPanParamsBuilder.setZoomPanParams0(zoomPanParams[0]);
    zoomPanParamsBuilder.setZoomPanParams1(zoomPanParams[1]);
    zoomPanParamsBuilder.setZoomPanParams2(zoomPanParams[2]);
    zoomPanParamsBuilder.setZoomPanParams3(zoomPanParams[3]);
    zoomPanParamsBuilder.setZoomPanParams4(zoomPanParams[4]);
}
void BuildCineData(echonous::capnp::CineData::Builder cineDataBuilder, CineBuffer &cinebuffer, uint32_t startFrame, uint32_t endFrame, uint32_t dataType, uint32_t dataSize)
{
	uint32_t numFrames = endFrame-startFrame+1;
	auto framesBuilder = cineDataBuilder.initRawFrames(numFrames);
	for (uint32_t i=0; i != numFrames; ++i)
	{
		uint32_t frameNum = i+startFrame;

		// Capnp builders for this frame
		auto frameBuilder = framesBuilder[i];
		auto frameHeaderBuilder = frameBuilder.getHeader();
		auto frameDataBuilder = frameBuilder.initFrame(dataSize);

		// Frame header from the CineBuffer
		CineBuffer::CineFrameHeader *frameHeader = reinterpret_cast<CineBuffer::CineFrameHeader*>(
			cinebuffer.getFrame(frameNum, dataType, CineBuffer::FrameContents::IncludeHeader));

		// Frame data from the CineBuffer
		uint8_t *frameData = cinebuffer.getFrame(frameNum, dataType, CineBuffer::FrameContents::DataOnly);

		frameHeaderBuilder.setFrameNum(frameHeader->frameNum);
		frameHeaderBuilder.setTimeStamp(frameHeader->timeStamp);
		frameHeaderBuilder.setNumSamples(frameHeader->numSamples);
		frameHeaderBuilder.setHeartRate(frameHeader->heartRate);
		frameHeaderBuilder.setStatusCode(frameHeader->statusCode);
		std::copy(frameData, frameData+dataSize, frameDataBuilder.begin());
	}
}
void BuildScrollingData(echonous::capnp::ScrollData::Builder scrollDataBuilder, CineBuffer &cinebuffer, uint32_t startFrame, uint32_t endFrame, uint32_t inDataType)
{
	// Todo...
    uint32_t numFrames = endFrame-startFrame+1;
    auto framesBuilder = scrollDataBuilder.initRawFrames(numFrames);
    uint32_t dataSize;
    uint32_t dataType = inDataType;
    uint32_t texFrameType = -1u;

    if (endFrame == startFrame)
    {
        // single frame data
        switch(inDataType)
        {
            case DAU_DATA_TYPE_DA:
                dataType = DAU_DATA_TYPE_DA_SCR;
                dataSize = MAX_DA_SCR_FRAME_SIZE;
                break;
            case DAU_DATA_TYPE_ECG:
                dataType = DAU_DATA_TYPE_ECG_SCR;
                dataSize = MAX_ECG_SCR_FRAME_SIZE;
                break;
            case DAU_DATA_TYPE_M:
                dataType = DAU_DATA_TYPE_TEX;
                dataSize = MAX_TEX_FRAME_SIZE;
                texFrameType = TEX_FRAME_TYPE_M;
                break;
            case DAU_DATA_TYPE_PW:
                dataType = DAU_DATA_TYPE_TEX;
                dataSize = MAX_TEX_FRAME_SIZE;
                texFrameType = TEX_FRAME_TYPE_PW;
                break;
            case DAU_DATA_TYPE_CW:
                dataType = DAU_DATA_TYPE_TEX;
                dataSize = MAX_TEX_FRAME_SIZE;
                texFrameType = TEX_FRAME_TYPE_CW;
                break;
			case DAU_DATA_TYPE_DOP_PM:
				dataType = DAU_DATA_TYPE_DPMAX_SCR;
				dataSize = MAX_DOP_PEAK_MEAN_SCR_SIZE;
            default:
                break;
        }
    }
    else
    {
        // Don't need to change dataType
        // single frame data
        switch(inDataType)
        {
            case DAU_DATA_TYPE_DA:
                dataSize = MAX_DA_FRAME_SIZE;
                break;
            case DAU_DATA_TYPE_ECG:
                dataSize = MAX_ECG_FRAME_SIZE;
                break;
            case DAU_DATA_TYPE_M:
                dataSize = MAX_M_FRAME_SIZE;
                break;
			case DAU_DATA_TYPE_PW:
				dataSize = MAX_PW_FRAME_SIZE;
				break;
			case DAU_DATA_TYPE_CW:
				dataSize = MAX_CW_FRAME_SIZE;
				break;
			case DAU_DATA_TYPE_PW_ADO:
				dataSize = MAX_PW_ADO_FRAME_SIZE;
				break;
			case DAU_DATA_TYPE_CW_ADO:
				dataSize = MAX_CW_ADO_FRAME_SIZE;
				break;
			case DAU_DATA_TYPE_DOP_PM:
				dataSize = MAX_DOP_PEAK_MEAN_SIZE;
				break;
            default:
                break;
        }
    }

    for (uint32_t i=0; i != numFrames; ++i)
    {
        uint32_t frameNum = i+startFrame;

        // Capnp builders for this frame
        auto frameBuilder = framesBuilder[i];
        auto frameHeaderBuilder = frameBuilder.getHeader();
        auto frameDataBuilder = frameBuilder.initFrame(dataSize);

        if (texFrameType != -1u)
        	frameNum = texFrameType;

        // Frame header from the CineBuffer
        CineBuffer::CineFrameHeader *frameHeader = reinterpret_cast<CineBuffer::CineFrameHeader*>(
                cinebuffer.getFrame(frameNum, dataType, CineBuffer::FrameContents::IncludeHeader));

        // Frame data from the CineBuffer
        uint8_t *frameData = cinebuffer.getFrame(frameNum, dataType, CineBuffer::FrameContents::DataOnly);

        frameHeaderBuilder.setFrameNum(frameHeader->frameNum);
        frameHeaderBuilder.setTimeStamp(frameHeader->timeStamp);
        frameHeaderBuilder.setNumSamples(frameHeader->numSamples);
        frameHeaderBuilder.setHeartRate(frameHeader->heartRate);
        frameHeaderBuilder.setStatusCode(frameHeader->statusCode);
        std::copy(frameData, frameData+dataSize, frameDataBuilder.begin());
    }
}
void BuildTransitionFrames(echonous::capnp::ScrollData::Builder scrollDataBuilder, CineBuffer &cinebuffer)
{
	uint32_t numFrames;
	uint32_t frameTypes[2];
	uint32_t dataType = DAU_DATA_TYPE_TEX;
	uint32_t dataSize = MAX_TEX_FRAME_SIZE;

	int renderedTransMode = cinebuffer.getParams().renderedTransitionImgMode;

	switch(renderedTransMode)
	{
		case IMAGING_MODE_B:
			numFrames = 1;
			frameTypes[0] = TEX_FRAME_TYPE_B;
			break;
		case IMAGING_MODE_COLOR:
			numFrames = 2;
			frameTypes[0] = TEX_FRAME_TYPE_B;
			frameTypes[1] = TEX_FRAME_TYPE_C;
			break;
		case IMAGING_MODE_PW:
			numFrames = 1;
			frameTypes[0] = TEX_FRAME_TYPE_PW;
			break;
		case DAU_DATA_TYPE_CW:
			numFrames = 1;
			frameTypes[0] = TEX_FRAME_TYPE_CW;
			break;
		default:
			numFrames = 0;
			break;
	}

	if (numFrames < 1)
		return;

	// framesBuilder
	auto framesBuilder = scrollDataBuilder.initRawFrames(numFrames);

	for (uint32_t i=0; i != numFrames; ++i)
	{
		// Capnp builders for this frame
		auto frameBuilder = framesBuilder[i];
		auto frameHeaderBuilder = frameBuilder.getHeader();
		auto frameDataBuilder = frameBuilder.initFrame(dataSize);

		// Frame header from the CineBuffer
		CineBuffer::CineFrameHeader *frameHeader = reinterpret_cast<CineBuffer::CineFrameHeader*>(
				cinebuffer.getFrame(frameTypes[i], dataType, CineBuffer::FrameContents::IncludeHeader));

		// Frame data from the CineBuffer
		uint8_t *frameData = cinebuffer.getFrame(frameTypes[i], dataType, CineBuffer::FrameContents::DataOnly);

		frameHeaderBuilder.setFrameNum(frameHeader->frameNum);
		frameHeaderBuilder.setTimeStamp(frameHeader->timeStamp);
		frameHeaderBuilder.setNumSamples(frameHeader->numSamples);
		frameHeaderBuilder.setHeartRate(frameHeader->heartRate);
		frameHeaderBuilder.setStatusCode(frameHeader->statusCode);
		std::copy(frameData, frameData+dataSize, frameDataBuilder.begin());
	}
}

//----------------------------------------------------------
// Reading
//

ThorStatus ReadThorClip(echonous::capnp::ThorClip::Reader thorClipReader, CineBuffer &cinebuffer, const char *dbPath)
{
	ThorStatus retVal = THOR_OK;

	// If the CineBuffer has data, then reset clients
	if (cinebuffer.getMaxFrameNum() > 0)
    {
        cinebuffer.freeClients();
        cinebuffer.reset();
    }

	// Ignore timestamp and probetype for now...
	uint32_t numFrames = thorClipReader.getNumFrames();
	
	CineBuffer::CineParams cineParams = cinebuffer.getParams();
	ReadCineParams(thorClipReader.getCineParams(), cineParams);

	// read thor db before updating cineParams
	retVal = cineParams.upsReader->open(dbPath, cineParams.probeType);
	if (IS_THOR_ERROR(retVal))
	{
		ALOGE("Unable to open the UPS");
		return retVal;
	}
	ALOGD("UPS version : %s", cineParams.upsReader->getVersion());

	// set/update cine params
	cinebuffer.setParams(cineParams);

	if (thorClipReader.hasBmode())
		ReadCineData(thorClipReader.getBmode(), cinebuffer, DAU_DATA_TYPE_B);
	if (thorClipReader.hasCmode())
		ReadCineData(thorClipReader.getCmode(), cinebuffer, DAU_DATA_TYPE_COLOR);
	if (thorClipReader.hasMmode())
		ReadScrollingData(thorClipReader.getMmode(), cinebuffer, DAU_DATA_TYPE_M);
	if (thorClipReader.hasDa())
		ReadScrollingData(thorClipReader.getDa(), cinebuffer, DAU_DATA_TYPE_DA);
	if (thorClipReader.hasEcg())
		ReadScrollingData(thorClipReader.getEcg(), cinebuffer, DAU_DATA_TYPE_ECG);
	if (thorClipReader.hasPwmode())
		ReadScrollingData(thorClipReader.getPwmode(), cinebuffer, DAU_DATA_TYPE_PW);
	if (thorClipReader.hasCwmode())
		ReadScrollingData(thorClipReader.getCwmode(), cinebuffer, DAU_DATA_TYPE_CW);
	if (thorClipReader.hasPwaudio())
		ReadScrollingData(thorClipReader.getPwaudio(), cinebuffer, DAU_DATA_TYPE_PW_ADO);
	if (thorClipReader.hasCwaudio())
		ReadScrollingData(thorClipReader.getCwaudio(), cinebuffer, DAU_DATA_TYPE_CW_ADO);
	if (thorClipReader.hasDopplerpeakmean())
		ReadScrollingData(thorClipReader.getDopplerpeakmean(), cinebuffer, DAU_DATA_TYPE_DOP_PM);
	if (thorClipReader.hasTransitionframe())
		ReadTransitionFrames(thorClipReader.getTransitionframe(), cinebuffer);

	cinebuffer.setBoundary(0, numFrames-1);
	cinebuffer.setCurFrame(0);

	return retVal;
}
void ReadCineParams(echonous::capnp::CineParams::Reader cineParamsReader, CineBuffer::CineParams &cineParams)
{
	ReadScanConverterParams(cineParamsReader.getBmodeParams(), cineParams.converterParams);
	ReadScanConverterParams(cineParamsReader.getCmodeParams(), cineParams.colorConverterParams);

	cineParams.imagingCaseId = cineParamsReader.getImagingCaseId();
	cineParams.probeType = cineParamsReader.getProbeType();
	cineParams.organId = cineParamsReader.getOrganId();
	cineParams.dauDataTypes = cineParamsReader.getDauDataTypes();
	cineParams.imagingMode = cineParamsReader.getImagingMode();
	cineParams.frameInterval = cineParamsReader.getFrameInterval();
	cineParams.frameRate = cineParamsReader.getFrameRate();

	cineParams.scrollSpeed = cineParamsReader.getScrollSpeed();
	cineParams.targetFrameRate = cineParamsReader.getTargetFrameRate();
	cineParams.mlinesPerFrame = cineParamsReader.getMlinesPerFrame();
	cineParams.stillTimeShift = cineParamsReader.getStillTimeShift();
	cineParams.stillMLineTime = cineParamsReader.getStillMLineTime();
	cineParams.colorMapIndex = cineParamsReader.getColorMapIndex();
	cineParams.isColorMapInverted = cineParamsReader.getIsColorMapInverted();
	cineParams.ecgSamplesPerFrame = cineParamsReader.getEcgSamplesPerFrame();
	cineParams.ecgSamplesPerSecond = cineParamsReader.getEcgSamplesPerSecond();
	cineParams.daEcgScrollSpeedIndex = cineParamsReader.getDaEcgScrollSpeedIndex();
	cineParams.daSamplesPerFrame = cineParamsReader.getDaSamplesPerFrame();
	cineParams.daSamplesPerSecond = cineParamsReader.getDaSamplesPerSecond();
	cineParams.ecgSingleFrameWidth = cineParamsReader.getEcgSingleFrameWidth();
	cineParams.ecgTraceIndex = cineParamsReader.getEcgTraceIndex();
	cineParams.daSingleFrameWidth = cineParamsReader.getDaSingleFrameWidth();
	cineParams.daTraceIndex = cineParamsReader.getDaTraceIndex();
	cineParams.isEcgExternal = cineParamsReader.getIsEcgExternal();
	cineParams.ecgLeadNumExternal = cineParamsReader.getEcgLeadNumExternal();
	cineParams.mSingleFrameWidth = cineParamsReader.getMSingleFrameWidth();

    // Doppler Params
    cineParams.pwPrfIndex = cineParamsReader.getPwPrfIndex();
    cineParams.pwGateIndex = cineParamsReader.getPwGateIndex();

    cineParams.cwPrfIndex = cineParamsReader.getCwPrfIndex();
    cineParams.cwDecimationFactor = cineParamsReader.getCwDecimationFactor();

    cineParams.dopplerBaselineShiftIndex = cineParamsReader.getDopplerBaselineShiftIndex();
    cineParams.dopplerBaselineInvert = cineParamsReader.getDopplerBaselineInvert();
    cineParams.dopplerSamplesPerFrame = cineParamsReader.getDopplerSamplesPerFrame();
    cineParams.dopplerFFTSize = cineParamsReader.getDopplerFFTSize();
    cineParams.dopplerNumLinesPerFrame = cineParamsReader.getDopplerNumLinesPerFrameFloat();
    cineParams.dopplerSweepSpeedIndex = cineParamsReader.getDopplerSweepSpeedIndex();
    cineParams.dopplerFFTNumLinesToAvg = cineParamsReader.getDopplerFFTNumLinesToAvg();
    cineParams.dopplerAudioUpsampleRatio = cineParamsReader.getDopplerAudioUpsampleRatio();
    cineParams.dopplerVelocityScale = cineParamsReader.getDopplerVelocityScale();
    cineParams.colorBThreshold = cineParamsReader.getColorBThreshold();
    cineParams.colorVelThreshold = cineParamsReader.getColorVelThreshold();
    cineParams.isTransitionRenderingReady = cineParamsReader.getIsTransitionRenderingReady();
    cineParams.transitionImagingMode = cineParamsReader.getTransitionImagingMode();
    cineParams.transitionAltImagingMode = cineParamsReader.getTransitionAltImagingMode();
    cineParams.renderedTransitionImgMode = cineParamsReader.getRenderedTransitionImgMode();

    // for older versions - stored in Integer numbers
    if (cineParams.dopplerNumLinesPerFrame < 0.1f)
		cineParams.dopplerNumLinesPerFrame = cineParamsReader.getDopplerNumLinesPerFrame();

    // zoomPan Params
    ReadZoomPanParams(cineParamsReader.getZoomPanParams(), cineParams.zoomPanParams);
	// trace index
    cineParams.traceIndex = cineParamsReader.getTraceIndex();
    cineParams.dopplerWFGain = cineParamsReader.getDopplerWFGain();
    cineParams.dopplerPeakDraw = cineParamsReader.getDopplerPeakDraw();
    cineParams.dopplerMeanDraw = cineParamsReader.getDopplerMeanDraw();
    cineParams.dopplerPeakMax = cineParamsReader.getDopplerPeakMax();
    // PWTDI
    cineParams.isPWTDI = cineParamsReader.getIsPWTDI();
    // M-mode Sweep Speed Index
    cineParams.mModeSweepSpeedIndex = cineParamsReader.getMModeSweepSpeedIndex();
    // Color Doppler Mode - CPD/CVD
    cineParams.colorDopplerMode = cineParamsReader.getColorDopplerMode();
}
void ReadScanConverterParams(echonous::capnp::ScanConverterParams::Reader scanConverterParamsReader, ScanConverterParams &scanConverterParams)
{
	scanConverterParams.numSampleMesh = scanConverterParamsReader.getNumSampleMesh();
    scanConverterParams.numRayMesh = scanConverterParamsReader.getNumRayMesh();
    scanConverterParams.numSamples = scanConverterParamsReader.getNumSamples();
    scanConverterParams.numRays = scanConverterParamsReader.getNumRays();
    scanConverterParams.startSampleMm = scanConverterParamsReader.getStartSampleMm();
    scanConverterParams.sampleSpacingMm = scanConverterParamsReader.getSampleSpacingMm();
    scanConverterParams.raySpacing = scanConverterParamsReader.getRaySpacing();
    scanConverterParams.startRayRadian = scanConverterParamsReader.getStartRayRadian();
    scanConverterParams.probeShape = scanConverterParamsReader.getProbeShape();
    scanConverterParams.startRayMm = scanConverterParamsReader.getStartRayMm();
    scanConverterParams.steeringAngleRad = scanConverterParamsReader.getSteeringAngleRad();
}
void ReadZoomPanParams(echonous::capnp::ZoomPanParams::Reader zoomPanParamsReader, float* zoomPanParams)
{
    zoomPanParams[0] = zoomPanParamsReader.getZoomPanParams0();
    zoomPanParams[1] = zoomPanParamsReader.getZoomPanParams1();
    zoomPanParams[2] = zoomPanParamsReader.getZoomPanParams2();
    zoomPanParams[3] = zoomPanParamsReader.getZoomPanParams3();
    zoomPanParams[4] = zoomPanParamsReader.getZoomPanParams4();
}
void ReadCineData(echonous::capnp::CineData::Reader cineDataReader, CineBuffer &cinebuffer, uint32_t dataType)
{
	auto framesReader = cineDataReader.getRawFrames();
	uint32_t numFrames = framesReader.size();
	for (uint32_t i=0; i != numFrames; ++i)
	{
		auto frameReader = framesReader[i];
		auto frameHeaderReader = frameReader.getHeader();
		auto frameDataReader = frameReader.getFrame();

		// Frame header from the CineBuffer
		CineBuffer::CineFrameHeader *frameHeader = reinterpret_cast<CineBuffer::CineFrameHeader*>(
			cinebuffer.getFrame(i, dataType, CineBuffer::FrameContents::IncludeHeader));

		// Frame data from the CineBuffer
		uint8_t *frameData = cinebuffer.getFrame(i, dataType, CineBuffer::FrameContents::DataOnly);

		frameHeader->frameNum = frameHeaderReader.getFrameNum();
		frameHeader->timeStamp = frameHeaderReader.getTimeStamp();
		frameHeader->numSamples = frameHeaderReader.getNumSamples();
		frameHeader->heartRate = frameHeaderReader.getHeartRate();
		frameHeader->statusCode = frameHeaderReader.getStatusCode();
		std::copy(frameDataReader.begin(), frameDataReader.end(), frameData);
	}
}
void ReadScrollingData(echonous::capnp::ScrollData::Reader scrollDataReader, CineBuffer &cinebuffer, uint32_t inDataType)
{
	auto framesReader = scrollDataReader.getRawFrames();
	uint32_t numFrames = framesReader.size();
	uint32_t dataType = inDataType;
	uint32_t texFrameType = -1u;

	if (numFrames < 2)
	{
		switch (inDataType)
		{
			case DAU_DATA_TYPE_M:
				dataType = DAU_DATA_TYPE_TEX;
				texFrameType = TEX_FRAME_TYPE_M;
				break;

			case DAU_DATA_TYPE_PW:
				dataType = DAU_DATA_TYPE_TEX;
				texFrameType = TEX_FRAME_TYPE_PW;
				break;

			case DAU_DATA_TYPE_CW:
				dataType = DAU_DATA_TYPE_TEX;
				texFrameType = TEX_FRAME_TYPE_CW;
				break;

			case DAU_DATA_TYPE_DA:
				dataType = DAU_DATA_TYPE_DA_SCR;
				break;

			case DAU_DATA_TYPE_ECG:
				dataType = DAU_DATA_TYPE_ECG_SCR;
				break;

			case DAU_DATA_TYPE_DOP_PM:
				dataType = DAU_DATA_TYPE_DPMAX_SCR;
				break;

			default:
				ALOGE("Unsupported dataType!");
				return;
		}
	}

	for (uint32_t i=0; i != numFrames; ++i)
	{
		auto frameReader = framesReader[i];
		auto frameHeaderReader = frameReader.getHeader();
		auto frameDataReader = frameReader.getFrame();
		auto curFrameNum = i;

		if (texFrameType != -1u)
			curFrameNum = texFrameType;

		// Frame header from the CineBuffer
		CineBuffer::CineFrameHeader *frameHeader = reinterpret_cast<CineBuffer::CineFrameHeader*>(
				cinebuffer.getFrame(curFrameNum, dataType, CineBuffer::FrameContents::IncludeHeader));

		// Frame data from the CineBuffer
		uint8_t *frameData = cinebuffer.getFrame(curFrameNum, dataType, CineBuffer::FrameContents::DataOnly);

		frameHeader->frameNum = frameHeaderReader.getFrameNum();
		frameHeader->timeStamp = frameHeaderReader.getTimeStamp();
		frameHeader->numSamples = frameHeaderReader.getNumSamples();
		frameHeader->heartRate = frameHeaderReader.getHeartRate();
		frameHeader->statusCode = frameHeaderReader.getStatusCode();
		std::copy(frameDataReader.begin(), frameDataReader.end(), frameData);
	}
}
void ReadTransitionFrames(echonous::capnp::ScrollData::Reader scrollDataReader, CineBuffer &cinebuffer)
{
	auto framesReader = scrollDataReader.getRawFrames();
	uint32_t numFrames = framesReader.size();
	uint32_t frameTypes[2];
	uint32_t dataType = DAU_DATA_TYPE_TEX;

	int renderedTransMode = cinebuffer.getParams().renderedTransitionImgMode;

	switch(renderedTransMode)
	{
		case IMAGING_MODE_B:
			numFrames = 1;
			frameTypes[0] = TEX_FRAME_TYPE_B;
			break;
		case IMAGING_MODE_COLOR:
			numFrames = 2;
			frameTypes[0] = TEX_FRAME_TYPE_B;
			frameTypes[1] = TEX_FRAME_TYPE_C;
			break;
		case IMAGING_MODE_PW:
			numFrames = 1;
			frameTypes[0] = TEX_FRAME_TYPE_PW;
			break;
		case DAU_DATA_TYPE_CW:
			numFrames = 1;
			frameTypes[0] = TEX_FRAME_TYPE_CW;
			break;
		default:
			numFrames = 0;
			break;
	}

	if ((numFrames < 1) || !cinebuffer.getParams().isTransitionRenderingReady)
		return;

	for (uint32_t i=0; i != numFrames; ++i)
	{
		auto frameReader = framesReader[i];
		auto frameHeaderReader = frameReader.getHeader();
		auto frameDataReader = frameReader.getFrame();

		// Frame header from the CineBuffer
		CineBuffer::CineFrameHeader *frameHeader = reinterpret_cast<CineBuffer::CineFrameHeader*>(
				cinebuffer.getFrame(frameTypes[i], dataType, CineBuffer::FrameContents::IncludeHeader));

		// Frame data from the CineBuffer
		uint8_t *frameData = cinebuffer.getFrame(frameTypes[i], dataType, CineBuffer::FrameContents::DataOnly);

		frameHeader->frameNum = frameHeaderReader.getFrameNum();
		frameHeader->timeStamp = frameHeaderReader.getTimeStamp();
		frameHeader->numSamples = frameHeaderReader.getNumSamples();
		frameHeader->heartRate = frameHeaderReader.getHeartRate();
		frameHeader->statusCode = frameHeaderReader.getStatusCode();
		std::copy(frameDataReader.begin(), frameDataReader.end(), frameData);
	}
}