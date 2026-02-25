//
// Copyright 2016 EchoNous Inc.
//
//

#define LOG_TAG "DauNative"
#include <cstdint>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <cerrno>
#include <poll.h>
#include <ThorUtils.h>
#include <ElapsedTime.h>
#include <ThorConstants.h>
#include <Dau.h>
#include <DauPower.h>
#include <BitfieldMacros.h>
#include <PciDauMemory.h>
#include <UsbDauMemory.h>
#include <PciDmaBuffer.h>
#include <UsbDmaBuffer.h>
#include <PciDauError.h>
#include <UsbDauError.h>
#include <ImageProcess.h>
#include <DauHw.h>
#include <DauHwLinear.h>
#include <DauHwPhased.h>
#include <ScanDescriptor.h>
#include <ApiManager.h>

#include <BSmoothProcess.h>
#include <BSpatialFilterProcess.h>
#include <BEdgeFilterProcess.h>
#include <PersistenceProcess.h>
#include <PciDauFwUpgrade.h>
#include <UsbDauFwUpgrade.h>

#define ENABLE_DAECG 1
#define DAU_V4 1
//#define THOR_DEBUG_FILEPATH "/data/user/0/com.echonous.kosmosapp/thordebug"
#ifdef THOR_DEBUG_FILEPATH
#include <FromRawFileProcess.h>
#include <Filesystem.h>
#include <dirent.h>
#endif
// Define this to enable detection of external ECG leads
#define ENABLE_EXTERNAL_ECG 1
#define ENABLE_TEMP_MONITOR 1

#define DO_ASIC_CHECK 1
#define ASIC_FAULT_SIM 0

#define CW_CENTER_FREQ 1.953125f        // Fixed for CW as per THSW-666

//-----------------------------------------------------------------------------
Dau::Dau(DauContext& dauContext,
         CineBuffer& cineBuffer,
         CineViewer& cineViewer,
         const std::string& appPath) :
    mDauContext(dauContext),
    mDauSignalHandlerPtr(nullptr),
    mCallback((void*) this),
    mAssetManagerPtr(nullptr),
    mUpsReaderSPtr(nullptr),
    mDaumSPtr(nullptr),
    mDaumBar2SPtr(nullptr),
    mDauHwSPtr(nullptr),
    mLutManagerPtr(nullptr),
    mSequenceBuilderPtr(nullptr),
    mTbfManagerPtr(nullptr),
    mSseManagerPtr(nullptr),
    mInputManagerSPtr(nullptr),
    mGainManagerPtr(nullptr),
    mApiManagerPtr(nullptr),
    mBModeProcessor(),
    mColorProcessor(),
    mPwModeCommonProcessor(),
    mPwModeSpectralProcessor(),
    mPwModeAudioProcessor(),
    mCwModeCommonProcessor(),
    mCwModeSpectralProcessor(),
    mCwModeAudioProcessor(),
    mEcgProcessPtr(nullptr),
    mDaProcessPtr(nullptr),
    mDauArrayTestPtr(nullptr),
    mDopplerPeakMeanProc(nullptr),
    mColorAcqPtr(nullptr),
    mColorAcqLinearPtr(nullptr),
    mDopplerAcqPtr(nullptr),
    mDopplerAcqLinearPtr(nullptr),
    mCwAcqPtr(nullptr),
    mMmodeAcqPtr(nullptr),
    mDmaBufferSPtr(nullptr),
    mDauErrorPtr(nullptr),
    mReportErrorFuncPtr(nullptr),
    mReportHidFuncPtr(nullptr),
    mImagingMode(IMAGING_MODE_B),
    mMLinesPerFrame(0),
    mUpsReleaseType(0),
    mHvVoltage(MIN_HVPS_VOLTS),
    mColorGainPercent(50),
    mUsbDataEventSPtr(nullptr),
    mUsbHidEventSPtr(nullptr),
    mUsbErrorEventSPtr(nullptr),
    mUsbDataHandlerSPtr(nullptr),
    mCineBuffer(cineBuffer),
    mCineViewer(cineViewer),
    mAppPath(appPath),
    mIsUsSignal(true),
    mIsECGSignal(false),
    mIsDASignal(false),
    mIsDauStopped(true),
    mIsCompounding(false),
    mIsDauAttached(true),
    mUpdateCompoundFrames(false),
    mTempExtEcg(false),
    mDebugFlag(false),
    mLeadNoExternal(2),
    mTimerFd(-1),
    mDauRegRWServicePtr(nullptr),
    mDauTempThresholdVal(0.0),
    mIsTempSafetyTest(false),
    mIsDataUnderflowTest(false),
    mIsWatchDogResetTest(false),
    mIsDataOverflowTest(false)
{
}

//-----------------------------------------------------------------------------
Dau::~Dau()
{
    ALOGD("%s", __func__);

    close();
}

//#define TEST_API_BLACKBOX 1
//-----------------------------------------------------------------------------
ThorStatus Dau::open(const std::shared_ptr<UpsReader>& upsReader,
                     reportErrorFunc errFuncPtr,
                     reportHidFunc hidFuncPtr,
                     reportExternalEcgFunc extEcgFuncPtr)
{
    ThorStatus                  retVal = THOR_ERROR;
    DauPower                    dauPower(&mDauContext);
    const UpsReader::TransducerInfo *transducerInfoPtr;
    bool                        AsicOK = false;

#ifdef THOR_DEBUG_FILEPATH
    std::string thorPath = "";

    static int thor_index = 0;
    DIR* dir = opendir(THOR_DEBUG_FILEPATH);
    LOGD(THOR_DEBUG_FILEPATH);
    if (!dir)
    {
        fprintf(stderr, "[ THOR INPUT ] Failed to open directory, errno = %d\n", errno);
    }

    struct dirent *entry;
    char path[256];
    std::vector<std::string> file_array;
    while ((entry = readdir(dir)))
    {
        LOGD("[ THOR INPUT ] Checking Entry %s", entry->d_name);
        if (!strstr(entry->d_name, ".thor")) {
            LOGD("[ THOR INPUT ] %s not a thor file", entry->d_name);
            continue;
        }

        //if (0 != strcmp(entry->d_name, "11012019f39d49eb-4dcb-45d5-9bce-c8bbf76fb92b.thor"))
        //    continue;
        std::string filesdir = std::string(THOR_DEBUG_FILEPATH);
        snprintf(path, sizeof(path), "%s/%s", filesdir.c_str(), entry->d_name);
        printf("[ THOR INPUT ] Adding file %s\n", path);
        file_array.push_back(std::string(path));
        //break;
    }
    closedir(dir);
    if(thor_index > file_array.size())
        thor_index = 0;
    if(file_array.size() > 0) {
        thorPath = file_array[thor_index++];
        LOGD("[ THOR INPUT ] Loading %s", thorPath.c_str());
    }
    else
        LOGD("[ THOR INPUT ] No Files Found in thor debug filepath" );
#endif
#if DO_ASIC_CHECK
    uint32_t                    retry_count = 0;
#endif
    
    ALOGD("%s", __func__);

    mReportErrorFuncPtr = errFuncPtr;
    mReportHidFuncPtr = hidFuncPtr;
    mExternalEcgFuncPtr = extEcgFuncPtr;

    mUpsReaderSPtr = upsReader;

    if (mDauContext.isUsbContext)
    {
        mDmaBufferSPtr = std::make_shared<UsbDmaBuffer>();
        mDaumSPtr = std::make_shared<UsbDauMemory>();
        mDaumBar2SPtr = mDaumSPtr;
        mDauErrorPtr = new UsbDauError();
    }
    else
    {
        mDmaBufferSPtr = std::make_shared<PciDmaBuffer>();
        mDaumSPtr = std::make_shared<PciDauMemory>();
        mDaumBar2SPtr = std::make_shared<PciDauMemory>();
        mDauErrorPtr = new PciDauError();
    }
    if (!mDmaBufferSPtr->open(&mDauContext))
    {
        ALOGE("Unable to access Dau DMA Buffer");
        retVal = TER_IE_DMA_OPEN;
        goto err_ret;
    }

    if (mDauContext.isUsbContext)
    {
        mUsbDataEventSPtr = std::make_shared<TriggeredValue>();
        mUsbDataEventSPtr->init();
        mDauContext.dataPollFd.initialize(mUsbDataEventSPtr);

        mUsbHidEventSPtr = std::make_shared<TriggeredValue>();
        mUsbHidEventSPtr->init();
        mDauContext.hidPollFd.initialize(mUsbHidEventSPtr);

        mUsbErrorEventSPtr = std::make_shared<TriggeredValue>();
        mUsbErrorEventSPtr->init();
        mDauContext.errorPollFd.initialize(mUsbErrorEventSPtr);

        mUsbExternalEcgEventSPtr = std::make_shared<TriggeredValue>();
        mUsbExternalEcgEventSPtr->init();
        mDauContext.externalEcgPollFd.initialize(mUsbExternalEcgEventSPtr);

        mUsbDataHandlerSPtr = std::make_shared<UsbDataHandler>(mDauContext,
                                                mUsbDataEventSPtr,
                                                mUsbHidEventSPtr,
                                                mUsbErrorEventSPtr,
                                                mUsbExternalEcgEventSPtr);

        if (!mUsbDataHandlerSPtr->init((uint8_t *) mDmaBufferSPtr->getBufferPtr(DmaBufferType::Fifo),
                                      DauInputManager::INPUT_FIFO_LENGTH))
        {
            ALOGE("Unable to initialize UsbDataHandler");
            retVal = TER_IE_USB_INIT;
            goto err_ret;
        }
    }

    // TODO: For now Usb Dau is always on if plugged in.  Later there may be
    //       a way to tell it go in/out of low power state.
    if (!mDauContext.isUsbContext)
    {
        if (!dauPower.powerOn())
        {
            if (dauPower.getPowerState() == DauPowerState::Reboot)
            {
                ALOGE("Unable to power the Dau on, a reboot is required.");
                retVal = TER_IE_DAU_POWER_REBOOT;
            }
            else
            {
                ALOGE("Unable to power the Dau on");
                retVal = TER_IE_DAU_POWER;
            }

            goto err_ret;
        }
        usleep(1000*1000);
    }

    readProbeInfo();

    if (!mDaumSPtr->map(&mDauContext))
    {
        ALOGE("DauMemory::map() failed");
        retVal = TER_IE_DAU_MEM;
        goto err_ret;
    }

    ALOGD("Start DauRegRWService");
    ALOGD("Debug Flag Value:%d",mDebugFlag);
    if (mDauContext.isUsbContext)
    {
        if(mDebugFlag) {
            ALOGD("Debug Flag Value:%d",mDebugFlag);
            mDauRegRWServicePtr = new DauRegRWService(mDaumSPtr);
            if (!mDauRegRWServicePtr->init()) {
                ALOGE("Unable to initialize DauRegRWService");
                retVal = TER_IE_USB_INIT;
                goto err_ret;
            }
        }
    }
    ALOGD("Start DauRegRWService Done");

    if (!mDauContext.isUsbContext)
    {
        size_t  maxDmaLength;

        maxDmaLength = mDmaBufferSPtr->getBufferLength(DmaBufferType::Fifo) +
                       mDmaBufferSPtr->getBufferLength(DmaBufferType::Sequence);

        {
            std::shared_ptr<PciDauMemory> pciDauMemSPtr;
            pciDauMemSPtr = std::static_pointer_cast<PciDauMemory>(mDaumBar2SPtr);
            pciDauMemSPtr->selectBar(2);
            pciDauMemSPtr->map(&mDauContext);
        }

        if (bootAppImage(mDaumBar2SPtr) != THOR_OK)
        {
            ALOGE("FCS Unable to boot App Image");
            goto err_ret;
        }
        else
        {
            ALOGD("FCS Successfully  boot App Image");
        }

        if ((mProbeInfo.probeType & DEV_PROBE_MASK) == PROBE_TYPE_LINEAR)
        {
            mDauHwSPtr = std::make_shared<DauHwLinear>(mDaumSPtr, mDaumBar2SPtr);

            // Set clock rate to 250MHz for linear probe
            mInputManagerSPtr->configureFCSSwitchRegs(mDaumBar2SPtr, AX_FCS_250MHz);
        }
        else
        {
            mDauHwSPtr = std::make_shared<DauHwPhased>(mDaumSPtr, mDaumBar2SPtr);
        }

        mDauHwSPtr->initDmaMemBounds(maxDmaLength);
    }
    else
    {
        if ((mProbeInfo.probeType & DEV_PROBE_MASK) == PROBE_TYPE_LINEAR)
        {
            mDauHwSPtr = std::make_shared<DauHwLinear>(mDaumSPtr, mDaumSPtr);

            // Set clock rate to 250MHz for linear probe
            mUsbDataHandlerSPtr->configureFCSSwitch(AX_FCS_250MHz);
        }
        else
        {
            mDauHwSPtr = std::make_shared<DauHwPhased>(mDaumSPtr, mDaumSPtr);
        }
    }

#ifdef ENABLE_EXTERNAL_ECG
    if (supportsEcg())
    {
        mDauHwSPtr->initExternalEcgDetection();
    }
#endif

    retVal = mUpsReaderSPtr->open(mAppPath.c_str(), mProbeInfo.probeType);
    if (IS_THOR_ERROR(retVal))
    {
        ALOGE("Unable to open the UPS");
        goto err_ret;
    }
    ALOGD("UPS version : %s", mUpsReaderSPtr->getVersion());

    if (!mUpsReaderSPtr->checkIntegrity())
    {
        ALOGE("UPS integrity check failed");
        return (TER_IE_UPS_INTEGRITY);
    }
    else
    {
        ALOGD("UPS integrity check passed");
    }
    mUpsReaderSPtr->loadTransducerInfo();
    mUpsReaderSPtr->logTransducerInfo();
    mUpsReaderSPtr->loadGlobals();
    mUpsReaderSPtr->logGlobals();

    // CineBuffer transition flag reset
    mCineBuffer.setTransitionReadyFlag(false);
    mCineBuffer.setRenderedTransitionImgMode(-1);

    mGainManagerPtr = new DauGainManager(mDaumSPtr, mUpsReaderSPtr);
    mApiManagerPtr = new ApiManager(mUpsReaderSPtr, mProbeInfo.probeType);
    mInputManagerSPtr = std::make_shared<DauInputManager>(mDaumSPtr,
                                                          mDaumBar2SPtr,
                                                          mUpsReaderSPtr,
                                                          mDmaBufferSPtr,
                                                          mDauHwSPtr,
                                                          mUsbDataHandlerSPtr,
                                                          supportsEcg());
    mLutManagerPtr = new DauLutManager(mDaumSPtr, mUpsReaderSPtr);
    mSseManagerPtr = new DauSseManager(mDaumSPtr, mDmaBufferSPtr);
    mTbfManagerPtr = new DauTbfManager(mDaumSPtr);
    mSequenceBuilderPtr = new DauSequenceBuilder(mDaumSPtr, mUpsReaderSPtr, mDauHwSPtr);

#if ENABLE_TEMP_MONITOR
    {
        float       probeTemp;

        retVal = mDauHwSPtr->initTmpSensor(DauHw::I2C_ADDR_TMP0);
        if (IS_THOR_ERROR(retVal))
        {
            ALOGE("Unable to initialize Dau temperature sensor");
            goto err_ret;
        }

        retVal = mDauHwSPtr->readTmpSensor(DauHw::I2C_ADDR_TMP0,
                                           probeTemp);

        if (IS_THOR_ERROR(retVal))
        {
            ALOGE("Unable to read Dau temperature sensor");
            goto err_ret;
        }
        else
        {
            ALOGD("Probe Temperature: %.1fC", probeTemp);

            if (probeTemp >= mUpsReaderSPtr->getGlobalsPtr()->maxProbeTemperature || (mIsTempSafetyTest && probeTemp >= mDauTempThresholdVal))
            {
                ALOGE("The Dau is too hot");
                retVal = TER_THERMAL_PROBE;
                goto err_ret;
            }
        }
    }

    // Set up timer for checking temperature
    {
        struct timespec now;

        mTimerFd = timerfd_create(CLOCK_REALTIME, TFD_NONBLOCK);
        if (-1 == mTimerFd)
        {
            ALOGE("Unable to create timer");
            goto err_ret;
        }

        if (-1 == clock_gettime(CLOCK_REALTIME, &now))
        {
            ALOGE("Unable to get time");
            goto err_ret;
        }

        ALOGD("now.tv_sec = %u", now.tv_sec);

        mTimerSpec.it_value.tv_sec = now.tv_sec;
        mTimerSpec.it_value.tv_nsec = 0;
        mTimerSpec.it_interval.tv_sec = 15;
        mTimerSpec.it_interval.tv_nsec = 0;

        if (-1 == timerfd_settime(mTimerFd, TFD_TIMER_ABSTIME, &mTimerSpec, NULL))
        {
            ALOGE("Unable to set timer. errno = %d", errno);
            goto err_ret;
        }

        mDauContext.temperaturePollFd.initialize(PollFdType::Timer,
                                                 ::dup(mTimerFd));
    }
#endif

    mDauSignalHandlerPtr = new DauSignalHandler(mDauContext);
    if (!mDauSignalHandlerPtr->init(&mCallback))
    {
        ALOGE("Unable to initialize signal handler");
        retVal = TER_IE_SIGHANDLE_INIT;
        goto err_ret;
    }

    // ASIC Integrity Check
#if DO_ASIC_CHECK
    while (!AsicOK)
    {
        LOGD("BF_CHECK on open");
        AsicOK = mTbfManagerPtr->ASICinit();
#if ASIC_FAULT_SIM
        if (retry_count==0){
            AsicOK = false;
        }
#endif
        if (!AsicOK)
        {
            if (retry_count > 4)
            {
                LOGD("BF_CHECK on open: hard fail, too many retry events");
                ALOGE("Unable to power the Dau on, a reboot is required.");
                retVal = TER_DAU_ASIC_CHECK;
                goto err_ret;
            }
            else
            {
                LOGD("BF_CHECK FAIL on open number %d  doing power down/up",retry_count);
                dauPower.powerOff();
                dauPower.powerOn();
                retry_count += 1;
            }
        }
        else
        {
            retry_count = 0;
            LOGD("BF_CHECK on open: PASS! ");
        }
    }
#else
    AsicOK = mTbfManagerPtr->ASICinit();
    if (AsicOK)
        LOGD("BF_CHECK on open: PASS!");
#endif

    if (supportsEcg() || supportsDa())
    {
        mInputManagerSPtr->openDaEkg();
    }

    mBModeProcessor.addSequentialStage<DeInterleaveProcess>(mUpsReaderSPtr,
                                                              mInputManagerSPtr);
    // Disabled processors are commented out
    // mBModeProcessor.addSequentialStage<AutoGainProcess>(mUpsReaderSPtr);
    mBModeProcessor.addParallelStage<BSmoothProcess>(mUpsReaderSPtr);
    mBModeProcessor.addSequentialStage<BSpatialFilterProcess>(mUpsReaderSPtr);

    if ((mProbeInfo.probeType & DEV_PROBE_MASK) == PROBE_TYPE_LINEAR)
    {
        mBModeProcessor.addSequentialStage<SpatialCompoundProcess>(mUpsReaderSPtr);
    }

    mBModeProcessor.addSequentialStage<PersistenceProcess>(mUpsReaderSPtr);
    mBModeProcessor.addParallelStage<SpeckleReductionProcess>(mUpsReaderSPtr);

    if ((mProbeInfo.probeType & DEV_PROBE_MASK) == PROBE_TYPE_LINEAR)
    {
        mBModeProcessor.addParallelStage<BEdgeFilterProcess>(mUpsReaderSPtr);
    }

#ifdef THOR_DEBUG_FILEPATH
    mBModeProcessor.addSequentialStage<FromRawFileProcess>(mUpsReaderSPtr, thorPath.c_str());
#endif
    mColorProcessor.addSequentialStage<ColorProcess>(mUpsReaderSPtr);
    mMModeProcessor.addSequentialStage<DeInterleaveMProcess>(mUpsReaderSPtr,
                                                             mInputManagerSPtr);
    mMModeProcessor.addSequentialStage<MFilterProcess>(mUpsReaderSPtr);

    mBModeProcessor.setCineBuffer(&mCineBuffer);
    mColorProcessor.setCineBuffer(&mCineBuffer);
    mMModeProcessor.setCineBuffer(&mCineBuffer);
        
    mBModeProcessor.init();
    mColorProcessor.init();
    mMModeProcessor.init();

    // TODO: update with more efficient PW Processor
    mPwModeCommonProcessor.addSequentialStage<PwLowPassDecimationFilterProcess>(mUpsReaderSPtr);
    mPwModeCommonProcessor.addSequentialStage<DopplerWallFilterProcess>(mUpsReaderSPtr);
    mPwModeSpectralProcessor.addSequentialStage<DopplerSpectralProcess>(mUpsReaderSPtr);
    mPwModeSpectralProcessor.addSequentialStage<DopplerSpectralSmoothingProcess>(mUpsReaderSPtr);
    mPwModeAudioProcessor.addSequentialStage<DopplerHilbertProcess>(mUpsReaderSPtr);
    mPwModeAudioProcessor.addSequentialStage<DopplerAudioUpsampleProcess>(mUpsReaderSPtr);

    mPwModeCommonProcessor.setCineBuffer(&mCineBuffer);
    mPwModeSpectralProcessor.setCineBuffer(&mCineBuffer);
    mPwModeAudioProcessor.setCineBuffer(&mCineBuffer);

    mPwModeCommonProcessor.init();
    mPwModeSpectralProcessor.init();
    mPwModeAudioProcessor.init();

    // CW Processor
    mCwModeCommonProcessor.addSequentialStage<CwBandPassFilterProcess>(mUpsReaderSPtr);
    mCwModeCommonProcessor.addSequentialStage<DopplerWallFilterProcess>(mUpsReaderSPtr);
    mCwModeSpectralProcessor.addSequentialStage<DopplerSpectralProcess>(mUpsReaderSPtr);
    mCwModeSpectralProcessor.addSequentialStage<DopplerSpectralSmoothingProcess>(mUpsReaderSPtr);
    mCwModeAudioProcessor.addSequentialStage<DopplerHilbertProcess>(mUpsReaderSPtr);
    mCwModeAudioProcessor.addSequentialStage<DopplerAudioUpsampleProcess>(mUpsReaderSPtr);

    mCwModeCommonProcessor.setCineBuffer(&mCineBuffer);
    mCwModeSpectralProcessor.setCineBuffer(&mCineBuffer);
    mCwModeAudioProcessor.setCineBuffer(&mCineBuffer);

    mCwModeCommonProcessor.init();
    mCwModeSpectralProcessor.init();
    mCwModeAudioProcessor.init();

    // PW/CW Doppler Peak Mean Process
    mDopplerPeakMeanProc = new DopplerPeakMeanProcessHandler(mUpsReaderSPtr);
    mDopplerPeakMeanProc->setCineBuffer(&mCineBuffer);
    mDopplerPeakMeanProc->init();

    if (supportsEcg())
    {
        mEcgProcessPtr = new EcgProcess(mUpsReaderSPtr, mDauContext.isUsbContext);
        mEcgProcessPtr->init();
        mEcgProcessPtr->setCineBuffer(&mCineBuffer);

#ifdef ECG_PERFORMANCE_TEST
        mEcgProcessPtr->setDMABufferSPtr(mDmaBufferSPtr);
#endif
    }

    if (supportsDa())
    {
        mDaProcessPtr = new DaProcess(mDaumSPtr, mUpsReaderSPtr);
        mDaProcessPtr->init();
        mDaProcessPtr->setCineBuffer(&mCineBuffer);
    }

    mColorAcqPtr = new ColorAcq(mUpsReaderSPtr);
    mColorAcqPtr->fetchGlobals();

    mColorAcqLinearPtr = new ColorAcqLinear(mUpsReaderSPtr);
    mColorAcqLinearPtr->fetchGlobals();

    mMmodeAcqPtr = new MmodeAcq(mUpsReaderSPtr);
    mDauArrayTestPtr = new DauArrayTest(mDaumSPtr, mUpsReaderSPtr, mDauHwSPtr, mProbeInfo.probeType);

    mDopplerAcqPtr = new DopplerAcq(mUpsReaderSPtr);
    mDopplerAcqLinearPtr = new DopplerAcqLinear(mUpsReaderSPtr);
    mCwAcqPtr = new CWAcq(mUpsReaderSPtr);

    //------------------------------------------------------------------
    if (!mTbfManagerPtr->init(*mLutManagerPtr))
    {
        ALOGE("Unable to initialize TBF");
        retVal = TER_IE_TBF_INIT;
        goto err_ret;
    }

    //------------------------------------------------------------------
    if (THOR_OK != mSseManagerPtr->init())
    {
        ALOGE("Unable to initialize SSE");
        retVal = TER_IE_SSE_INIT;
        goto err_ret;
    }

    // The following is the code block to use V3 DAUs with faulty AFE connections
    // to the AX ASIC. Requires UPS (thor.db) version 34.27 or newer 
    {
        UpsReader::VersionInfo upsVersion;
        if ( mUpsReaderSPtr->getVersionInfo(upsVersion) )
        {
            ALOGD("UPS version %d:%d:%d",
                  upsVersion.releaseNumMajor,
                  upsVersion.releaseNumMinor,
                  upsVersion.releaseType );
            mUpsReleaseType = upsVersion.releaseType;
            
            if (mUpsReleaseType == -1)
            {
                ALOGD("disabling AFE interface for V3 board with no AFE connection");
                uint32_t val;
                val = 0x3;
                mDaumSPtr->write( &val, TBF_DP16 + TBF_BASE, 1 );
                val = 0x10300;
                mDaumSPtr->write( &val, TBF_DP12 + TBF_BASE, 1 );
            }
        }
        else
        {
            ALOGE("failure to read version info from UPS");
        }
    }

    if ( (mUpsReleaseType == UpsReader::RELEASE_TYPE_CLINICAL) ||
            (mUpsReleaseType == UpsReader::RELEASE_TYPE_PRODUCTION) )
    {
#ifdef TEST_API_BLACKBOX
        mApiManagerPtr->test();
#endif
        retVal = mApiManagerPtr->initApiTable();
        if (THOR_OK != retVal)
        {
            goto err_ret;
        }
        else
        {
#ifdef TEST_API_BLACKBOX
            mApiManagerPtr->logApiTable();
#endif
        }
    }

    //--------------------------------------------------------------------
    // TODO: following is a placeholder for doing a quick Dau self testing
    if (!mDauContext.isUsbContext)   // TODO: this check is needed until we have USB i/f to DAU
    {
        // BUGBUG: Temporarily removing this for V3 board.  The accelerometer
        //         and gyroscope are now combined on one chip.
        //retVal = DauHw::selfTest( mDaumSPtr);
        retVal = THOR_OK;
    }
    else
    {
        retVal = THOR_OK;
    }

    mDauHwSPtr->hvMonitorDisableInterrupts();

err_ret:
    return(retVal);
}

//-----------------------------------------------------------------------------
void Dau::close()
{
    ALOGD("%s", __func__);

    if (nullptr != mDauSignalHandlerPtr)
    {
        mDauSignalHandlerPtr->terminate();
        delete mDauSignalHandlerPtr;
        mDauSignalHandlerPtr = nullptr;
    }

    if (-1 != mTimerFd)
    {
        mTimerSpec.it_value.tv_sec = 0;
        mTimerSpec.it_value.tv_nsec = 0;
        mTimerSpec.it_value.tv_sec = 0;
        mTimerSpec.it_value.tv_sec = 0;

        timerfd_settime(mTimerFd, TFD_TIMER_ABSTIME, &mTimerSpec, NULL);

        ::close(mTimerFd);
        mTimerFd = -1;
    }

    if (nullptr != mGainManagerPtr)
    {
        delete mGainManagerPtr;
        mGainManagerPtr = nullptr;
    }

    if (nullptr != mApiManagerPtr)
    {
        delete mApiManagerPtr;
        mApiManagerPtr = nullptr;
    }

    mInputManagerSPtr = nullptr;

    if (nullptr != mSseManagerPtr)
    {
        delete mSseManagerPtr;
        mSseManagerPtr = nullptr;
    }

    if (nullptr != mTbfManagerPtr)
    {
        delete mTbfManagerPtr;
        mTbfManagerPtr = nullptr;
    }

    if (nullptr != mSequenceBuilderPtr)
    {
        delete mSequenceBuilderPtr;
        mSequenceBuilderPtr = nullptr;
    }

    if (mDauContext.isUsbContext && nullptr != mDauRegRWServicePtr)
    {
        delete mDauRegRWServicePtr;
        mDauRegRWServicePtr = nullptr;
    }

    if (mDauContext.isUsbContext)
    {
        // USB mode. No shutdown, but reset and config for low power state
        if(mDaumSPtr != nullptr && mIsDauAttached)
        {
            uint32_t val = 0;
            mDaumSPtr->write(&val, SYS_MASK_INT0_ADDR, 1);

            LOGD("Resetting BF USB...");
            usleep(50000);

            val = ~BIT(FTSCU_SW_RESET0_BF_BIT);
            mDaumSPtr->write(&val, FTSCU_SW_RESET0_ADDR, 1);

            // Following writes to RESET1 and RESET2 registers are needed
            // to make sure that no other interrupt is enabled.
            val = 0xffffffff;
            mDaumSPtr->write(&val, FTSCU_SW_RESET1_ADDR, 1);
            mDaumSPtr->write(&val, FTSCU_SW_RESET2_ADDR, 1);
            usleep(10);

            mDaumSPtr->read(FTSCU_PWRMOD_ADDR, &val, 1);
            val |= BIT(FTSCU_PWRMOD_SW_RST_BIT);
            mDaumSPtr->write(&val, FTSCU_PWRMOD_ADDR, 1);
            usleep(10000);

            val = 0xFFFFFFFF;
            mDaumSPtr->write(&val, FTSCU_SW_RESET0_ADDR, 1);

            LOGD("After reseting BF");
            val = 0xFE9E;
            mDaumSPtr->write(&val, SYS_GPIOD_ADDR, 1);

            // BF out of reset
            val = 0x0;
            mDaumSPtr->write(&val, TBF_REG_TR0 + TBF_BASE, 1);

            // Disable analog power
            val = 0xE;
            mDaumSPtr->write( &val, TBF_REG_PS0 + TBF_BASE, 1);

            // Put TX chips in low power state
            if (nullptr != mLutManagerPtr)
            {
                mLutManagerPtr->TXchipLP();
            }

            // Enable clock gating for power savings in frozen state
            val = 0x1F;
            mDaumSPtr->write(&val, TBF_REG_CK0 + TBF_BASE, 1);
        }

        if (nullptr != mUsbDataHandlerSPtr)
        {
            mUsbDataHandlerSPtr->terminate();
            mUsbDataHandlerSPtr = nullptr;
        }
    }

    if (nullptr != mDauErrorPtr)
    {
        delete mDauErrorPtr;
        mDauErrorPtr = nullptr;
    }

    if (nullptr != mDmaBufferSPtr)
    {
        mDmaBufferSPtr->close();
        mDmaBufferSPtr = nullptr;
    }

    if (nullptr != mLutManagerPtr)
    {
        delete mLutManagerPtr;
        mLutManagerPtr = nullptr;
    }

    mBModeProcessor.terminate();
    mColorProcessor.terminate();
    mMModeProcessor.terminate();
    mPwModeCommonProcessor.terminate();
    mPwModeSpectralProcessor.terminate();
    mPwModeAudioProcessor.terminate();
    mCwModeCommonProcessor.terminate();
    mCwModeSpectralProcessor.terminate();
    mCwModeAudioProcessor.terminate();
    mDopplerPeakMeanProc->terminate();

    if (nullptr != mDopplerPeakMeanProc)
    {
        delete mDopplerPeakMeanProc;
        mDopplerPeakMeanProc = nullptr;
    }
    if (nullptr != mEcgProcessPtr)
    {
        delete mEcgProcessPtr;
        mEcgProcessPtr = nullptr;
    }
    if (nullptr != mDaProcessPtr)
    {
        delete mDaProcessPtr;
        mDaProcessPtr = nullptr;
    }

    if (nullptr != mColorAcqPtr)
    {
        delete mColorAcqPtr;
        mColorAcqPtr = nullptr;
    }

    if (nullptr != mColorAcqLinearPtr)
    {
        delete mColorAcqLinearPtr;
        mColorAcqLinearPtr = nullptr;
    }


    if (nullptr != mDopplerAcqPtr)
    {
        delete mDopplerAcqPtr;
        mDopplerAcqPtr = nullptr;
    }

    if (nullptr != mDopplerAcqLinearPtr)
    {
        delete mDopplerAcqLinearPtr;
        mDopplerAcqLinearPtr = nullptr;
    }

    if (nullptr != mCwAcqPtr)
    {
        delete mCwAcqPtr;
        mCwAcqPtr = nullptr;
    }

    if (nullptr != mMmodeAcqPtr)
    {
        delete mMmodeAcqPtr;
        mMmodeAcqPtr = nullptr;
    }

    if (nullptr != mDauArrayTestPtr)
    {
        delete mDauArrayTestPtr;
        mDauArrayTestPtr = nullptr;
    }

    // reset transition ready flag
    mCineBuffer.setTransitionReadyFlag(false);

    if (!mDauContext.isUsbContext && (-1 != mDauContext.dauDataLinkFd))
    {
        DauPower    dauPower(&mDauContext);

        dauPower.powerOff();
    }

    mDaumSPtr = nullptr;
    mDaumBar2SPtr = nullptr;
    mDauHwSPtr = nullptr;

    mIsDataUnderflowTest = false;
    mIsTempSafetyTest = false;
    mIsDataOverflowTest = false;
    mIsWatchDogResetTest = false;
    mDauTempThresholdVal = 0.0;
    mDauContext.clear();
}

//-----------------------------------------------------------------------------
ThorStatus Dau::runTransducerElementCheck(int results[])
{
    // TODO: need to check if test parameters have been initialized
    results[0] = mDauArrayTestPtr->run();
    return (THOR_OK);
}

bool Dau::isTransducerElementCheckRunning() {
    return mDauArrayTestPtr->isElementCheckRunning();
}

//-----------------------------------------------------------------------------
bool Dau::start()
{
    std::unique_lock<std::mutex> lock(mLock);
    bool            retVal = false;
    bfDtEnable      mapDtEP= {0};
    CineBuffer::CineParams cParams;
    uint32_t        regVal = 0;
    ALOGD("%s", __func__);

#ifdef GENERATE_TEST_IMAGES
    populateDmaFifo();
#endif
    mCineViewer.setLive(true);

    mDauDataTypes = getDauDataTypes(mImagingMode, mIsDASignal, mIsECGSignal, mIsUsSignal);

    cParams = mCineBuffer.getParams();
    cParams.dauDataTypes = mDauDataTypes;
    cParams.isEcgExternal = mTempExtEcg;
    cParams.ecgLeadNumExternal = mLeadNoExternal;
    cParams.probeType = mProbeInfo.probeType;
    mCineBuffer.setParams(cParams);

    // Tint adjustment - CineViewer.
    float r,g,b;
    mUpsReaderSPtr->getTint(r,g,b);
    mCineViewer.setTintAdjustment(r, g, b);
    //
    // Enable analog power
    uint32_t val = 0xF;
    mDaumSPtr->write( &val, TBF_REG_PS0 + TBF_BASE, 1);
    usleep(1000*100);
    //
    // Disable clock gating
    uint32_t ckval = 0x100;
    mDaumSPtr->write( &ckval, TBF_REG_CK0 + TBF_BASE, 1);

    // Prepare CW mode if CW mode
    if (mIsUsSignal && (mImagingMode == IMAGING_MODE_CW))
    {
        // Load CW BLOB
        uint32_t numReg = 62;
        // TODO: load BLOB
        mDaumSPtr->write(mCwAcqPtr->getSequenceBlobPtr(), TBF_REG_BP0 + TBF_BASE, numReg);

        // The number of AFE config words for CW is fixed at 15
        // For CW we use the direct write queue MSRTAFE rather than the init LUT
        // These AFE spi writes will be done as soon words are written to the queue
        // The configuration done in CWAcq.cpp still writes to the table for MSAFEINIT
        // This is usually handled by a sequencer but CW does not use one.
        uint32_t val = 0x4;
        mDaumSPtr->write(&val, TBF_REG_TR0 + TBF_BASE,1);
        val = 0x0;
        mDaumSPtr->write(&val, TBF_REG_TR0 + TBF_BASE,1);
        uint32_t* lutPtr = mCwAcqPtr->getLutMSAFEINIT();
        uint32_t bufSize = 15;
        mDaumSPtr->write(lutPtr, TBF_LUT_MSRTAFE + TBF_BASE, bufSize);

        // update registers, change divclks
        // initCWImaging gets I2S started
        mInputManagerSPtr->initCwImaging();
    }

    mInputManagerSPtr->setImagingCase(mImagingCase, mImagingMode, mIsDASignal, mIsECGSignal, mIsUsSignal);

    if ( mIsECGSignal || mIsDASignal )
    {
        if (nullptr != mDaProcessPtr)
        {
            mDaProcessPtr->initializeProcessIndices();
        }

        if (nullptr != mEcgProcessPtr)
        {
            mEcgProcessPtr->initializeProcessIndices();
        }

        // Normally this is set by the sequencer, but in no US mode it is not.
        if (!mIsUsSignal) {
            uint32_t psclk;
            mDaumSPtr->read(TBF_REG_PS1+TBF_BASE, &psclk, 1 );
            psclk = (0x7f << 16) | (psclk & 0xff00ffff);
            mDaumSPtr->write( &psclk, TBF_REG_PS1+TBF_BASE, 1 );
        }

        mInputManagerSPtr->setExternalECG(mTempExtEcg);
        //Pass lead number every time is fine, Lead number will be used only when External ECG is Enable
        mInputManagerSPtr->setLeadNumber(mLeadNoExternal);
        mInputManagerSPtr->initDaEkg(mIsDASignal, mIsECGSignal);
    }

    if (!mDauSignalHandlerPtr->start())
    {
        ALOGE("Unable to start signal handler");
        goto err_ret;
    }

    if (mDauContext.isUsbContext)
    {
        regVal = SYS_MSICTL_INL_MASK;
        mDaumSPtr->write(&regVal, SYS_MSICTL_ADDR, 1);

        switch(mImagingMode)
        {
            case IMAGING_MODE_B:
                ALOGD("B-MODE Is Activated");
                mapDtEP.dt0 = EP4;
                if(mIsDASignal || mIsECGSignal)
                {
                    mInputManagerSPtr->setExternalECG(mTempExtEcg);
                    //Pass lead number every time is fine, Lead number will be used only when External ECG is Enable
                    mInputManagerSPtr->setLeadNumber(mLeadNoExternal);
                    mInputManagerSPtr->initDaEkg(mIsDASignal, mIsECGSignal);
                    mInputManagerSPtr->startDaEkg();

                    mUsbDataHandlerSPtr->setDataTypeOffset(DAU_DATA_TYPE_ECG,DauInputManager::INPUT_BASE_OFFSET_ECG);
                    mUsbDataHandlerSPtr->setDataTypeOffset(DAU_DATA_TYPE_DA,DauInputManager::INPUT_BASE_OFFSET_DA);
                    mUsbDataHandlerSPtr->setFrameSize(mInputManagerSPtr->getDaFrameStride(),DAU_DATA_TYPE_DA);
                    mUsbDataHandlerSPtr->setFrameSize(mInputManagerSPtr->getEcgFrameStride(),DAU_DATA_TYPE_ECG);
                    mUsbDataHandlerSPtr->enableUS(mIsUsSignal);
                    mUsbDataHandlerSPtr->enableECG(mIsECGSignal);
                    mUsbDataHandlerSPtr->enableDa(mIsDASignal);
                    if (mIsDASignal)
                        mapDtEP.dt7 = EP11;

                    if (mIsECGSignal)
                        mapDtEP.dt6 = EP10;
                }

                if(!mUsbDataHandlerSPtr->mapDtEp(mapDtEP)) /* Endpoint Associativity */
                {
                    ALOGE("Endpoint associativity failed");
                    goto err_ret;
                }
                mUsbDataHandlerSPtr->setDataTypeOffset(IMAGING_MODE_B,DauInputManager::INPUT_BASE_OFFSET_B);
                mUsbDataHandlerSPtr->start(IMAGING_MODE_B);
                break;
            case IMAGING_MODE_COLOR:
                ALOGD("COLOR Mode Is Activated");
                mapDtEP.dt0 = EP4;
                mapDtEP.dt1 = EP5;
                if (mIsDASignal || mIsECGSignal)
                {
                    mInputManagerSPtr->setExternalECG(mTempExtEcg);
                    //Pass lead number every time is fine, Lead number will be used only when External ECG is Enable
                    mInputManagerSPtr->setLeadNumber(mLeadNoExternal);
                    mInputManagerSPtr->initDaEkg(mIsDASignal, mIsECGSignal);
                    mInputManagerSPtr->startDaEkg();

                    mUsbDataHandlerSPtr->setDataTypeOffset(DAU_DATA_TYPE_ECG,DauInputManager::INPUT_BASE_OFFSET_ECG);
                    mUsbDataHandlerSPtr->setDataTypeOffset(DAU_DATA_TYPE_DA,DauInputManager::INPUT_BASE_OFFSET_DA);
                    mUsbDataHandlerSPtr->setFrameSize(mInputManagerSPtr->getDaFrameStride(),DAU_DATA_TYPE_DA);
                    mUsbDataHandlerSPtr->setFrameSize(mInputManagerSPtr->getEcgFrameStride(),DAU_DATA_TYPE_ECG);
                    mUsbDataHandlerSPtr->enableUS(mIsUsSignal);
                    mUsbDataHandlerSPtr->enableECG(mIsECGSignal);
                    mUsbDataHandlerSPtr->enableDa(mIsDASignal);

                    if (mIsDASignal)
                        mapDtEP.dt7 = EP11;

                    if (mIsECGSignal)
                        mapDtEP.dt6 = EP10;
                }

                if(!mUsbDataHandlerSPtr->mapDtEp(mapDtEP)) /* Endpoint Associativity */
                {
                    ALOGE("Endpoint associativity failed");
                    goto err_ret;
                }
                mUsbDataHandlerSPtr->setDataTypeOffset(IMAGING_MODE_COLOR,DauInputManager::INPUT_BASE_OFFSET_COLOR);
                mUsbDataHandlerSPtr->start(IMAGING_MODE_COLOR);
                break;
            case IMAGING_MODE_M:
                ALOGD("M-Mode Is Activated");
                mapDtEP.dt0 = EP4;
                mapDtEP.dt3 = EP7;

                if(!mUsbDataHandlerSPtr->mapDtEp(mapDtEP)) /* Endpoint Associativity */
                {
                    ALOGE("Endpoint associativity failed");
                    goto err_ret;
                }
                mUsbDataHandlerSPtr->setDataTypeOffset(IMAGING_MODE_M,DauInputManager::INPUT_BASE_OFFSET_M);
                mUsbDataHandlerSPtr->start(IMAGING_MODE_M);
                break;
            case IMAGING_MODE_PW:
                ALOGD("PW Is Activated");
                mapDtEP.dt2 = EP6;
                if(!mUsbDataHandlerSPtr->mapDtEp(mapDtEP)) /* Endpoint Associativity */
                {
                    ALOGE("Endpoint associativity failed");
                    goto err_ret;
                }
                mUsbDataHandlerSPtr->setDataTypeOffset(IMAGING_MODE_PW,DauInputManager::INPUT_BASE_OFFSET_PW);
                mUsbDataHandlerSPtr->start(IMAGING_MODE_PW);
                break;
            case IMAGING_MODE_CW:
                ALOGD("CW Is Activated");
                mapDtEP.dt4 = EP8;
                if(!mUsbDataHandlerSPtr->mapDtEp(mapDtEP)) /* Endpoint Associativity */
                {
                    ALOGE("Endpoint associativity failed");
                    goto err_ret;
                }
                mUsbDataHandlerSPtr->setDataTypeOffset(IMAGING_MODE_CW,DauInputManager::INPUT_BASE_OFFSET_CW);
                mUsbDataHandlerSPtr->start(IMAGING_MODE_CW);
                break;
            default:
                ALOGE("Invalid Imaging Mode");
                goto err_ret;
        }
    }

    mInputManagerSPtr->start();
    mTbfManagerPtr->start();

    // Note that HV is configured for CW mode in DauInputManager InitCWImaging()
    if (IMAGING_MODE_CW != mImagingMode)
    {
        // Enable TX chip low power state prior to HV ramp up
        mLutManagerPtr->TXchipLP();
        ALOGD("PMONCTRL starting HV power supply, voltage = %f", mHvVoltage);
        if (mImagingMode != IMAGING_MODE_CW)
            mDauHwSPtr->startHvSupply(mHvVoltage);
        // Wake up TX chips from low power state
        mLutManagerPtr->TXchipON();
    }

    if ( mIsECGSignal || mIsDASignal )
    {
        mInputManagerSPtr->startDaEkg();

        // Normally this is set by the sequencer, but in no US mode it is not.
        if (!mIsUsSignal)
        {
            uint32_t psclk;
            mDaumSPtr->read(TBF_REG_PS1+TBF_BASE, &psclk, 1 );
            psclk = (0x7f << 16) | (psclk & 0xff00ffff);
            mDaumSPtr->write( &psclk, TBF_REG_PS1+TBF_BASE, 1 );
        }
        else if (mSequenceBuilderPtr->getPmNodeDuration() < 300000)
            usleep(550);
    }

    if (mIsUsSignal)
    {
        if (mImagingMode != IMAGING_MODE_CW)
        {
            if (THOR_OK != mSseManagerPtr->start())
                goto err_ret;
        }
        else
        {
            // start CW Imaging
            mInputManagerSPtr->startCwImaging();
        }

        if (mIsCompounding)
            mUpdateCompoundFrames = true;
        else
            mUpdateCompoundFrames = false;
    }

    mDauHwSPtr->hvMonitorSetSampleCount(mSequenceBuilderPtr->getActiveImagingInterval());
    ALOGD("ECG is on = %d, DA is on = %d", mIsECGSignal, mIsDASignal);

    mDauHwSPtr->initWatchdogTimer();

    retVal = true;

err_ret:
    if (retVal)
    {
        mIsDauStopped = false;
    }
    return(retVal);
}

//-----------------------------------------------------------------------------
ThorStatus Dau::stop()
{
    std::unique_lock<std::mutex> lock(mLock);
    mIsDauStopped = true;
    ThorStatus retVal = THOR_ERROR;
    ALOGD("%s", __func__);

    if (!mIsDauAttached)
    {
        return(THOR_OK);
    }

    if (mIsDASignal || mIsECGSignal)
    {
        mInputManagerSPtr->stopDaEkg();
    }

    mInputManagerSPtr->stop();

    if (mImagingMode == IMAGING_MODE_CW)
        retVal = THOR_OK;
    else
        retVal = mSseManagerPtr->stop(mSequenceBuilderPtr->getFrameIntervalMs());

    if (retVal != THOR_OK)
    {
        return (retVal);
    }
    mTbfManagerPtr->stop();

    if (mDauContext.isUsbContext)
    {
        mUsbDataHandlerSPtr->stop();
    }

    mDauSignalHandlerPtr->stop();
    mCineViewer.setLive(false);

    // Stop I2S and turn clkDiv back
    if (mIsUsSignal && (mImagingMode == IMAGING_MODE_CW))
    {
        // stop CW Imaging
        mInputManagerSPtr->stopCwImaging();

        // reset process indices
        mCwModeCommonProcessor.getStage<CwBandPassFilterProcess>().each(&CwBandPassFilterProcess::resetIndices);
    }

    // set Reset Flag for BMode Persistence Process
    if (mIsUsSignal)
    {
        mBModeProcessor.getStage<PersistenceProcess>().each(&PersistenceProcess::setResetFlag);
    }

    mDauHwSPtr->stopHvSupply();

    reset();

    // Put TX chips in low power state
    mLutManagerPtr->TXchipLP();

    // Disable 100MHz clock
    uint32_t val = 0x302;
    mDaumSPtr->write(&val, SYS_TCTL_ADDR, 1);
    val = 0x400;
    mDaumSPtr->write(&val, SEQ_DEBUG_ADDR, 1);
    val = 0x0102;
    mDaumSPtr->write(&val, TBF_DP17 + TBF_BASE, 1);

    mDauHwSPtr->hvMonitorDisableInterrupts();

    mDauHwSPtr->cancelWatchdogTimer();

    // overwrite first two frames when compounding is enabled.
    if ( mUpdateCompoundFrames && (mImagingMode == IMAGING_MODE_B) &&
            (mCineBuffer.getMaxFrameNum() > 1) && (mCineBuffer.getMinFrameNum() == 0) )
    {
        // copy frame no. 2 to 0 and 1
        memcpy(mCineBuffer.getFrame(0, DAU_DATA_TYPE_B),
               mCineBuffer.getFrame(2, DAU_DATA_TYPE_B), MAX_B_FRAME_SIZE);
        memcpy(mCineBuffer.getFrame(1, DAU_DATA_TYPE_B),
               mCineBuffer.getFrame(2, DAU_DATA_TYPE_B), MAX_B_FRAME_SIZE);

        mUpdateCompoundFrames = false;
    }

    return (retVal);
}

//-----------------------------------------------------------------------------
ThorStatus Dau::reset()
{
    uint32_t val=0;
    ThorStatus retVal = THOR_OK;

    // TODO: when we need to power down/up dau
    // DauPower                    dauPower(&mDauContext);

    if (!mDauContext.isUsbContext)
    {
        // TODO: DAU Power Cycle
        // Trying a DAU power cycle on every reset
        //dauPower.powerOff();
        //dauPower.powerOn();

        LOGD("Resetting BF PCIe...");
        usleep(50000);
        val = ~BIT(FTSCU_SW_RESET0_BF_BIT);
        mDaumBar2SPtr->write( &val, FTSCU_SW_RESET0_ADDR, 1);
        // Following writes to RESET1 and RESET2 registers are needed
        // to make sure that no other interrupt is enabled.
        val = 0xffffffff;
        mDaumBar2SPtr->write( &val, FTSCU_SW_RESET1_ADDR, 1);
        mDaumBar2SPtr->write( &val, FTSCU_SW_RESET2_ADDR, 1);
        usleep(1000);
        mDaumBar2SPtr->read(FTSCU_PWRMOD_ADDR, &val, 1);
        val |= BIT(FTSCU_PWRMOD_SW_RST_BIT);
        mDaumBar2SPtr->write( &val, FTSCU_PWRMOD_ADDR, 1);
        usleep(10000);
        val = 0xFFFFFFFF;
        mDaumBar2SPtr->write( &val, FTSCU_SW_RESET0_ADDR, 1);
        LOGD("After Resetting BF");
        val = 0xFE9E;
        mDaumSPtr->write( &val, SYS_GPIOD_ADDR, 1);

        //
        // Enable clock gating for power savings in frozen state
        val = 0xF;
        mDaumSPtr->write( &val, TBF_REG_CK0 + TBF_BASE, 1);
        //
        // Disable analog power in frozen state
        val = 0xE;
        mDaumSPtr->write( &val, TBF_REG_PS0 + TBF_BASE, 1);

        mTbfManagerPtr->reinit();
        mSseManagerPtr->init();

        if (mImagingMode == IMAGING_MODE_COLOR)
        {
            if ((mProbeInfo.probeType & DEV_PROBE_MASK) == PROBE_TYPE_LINEAR)
                mLutManagerPtr->startSpiWrite(mColorAcqLinearPtr->getBaseConfigLength());
            else
                mLutManagerPtr->startSpiWrite(mColorAcqPtr->getBaseConfigLength());
        }
        else if (mImagingMode == IMAGING_MODE_PW)
        {
            if ((mProbeInfo.probeType & DEV_PROBE_MASK) == PROBE_TYPE_LINEAR)
            {
                mLutManagerPtr->startSpiWrite(mDopplerAcqLinearPtr->getBaseConfigLength());
            }
            else
                mLutManagerPtr->startSpiWrite(mDopplerAcqPtr->getBaseConfigLength());
        }
        else if (mImagingMode == IMAGING_MODE_CW)
        {
            mLutManagerPtr->startSpiWrite(mCwAcqPtr->getBaseConfigLength());
        }
        else
        {
            mLutManagerPtr->startSpiWrite(mUpsReaderSPtr->getBaseConfigLength(mImagingCase));
        }

        if (supportsEcg() || supportsDa())
        {
            mInputManagerSPtr->openDaEkg();
        }
    }
    else
    {
        val = 0;
        mDaumSPtr->write(&val, SYS_MASK_INT0_ADDR, 1);

        LOGD("Resetting BF USB...");
        usleep(50000);

        val = ~BIT(FTSCU_SW_RESET0_BF_BIT);
        mDaumSPtr->write( &val, FTSCU_SW_RESET0_ADDR, 1);

        // Following writes to RESET1 and RESET2 registers are needed
        // to make sure that no other interrupt is enabled.
        val = 0xffffffff;
        mDaumSPtr->write( &val, FTSCU_SW_RESET1_ADDR, 1);
        mDaumSPtr->write( &val, FTSCU_SW_RESET2_ADDR, 1);
        usleep(10);

        /* TODO: Discuss with client . Here Application get crashed*/

        mDaumSPtr->read(FTSCU_PWRMOD_ADDR, &val, 1);
        val |= BIT(FTSCU_PWRMOD_SW_RST_BIT);
        mDaumSPtr->write( &val, FTSCU_PWRMOD_ADDR, 1);
        usleep(10000);

        val = 0xFFFFFFFF;
        mDaumSPtr->write( &val, FTSCU_SW_RESET0_ADDR, 1);

        LOGD("After reseting BF");
        val = 0xFE9E;
        mDaumSPtr->write( &val, SYS_GPIOD_ADDR, 1);
        //
        // Enable clock gating for power savings in frozen state
        val = 0xF;
        mDaumSPtr->write( &val, TBF_REG_CK0 + TBF_BASE, 1);

        mTbfManagerPtr->reinit();
        mSseManagerPtr->init();
        mInputManagerSPtr->setImagingCase(mImagingCase, mImagingMode,mIsDASignal,mIsECGSignal,mIsUsSignal);
        if (mImagingMode == IMAGING_MODE_COLOR)
        {
            if ((mProbeInfo.probeType & DEV_PROBE_MASK) == PROBE_TYPE_LINEAR)
                mLutManagerPtr->startSpiWrite(mColorAcqLinearPtr->getBaseConfigLength());
            else
                mLutManagerPtr->startSpiWrite(mColorAcqPtr->getBaseConfigLength());
        }
        else
        {
            mLutManagerPtr->startSpiWrite(mUpsReaderSPtr->getBaseConfigLength(mImagingCase));
        }

        if (supportsEcg() || supportsDa())
        {
            mInputManagerSPtr->openDaEkg();
        }

        // Disable analog power in frozen state
        val = 0xE;
        mDaumSPtr->write( &val, TBF_REG_PS0 + TBF_BASE, 1);
    }
#ifdef ENABLE_EXTERNAL_ECG
    if (supportsEcg())
    {
        mDauHwSPtr->initExternalEcgDetection();
    }
#endif

    return (retVal);
}

//-----------------------------------------------------------------------------
ThorStatus Dau::bootAppImage(const std::shared_ptr<DauMemory>& daumSPtr) {

    ThorStatus  retStat = THOR_ERROR;
    uint32_t    respTimeOut = 0x1FFF;  // ~8 Sec timeout
    uint32_t    val = 0;
    
    ALOGD("Send Boot Application Image Command\n");
    daumSPtr->write(&val, PCI_BIST_RESP_ADDR, 1);
    usleep(1000);

    val = TLV_TAG_BOOT_APPLICATION_IMAGE;
    daumSPtr->write(&val, PCI_BIST_CMD_ADDR, 1);
    ALOGD("Send Boot Application Image Command Done\n");
    usleep(10000); // Firmware application start to execute timing

    do {
        val = 0;
        daumSPtr->read(PCI_BIST_RESP_ADDR, &val, 1);
        ALOGD("Boot Application Read: 0x%x",val);
        switch (val) {
            case PCI_RESP_OK:
                ALOGD("Successfully Boot Application \n");
                retStat = THOR_OK;
                break;
            case PCI_RESP_ERR:
                ALOGE("Failed to Boot Application\n");
                goto err_ret;
            case PCI_FW_APPLICATION:
                ALOGD("Application FW is already in execution\n");
                retStat = THOR_OK;
                break;
        }
        usleep(1000);
    } while ((val != PCI_RESP_OK) && (val != PCI_FW_APPLICATION) && respTimeOut-- );

err_ret:
    return retStat;
}

/**
 * @brief Set imaging case for B mode 
 *
 * @param imagingCaseId - ID of the imaging case listed in ImagingCases table of UPS
 *
 * @param apiEstimates - MI, TI, .. estimated by AP&I Manager
 *     [0] : MI
 *     [1] : TI
 *     [2] : TIB
 *     [3] : TIS
 *     [4..7] :  to be deremined
 *
 * @param imagingInfo
 *     [0] : B centerFrequency
 *     [1] : unused (relevant in colorflow)
 *     [2] : frame rate
 *     [3] : dutyCycle
 *     [4] : txVoltage
 *     [5..7] : to be determined
 *
 * @return  status - THOR_OK if no errors are encountered.
 */
ThorStatus Dau::setImagingCase(uint32_t imagingCaseId,
                               float apiEstimates[8],
                               float imagingInfo[8])
{
    std::unique_lock<std::mutex> lock(mLock);
    ThorStatus              retVal = THOR_ERROR;
    CineBuffer::CineParams  cineParams;
    uint32_t                mask;
    ScanConverterParams     converterParams;
    ScanConverterParams     colorConverterParams;
    uint32_t                numViews = 1;
    uint32_t                crossWidth;
    uint32_t                organId;
    uint32_t                organGlobalId;
    float                   steeringAngleDegrees = 0.0f;


    ALOGD("Dau::setImagingCase(%d)", imagingCaseId);
    if ( 0 != mUpsReaderSPtr->getErrorCount() )
    {
        ALOGE("Dau::setImagingCase() UPS error count was not zero on entry");
        return (TER_IE_UPS_READ_FAILURE);
    }
    mImagingCase = imagingCaseId;
    mImagingMode = mUpsReaderSPtr->getImagingMode(imagingCaseId);
    mDauDataTypes = getDauDataTypes(mImagingMode, mIsDASignal, mIsECGSignal);
    organId = mUpsReaderSPtr->getImagingOrgan(mImagingCase);
    organGlobalId = mUpsReaderSPtr->getOrganGlobalId(organId);
    ALOGD("Dau::mImagingMode = %d, mDauDataType = %08X probeType = %d", mImagingMode, mDauDataTypes, mProbeInfo.probeType);

    // retrieve values in the current cineParams
    cineParams = mCineBuffer.getParams();

    if ((mProbeInfo.probeType & DEV_PROBE_MASK) == PROBE_TYPE_LINEAR)
    {
        converterParams.probeShape = PROBE_SHAPE_LINEAR;
        mUpsReaderSPtr->getSCParams(imagingCaseId, numViews, steeringAngleDegrees, crossWidth);
        // this is scan converter steering angle, not compounding steering angle
        converterParams.steeringAngleRad = 0.0f;
    }
    else
    {
        if (organGlobalId == BLADDER_GLOBAL_ID)
            converterParams.probeShape = PROBE_SHAPE_PHASED_ARRAY_FLATTOP;
        else
            converterParams.probeShape = PROBE_SHAPE_PHASED_ARRAY;
    }
    mUpsReaderSPtr->getScanConverterParams(imagingCaseId,
                                           converterParams,
                                           mImagingMode);
#ifdef GENERATE_TEST_IMAGES
    populateDmaFifo();
#endif

    /*
     * Setting up the signal path starting from downstream to upstream
     */

    //------- Image Processing ---------------
    ImageProcess::ProcessParams   processParams = 
    {
        .imagingCaseId  = imagingCaseId,
        .imagingMode    = mImagingMode,
        .numSamples     = converterParams.numSamples,
        .numRays        = converterParams.numRays
    };
    mBModeProcessor.setProcessParams(processParams);

    //------- Frame Sequence -----------------
    if (numViews > 1) {
        retVal = mSequenceBuilderPtr->buildBCompoundSequence(imagingCaseId, numViews, mImagingMode);
        mIsCompounding = true;
    }
    else {
        retVal = mSequenceBuilderPtr->buildBSequence(imagingCaseId, mImagingMode);
        mIsCompounding = false;
    }

    if (THOR_OK != retVal)
    {
        ALOGE("buildCSequence returned 0x%08X", retVal);
        return (retVal);
    }
    
    //------- Beamformer & Signal Processing implemented in ASIC/FPGA
    mTbfManagerPtr->setImagingCase(*mLutManagerPtr, imagingCaseId, mImagingMode);  
    mInputManagerSPtr->setBFrameSize(converterParams.numSamples, converterParams.numRays);
    if (supportsEcg())
    {
        mInputManagerSPtr->setEcgFrameSize( mSequenceBuilderPtr->getNumEcgSamplesPerFrame() );
    }
    mInputManagerSPtr->setDaFrameSize( mSequenceBuilderPtr->getNumDaSamplesPerFrame() );
#ifdef ENABLE_DAECG
    if (supportsEcg() || supportsDa())
    {
        setDaEcgCineParams(cineParams);
    }
#endif
    mInputManagerSPtr->setImagingCase(imagingCaseId, mImagingMode, mIsDASignal, mIsECGSignal, mIsUsSignal);

    if(mDauContext.isUsbContext)
    {
        mUsbDataHandlerSPtr->enableUS(mIsUsSignal);
        mUsbDataHandlerSPtr->enableDa(mIsDASignal);
        mUsbDataHandlerSPtr->enableECG(mIsECGSignal);
    }

    cineParams.organId = mUpsReaderSPtr->getImagingOrgan(mImagingCase);
    cineParams.converterParams = converterParams;
    cineParams.colorConverterParams = colorConverterParams;
    cineParams.imagingCaseId = imagingCaseId;
    cineParams.dauDataTypes = mDauDataTypes;
    cineParams.imagingMode = mImagingMode;
    cineParams.frameInterval = mSequenceBuilderPtr->getFrameIntervalMs();
    cineParams.frameRate = mSequenceBuilderPtr->getFrameRate();
    cineParams.upsReader = mUpsReaderSPtr;
    cineParams.colorMapIndex = 0;
    cineParams.isColorMapInverted = false;

    mCineBuffer.setCineParamsOnly(cineParams);

    if (mDauContext.isUsbContext)
    {
        mUsbDataHandlerSPtr->setFrameSize(mInputManagerSPtr->getBFrameStride(), IMAGING_MODE_B);
    }

    if ( 0 != mUpsReaderSPtr->getErrorCount())
    {
        ALOGE("Dau::setImagingCase : UPS read errors = %d", mUpsReaderSPtr->getErrorCount());
        return (TER_IE_UPS_READ_FAILURE);
    }

    retVal = getApiEstimates( imagingCaseId, apiEstimates);
    if (THOR_OK != retVal )
    {
        return (retVal);
    }
    getImagingInfo( imagingCaseId, imagingInfo);
    if ( 0 != mUpsReaderSPtr->getErrorCount())
    {
        ALOGE("Dau::setImagingCase : UPS read errors = %d", mUpsReaderSPtr->getErrorCount());
        mUpsReaderSPtr->clearErrorCount();
        return (TER_IE_UPS_READ_FAILURE);
    }
    else
    {
        ALOGD("Dau::setImagingCase : NO UPS READ ERRORS");
        return (mSequenceBuilderPtr->loadSequence(mImagingMode));
    }
}

/**
 * @brief Set imaging case for color flow
 *
 * @param imagingCaseId - ID of the imaging case listed in ImagingCases table of UPS
 *
 * @param colorParams - additional input parameters specific for colorflow imaging
 *     [0] : PRF Index
 *     [1] : Flow Speed Index
 *     [2] : Wall Filter Index
 *     [3] : Max PRF Index
 *     [4] : ColorMode: CVD/CPD
 *        
 * @param roiParams - attributes of color ROI
 *     [0] : axialStart
 *     [1] : axialSpan
 *     [2] : azimuthStart
 *     [3] : azimuthSpan
 *
 * @param apiEstimates - MI, TI, .. estimated by AP&I Manager
 *     [0] : MI
 *     [1] : TI
 *     [2] : TIB
 *     [3] : TIS
 *     [4..7] :  to be deremined
 *
 * @param imagingInfo
 *     [0] : B centerFrequency
 *     [1] : Color centerFrequency
 *     [2] : frame rate
 *     [3] : dutyCycle
 *     [4] : txVoltage
 *     [5..7] : to be determined
 *
 * @return  status - THOR_OK if no errors are encountered.
 */
ThorStatus Dau::setColorImagingCase(uint32_t imagingCaseId,
                                    int colorParams[5],
                                    float roiParams[5],
                                    float apiEstimates[8],
                                    float imagingInfo[8])
{
    std::unique_lock<std::mutex> lock(mLock);
    ThorStatus  retVal = THOR_OK;
    ScanDescriptor requestedRoi;
    ScanDescriptor actualRoi;
    uint32_t requestedPrfIndex = (uint32_t)colorParams[0];
    uint32_t actualPrfIndex = requestedPrfIndex;
    uint32_t maxAchievablePrfIndex = 0;
    ScanConverterParams converterParams;
    ScanConverterParams colorConverterParams;
    uint32_t            defaultColorMapIndex;
    CineBuffer::CineParams  cineParams;
    bool                    isLinearProbe;
    float                   colorRxPitch = 0.0f;
    float                   imagingDepth;
    float                   minRoiAxialStart;
    float                   minRoiAxialSpan;

    ALOGD("Dau::setColorImagingCase(%d)", imagingCaseId);

    if ( 0 != mUpsReaderSPtr->getErrorCount() )
    {
        ALOGE("%s UPS error count was not zero on entry", __func__);
        return (TER_IE_UPS_READ_FAILURE);
    }

    mImagingCase = imagingCaseId;
    mImagingMode = mUpsReaderSPtr->getImagingMode(imagingCaseId);
    mDauDataTypes = getDauDataTypes(mImagingMode, mIsDASignal, mIsECGSignal);
    mUpsReaderSPtr->getDefaultColorMapIndex(imagingCaseId, defaultColorMapIndex);
    mUpsReaderSPtr->getImagingDepthMm( imagingCaseId, imagingDepth );
    minRoiAxialSpan = mUpsReaderSPtr->getGlobalsPtr()->minRoiAxialSpan;
    minRoiAxialStart = mUpsReaderSPtr->getGlobalsPtr()->minRoiAxialStart;

    // retrieve values in the current cineParams
    cineParams = mCineBuffer.getParams();

    // Setup B-mode params
    {
        if ((mProbeInfo.probeType & DEV_PROBE_MASK) == PROBE_TYPE_LINEAR)
        {
            converterParams.probeShape = PROBE_SHAPE_LINEAR;
            colorConverterParams.probeShape = PROBE_SHAPE_LINEAR;
            colorConverterParams.steeringAngleRad = roiParams[4]/180.0f * PI;
            isLinearProbe = true;
        }
        else
        {
            converterParams.probeShape = PROBE_SHAPE_PHASED_ARRAY;
            colorConverterParams.probeShape = PROBE_SHAPE_PHASED_ARRAY;
            isLinearProbe = false;
        }
        mUpsReaderSPtr->getScanConverterParams(imagingCaseId, converterParams, mImagingMode);

        //------- Image Processing ---------------
        ImageProcess::ProcessParams   processParams = 
        {
            .imagingCaseId  = imagingCaseId,
            .imagingMode    = mImagingMode,
            .numSamples     = converterParams.numSamples,
            .numRays        = converterParams.numRays
        };

        mBModeProcessor.setProcessParams(processParams);
        mIsCompounding = false;
    }

    // Setup Colorflow params
    {
        // Compute actual ROI params, Build LUT and Sequence Blob
        {
            if (isLinearProbe)
                mColorAcqLinearPtr->getImagingCaseConstants( imagingCaseId, colorParams[2] );
            else
                mColorAcqPtr->getImagingCaseConstants( imagingCaseId, colorParams[2] );

            if ( 0 != mUpsReaderSPtr->getErrorCount())
            {
                ALOGE("Dau::setColorImagingCase : UPS read errors = %d", mUpsReaderSPtr->getErrorCount());
                mUpsReaderSPtr->clearErrorCount();
                return (TER_IE_UPS_READ_FAILURE);
            }
            // Compute actual ROI and actual PRF Index
            {
                ALOGD("%s : REQUESTED pri = %4d, fs = %d, wf = %d, ax.start = %3.2f, ax.span = %3.2f, az.start = %2.4f, az.span = %2.4f, roiA = %2.4f",
                      __func__,
                      colorParams[0], colorParams[1], colorParams[2],
                      roiParams[0], roiParams[1], roiParams[2], roiParams[3], roiParams[4] );

                // GUI coordinate to native coordinate - color sampleSpacing = 1/2 * B sampleSpacing
                requestedRoi =
                {
                    .axialStart   = roiParams[0] + (converterParams.sampleSpacingMm),
                    .axialSpan    = roiParams[1] - (converterParams.sampleSpacingMm),
                    .azimuthStart = roiParams[2] + (converterParams.raySpacing/2.0f),
                    .azimuthSpan  = roiParams[3],
                    .roiA         = (roiParams[4]/180.0f) * PI
                };

                if (!isLinearProbe)
                    requestedRoi.axialSpan -= (converterParams.sampleSpacingMm);

                // handle a corner case - when axialStart >= imagingDepth
                if (requestedRoi.axialStart >= imagingDepth)
                {
                    requestedRoi.axialStart = (0.99f - minRoiAxialSpan) * imagingDepth;             //0.01f extra
                    ALOGD("%d: REQUESTED ax,start is adjusted to %3.2f", requestedRoi.axialStart);
                }

                if (requestedRoi.axialStart < (minRoiAxialStart * imagingDepth))
                {
                    requestedRoi.axialStart = minRoiAxialStart * imagingDepth;
                    ALOGD("%d: REQUESTED ax,start is adjusted to %3.2f", requestedRoi.axialStart);
                }

                if (requestedRoi.axialSpan < (minRoiAxialSpan * imagingDepth))
                {
                    requestedRoi.axialSpan = minRoiAxialSpan * imagingDepth;
                    ALOGD("%d: REQUESTED ax,span is adjusted to %3.2f", requestedRoi.axialSpan);
                }

                // need to adjust coordinate for Linear
                if (isLinearProbe) {
                    colorRxPitch = mUpsReaderSPtr->getColorRxPitch(imagingCaseId);

                    // adjust Roi Azimuth
                    requestedRoi.azimuthStart += colorRxPitch/2.0f;
                    requestedRoi.azimuthSpan += colorRxPitch;

                    requestedRoi.azimuthStart = -(roiParams[2] + roiParams[3]);
                    requestedRoi.roiA = -requestedRoi.roiA;
                }

                if (isLinearProbe)
                {
                    if ((retVal = mColorAcqLinearPtr->calculateBeamParams(imagingCaseId,
                                                                    requestedPrfIndex,
                                                                    requestedRoi,
                                                                    actualPrfIndex,
                                                                    actualRoi)) != THOR_OK) {
                        return (retVal);
                    }

                }
                else {
                    if ((retVal = mColorAcqPtr->calculateBeamParams(imagingCaseId,
                                                                    requestedPrfIndex,
                                                                    requestedRoi,
                                                                    actualPrfIndex,
                                                                    actualRoi)) != THOR_OK) {
                        return (retVal);
                    }
                }

                // Return actual achievable ROI attributes and PRF index to the GUI layer
                roiParams[0] = actualRoi.axialStart;
                roiParams[1] = actualRoi.axialSpan;
                roiParams[2] = actualRoi.azimuthStart;
                roiParams[3] = actualRoi.azimuthSpan;
                roiParams[4] = (actualRoi.roiA / PI) * 180.0f;
                colorParams[0] = (int)actualPrfIndex;

                // adjust the native to GUI coordinate
                {
                    roiParams[0] = roiParams[0] - (converterParams.sampleSpacingMm);
                    roiParams[1] = roiParams[1] + (converterParams.sampleSpacingMm);
                    roiParams[2] = roiParams[2] - (converterParams.raySpacing/2.0f);

                    if (!isLinearProbe)
                        roiParams[1] = roiParams[1] + (converterParams.sampleSpacingMm);
                }

                // for Linear only
                if (isLinearProbe) {
                    roiParams[2] = -(actualRoi.azimuthStart + actualRoi.azimuthSpan);
                    roiParams[4] = -roiParams[4];

                    // adjust roi location
                    roiParams[2] -= colorRxPitch/2.0f;
                    roiParams[3] -= colorRxPitch;
                }

                ALOGD("%s : ACTUAL    pri = %4d, fs = %d, wf = %d, ax.start = %3.2f, ax.span = %3.2f, az.start = %2.4f, az.span = %2.4f, roiA = %2.4f",
                      __func__,
                      colorParams[0], colorParams[1], colorParams[2],
                      roiParams[0], roiParams[1], roiParams[2], roiParams[3], roiParams[4] );

                // calculate max Prf Index
                if (isLinearProbe)
                    mColorAcqLinearPtr->calculateMaxPrfIndex(actualRoi, maxAchievablePrfIndex);
                else
                    mColorAcqPtr->calculateMaxPrfIndex(actualRoi, maxAchievablePrfIndex);

                colorParams[3] = (int) maxAchievablePrfIndex;
                ALOGD("%s: MAX Prf Index: %d", __func__, maxAchievablePrfIndex);
            }

            if (isLinearProbe)
            {
                mColorAcqLinearPtr->buildLuts( imagingCaseId, actualRoi );
                mColorAcqLinearPtr->buildSequenceBlob( imagingCaseId );
            }
            else
            {
                mColorAcqPtr->buildLuts( imagingCaseId, actualRoi );
                mColorAcqPtr->buildSequenceBlob( imagingCaseId );
            }

            // TODO: update - color converter param for Linear
            if (isLinearProbe)
            {
                mColorAcqLinearPtr->calculateScanConverterParams( actualRoi, colorConverterParams );

                // convert startRayRadian to startRayMm
                colorConverterParams.startRayMm = colorConverterParams.startRayRadian;
            }
            else
            {
                mColorAcqPtr->calculateScanConverterParams( actualRoi, colorConverterParams );
            }

#if 0
            LOGD("numSamples = %d, numRays = %d, startSampleMm = %f, sampleSpacingMm = %f, raySpacing = %f, startRayRadian = %f, startRayMm = %f",
                 colorConverterParams.numSamples,
                 colorConverterParams.numRays,
                 colorConverterParams.startSampleMm,
                 colorConverterParams.sampleSpacingMm,
                 colorConverterParams.raySpacing,
                 colorConverterParams.startRayRadian,
                 colorConverterParams.startRayMm );
#endif
        }
        // Color Processor needs additional parameters
        mColorProcessor.getStage<ColorProcess>().each<ThorStatus(ColorProcess::*)(uint32_t, ScanConverterParams, uint32_t)>
                (&ColorProcess::setProcessParams, imagingCaseId, colorConverterParams, colorParams[4]);
    }

    // Load LUTs
    if (isLinearProbe)
        mTbfManagerPtr->setColorImagingCaseLinear(*mLutManagerPtr, imagingCaseId, mColorAcqLinearPtr, colorParams[2] );
    else
        mTbfManagerPtr->setColorImagingCase(*mLutManagerPtr, imagingCaseId, mColorAcqPtr, colorParams[2] );

    // Build and load the sequence
    if (isLinearProbe)
    {
        mSequenceBuilderPtr->buildBCSequenceLinear( imagingCaseId,
                                              mColorAcqLinearPtr->getSequenceBlobPtr(),
                                              mColorAcqLinearPtr->getFiringCount(),
                                              mColorAcqLinearPtr->getLutMSRTAFE(),
                                              mColorAcqLinearPtr->getMSRTAFElength(),
                                              mColorAcqLinearPtr->getLutMSSTTXC(),
                                              mColorAcqLinearPtr->getMSSTTXClength());
    }
    else
    {
        mSequenceBuilderPtr->buildBCSequence( imagingCaseId,
                                              mColorAcqPtr->getSequenceBlobPtr(),
                                              mColorAcqPtr->getFiringCount(),
                                              mColorAcqPtr->getLutMSRTAFE(),
                                              mColorAcqPtr->getMSRTAFElength());
    }

    // Restore color gain
    mColorProcessor.getStage<ColorProcess>().each(&ColorProcess::setGain, mColorGainPercent);

    // setup InputManager
    {
        mInputManagerSPtr->setBFrameSize(converterParams.numSamples, converterParams.numRays);
        mInputManagerSPtr->setColorFrameSize(colorConverterParams.numSamples, colorConverterParams.numRays);
        mInputManagerSPtr->setImagingCase(imagingCaseId, mImagingMode, mIsDASignal, mIsECGSignal, mIsUsSignal);
        if(mDauContext.isUsbContext)
        {
            mUsbDataHandlerSPtr->enableUS(mIsUsSignal);
            mUsbDataHandlerSPtr->enableDa(mIsDASignal);
            mUsbDataHandlerSPtr->enableECG(mIsECGSignal);
        }
        if (supportsEcg())
        {
            mInputManagerSPtr->setEcgFrameSize( mSequenceBuilderPtr->getNumEcgSamplesPerFrame() );
        }
        if (supportsDa())
        {
            mInputManagerSPtr->setDaFrameSize(mSequenceBuilderPtr->getNumDaSamplesPerFrame());
        }
#ifdef ENABLE_DAECG
        if (supportsEcg() || supportsDa())
        {
            setDaEcgCineParams(cineParams);
        }
#endif
    }

    cineParams.organId = mUpsReaderSPtr->getImagingOrgan(mImagingCase);
    cineParams.converterParams = converterParams;
    cineParams.colorConverterParams = colorConverterParams;
    cineParams.imagingCaseId = imagingCaseId;
    cineParams.dauDataTypes = mDauDataTypes;
    cineParams.imagingMode = mImagingMode;
    cineParams.frameInterval = mSequenceBuilderPtr->getFrameIntervalMs();
    cineParams.frameRate = mSequenceBuilderPtr->getFrameRate();
    cineParams.upsReader = mUpsReaderSPtr;
    cineParams.colorMapIndex = defaultColorMapIndex;
    cineParams.isColorMapInverted = false;

    // CVD / CPD - Color Mode
    cineParams.colorDopplerMode = colorParams[4];

    mCineBuffer.setCineParamsOnly(cineParams);

    if(mDauContext.isUsbContext)
    {
        mUsbDataHandlerSPtr->setFrameSize(mInputManagerSPtr->getBFrameStride(),IMAGING_MODE_B);
        mUsbDataHandlerSPtr->setFrameSize(mInputManagerSPtr->getCFrameStride(),IMAGING_MODE_COLOR);
    }

    if ( 0 != mUpsReaderSPtr->getErrorCount())
    {
        ALOGE("Dau::setColorImagingCase : UPS read errors = %d", mUpsReaderSPtr->getErrorCount());
        mUpsReaderSPtr->clearErrorCount();
        return (TER_IE_UPS_READ_FAILURE);
    }

    retVal = getApiEstimates( imagingCaseId, apiEstimates);
    if (THOR_OK != retVal )
    {
        return (retVal);
    }

    getImagingInfo( imagingCaseId, imagingInfo);
    if ( 0 != mUpsReaderSPtr->getErrorCount())
    {
        ALOGE("Dau::setColorImagingCase : UPS read errors = %d", mUpsReaderSPtr->getErrorCount());
        mUpsReaderSPtr->clearErrorCount();
        return (TER_IE_UPS_READ_FAILURE);
    }
    else
    {
        ALOGD("Dau::setColorImagingCase : NO UPS READ ERRORS");
        return (mSequenceBuilderPtr->loadSequence(mImagingMode));
    }
}

/**
 * @brief Set imaging case for M mode
 *
 * @param imagingCaseId - ID of the imaging case listed in ImagingCases table of UPS
 *
 * @param scrollSpeedId
 *
 * @param mLinePosition
 *
 * @param apiEstimates - MI, TI, .. estimated by AP&I Manager
 *     [0] : MI
 *     [1] : TI
 *     [2] : TIB
 *     [3] : TIS
 *     [4..7] :  to be deremined
 *
 * @param imagingInfo
 *     [0] : B centerFrequency
 *     [1] : unused (relevant in colorflow)
 *     [2] : frame rate
 *     [3] : dutyCycle
 *     [4] : txVoltage
 *     [5..7] : to be determined
 *
 * @return  status - THOR_OK if no errors are encountered.
 */
ThorStatus Dau::setMmodeImagingCase( uint32_t imagingCaseId,
                                     uint32_t scrollSpeedId,
                                     float mLinePosition,
                                     float apiEstimates[8],
                                     float imagingInfo[8] )
{
    std::unique_lock<std::mutex> lock(mLock);
    ThorStatus  retVal = THOR_OK;

    ALOGD("Dau::setMmodeImagingCase(%d, %d, %f)", imagingCaseId, scrollSpeedId, mLinePosition);
    if ( 0 != mUpsReaderSPtr->getErrorCount() )
    {
        ALOGE("%s UPS error count was not zero on entry", __func__);
        return (TER_IE_UPS_READ_FAILURE);
    }

    mImagingCase = imagingCaseId;
    mImagingMode = mUpsReaderSPtr->getImagingMode(imagingCaseId);
    mDauDataTypes = getDauDataTypes(mImagingMode, false, false);

    // Setup B-mode params
    ScanConverterParams     converterParams;
    if ((mProbeInfo.probeType & DEV_PROBE_MASK) == PROBE_TYPE_LINEAR)
    {
        converterParams.probeShape = PROBE_SHAPE_LINEAR;
    }
    else
    {
        converterParams.probeShape = PROBE_SHAPE_PHASED_ARRAY;
    }
    mUpsReaderSPtr->getScanConverterParams(imagingCaseId, converterParams, mImagingMode);
    mIsCompounding = false;

    //------- Image Processing ---------------
    ImageProcess::ProcessParams   processParams = 
        {
            .imagingCaseId  = imagingCaseId,
            .imagingMode    = mImagingMode,
            .numSamples     = converterParams.numSamples,
            .numRays        = converterParams.numRays
        };
    
    mBModeProcessor.setProcessParams(processParams);

    mMModeProcessor.setProcessParams(processParams);
    
    // Setup M-mode params
    uint32_t beamNumber;
    uint32_t mlSelection;
    bool     isLinearProbe = (mProbeInfo.probeType & DEV_PROBE_MASK) == PROBE_TYPE_LINEAR;
    mMmodeAcqPtr->getNearestBBeam(imagingCaseId, mLinePosition, beamNumber, mlSelection, isLinearProbe);
    mMModeProcessor.getStage<DeInterleaveMProcess>().each(&DeInterleaveMProcess::setMultilineSelection, mlSelection);
    mMModeProcessor.getStage<MFilterProcess>().each(&MFilterProcess::setScrollSpeed, imagingCaseId, scrollSpeedId);

    mTbfManagerPtr->setImagingCase(*mLutManagerPtr, imagingCaseId, IMAGING_MODE_B);

    LOGD("setMmodeImagingCase beamNumber = %d, mlSelection = %d", beamNumber, mlSelection);
    mSequenceBuilderPtr->buildBMSequence( imagingCaseId, beamNumber );

    // Setup InputManager
    {
        mInputManagerSPtr->setBFrameSize(converterParams.numSamples, converterParams.numRays);
        mInputManagerSPtr->setImagingCase(imagingCaseId, mImagingMode, false, false, true);
    }
    
    // Set CINE params
    {
        CineBuffer::CineParams cineParams;

        // retrieve values in the current cineParams
        cineParams = mCineBuffer.getParams();

        cineParams.organId = mUpsReaderSPtr->getImagingOrgan(mImagingCase);
        cineParams.converterParams = converterParams; // B-Mode converter parameters for rendering
        cineParams.imagingCaseId = imagingCaseId;
        cineParams.dauDataTypes = mDauDataTypes;
        cineParams.imagingMode = mImagingMode;
        cineParams.frameInterval = mSequenceBuilderPtr->getFrameIntervalMs();
        cineParams.frameRate = mSequenceBuilderPtr->getFrameRate();
        cineParams.upsReader = mUpsReaderSPtr;
        uint32_t numTxBeams = mUpsReaderSPtr->getNumTxBeamsB(imagingCaseId, IMAGING_MODE_M);
        uint32_t linesAveraged = getMLinesToAverage(scrollSpeedId);
        cineParams.targetFrameRate = 30;
        cineParams.mlinesPerFrame = numTxBeams/linesAveraged;
        cineParams.scrollSpeed = cineParams.mlinesPerFrame*1000/cineParams.frameInterval;
        cineParams.stillMLineTime = -1.0f;
        cineParams.stillTimeShift = -1.0f;
        mMLinesPerFrame = cineParams.mlinesPerFrame;
        LOGD("M-Mode Cine Params: frame interval (ms) = %d, targetFrameRate = %d, mlinesPerFrame = %d, scrollSpeed = %f (lines/sec)",
             cineParams.frameInterval, cineParams.targetFrameRate, cineParams.mlinesPerFrame, cineParams.scrollSpeed);
        // To Prevent errors.
        cineParams.daSamplesPerFrame = 0;
        cineParams.ecgSamplesPerFrame = 0;
        cineParams.mModeSweepSpeedIndex = scrollSpeedId;
        mCineBuffer.setCineParamsOnly(cineParams);
    }

    if(mDauContext.isUsbContext)
    {
        mUsbDataHandlerSPtr->setFrameSize(mInputManagerSPtr->getBFrameStride(),IMAGING_MODE_B);
        mUsbDataHandlerSPtr->setFrameSize(mInputManagerSPtr->getMFrameStride(),IMAGING_MODE_M);
    }

    if ( 0 != mUpsReaderSPtr->getErrorCount())
    {
        ALOGE("Dau::setMmodeImagingCase : UPS read errors = %d", mUpsReaderSPtr->getErrorCount());
        mUpsReaderSPtr->clearErrorCount();
        return (TER_IE_UPS_READ_FAILURE);
    }

    retVal = getApiEstimates( imagingCaseId, apiEstimates);
    if (THOR_OK != retVal )
    {
        return (retVal);
    }
    getImagingInfo( imagingCaseId, imagingInfo);

    if ( 0 != mUpsReaderSPtr->getErrorCount())
    {
        ALOGE("Dau::setMmodeImagingCase : UPS read errors = %d", mUpsReaderSPtr->getErrorCount());
        mUpsReaderSPtr->clearErrorCount();
        return (TER_IE_UPS_READ_FAILURE);
    }
    else
    {
        ALOGD("Dau::setMmodeImagingCase : NO UPS READ ERRORS");
        return (mSequenceBuilderPtr->loadSequence(mImagingMode));
    }
}

/**
 * @brief Set imaging case for PW Doppler
 *
 * @param imagingCaseId - ID of the imaging case listed in ImagingCases table of UPS
 *
 * @param pwIndices - additional input parameters specific for PW imaging
 *     [0] : PRF Index
 *     [1] : Wall Filter Index
 *     [2] : Gate Index
 *     [3] : Baseline index
 *     [4] : sweep speed index
 *     [5] : isInvert
 *     [6] : Max PRF Index
 *     [7] : Beam Steering Angle Index for PW Linear
 *
 * @param gateParams - attributes of range gate
 *     [0] : gateStart mm
 *     [1] : gateLocation Azimuth rad
 *     [2] : gateAngle rad
 *
 * @param apiEstimates - MI, TI, .. estimated by AP&I Manager
 *     [0] : MI
 *     [1] : TI
 *     [2] : TIB
 *     [3] : TIS
 *     [4..7] :  to be determined
 *
 * @param imagingInfo
 *     [0] : PW center frequency
 *     [1] : N/A
 *     [2] : N/A
 *     [3] : N/A
 *     [4] : txVoltage
 *     [5..7] : to be determined
 *
 * @return  status - THOR_OK if no errors are encountered.
 */
ThorStatus Dau::setPwImagingCase(uint32_t imagingCaseId,
                                    int pwIndices[8],
                                    float gateParams[3],
                                    float apiEstimates[8],
                                    float imagingInfo[8],
                                    bool isTDI)
{
    std::unique_lock<std::mutex> lock(mLock);
    ThorStatus retVal = THOR_OK;

    uint32_t requestedPrfIndex = pwIndices[0];
    uint32_t actualPrfIndex;
    uint32_t wallFilterIndex = pwIndices[1];
    uint32_t gateIndex = pwIndices[2];
    uint32_t baselineShiftIndex = pwIndices[3];
    uint32_t sweepSpeedIndex = pwIndices[4];
    uint32_t maxAchievablePrfIndex = 0;
    uint32_t PWSteeringAngleIndex = pwIndices[7];
    bool     baselineInvert  = false;

    if (pwIndices[5] == 1)
        baselineInvert = true;

    float gateStart = gateParams[0];
    float gateAzimuth = gateParams[1];
    float gateAngleAdjust = gateParams[2];

    float gateAngle = 0.0f;
    float PWSteeringAngleDeg = 0.0f;

    uint32_t samplesPerLine; // coming from DAU
    uint32_t samplesPerFrame;
    float gateRange;

    bool     isLinearProbe = (mProbeInfo.probeType & DEV_PROBE_MASK) == PROBE_TYPE_LINEAR;

    CineBuffer::CineParams  cineParams;

    ALOGD("Dau::setPwImagingCase(%d)", imagingCaseId);
    if ( 0 != mUpsReaderSPtr->getErrorCount() )
    {
        ALOGE("Dau::setPwImagingCase() UPS error count was not zero on entry");
        return (TER_IE_UPS_READ_FAILURE);
    }

    mImagingCase = imagingCaseId;
    mImagingMode = IMAGING_MODE_PW;
    mDauDataTypes = getDauDataTypes(mImagingMode, mIsDASignal, mIsECGSignal);
    ALOGD("Dau::mImagingMode = %d, mDauDataType = %08X, isTdi = %d", mImagingMode, mDauDataTypes,isTDI);

    // retrieve values in the current cineParams
    cineParams = mCineBuffer.getParams();
    mIsCompounding = false;

    // TODO: Linear PW
    if (isLinearProbe)
    {
        mUpsReaderSPtr->getPWSteeringAngle(PWSteeringAngleIndex, PWSteeringAngleDeg);
        // PW Beam Steering Angle
        gateAngle = PI * -PWSteeringAngleDeg / 180.0f;
        mDopplerAcqLinearPtr->calculateActualPrfIndex(imagingCaseId, gateStart, pwIndices[2], requestedPrfIndex, actualPrfIndex, isTDI );
        pwIndices[0] = actualPrfIndex; // return to UI layer
        mDopplerAcqLinearPtr->calculateBeamParams(imagingCaseId, gateStart, pwIndices[2], -gateAzimuth, gateAngle, actualPrfIndex, isTDI);
        mDopplerAcqLinearPtr->buildLuts(imagingCaseId, isTDI);
        mDopplerAcqLinearPtr->buildPwSequenceBlob();
        samplesPerFrame = mUpsReaderSPtr->getNumPwSamplesPerFrame(actualPrfIndex, isTDI);

        // calculate Max Achievable Index
        mDopplerAcqLinearPtr->calculateMaxPrfIndex(imagingCaseId, gateStart, gateIndex, maxAchievablePrfIndex, isTDI );
        pwIndices[6] = (int) maxAchievablePrfIndex;
        ALOGD("%s: MAX Prf Index: %d", __func__, maxAchievablePrfIndex);
    }
    else
        // Builds LUTs and sequence blob
    {
        mDopplerAcqPtr->calculateActualPrfIndex(imagingCaseId, gateStart, pwIndices[2], requestedPrfIndex, actualPrfIndex, isTDI );
        pwIndices[0] = actualPrfIndex; // return to UI layer
        mDopplerAcqPtr->calculateBeamParams(imagingCaseId, gateStart, pwIndices[2], gateAzimuth, actualPrfIndex, isTDI);
        mDopplerAcqPtr->buildLuts(imagingCaseId, isTDI);
        mDopplerAcqPtr->buildPwSequenceBlob();
        samplesPerFrame = mUpsReaderSPtr->getNumPwSamplesPerFrame(actualPrfIndex, isTDI);

        // calculate Max Achievable Index
        mDopplerAcqPtr->calculateMaxPrfIndex(imagingCaseId, gateStart, gateIndex, maxAchievablePrfIndex, isTDI );
        pwIndices[6] = (int) maxAchievablePrfIndex;
        ALOGD("%s: MAX Prf Index: %d", __func__, maxAchievablePrfIndex);
    }

    if (isLinearProbe)
        mTbfManagerPtr->setPwImagingCaseLinear(*mLutManagerPtr, imagingCaseId, mDopplerAcqLinearPtr);
    else
        mTbfManagerPtr->setPwImagingCase(*mLutManagerPtr, imagingCaseId, mDopplerAcqPtr);

    if (isLinearProbe)
        retVal = mSequenceBuilderPtr->buildPwSequence(imagingCaseId, actualPrfIndex, mDopplerAcqLinearPtr->getSequenceBlobPtr(), isTDI );
    else
        retVal = mSequenceBuilderPtr->buildPwSequence(imagingCaseId, actualPrfIndex, mDopplerAcqPtr->getSequenceBlobPtr(), isTDI );

    if (THOR_OK != retVal)
    {
        ALOGE("buildPwSequence returned 0x%08X", retVal);
        return (retVal);
    }

    // Initialize InputManager
    {
        mUpsReaderSPtr->getPwGateSamples( gateIndex, samplesPerLine );
        mInputManagerSPtr->setPwFrameSize( samplesPerLine, samplesPerFrame );
        mInputManagerSPtr->setEcgFrameSize( mSequenceBuilderPtr->getNumEcgSamplesPerFrame() );
        mInputManagerSPtr->setDaFrameSize( mSequenceBuilderPtr->getNumDaSamplesPerFrame() );

#ifdef ENABLE_DAECG
        setDaEcgCineParams(cineParams);
#endif
        mInputManagerSPtr->setImagingCase( imagingCaseId, mImagingMode, mIsDASignal, mIsECGSignal, mIsUsSignal );
    }

    // update cineParams
    cineParams.organId = mUpsReaderSPtr->getImagingOrgan(mImagingCase);
    cineParams.imagingCaseId = mImagingCase;
    cineParams.dauDataTypes = mDauDataTypes;
    cineParams.imagingMode = mImagingMode;
    cineParams.targetFrameRate = 30;
    cineParams.frameInterval = mSequenceBuilderPtr->getFrameIntervalMs();
    cineParams.frameRate = mSequenceBuilderPtr->getFrameRate();
    cineParams.upsReader = mUpsReaderSPtr;
    cineParams.pwPrfIndex = actualPrfIndex;
    cineParams.pwGateIndex = gateIndex;
    cineParams.dopplerBaselineShiftIndex = baselineShiftIndex;
    cineParams.dopplerBaselineInvert = baselineInvert;
    cineParams.dopplerSamplesPerFrame = mUpsReaderSPtr->getNumPwSamplesPerFrame(actualPrfIndex, isTDI);
    cineParams.dopplerSweepSpeedIndex = sweepSpeedIndex;
    cineParams.isPWTDI = isTDI;

    uint32_t fftSize, outRateAfeClks;
    mUpsReaderSPtr->getPwFftSizeRate(fftSize, outRateAfeClks);
    cineParams.dopplerFFTSize = fftSize;
    uint32_t organGlobalId = mUpsReaderSPtr->getOrganGlobalId(cineParams.organId);

    if(mDauContext.isUsbContext)
    {
        mUsbDataHandlerSPtr->setFrameSize(mInputManagerSPtr->getPwFrameStride(),IMAGING_MODE_PW);
    }
    // calculate FFT output
    float outputRatio = outRateAfeClks/((float) mUpsReaderSPtr->getAfeClksPerPwSample(actualPrfIndex, isTDI));
    float pwFFTNumLinesPerFrameBeforeAvg = cineParams.dopplerSamplesPerFrame / outputRatio;

    // original scrollSpeed before FFT output average
    float scrollSpeedBeforeAvg = pwFFTNumLinesPerFrameBeforeAvg * cineParams.frameRate;
    float timeSpan = getTimeSpan(sweepSpeedIndex, organGlobalId == TCD_GLOBAL_ID);

    // NumLinesToAverage (from UPS)
    uint32_t fftNumLinesToAverage, fftSmoothingNum;
    uint32_t fftAvgIndex = sweepSpeedIndex; //This is Sweepspeed index
    // TODO : Need to update PWFft table to support 3,4,5 speedIndex for TDI
    if (!mUpsReaderSPtr->getPwFftAverageNum(fftAvgIndex, fftNumLinesToAverage, fftSmoothingNum))
    {
        fftNumLinesToAverage = 1;
        fftSmoothingNum = 1;
    }

    cineParams.dopplerFftSmoothingNum = fftSmoothingNum;

    cineParams.dopplerFFTNumLinesToAvg = fftNumLinesToAverage;
    cineParams.scrollSpeed = scrollSpeedBeforeAvg / cineParams.dopplerFFTNumLinesToAvg;
    cineParams.dopplerNumLinesPerFrame = pwFFTNumLinesPerFrameBeforeAvg / cineParams.dopplerFFTNumLinesToAvg;

    ALOGD("Dau::setPwImagingCase: timeSpan: %f, pwNumLinesAvg: %u, pwNumLinesPerFrame: %f, pre-ScrollSpeed: %f",
          timeSpan, cineParams.dopplerFFTNumLinesToAvg, cineParams.dopplerNumLinesPerFrame, cineParams.scrollSpeed);
    ALOGD("Dau::setPwImagingCase: fftNumLinesToAvg (decim): %d, fftSmoothingNum: %d",
          cineParams.dopplerFFTNumLinesToAvg, cineParams.dopplerFftSmoothingNum);

    float prfHz = mUpsReaderSPtr->getGlobalsPtr()->samplingFrequency/
                  ((float) mUpsReaderSPtr->getAfeClksPerPwSample(actualPrfIndex, isTDI))*1e6;
    // Doppler prf
    imagingInfo[5] = prfHz;
    // SweepSpeed in mm/sec
    imagingInfo[6] = getSweepSpeedMmSec(sweepSpeedIndex, organGlobalId == TCD_GLOBAL_ID);

    uint32_t audioUpsampleRatio = 1;      // default

    if (prfHz < 8000)
        audioUpsampleRatio = (uint32_t) ceil(8000.0f/prfHz);

    cineParams.dopplerAudioUpsampleRatio = audioUpsampleRatio;

    ALOGD("Dau::setPwImagingCase: pwPrfHz: %f, audioUpsampleRatio: %d, pwSamplesPerFrame: %u, frameRate: %f",
          prfHz, audioUpsampleRatio, cineParams.dopplerSamplesPerFrame, cineParams.frameRate);

    // pw velocity scale
    float sos = mUpsReaderSPtr->getSpeedOfSound(imagingCaseId);
    uint32_t dtxele;
    float    dcenterfrequency;
    if (isLinearProbe)
        dcenterfrequency = mDopplerAcqLinearPtr->getCenterFrequency();
    else
        mUpsReaderSPtr->getPwWaveformInfo( dtxele, dcenterfrequency, isTDI );

    // velocity scale: -pwVelocityScale ~ +pwVelocityScale
    // half of the full screen scale for consistency with UI
    cineParams.dopplerVelocityScale = 0.5f * 0.1f * prfHz *  sos / (2.0f * dcenterfrequency * cos(gateAngleAdjust));

    ALOGD("Dau::setPwImagingCase: velocityScale: %f dcenterfrequency %f ", cineParams.dopplerVelocityScale, dcenterfrequency);

    // TODO: update
    uint32_t hilbertFilterIndex = 0;
    uint32_t pwSpectralCompressionCurveIndex = 34;
    uint32_t globalId = mUpsReaderSPtr->getImagingGlobalID(imagingCaseId);
    if (globalId == 10)
    {
        pwSpectralCompressionCurveIndex = 37;
    }
    // set pw process params
    //------- Image Processing ---------------
    ImageProcess::ProcessParams   processParams =
            {
                    .imagingCaseId  = imagingCaseId,
                    .imagingMode    = mImagingMode,
                    .numSamples     = 0,
                    .numRays        = 0
            };

    mPwModeCommonProcessor.setProcessParams(processParams);
    mPwModeSpectralProcessor.setProcessParams(processParams);
    mPwModeAudioProcessor.setProcessParams(processParams);

    // Setup pw-mode specific params
    mPwModeCommonProcessor.getStage<PwLowPassDecimationFilterProcess>().each(&PwLowPassDecimationFilterProcess::setProcessIndices,
                                                                             cineParams.pwGateIndex, cineParams.pwPrfIndex, isTDI);
    mPwModeCommonProcessor.getStage<DopplerWallFilterProcess>().each(&DopplerWallFilterProcess::setProcessIndices,
                                                                     pwIndices[1], cineParams.pwPrfIndex, isTDI);
    mPwModeSpectralProcessor.getStage<DopplerSpectralProcess>().each(&DopplerSpectralProcess::setProcessIndices,
                                                                     cineParams.pwPrfIndex, cineParams.dopplerFFTNumLinesToAvg, pwSpectralCompressionCurveIndex, isTDI);
    mPwModeSpectralProcessor.getStage<DopplerSpectralSmoothingProcess>().each(&DopplerSpectralSmoothingProcess::setProcessIndices,
                                                                              cineParams.dopplerFFTSize, cineParams.dopplerFftSmoothingNum);
    mPwModeAudioProcessor.getStage<DopplerHilbertProcess>().each(&DopplerHilbertProcess::setProcessIndices, cineParams.pwPrfIndex, hilbertFilterIndex, isTDI);
    mPwModeAudioProcessor.getStage<DopplerAudioUpsampleProcess>().each(&DopplerAudioUpsampleProcess::setProcessingIndices,
                                                                       cineParams.pwPrfIndex, hilbertFilterIndex, isTDI, cineParams.dopplerAudioUpsampleRatio, false);

    float refGaindB, peakThresholdPW, defaultGaindB;
    float gainOffset, gainRange;
    mUpsReaderSPtr->getPWPeakThresholdParams(cineParams.organId, refGaindB, peakThresholdPW);

    // Default PW gain
    mUpsReaderSPtr->getPWGainParams(cineParams.organId,
                                    gainOffset, gainRange, cineParams.isPWTDI);
    defaultGaindB = gainOffset + (gainRange * 0.5f);
    mDopplerPeakMeanProc->setProcessIndices(cineParams.dopplerFFTSize, defaultGaindB, peakThresholdPW);
    mDopplerPeakMeanProc->setDataType(DAU_DATA_TYPE_PW, DAU_DATA_TYPE_DOP_PM);

    mCineBuffer.setCineParamsOnly(cineParams);

    mUpsReaderSPtr->getPwGateRange( gateIndex, gateRange );
    if (isLinearProbe)
        retVal = getApiEstimatesPW( imagingCaseId, cineParams.organId, gateRange, apiEstimates);
    else
        retVal = getApiEstimates( imagingCaseId, apiEstimates);

    if (THOR_OK != retVal )
    {
        return (retVal);
    }

    if(isLinearProbe) {
        getImagingInfoLinear(imagingCaseId, imagingInfo,gateStart,pwIndices[2], gateAzimuth, gateAngle);
    } else {
        getImagingInfo( imagingCaseId, imagingInfo);
    }

    if ( 0 != mUpsReaderSPtr->getErrorCount())
    {
        ALOGE("Dau::setPwImagingCase : UPS read errors = %d", mUpsReaderSPtr->getErrorCount());
        mUpsReaderSPtr->clearErrorCount();
        return (TER_IE_UPS_READ_FAILURE);
    }
    else
    {
        ALOGD("Dau::setPwImagingCase : NO UPS READ ERRORS");
        return (mSequenceBuilderPtr->loadSequence(mImagingMode));
    }
}

/**
 * @brief Set imaging case for CW Doppler
 *
 * @param imagingCaseId - ID of the imaging case listed in ImagingCases table of UPS
 *
 * @param cwIndices - additional input parameters specific for CW imaging
 *     [0] : PRF Index
 *     [1] : Wall Filter Index
 *     [2] : Gate Index (Unused)
 *     [3] : Baseline index
 *     [4] : sweep speed index
 *     [5] : isInvert
 *
 * @param gateParams - attributes of range gate
 *     [0] : gateStart mm
 *     [1] : beamAngle rad
 *     [2] : gateAngle rad
 *
 * @param apiEstimates - MI, TI, .. estimated by AP&I Manager
 *     [0] : MI
 *     [1] : TI
 *     [2] : TIB
 *     [3] : TIS
 *     [4..7] :  to be determined
 *
 * @param imagingInfo
 *     [0] : PW center frequency
 *     [1] : N/A
 *     [2] : N/A
 *     [3] : N/A
 *     [4] : txVoltage
 *     [5..7] : to be determined
 *
 * @return  status - THOR_OK if no errors are encountered.
 */
ThorStatus Dau::setCwImagingCase(uint32_t imagingCaseId,
                                 int cwIndices[6],
                                 float gateParams[2],
                                 float apiEstimates[8],
                                 float imagingInfo[8]) {
    std::unique_lock<std::mutex> lock(mLock);
    ThorStatus retVal = THOR_OK;

    uint32_t cwPrfIndex = cwIndices[0];
    uint32_t wallFilterIndex = cwIndices[1];
    uint32_t baselineShiftIndex = cwIndices[3];
    uint32_t sweepSpeedIndex = cwIndices[4];
    bool baselineInvert = false;

    if (cwIndices[5] == 1)
        baselineInvert = true;

    float gateStart = gateParams[0];
    float gateAzimuth = gateParams[1];
    float gateAngleAdjust = gateParams[2];

    uint32_t samplesPerLine;
    uint32_t samplesPerFrame;

    CineBuffer::CineParams cineParams;

    ALOGD("Dau::setCwImagingCase(%d)", imagingCaseId);
    if (0 != mUpsReaderSPtr->getErrorCount()) {
        ALOGE("Dau::setCwImagingCase() UPS error count was not zero on entry");
        return (TER_IE_UPS_READ_FAILURE);
    }
    mImagingCase = imagingCaseId;
    mImagingMode = IMAGING_MODE_CW;
    mDauDataTypes = getDauDataTypes(mImagingMode, mIsDASignal, mIsECGSignal);
    ALOGD("Dau::mImagingMode = %d, mDauDataType = %08X", mImagingMode, mDauDataTypes);

    // retrieve values in the current cineParams
    cineParams = mCineBuffer.getParams();
    mIsCompounding = false;

    // Builds LUTs and sequence blob
    {
        mCwAcqPtr->buildLuts(imagingCaseId, gateStart, gateAzimuth);
        mCwAcqPtr->buildCwSequenceBlob();

        // TODO: update with CwSamplesPerFrame
        // CwSampling: always 195312.5 samples per second
        samplesPerLine = 1;
        samplesPerFrame = mUpsReaderSPtr->getNumCwSamplesPerFrame(cwPrfIndex);
    }

    mTbfManagerPtr->setCwImagingCase(*mLutManagerPtr, imagingCaseId, mCwAcqPtr);

    // Initialize InputManager
    {
        mInputManagerSPtr->setCwFrameSize(samplesPerLine, samplesPerFrame);
        mInputManagerSPtr->setEcgFrameSize(mSequenceBuilderPtr->getNumEcgSamplesPerFrame());
        mInputManagerSPtr->setDaFrameSize(mSequenceBuilderPtr->getNumDaSamplesPerFrame());

#ifdef ENABLE_DAECG
        setDaEcgCineParams(cineParams);
#endif
        mInputManagerSPtr->setImagingCase(imagingCaseId, mImagingMode, mIsDASignal, mIsECGSignal,
                                          mIsUsSignal);
    }

    // update cineParams
    cineParams.organId = mUpsReaderSPtr->getImagingOrgan(mImagingCase);
    cineParams.imagingCaseId = mImagingCase;
    cineParams.dauDataTypes = mDauDataTypes;
    cineParams.imagingMode = mImagingMode;
    cineParams.targetFrameRate = 30;

    // TODO: update - placeholder for frame rate from sequencebuilder
    cineParams.frameRate = 195312.5f / samplesPerFrame;         //CW does not use Sequencer
    cineParams.frameInterval = (uint32_t) (1000.0f / cineParams.frameRate);
    cineParams.upsReader = mUpsReaderSPtr;
    cineParams.cwPrfIndex = cwPrfIndex;

    uint32_t decimationFactor, decimSamplesPerFrame;
    mUpsReaderSPtr->getCwDecFactorSamples(cwPrfIndex, decimationFactor, decimSamplesPerFrame);

    cineParams.dopplerBaselineShiftIndex = baselineShiftIndex;
    cineParams.dopplerBaselineInvert = baselineInvert;
    cineParams.dopplerSamplesPerFrame = samplesPerFrame;
    cineParams.dopplerSweepSpeedIndex = sweepSpeedIndex;
    cineParams.cwDecimationFactor = decimationFactor;

    ALOGD("Dau::setCwImagingCase: CwSamplesPerFrame: %d, CwDecimationFactor: %d, frameRate: %f",
          cineParams.dopplerSamplesPerFrame, cineParams.cwDecimationFactor, cineParams.frameRate);

    if(mDauContext.isUsbContext)
    {
        mUsbDataHandlerSPtr->setFrameSize(mInputManagerSPtr->getCwFrameStride(),IMAGING_MODE_CW);
    }
    // TODO: placeholder for CW related parameters //////////////////////////////////////////////////
    uint32_t fftSize, outRateCwClks;
    mUpsReaderSPtr->getCwFftSizeRate(fftSize, outRateCwClks);
    cineParams.dopplerFFTSize = fftSize;
    uint32_t organGlobalId = mUpsReaderSPtr->getOrganGlobalId(cineParams.organId);

    // calculate FFT output
    float outputRatio = outRateCwClks / ((float) mUpsReaderSPtr->getCwClksPerCwSample(cwPrfIndex));
    float cwFFTNumLinesPerFrameBeforeAvg = cineParams.dopplerSamplesPerFrame / outputRatio;

    // original scrollSpeed before FFT output average
    float scrollSpeedBeforeAvg = cwFFTNumLinesPerFrameBeforeAvg * cineParams.frameRate;
    float timeSpan = getTimeSpan(sweepSpeedIndex, organGlobalId == TCD_GLOBAL_ID);

    // NumLinesToAverage (from UPS)
    uint32_t fftNumLinesToAverage, fftSmoothingNum;
    uint32_t fftAvgIndex = sweepSpeedIndex; //This is Sweepspeed index
    if (!mUpsReaderSPtr->getCwFftAverageNum(fftAvgIndex, fftNumLinesToAverage, fftSmoothingNum))
    {
        fftNumLinesToAverage = 1;
        fftSmoothingNum = 1;
    }

    cineParams.dopplerFftSmoothingNum = fftSmoothingNum;

    cineParams.dopplerFFTNumLinesToAvg = fftNumLinesToAverage;
    cineParams.scrollSpeed = scrollSpeedBeforeAvg / cineParams.dopplerFFTNumLinesToAvg;
    cineParams.dopplerNumLinesPerFrame = cwFFTNumLinesPerFrameBeforeAvg / cineParams.dopplerFFTNumLinesToAvg;

    ALOGD("Dau::setCwImagingCase: timeSpan: %f, dopplerNumLinesAvg: %u, dopplerNumLinesPerFrame: %f, pre-ScrollSpeed: %f",
          timeSpan, cineParams.dopplerFFTNumLinesToAvg, cineParams.dopplerNumLinesPerFrame, cineParams.scrollSpeed);

    // TODO: update - placeholder for CW Audio related parameters
    float prfHz = 195312.5/cineParams.cwDecimationFactor;   // 195312.5 fixed for CW

    // Doppler prf
    imagingInfo[5] = prfHz;
    // SweepSpeed in mm/sec
    imagingInfo[6] = getSweepSpeedMmSec(sweepSpeedIndex, organGlobalId == TCD_GLOBAL_ID);

    uint32_t audioUpsampleRatio = 1;      // default

    if (prfHz < 8000)
        audioUpsampleRatio = (uint32_t) ceil(8000.0f/prfHz);

    cineParams.dopplerAudioUpsampleRatio = audioUpsampleRatio;

    ALOGD("Dau::setCwImagingCase: cwPrfHz: %f, audioUpsampleRatio: %d, cwSamplesPerFrame: %u, frameRate: %f",
          prfHz, audioUpsampleRatio, cineParams.dopplerSamplesPerFrame, cineParams.frameRate);
    ALOGD("Dau::setCwImagingCase: fftNumLinesToAvg (decim): %d, fftSmoothingNum: %d",
          cineParams.dopplerFFTNumLinesToAvg, cineParams.dopplerFftSmoothingNum);

    // TODO: placeholder for CW velocity scale
    float sos = mUpsReaderSPtr->getSpeedOfSound(imagingCaseId);
    float dcenterfrequency = CW_CENTER_FREQ; // Fixed for CW as per THSW-666
    // velocity scale: -pwVelocityScale ~ +pwVelocityScale
    // half of the full screen scale for consistency with UI
    // CW - half the scale of PW (512 FFT)
    cineParams.dopplerVelocityScale = 0.5f * 0.1f * prfHz *  sos / (2.0f * dcenterfrequency * cos(gateAngleAdjust)) * 0.5f;

    ALOGD("Dau::setCwImagingCase: velocityScale: %f  dcenterfrequency %f ", cineParams.dopplerVelocityScale,dcenterfrequency);

    // setup CW process params
    //------- Image Processing ---------------
    ImageProcess::ProcessParams   processParams =
            {
                    .imagingCaseId  = imagingCaseId,
                    .imagingMode    = mImagingMode,
                    .numSamples     = 0,
                    .numRays        = 0
            };

    mCwModeCommonProcessor.setProcessParams(processParams);
    mCwModeSpectralProcessor.setProcessParams(processParams);
    mCwModeAudioProcessor.setProcessParams(processParams);

    // TODO: update
    uint32_t cwBandpassFilterIndex = 0;
    uint32_t cwSpectralCompressionCurveIndex = 33;
    uint32_t hilbertFilterIndex = 0;

    // Setup CW specific params
    mCwModeCommonProcessor.getStage<CwBandPassFilterProcess>().each(&CwBandPassFilterProcess::setProcessIndices,
            cwBandpassFilterIndex, cineParams.cwPrfIndex);
    mCwModeCommonProcessor.getStage<DopplerWallFilterProcess>().each(&DopplerWallFilterProcess::setCwProcessIndices,
                                                                     cwIndices[1], cineParams.cwPrfIndex);
    mCwModeSpectralProcessor.getStage<DopplerSpectralProcess>().each(&DopplerSpectralProcess::setCwProcessIndices,
                             cineParams.cwPrfIndex, cineParams.dopplerFFTNumLinesToAvg, cwSpectralCompressionCurveIndex);
    mCwModeSpectralProcessor.getStage<DopplerSpectralSmoothingProcess>().each(&DopplerSpectralSmoothingProcess::setProcessIndices,
                                                                              cineParams.dopplerFFTSize, cineParams.dopplerFftSmoothingNum);
    mCwModeAudioProcessor.getStage<DopplerHilbertProcess>().each(&DopplerHilbertProcess::setCwProcessIndices, cineParams.cwPrfIndex, hilbertFilterIndex);
    mCwModeAudioProcessor.getStage<DopplerAudioUpsampleProcess>().each(&DopplerAudioUpsampleProcess::setCwProcessingIndices,
                                                                       cineParams.cwPrfIndex, hilbertFilterIndex, cineParams.dopplerAudioUpsampleRatio, false);

    float refGaindB, peakThresholdCW, defaultGaindB;
    float gainOffset, gainRange;
    mUpsReaderSPtr->getCWPeakThresholdParams(cineParams.organId, refGaindB, peakThresholdCW);

    // Default CW gain
    mUpsReaderSPtr->getCWGainParams(cineParams.organId,
                                    gainOffset, gainRange);
    defaultGaindB = gainOffset + (gainRange * 0.5f);

    mDopplerPeakMeanProc->setProcessIndices(cineParams.dopplerFFTSize, defaultGaindB, peakThresholdCW);
    mDopplerPeakMeanProc->setDataType(DAU_DATA_TYPE_CW, DAU_DATA_TYPE_DOP_PM);

    mCineBuffer.setCineParamsOnly(cineParams);

    retVal = getApiEstimates( imagingCaseId, apiEstimates);
    if (THOR_OK != retVal )
    {
        return (retVal);
    }
    getImagingInfo( imagingCaseId, imagingInfo);

    if ( 0 != mUpsReaderSPtr->getErrorCount())
    {
        ALOGE("Dau::setCwImagingCase : UPS read errors = %d", mUpsReaderSPtr->getErrorCount());
        mUpsReaderSPtr->clearErrorCount();
        return (TER_IE_UPS_READ_FAILURE);
    }
    else
    {
        ALOGD("Dau::setCwImagingCase : NO UPS READ ERRORS");
        // Typically sequencer blob is loaded here, but not sequencer for CW
        // This routine is moved to Dau::start()
        return THOR_OK;
    }
}

//-----------------------------------------------------------------------------
ThorStatus Dau::getApiEstimates( uint32_t imagingCaseId, float apiEstimates[8] )
{
    ThorStatus status = THOR_OK;
    float txVoltage = MIN_HVPS_VOLTS;
    bool  isLinearProbe = (mProbeInfo.probeType & DEV_PROBE_MASK) == PROBE_TYPE_LINEAR;

    apiEstimates[0] = 0.0f;
    apiEstimates[1] = 0.0f;
    apiEstimates[2] = 0.0f;
    apiEstimates[3] = 0.0f;
    bool isPWTDI = mCineBuffer.getParams().isPWTDI;
    // TODO:: clean up the PW/CW case once API black box is integrated for these imaging cases
    if ( (IMAGING_MODE_PW == mImagingMode) || (IMAGING_MODE_CW == mImagingMode) )
    {
        float hvpb, hvnb;
        if (IMAGING_MODE_PW == mImagingMode)
            mUpsReaderSPtr->getPwHvpsValues(imagingCaseId, hvpb, hvnb);
        else
            mUpsReaderSPtr->getCwHvpsValues(imagingCaseId, hvpb, hvnb);

        mHvVoltage = hvpb;
        //return (THOR_OK);
    }

    switch (mUpsReleaseType)
    {
        case UpsReader::RELEASE_TYPE_PRODUCTION:
            status = mApiManagerPtr->estimateIndices(imagingCaseId,
                                                     mImagingMode,
                                                     isLinearProbe,
                                                     mSequenceBuilderPtr,
                                                     mColorAcqPtr, mColorAcqLinearPtr,
                                                     mDopplerAcqPtr, mDopplerAcqLinearPtr, mCwAcqPtr,
                                                     apiEstimates, txVoltage, isPWTDI);
            if (THOR_OK != status)
            {
                txVoltage = MIN_HVPS_VOLTS;
            }
            mSequenceBuilderPtr->updateHvPsSetting(txVoltage);
            break;

        case UpsReader::RELEASE_TYPE_CLINICAL:
            status = mApiManagerPtr->estimateIndices(imagingCaseId,
                                                     mImagingMode,
                                                     isLinearProbe,
                                                     mSequenceBuilderPtr,
                                                     mColorAcqPtr, mColorAcqLinearPtr,
                                                     mDopplerAcqPtr, mDopplerAcqLinearPtr, mCwAcqPtr,
                                                     apiEstimates, txVoltage, isPWTDI);
            if (THOR_OK == status)
            {
                mSequenceBuilderPtr->updateHvPsSetting(txVoltage);
            } // TODO: need to handle the else appropriately. status != THOR_OK is not necessarily an error condition
            // for ReleaseType != RELEASE_TYPE_PRODUCTION
            status = THOR_OK;
            break;

        default:
        {
            float hvpb, hvnb;
            if (IMAGING_MODE_PW == mImagingMode)
            {
                mUpsReaderSPtr->getPwHvpsValues(imagingCaseId, hvpb, hvnb);
            }
            else
            {
                mUpsReaderSPtr->getHvpsValues(imagingCaseId, hvpb, hvnb);
            }
            txVoltage = hvpb;
            break;
        }

    }
    mHvVoltage = txVoltage;
    return (status);
}

//-----------------------------------------------------------------------------
ThorStatus Dau::getApiEstimatesPW( uint32_t imagingCaseId, uint32_t organId, float gateRange,  float apiEstimates[8] )
{
    ThorStatus status = THOR_OK;
    float txVoltage = MIN_HVPS_VOLTS;
    bool  isLinearProbe = (mProbeInfo.probeType & DEV_PROBE_MASK) == PROBE_TYPE_LINEAR;
    float focalDepthMm;
    uint32_t halfcycle;
    bool utpsFound;
    int32_t utpIds[3];
    focalDepthMm = mDopplerAcqLinearPtr->getFocalDepth();
    halfcycle = mDopplerAcqLinearPtr->getHalfCycle();
    if (focalDepthMm<10.0)
        focalDepthMm=10.0;
    if (focalDepthMm>90.0)
        focalDepthMm=90.0;
    utpsFound = mUpsReaderSPtr->getUtpIdsPW(imagingCaseId, mImagingMode, utpIds,
                                            focalDepthMm, halfcycle, 0, organId, gateRange);
    apiEstimates[0] = 0.0f;
    apiEstimates[1] = 0.0f;
    apiEstimates[2] = 0.0f;
    apiEstimates[3] = 0.0f;
    bool isPWTDI = mCineBuffer.getParams().isPWTDI;
    // TODO:: clean up the PW/CW case once API black box is integrated for these imaging cases
    if ( (IMAGING_MODE_PW == mImagingMode) || (IMAGING_MODE_CW == mImagingMode) )
    {
        float hvpb, hvnb;
        if (IMAGING_MODE_PW == mImagingMode)
            mUpsReaderSPtr->getPwHvpsValues(imagingCaseId, hvpb, hvnb);
        else
            mUpsReaderSPtr->getCwHvpsValues(imagingCaseId, hvpb, hvnb);

        mHvVoltage = hvpb;
        if (gateRange > 2.0 or utpIds[0] == 405 or utpIds[0] == 410)
            mHvVoltage = 10.0;
        if (utpIds[0] == 371)
            mHvVoltage = 11.0;
        if (utpIds[0] == 389)
            mHvVoltage = 9.0;
        if (utpIds[0] >=332 and utpIds[0]<=332)
            mHvVoltage = 12;
        if (utpIds[0] >=411 and utpIds[0]<=411)
            mHvVoltage = 8.0;
        if (utpIds[0] >=412 and utpIds[0]<=412)
            mHvVoltage = 10;
        if (utpIds[0] >=426 and utpIds[0]<=426)
            mHvVoltage = 8;
        //return (THOR_OK);
    }

    switch (mUpsReleaseType)
    {
        case UpsReader::RELEASE_TYPE_PRODUCTION:
            status = mApiManagerPtr->estimateIndicesPW(imagingCaseId,
                                                       mImagingMode,
                                                       isLinearProbe,
                                                       mSequenceBuilderPtr,
                                                       mColorAcqPtr, mColorAcqLinearPtr,
                                                       mDopplerAcqPtr, mDopplerAcqLinearPtr, mCwAcqPtr,
                                                       apiEstimates, txVoltage, isPWTDI, organId, gateRange);
            if (THOR_OK != status)
            {
                txVoltage = MIN_HVPS_VOLTS;
            }
            mSequenceBuilderPtr->updateHvPsSetting(txVoltage);
            break;

        case UpsReader::RELEASE_TYPE_CLINICAL:
            status = mApiManagerPtr->estimateIndicesPW(imagingCaseId,
                                                       mImagingMode,
                                                       isLinearProbe,
                                                       mSequenceBuilderPtr,
                                                       mColorAcqPtr, mColorAcqLinearPtr,
                                                       mDopplerAcqPtr, mDopplerAcqLinearPtr, mCwAcqPtr,
                                                       apiEstimates, txVoltage, isPWTDI, organId, gateRange);
            if (THOR_OK == status)
            {
                mSequenceBuilderPtr->updateHvPsSetting(txVoltage);
            } // TODO: need to handle the else appropriately. status != THOR_OK is not necessarily an error condition
            // for ReleaseType != RELEASE_TYPE_PRODUCTION
            status = THOR_OK;
            break;

        default:
        {
            float hvpb, hvnb;
            if (IMAGING_MODE_PW == mImagingMode)
            {
                mUpsReaderSPtr->getPwHvpsValues(imagingCaseId, hvpb, hvnb);
            }
            else
            {
                mUpsReaderSPtr->getHvpsValues(imagingCaseId, hvpb, hvnb);
            }
            txVoltage = hvpb;
            break;
        }

    }
    if (gateRange > 2.0 or utpIds[0] == 405 or utpIds[0] == 410)
        mHvVoltage = 10.0;
    if (utpIds[0] == 371)
        mHvVoltage = 11.0;
    if (utpIds[0] == 389)
        mHvVoltage = 9.0;
    if (utpIds[0] >=332 and utpIds[0]<=332)
        mHvVoltage = 12;
    if (utpIds[0] >=411 and utpIds[0]<=411)
        mHvVoltage = 8.0;
    if (utpIds[0] >=412 and utpIds[0]<=412)
        mHvVoltage = 10;
    if (utpIds[0] >=426 and utpIds[0]<=426)
        mHvVoltage = 8;
    mHvVoltage = txVoltage;
    return (status);
}

//-----------------------------------------------------------------------------
void Dau::getImagingInfo( uint32_t imagingCaseId, float imagingInfo[8] )
{
    float frameRate = mSequenceBuilderPtr->getFrameRate();
    float dutyCycle = mSequenceBuilderPtr->getDutyCycle();
    bool isPWTDI = mCineBuffer.getParams().isPWTDI;
    {
        float centerFrequency;
        uint32_t txApertureElement;
        uint32_t numHalfCycles;
        mUpsReaderSPtr->getBWaveformInfo(imagingCaseId, txApertureElement, centerFrequency, numHalfCycles);
        imagingInfo[0] = centerFrequency;
        if (mImagingMode == IMAGING_MODE_COLOR)
        {
            mUpsReaderSPtr->getColorWaveformInfo(imagingCaseId, txApertureElement, centerFrequency,
                                                 numHalfCycles);
            imagingInfo[1] = centerFrequency;
        }
        else if (mImagingMode == IMAGING_MODE_PW)
        {
            mUpsReaderSPtr->getPwWaveformInfo(txApertureElement, centerFrequency, isPWTDI);
            imagingInfo[0] = centerFrequency;
        }
        else if (mImagingMode == IMAGING_MODE_CW)
        {
            imagingInfo[0] = CW_CENTER_FREQ;
        }
        else
        {
            imagingInfo[1] = 0.0f;
        }
    }

    imagingInfo[2] = frameRate;
    imagingInfo[3] = dutyCycle;
    LOGI("Imaging case: %d, frame rate = %f, duty cycle = %f", imagingCaseId, frameRate, dutyCycle);
}

//-----------------------------------------------------------------------------
void Dau::getImagingInfoLinear( uint32_t imagingCaseId, float imagingInfo[8] ,float gateAxialStart, uint32_t gateIndex,
                                float gateAzimuthLoc, float gateAngle)
{
    float frameRate = mSequenceBuilderPtr->getFrameRate();
    float dutyCycle = mSequenceBuilderPtr->getDutyCycle();
    bool isPWTDI = mCineBuffer.getParams().isPWTDI;
    {
        float centerFrequency;
        uint32_t txApertureElement;
        uint32_t numHalfCycles;
        mUpsReaderSPtr->getBWaveformInfo(imagingCaseId, txApertureElement, centerFrequency, numHalfCycles);
        imagingInfo[0] = centerFrequency;
        if (mImagingMode == IMAGING_MODE_COLOR)
        {
            mUpsReaderSPtr->getColorWaveformInfo(imagingCaseId, txApertureElement, centerFrequency,
                                                 numHalfCycles);
            imagingInfo[1] = centerFrequency;
        }
        else if (mImagingMode == IMAGING_MODE_PW)
        {
            float gateRange = mUpsReaderSPtr->getGateSizeMm(gateIndex);
            uint32_t organId = mUpsReaderSPtr->getImagingOrgan(imagingCaseId);
            float pwFocus = 10.0f*floor((gateAxialStart + 0.5*gateRange)/10.0);
            mUpsReaderSPtr->getPwCenterFrequencyLinear(organId,pwFocus,gateRange,centerFrequency);
            imagingInfo[0] = centerFrequency;
        }
        else
        {
            imagingInfo[1] = 0.0f;
        }
    }
    imagingInfo[2] = frameRate;
    imagingInfo[3] = dutyCycle;
    LOGI("Imaging case: %d, frame rate = %f, duty cycle = %f", imagingCaseId, frameRate, dutyCycle);
}

//-----------------------------------------------------------------------------
void Dau::setDaEcgCineParams(CineBuffer::CineParams& cineParams)
{
    uint32_t ecgSamplesPerFrame;
    uint32_t daSamplesPerFrame;
    uint32_t afeClksPerDaSample, afeClksPerEcgSample;
    uint32_t ecgADCMax;
    const UpsReader::Globals* glbPtr = mUpsReaderSPtr->getGlobalsPtr();
    float samplingFrequency = glbPtr->samplingFrequency;

    ecgSamplesPerFrame = mSequenceBuilderPtr->getNumEcgSamplesPerFrame();
    daSamplesPerFrame = mSequenceBuilderPtr->getNumDaSamplesPerFrame();
    mUpsReaderSPtr->getAfeClksPerDaEcgSample(0, afeClksPerDaSample, afeClksPerEcgSample);
    mUpsReaderSPtr->getEcgADCMax(0, ecgADCMax);
    cineParams.ecgSamplesPerSecond = samplingFrequency*1e6/afeClksPerEcgSample;
    cineParams.daSamplesPerSecond = samplingFrequency*1e6/afeClksPerDaSample;

    if ( nullptr != mEcgProcessPtr)
    {
        mEcgProcessPtr->setADCMax(ecgADCMax);
        mEcgProcessPtr->setProcessParams(ecgSamplesPerFrame, cineParams.ecgSamplesPerSecond);
    }

    cineParams.organId = mUpsReaderSPtr->getImagingOrgan(mImagingCase);

    if ( nullptr != mDaProcessPtr)
    {
        mDaProcessPtr->setProcessParams(daSamplesPerFrame, cineParams.organId);
    }

    // get daEcgScrollparam from CineBuffer
    cineParams.daEcgScrollSpeedIndex = mCineBuffer.getParams().daEcgScrollSpeedIndex;

    cineParams.ecgSamplesPerFrame = ecgSamplesPerFrame;
    cineParams.daSamplesPerFrame = daSamplesPerFrame;
    cineParams.targetFrameRate = 30;
}

//-----------------------------------------------------------------------------
ThorStatus Dau::setMLine(float mLinePosition)
{
    ThorStatus  retVal;

    if (mIsDauStopped)
    {
        ALOGD("Dau::setMLine - dau stopped - skipping");
        return THOR_OK;
    }

    ALOGD("Dau::setMLine(%f)", mLinePosition);

    retVal = stop();

    if (THOR_OK != retVal)
    {
        return(retVal);
    }
    
    uint32_t beamNumber;
    uint32_t mlSelection;
    bool     isLinearProbe = (mProbeInfo.probeType & DEV_PROBE_MASK) == PROBE_TYPE_LINEAR;
    
    mMmodeAcqPtr->getNearestBBeam(mImagingCase, mLinePosition, beamNumber, mlSelection, isLinearProbe);
    ALOGD("M-Mode beam is %d, ML selection = %d", beamNumber, mlSelection);
    mMModeProcessor.getStage<DeInterleaveMProcess>().each(&DeInterleaveMProcess::setMultilineSelection, mlSelection);
    mSequenceBuilderPtr->updateMLine(beamNumber);
    if ( 0 != mUpsReaderSPtr->getErrorCount())
    {
        ALOGE("Dau::setMLine : UPS read errors = %d", mUpsReaderSPtr->getErrorCount());
        return (TER_IE_UPS_READ_FAILURE);
    }
    else
    {
        ALOGD("Dau::setMLine : NO UPS READ ERRORS");
    }

    retVal = start();
    
    return( retVal );
}

//-----------------------------------------------------------------------------
void Dau::enableImageProcessing(int type, bool enable)
{
    static const int DEFAULT_GAIN = 50;
    static const int DEFAULT_AUTO_GAIN = 12;

    switch (type)
    {
        case IMAGE_PROCESS_AUTO_GAIN:
            {
                auto autoGainStage = mBModeProcessor.getStage<AutoGainProcess>();
                if (!autoGainStage)
                {
                    return;
                }
                if (!autoGainStage.isEnabled())
                {
                    // Restore tradition gain before enabling auto-gain
                    setGain(DEFAULT_GAIN);
                }
                autoGainStage.setEnabled(enable);
                if (enable)
                {
                    setGain(DEFAULT_AUTO_GAIN);
                }
            }
            break;

        case IMAGE_PROCESS_DESPECKLE:
            {
                auto speckleStage = mBModeProcessor.getStage<SpeckleReductionProcess>();
                speckleStage.setEnabled(enable);
            }
            break;

        default:
            break;
    }
}

//-----------------------------------------------------------------------------
void Dau::setGain(int gain)
{
    auto newGain = (float) gain;
#if 0
    auto autoGainStage = mBModeProcessor.getStage<AutoGainProcess>();

    if (autoGainStage && autoGainStage.isEnabled())
    {
        // Auto gain setting
        newGain = 0.125 + (7.875 * (newGain / 100));

        ALOGD("New auto-gain value = %f", newGain);

        autoGainStage.each(&AutoGainProcess::setUserGain, newGain);
    }
    else
#endif
    {
        // Traditional gain setting
        newGain = -15.0f + (30.0f * (newGain / 100));

        ALOGD("New gain value = %f", newGain);

        mGainManagerPtr->applyBModeGain(*mLutManagerPtr, newGain, newGain, mImagingCase);
    }
}

//-----------------------------------------------------------------------------
void Dau::setGain(int nearGain, int farGain)
{
    auto nearGainF = (float) nearGain;
    auto farGainF  = (float) farGain;

    nearGainF = -15.0f + (30.0f * (nearGainF / 100));
    farGainF  = -15.0f + (30.0f * ( farGainF / 100));

    mGainManagerPtr->applyBModeGain(*mLutManagerPtr, nearGainF, farGainF, mImagingCase);
}

//-----------------------------------------------------------------------------
void Dau::setColorGain(int gain)
{
    mColorGainPercent = gain;
    mColorProcessor.getStage<ColorProcess>().each(&ColorProcess::setGain, gain);
}

//-----------------------------------------------------------------------------
void Dau::setEcgGain(float percentGain)
{
    if (!supportsEcg())
    {
        return;
    }

    bool isInternal = true;
    float gainOffset, gainRange;

    mUpsReaderSPtr->getEcgGainConstants( isInternal, gainOffset, gainRange );
    float dBGain = gainOffset + (gainRange * (percentGain / 100));
    ALOGD("ECG gain is %f dB", dBGain);
    mEcgProcessPtr->setGain(dBGain);
}

//-----------------------------------------------------------------------------
void Dau::setDaGain(float percentGain)
{
    if (!supportsDa())
    {
        return;
    }

    bool isInternal = true;
    float gainOffset, gainRange;

    mUpsReaderSPtr->getDaGainConstants( gainOffset, gainRange );
    float dBGain = gainOffset + (gainRange * (percentGain / 100));
    ALOGD("DA gain is %f dB", dBGain);
    mDaProcessPtr->setGain(dBGain);

}

//-----------------------------------------------------------------------------
void Dau::setPwGain(int percentGain)
{
    float gainOffset, gainRange;
    bool isPWTDI = mCineBuffer.getParams().isPWTDI;

    // TODO: update with the real gain adjustment
    mUpsReaderSPtr->getPWGainParams(mUpsReaderSPtr->getImagingOrgan(mImagingCase),
                                    gainOffset, gainRange, isPWTDI);
    //gainOffset = -15.0f;
    //gainRange = 60.0f;

    float dBGain = gainOffset + (gainRange * (((float) percentGain) / 100.0f));
    if (isPWTDI)
        ALOGD("TDI gain is %f dB", dBGain);
    else
        ALOGD("PW gain is %f dB", dBGain);

    mPwModeCommonProcessor.getStage<DopplerWallFilterProcess>().each(&DopplerWallFilterProcess::setWFGain, dBGain);
    mDopplerPeakMeanProc->setWFGain(dBGain);

    // store dBGain to cineParams for Peak Mean reference
    mCineBuffer.getParams().dopplerWFGain = dBGain;
}

//-----------------------------------------------------------------------------
void Dau::setCwGain(int percentGain)
{
    float gainOffset, gainRange;

    // TODO: update with the real gain adjustment
    mUpsReaderSPtr->getCWGainParams(mUpsReaderSPtr->getImagingOrgan(mImagingCase), gainOffset, gainRange);
    //gainOffset = 0.0f;
    //gainRange = 60.0f;

    float dBGain = gainOffset + (gainRange * (((float) percentGain) / 100.0f));
    ALOGD("CW gain is %f dB", dBGain);

    mCwModeCommonProcessor.getStage<DopplerWallFilterProcess>().each(&DopplerWallFilterProcess::setWFGain, dBGain);
    mDopplerPeakMeanProc->setWFGain(dBGain);

    // store dBGain to cineParams
    mCineBuffer.getParams().dopplerWFGain = dBGain;
}

//-----------------------------------------------------------------------------
void Dau::setPwAudioGain(int percentGain)
{
    float gainOffset, gainRange;

    // TODO: update with the real gain adjustment
    mUpsReaderSPtr->getPWGainAudioParams(mUpsReaderSPtr->getImagingOrgan(mImagingCase), gainOffset, gainRange);
    //gainOffset = -20.0f;
    //gainRange = 40.0f;

    float dBGain = gainOffset + (gainRange * (((float) percentGain) / 100.0f));
    ALOGD("PW Audio gain is %f dB", dBGain);

    mPwModeAudioProcessor.getStage<DopplerAudioUpsampleProcess>().each(&DopplerAudioUpsampleProcess::setAudioGain, dBGain);
}

//-----------------------------------------------------------------------------
void Dau::setCwAudioGain(int percentGain)
{
    float gainOffset, gainRange;

    // TODO: update with the real gain adjustment
    mUpsReaderSPtr->getCWGainAudioParams(mUpsReaderSPtr->getImagingOrgan(mImagingCase), gainOffset, gainRange);
    //gainOffset = 0.0f;
    //gainRange = 40.0f;

    float dBGain = gainOffset + (gainRange * (((float) percentGain) / 100.0f));
    ALOGD("CW Audio gain is %f dB", dBGain);

    mCwModeAudioProcessor.getStage<DopplerAudioUpsampleProcess>().each(&DopplerAudioUpsampleProcess::setAudioGain, dBGain);
}

//SetUSSignal On/Off
void Dau::setUSSignal(bool isUsSignal)
{
    mIsUsSignal = isUsSignal;
}

//SetECGSignal On/Off
void Dau::setECGSignal(bool isECGSignal)
{
    if (isECGSignal && !supportsEcg())
    {
        // Ignore if the probe doesn't support ECG
        ALOGE("ECG is not supported on this probe");
        return;
    }

    mIsECGSignal = isECGSignal;
}

//Set DASignal On/Off
void Dau::setDASignal(bool isDASignal)
{
    if (isDASignal && !supportsDa())
    {
        // Ignore if the probe doesn't support DA
        ALOGE("DA is not supported on this probe");
        return;
    }

    mIsDASignal = isDASignal;
}

//Set External ECG from Application side
void Dau::setExternalEcg(bool isExternalECG)
{
    LOGD("%s ECG signal source is %s", __func__, isExternalECG ? "EXTERNAL" : "INTERNAL");
    mTempExtEcg = isExternalECG;
}
/**
 *
 * @param leadNumber Set lead number used for External ECG signal path
 */
void Dau::setLeadNumber(int leadNumber)
{
   mLeadNoExternal=(uint32_t)leadNumber;
}
//-----------------------------------------------------------------------------
void Dau::getFov(int depthId, int imagingMode,float fov[])
{
    ScanDescriptor sd;

    LOGD("getFov");
    mUpsReaderSPtr->getFov(depthId, sd, imagingMode, mCineBuffer.getParams().organId);
    fov[0] = sd.axialStart;
    fov[1] = sd.axialSpan;
    fov[2] = sd.azimuthStart;
    fov[3] = sd.azimuthSpan;
}

//-----------------------------------------------------------------------------
void Dau::getDefaultRoi(int organId, int depthId, float roi[])
{
    ScanDescriptor sd;
    LOGD("getDefaultR");
    mUpsReaderSPtr->getDefaultRoi(organId, depthId, sd);
    roi[0] = sd.axialStart;
    roi[1] = sd.axialSpan;
    roi[2] = sd.azimuthStart;
    roi[3] = sd.azimuthSpan;
    
}

//-----------------------------------------------------------------------------
uint32_t Dau::getMLinesPerFrame()
{
    return(mMLinesPerFrame);
}

//-----------------------------------------------------------------------------
uint32_t Dau::getFrameIntervalMs()
{
    uint32_t retVal = 0;

    if (mSequenceBuilderPtr != nullptr)
    {
        retVal = mSequenceBuilderPtr->getFrameIntervalMs();
    }

    return(retVal);
}

//-----------------------------------------------------------------------------
bool Dau::getExternalEcg()
{
    bool    isConnected;

    mTempExtEcgLock.enter();
    isConnected = mTempExtEcg;
    mTempExtEcgLock.leave();

#ifdef ENABLE_EXTERNAL_ECG
    if (supportsEcg())
    {
        isConnected = mDauHwSPtr->isExternalEcgConnected();
    }
#endif

    return(isConnected); 
}

//-----------------------------------------------------------------------------
void Dau::getProbeInfo(ProbeInfo *probeData)
{
    probeData->probeType = mProbeInfo.probeType;
    probeData->serialNo = mProbeInfo.serialNo;
    probeData->part = mProbeInfo.part;
    probeData->fwVersion = mProbeInfo.fwVersion;
}

//-----------------------------------------------------------------------------
bool Dau::supportsEcg()
{
    bool retVal = false;

    // TODO: Instead of checking the probe type, this may come from
    //       the UPS in the future.
    if ((mProbeInfo.probeType & DEV_PROBE_MASK) == PROBE_TYPE_TORSO3)
    {
        retVal = true;
    }

    ALOGD("SupportsEcg: %s", retVal ? "True" : "False");

    return(retVal);
}

//-----------------------------------------------------------------------------
bool Dau::supportsDa()
{
    bool retVal = false;

    // TODO: Instead of checking the probe type, this may come from
    //       the UPS in the future.
    if ((mProbeInfo.probeType & DEV_PROBE_MASK) == PROBE_TYPE_TORSO3)
    {
        retVal = true;
    }

    return(retVal);
}

//-----------------------------------------------------------------------------
const char* Dau::getDbName()
{
    return(mUpsReaderSPtr->getDbName(mProbeInfo.probeType));
}

//-----------------------------------------------------------------------------
void Dau::readProbeInfo()
{
//    uint32_t            readData[1024];
    FwInfo              fwInfo;
    FactoryData         fData;
    std::string         majV;
    std::string         minV;
    std::string         patV;
    std::string         appVersion;

    memset(&mProbeInfo, '\0', sizeof(ProbeInfo));

    if (mDauContext.isUsbContext)
    {
        ALOGD("USB probe firmware information");
        UsbDauFwUpgrade     usbFwUpgrade;
        if (!usbFwUpgrade.init(&mDauContext))
        {
            ALOGE("Unable to intialize UsbDauFwUpgrade");
            goto err_ret;
        }
        if (!usbFwUpgrade.readFactData((uint32_t *)&fData,FACTORY_DATA_START, sizeof(FactoryData)))
        {
            ALOGE("Unable to read factory data");
            goto err_ret;
        }
        if (!usbFwUpgrade.getFwInfo(&fwInfo))
        {
            ALOGE("Unable to read probe firmware information");
            goto err_ret;
        }
    }
    else
    {
        PciDauFwUpgrade     pciFwUpgrade;

        if (!pciFwUpgrade.init(nullptr))
        {
            ALOGE("Unable to intialize PciDauFwUpgrade");
            goto err_ret;
        }

        if (!pciFwUpgrade.readFactData((uint32_t*) &fData,FACTORY_DATA_START, sizeof(FactoryData)))
        {
            ALOGE("Unable to read factory data");
            goto err_ret;
        }

        if (!pciFwUpgrade.getFwInfo(&fwInfo))
        {
            ALOGE("Unable to read probe firmware information");
            goto err_ret;
        }
        if(!pciFwUpgrade.deinit())
             ALOGE("Failed to deinit PciDauFwUpgrade");
    }

    mProbeInfo.probeType = fData.probeType;
    if(fData.probeType != -1)
    {
        mProbeInfo.serialNo = fData.serialNum;
        mProbeInfo.part = fData.partNum;
        ALOGD("probeType : 0x%x\n\r", fData.probeType);
        ALOGD("serialNum : %s\n\r", fData.serialNum);
        ALOGD("partNum : %s\n\r", fData.partNum);
    }
    else
    {
        ALOGE("probeType : 0x%x\n\r", fData.probeType);
        ALOGE("serialNum : %s\n\r", fData.serialNum);
        ALOGE("partNum : %s\n\r", fData.partNum);
        mProbeInfo.serialNo = "00000000";
        mProbeInfo.part = "00000000";
    }

    if(fwInfo.bootUpdateFlag1 == 1)
    {
        ALOGD("Application1 v%d.%d.%d\n\r", fwInfo.app1Version.majorV,fwInfo.app1Version.minorV,fwInfo.app1Version.patchV);
        ALOGD("Boot Flag 1 : 0x%x\n\r", fwInfo.bootUpdateFlag1);

        majV = std::to_string(fwInfo.app1Version.majorV);
        minV = std::to_string(fwInfo.app1Version.minorV);
        patV = std::to_string(fwInfo.app1Version.patchV);

        appVersion.append(majV);
        appVersion.append(1u,'.');
        appVersion.append(minV);
        appVersion.append(1u,'.');
        appVersion.append(patV);
    }
    else if(fwInfo.bootUpdateFlag2 == 1)
    {
        ALOGD("Application2 v%d.%d.%d\n\r", fwInfo.app2Version.majorV,fwInfo.app2Version.minorV,fwInfo.app2Version.patchV);
        ALOGD("Boot Flag 2 : 0x%x\n\r", fwInfo.bootUpdateFlag2);

        majV = std::to_string(fwInfo.app2Version.majorV);
        minV = std::to_string(fwInfo.app2Version.minorV);
        patV = std::to_string(fwInfo.app2Version.patchV);

        appVersion.append(majV);
        appVersion.append(1u,'.');
        appVersion.append(minV);
        appVersion.append(1u,'.');
        appVersion.append(patV);
    } else{
        ALOGE("Boot Flag 1 : 0x%x\n\r", fwInfo.bootUpdateFlag1);
        ALOGE("Boot Flag 2 : 0x%x\n\r", fwInfo.bootUpdateFlag2);
        appVersion = "0.0.0";
    }

    mProbeInfo.fwVersion = appVersion;
    ALOGD("Firmware version : %s\n\r", mProbeInfo.fwVersion.c_str());

    ALOGD("Probe Type : 0x%X\n\r", mProbeInfo.probeType);
    ALOGD("Serial Number : %s\n\r", mProbeInfo.serialNo.c_str());
    ALOGD("part : %s\n\r", mProbeInfo.part.c_str());

err_ret:
    ;
}

//-----------------------------------------------------------------------------
void Dau::setDebuggingFlag(bool flag)
{
    ALOGD("DEBUGFLAG:: Dau Debugging flag value : %d\n",flag);
    mDebugFlag = flag;
    ALOGD("DEBUGFLAG:: Dau Debugging mDebugFlag value : %d\n",mDebugFlag);

}

//-----------------------------------------------------------------------------
void Dau::setDauSafetyFeatureTestOption(int gSafetyTestSelectedOption, float gDauThresholdTemperatureForTest)
{
    ALOGD("SAFETYTEST:: Dau Safety Test Selected Option value : %d\n", gSafetyTestSelectedOption);
    ALOGD("SAFETYTEST:: Dau Safety Test Selected Temperature value : %f\n", gDauThresholdTemperatureForTest);

    switch(gSafetyTestSelectedOption)
    {
        case SAFETY_DAU_TEMP_TEST:
            mIsTempSafetyTest = true;
            mDauTempThresholdVal = gDauThresholdTemperatureForTest;
            break;

        case SAFETY_DAU_WD_RESET_TEST:
            mIsWatchDogResetTest = true;
            break;

        case SAFETY_DATA_UNDERFLOW_TEST:
            mIsDataUnderflowTest = true;
            break;

        case SAFET_DATA_OVERFLOW_TEST:
            mIsDataOverflowTest = true;
            break;
        default:
            mIsTempSafetyTest = false;
            mDauTempThresholdVal = 0.0;
            mIsWatchDogResetTest = false;
            mIsDataOverflowTest = false;
            mIsDataUnderflowTest = false;
            break;
    }

}

#ifdef GENERATE_TEST_IMAGES
//-----------------------------------------------------------------------------
// Setup up some dummy data
void Dau::populateDmaFifo()
{
    float       ux = 255.0;
    uint8_t*    dmaBufferPtr = (uint8_t*) mDmaBufferSPtr->getBufferPtr(DmaBufferType::Fifo);
    uint8_t     headerOffset = DauInputManager::INPUT_HEADER_SIZE_BYTES;

    // Writting 255 to each location will result in a white image if there is
    // missing data.  Acts as a visual aid for detecting issues.
    memset(dmaBufferPtr, 255, mDmaBufferSPtr->getBufferLength(DmaBufferType::Fifo));
}
#endif

//-----------------------------------------------------------------------------
ThorStatus Dau::SignalCallback::onInit()
{
    Dau*        thisPtr = (Dau*) getDataPtr();
    ThorStatus  openStatus = THOR_OK;

    mImagingMode = thisPtr->mImagingMode;
    mDauDataTypes = thisPtr->mDauDataTypes;
    mDmaFifoPtr = (uint8_t*) thisPtr->mDmaBufferSPtr->getBufferPtr(DmaBufferType::Fifo);
    mColorDmaFifoPtr = mDmaFifoPtr + DauInputManager::INPUT_BASE_OFFSET_COLOR;
    mMModeDmaFifoPtr = mDmaFifoPtr + DauInputManager::INPUT_BASE_OFFSET_M;
    mPwModeDmaFifoPtr = mDmaFifoPtr + DauInputManager::INPUT_BASE_OFFSET_PW;
    mCwModeDmaFifoPtr = mDmaFifoPtr + DauInputManager::INPUT_BASE_OFFSET_CW;
    mEcgFifoPtr = mDmaFifoPtr + DauInputManager::INPUT_BASE_OFFSET_ECG;
    mDaFifoPtr = mDmaFifoPtr + DauInputManager::INPUT_BASE_OFFSET_DA;
    if (nullptr == mDmaFifoPtr)
    {
        ALOGE("Dma Fifo buffer is null");
        openStatus = TER_IE_DMA_FIFO_NULL;
        goto err_ret;
    }

    thisPtr->mCineBuffer.reset();
    openStatus = thisPtr->mCineBuffer.prepareClients();
    if (IS_THOR_ERROR(openStatus))
    {
        goto err_ret;
    }

    if (IMAGING_MODE_COLOR == mImagingMode)
    {
        thisPtr->mColorProcessor.getStage<ColorProcess>().each(&ColorProcess::resetFrameCount);
        thisPtr->mColorProcessor.setFramesCompleted(0u-1u);
    }

    if (IMAGING_MODE_M == mImagingMode)
    {
        thisPtr->mMModeProcessor.setFramesCompleted(0u-1u);
    }

    if (IMAGING_MODE_PW == mImagingMode)
    {
        thisPtr->mPwModeCommonProcessor.setFramesCompleted(0u-1u);
        thisPtr->mPwModeSpectralProcessor.setFramesCompleted(0u-1u);
        thisPtr->mPwModeAudioProcessor.setFramesCompleted(0u-1u);
    }

    if (IMAGING_MODE_CW == mImagingMode)
    {
        thisPtr->mCwModeCommonProcessor.setFramesCompleted(0u-1u);
        thisPtr->mCwModeSpectralProcessor.setFramesCompleted(0u-1u);
        thisPtr->mCwModeAudioProcessor.setFramesCompleted(0u-1u);
    }

    thisPtr->mBModeProcessor.setFramesCompleted(0u-1u);

err_ret:
    return(openStatus);
}

//-----------------------------------------------------------------------------
ThorStatus Dau::SignalCallback::onData(uint32_t count)
{
    Dau*            thisPtr = (Dau*) getDataPtr();
    uint8_t*        inputPtr;
    uint8_t*        outputPtr;
    uint32_t        inputIndex = count % DauInputManager::INPUT_FIFO_LENGTH;
    uint32_t        headerCount;
    uint32_t        daHeaderCount;
    uint32_t        ecgHeaderCount;
    CineHeader*     cineHeaderPtr;
    uint32_t        frameStride;
    uint32_t        bufIndex;

    if (thisPtr->mIsDauStopped)
    {
        return (THOR_OK);
    }

    if ((0 == (count % 10)) && (!thisPtr->mIsWatchDogResetTest))
    {
        thisPtr->mDauHwSPtr->resetWatchdogTimer();
    }

    if(thisPtr->mIsDataUnderflowTest)
    {
        if(count > 100)
        {
            uint32_t val = 0x0;
            if((mImagingMode == IMAGING_MODE_B) ||
               (mImagingMode == IMAGING_MODE_M) ||
               (mImagingMode == IMAGING_MODE_COLOR))
            {
                thisPtr->mDaumSPtr->write(&val, SE_DT0_CTL_ADDR, 1);
                thisPtr->mDaumSPtr->write(&val, SE_DT1_CTL_ADDR, 1);
                thisPtr->mDaumSPtr->write(&val, SE_DT3_CTL_ADDR, 1);
            }
            else if(mImagingMode == IMAGING_MODE_CW)
                thisPtr->mDaumSPtr->write(&val, SE_DT4_CTL_ADDR, 1);
            else if(mImagingMode == IMAGING_MODE_PW)
                thisPtr->mDaumSPtr->write(&val, SE_DT2_CTL_ADDR, 1);
        }
    }

    if (thisPtr->mIsUsSignal)
    {
        inputIndex = count % DauInputManager::INPUT_FIFO_LENGTH;
        if ((nullptr == mDmaFifoPtr))
        {
            LOGI("onData(): null pointer detected.  count = %d", count);
            return (TER_IE_DMA_FIFO_NULL);
        }

        if (mImagingMode == IMAGING_MODE_PW)
            frameStride =  thisPtr->mInputManagerSPtr->getPwFrameStride();
        else if (mImagingMode == IMAGING_MODE_CW)
            frameStride = thisPtr->mInputManagerSPtr->getCwFrameStride();
        else
            frameStride = thisPtr->mInputManagerSPtr->getBFrameStride();

        inputPtr = mDmaFifoPtr + (inputIndex * frameStride);
        headerCount = ((DauInputManager::FrameHeader *) inputPtr)->index;

        if(thisPtr->mIsDataOverflowTest && (count > 200))
        {
            headerCount = count + 1 + DauInputManager::INPUT_FIFO_LENGTH;
        }

        if (headerCount != count)
        {
            ALOGE("Count mismatch!  count = %u,  headerCount = %u,  inputIndex = %d",
                  count, headerCount, inputIndex);

            if ((headerCount >= (count + DauInputManager::INPUT_FIFO_LENGTH)) &&
                (headerCount != UINT_MAX)) // Have seen all 0xFs in header before
            {
                ALOGE("Overflow detected");
                return (TER_IE_OVERFLOW);
            }
            return (TER_IE_INVALID_HEADER);
        }
        else if (0 == (count % 120))
        {
            ALOGD("count = %u,  headerCount = %u,  inputIndex = %d",
                  count, headerCount, inputIndex);

            float hvpAvg = thisPtr->mDauHwSPtr->hvMonitorGetHvpAvg();
            float hvnAvg = thisPtr->mDauHwSPtr->hvMonitorGetHvnAvg();
            float hvpiAvg = thisPtr->mDauHwSPtr->hvMonitorGetHvpiAvg(hvpAvg);
            float hvniAvg = thisPtr->mDauHwSPtr->hvMonitorGetHvniAvg(hvnAvg);
            ALOGD("hvpAvg = %f, hvnAvg = %f, hvpiAvg = %f,hvniAvg = %f",
                  hvpAvg, hvnAvg, hvpiAvg, hvniAvg);
        }
        //Todo Remove this if endif for release build
#if DAU_V4
        if (count == 90)  // set HV limits after receiving first 90 frames
        {
            float hvpAvg = thisPtr->mDauHwSPtr->hvMonitorGetHvpAvg();
            float hvnAvg = thisPtr->mDauHwSPtr->hvMonitorGetHvnAvg();
            float hvpiAvg = thisPtr->mDauHwSPtr->hvMonitorGetHvpiAvg(hvpAvg);
            float hvniAvg = thisPtr->mDauHwSPtr->hvMonitorGetHvniAvg(hvnAvg);
            ALOGD("PMONCTRL: hvpAvg = %f, hvnAvg = %f, hvpiAvg = %f,hvniAvg = %f",
                  hvpAvg, hvnAvg, hvpiAvg, hvniAvg);
            thisPtr->mDauHwSPtr->hvSetLimits(thisPtr->mHvVoltage);
            thisPtr->mDauHwSPtr->hvStartMonitoring(mImagingMode == IMAGING_MODE_CW);
        }
#endif

        switch (mImagingMode)
        {
            case IMAGING_MODE_B:
            case IMAGING_MODE_COLOR:
            case IMAGING_MODE_M:

                thisPtr->mBModeProcessor.processImageAsync(inputPtr, count, DAU_DATA_TYPE_B);

                // overwrite first two frames when compounding is enabled.
                if ( thisPtr->mUpdateCompoundFrames && (thisPtr->mCineBuffer.getMaxFrameNum() > 2) )
                {
                    // copy frame no. 2 to 0 and 1
                    memcpy(thisPtr->mCineBuffer.getFrame(0, DAU_DATA_TYPE_B),
                           thisPtr->mCineBuffer.getFrame(2, DAU_DATA_TYPE_B), MAX_B_FRAME_SIZE);
                    memcpy(thisPtr->mCineBuffer.getFrame(1, DAU_DATA_TYPE_B),
                           thisPtr->mCineBuffer.getFrame(2, DAU_DATA_TYPE_B), MAX_B_FRAME_SIZE);

                    thisPtr->mUpdateCompoundFrames = false;
                }

#ifdef ENABLE_DAECG
                if (0 != BF_GET(mDauDataTypes, DAU_DATA_TYPE_DA, 1))
                {
                    inputPtr = mDaFifoPtr +
                               (inputIndex * thisPtr->mInputManagerSPtr->getDaFrameStride());
                    daHeaderCount = ((DauInputManager::FrameHeader *) inputPtr)->index;
                    thisPtr->mDaProcessPtr->process(inputPtr, count);
                    thisPtr->mCineBuffer.setFrameComplete(count, DAU_DATA_TYPE_DA);
                    if (daHeaderCount != headerCount)
                    {
                        ALOGE("image frame header count (%08x) and DA header count (%08x) do not match",
                              headerCount, daHeaderCount);
                        return (TER_IE_DAECG_FRAME_INDEX);
                    }
                }

                if (0 != BF_GET(mDauDataTypes, DAU_DATA_TYPE_ECG, 1))
                {
                    inputPtr = mEcgFifoPtr +
                               (inputIndex * thisPtr->mInputManagerSPtr->getEcgFrameStride());
                    ecgHeaderCount = ((DauInputManager::FrameHeader *) inputPtr)->index;
                    thisPtr->mEcgProcessPtr->process(inputPtr, count);
                    thisPtr->mCineBuffer.setFrameComplete(count, DAU_DATA_TYPE_ECG);
                    if (ecgHeaderCount != headerCount)
                    {
                        ALOGE("image frame header count (%08x) and ECG header count (%08x) do not match",
                              headerCount, ecgHeaderCount);
                        return (TER_IE_DAECG_FRAME_INDEX);
                    }
                }
#endif

                if (IMAGING_MODE_COLOR == mImagingMode)
                {
                    inputPtr = mColorDmaFifoPtr +
                               (inputIndex * thisPtr->mInputManagerSPtr->getCFrameStride());

                    thisPtr->mColorProcessor.processImageAsync(inputPtr, count,
                                                               DAU_DATA_TYPE_COLOR);
                }
                else if (IMAGING_MODE_M == mImagingMode)
                {
                    inputPtr = mMModeDmaFifoPtr +
                               (inputIndex * thisPtr->mInputManagerSPtr->getMFrameStride());

                    thisPtr->mMModeProcessor.processImageAsync(inputPtr, count, DAU_DATA_TYPE_M);
                }
                break;

            case IMAGING_MODE_PW:
                inputPtr = mPwModeDmaFifoPtr +
                           (inputIndex * thisPtr->mInputManagerSPtr->getPwFrameStride());
                bufIndex = count%2;

                // Link processors first - these are executed after the following asyncLinked process is completed.
                thisPtr->mPwModeCommonProcessor.linkParallelProcs(&thisPtr->mPwModeSpectralProcessor,
                        &(thisPtr->mDopplerIntBuffer[bufIndex][sizeof(CineBuffer::CineFrameHeader)]), count, DAU_DATA_TYPE_PW);
                thisPtr->mPwModeCommonProcessor.linkParallelProcs(&thisPtr->mPwModeAudioProcessor,
                        &(thisPtr->mDopplerIntBuffer[bufIndex][sizeof(CineBuffer::CineFrameHeader)]), count, DAU_DATA_TYPE_PW_ADO);

                // output ptr should point to the header of the temporary buffer
                thisPtr->mPwModeCommonProcessor.processImageAsyncLinked(inputPtr, &(thisPtr->mDopplerIntBuffer[bufIndex][0]),
                                                count, DAU_DATA_TYPE_PW);

                // TODO: re-order/consolidate the following section once M-mode also supports DA/ECG.
#ifdef ENABLE_DAECG
                if (0 != BF_GET(mDauDataTypes, DAU_DATA_TYPE_DA, 1))
                {
                    inputPtr = mDaFifoPtr +
                               (inputIndex * thisPtr->mInputManagerSPtr->getDaFrameStride());
                    daHeaderCount = ((DauInputManager::FrameHeader *) inputPtr)->index;
                    thisPtr->mDaProcessPtr->process(inputPtr, count);
                    thisPtr->mCineBuffer.setFrameComplete(count, DAU_DATA_TYPE_DA);
                    if (daHeaderCount != headerCount)
                    {
                        ALOGE("image frame header count (%08x) and DA header count (%08x) do not match",
                              headerCount, daHeaderCount);
                        return (TER_IE_DAECG_FRAME_INDEX);
                    }
                }

                if (0 != BF_GET(mDauDataTypes, DAU_DATA_TYPE_ECG, 1))
                {
                    inputPtr = mEcgFifoPtr +
                               (inputIndex * thisPtr->mInputManagerSPtr->getEcgFrameStride());
                    ecgHeaderCount = ((DauInputManager::FrameHeader *) inputPtr)->index;
                    thisPtr->mEcgProcessPtr->process(inputPtr, count);
                    thisPtr->mCineBuffer.setFrameComplete(count, DAU_DATA_TYPE_ECG);
                    if (ecgHeaderCount != headerCount)
                    {
                        ALOGE("image frame header count (%08x) and ECG header count (%08x) do not match",
                              headerCount, ecgHeaderCount);
                        return (TER_IE_DAECG_FRAME_INDEX);
                    }
                }
#endif
                break;

            case IMAGING_MODE_CW:
                inputPtr = mCwModeDmaFifoPtr +
                           (inputIndex * thisPtr->mInputManagerSPtr->getCwFrameStride());
                bufIndex = count%2;

                // Link processors first - these are executed after the following asynclinked process is completed.
                thisPtr->mCwModeCommonProcessor.linkParallelProcs(&thisPtr->mCwModeSpectralProcessor,
                        &thisPtr->mDopplerIntBuffer[bufIndex][sizeof(CineBuffer::CineFrameHeader)], count, DAU_DATA_TYPE_CW);
                thisPtr->mCwModeCommonProcessor.linkParallelProcs(&thisPtr->mCwModeAudioProcessor,
                        &thisPtr->mDopplerIntBuffer[bufIndex][sizeof(CineBuffer::CineFrameHeader)], count, DAU_DATA_TYPE_CW_ADO);

                // output ptr should point to the header of the temporary buffer
                thisPtr->mCwModeCommonProcessor.processImageAsyncLinked(inputPtr, &(thisPtr->mDopplerIntBuffer[bufIndex][0]),
                                                                 count, DAU_DATA_TYPE_CW);

                break;

            default:
                break;
        }
    }
    else
    {
        inputIndex = count % DauInputManager::INPUT_FIFO_LENGTH;
        if ((nullptr == mDmaFifoPtr))
        {
            LOGI("onData(): null pointer detected.  count = %d", count);
            return (TER_IE_DMA_FIFO_NULL);
        }

        if (0 != BF_GET(mDauDataTypes, DAU_DATA_TYPE_DA, 1))
        {
            inputPtr = mDaFifoPtr +
                       (inputIndex * thisPtr->mInputManagerSPtr->getDaFrameStride());
            daHeaderCount = ((DauInputManager::FrameHeader *) inputPtr)->index;
            if (daHeaderCount != count)
            {
                ALOGE("Count mismatch!  count = %u,  headerCount = %u,  inputIndex = %d",
                      count, daHeaderCount, inputIndex);

                if ((daHeaderCount >= (count + DauInputManager::INPUT_FIFO_LENGTH)) &&
                    (daHeaderCount != UINT_MAX)) // Have seen all 0xFs in header before
                {
                    ALOGE("Overflow detected");
                    return (TER_IE_OVERFLOW);
                }
                return (TER_IE_INVALID_HEADER);
            }
            thisPtr->mDaProcessPtr->process(inputPtr, count);
            thisPtr->mCineBuffer.setFrameComplete(count, DAU_DATA_TYPE_DA);
        }
        if (0 != BF_GET(mDauDataTypes, DAU_DATA_TYPE_ECG, 1))
        {
            inputPtr = mEcgFifoPtr +
                       (inputIndex * thisPtr->mInputManagerSPtr->getEcgFrameStride());
            ecgHeaderCount = ((DauInputManager::FrameHeader *) inputPtr)->index;
            if (ecgHeaderCount != count)
            {
                ALOGE("Count mismatch!  count = %u,  headerCount = %u,  inputIndex = %d",
                      count, ecgHeaderCount, inputIndex);

                if ((ecgHeaderCount >= (count + DauInputManager::INPUT_FIFO_LENGTH)) &&
                    (ecgHeaderCount != UINT_MAX)) // Have seen all 0xFs in header before
                {
                    ALOGE("Overflow detected");
                    return (TER_IE_OVERFLOW);
                }
                return (TER_IE_INVALID_HEADER);
            }
            thisPtr->mEcgProcessPtr->process(inputPtr, count);
            thisPtr->mCineBuffer.setFrameComplete(count, DAU_DATA_TYPE_ECG);
        }
    }
    return(THOR_OK); 
}

//-----------------------------------------------------------------------------
void Dau::SignalCallback::onHid(uint32_t hidId)
{
    Dau*     thisPtr = (Dau*) getDataPtr();

    if (nullptr != thisPtr->mReportHidFuncPtr)
    {
        thisPtr->mReportHidFuncPtr(hidId);
    }
}

//-----------------------------------------------------------------------------
void Dau::SignalCallback::onExternalEcg(bool isConnected)
{
    Dau*        thisPtr = (Dau*) getDataPtr();
    uint32_t    val;

    if (!thisPtr->supportsEcg())
    {
        return;
    }

#ifdef ENABLE_EXTERNAL_ECG
    isConnected = thisPtr->mDauHwSPtr->handleExternalEcgDetect();
#endif

    ALOGD("External ECG Lead: isConnected = %s", isConnected ? "True" : "False");

    if (nullptr != thisPtr->mExternalEcgFuncPtr &&
        mPrevEcgConnect != isConnected)
    {
        thisPtr->mExternalEcgFuncPtr(isConnected);
    }
    mPrevEcgConnect = isConnected;
}

//-----------------------------------------------------------------------------
ThorStatus Dau::SignalCallback::onCheckTemperature()
{
    ThorStatus  retVal = THOR_OK;

#if ENABLE_TEMP_MONITOR
    Dau*        thisPtr = (Dau*) getDataPtr();
    float       probeTemp;

    retVal = thisPtr->mDauHwSPtr->readTmpSensor(DauHw::I2C_ADDR_TMP0,
                                                probeTemp);

    if (IS_THOR_ERROR(retVal))
    {
        retVal = TER_THERMAL_PROBE_FAIL;
    }
    else
    {
        ALOGD("Probe Temperature: %.1fC", probeTemp);

        if (probeTemp >= thisPtr->mUpsReaderSPtr->getGlobalsPtr()->maxProbeTemperature || (thisPtr->mIsTempSafetyTest && probeTemp >= thisPtr->mDauTempThresholdVal))
        {
            ALOGE("The Dau is too hot");
            retVal = TER_THERMAL_PROBE;
        }
    }
#endif

    return(retVal);
}

//-----------------------------------------------------------------------------
void Dau::SignalCallback::onError(uint32_t errorCode)
{
    Dau*     thisPtr = (Dau*) getDataPtr();

    // BUGBUG: Calling stop from here seemed to cause more problems than it
    //         solved.  May re-introduce later if can be made reliable.
    //thisPtr->stop();

    if ((TER_IE_USB_NO_DEVICE == errorCode) || (TER_IE_USB_ERROR == errorCode))
    {
        thisPtr->mIsDauAttached = false;
    }

    if (nullptr != thisPtr->mReportErrorFuncPtr)
    {
        if (!IS_THOR_ERROR(errorCode))
        {
            // We were given a raw error code from the Dau.  Need to convert
            // to a ThorStatus.
            errorCode = thisPtr->mDauErrorPtr->convertDauError(errorCode);
        }
        thisPtr->mReportErrorFuncPtr(errorCode);
    }
}

//-----------------------------------------------------------------------------
void Dau::SignalCallback::onTest(void* testDataPtr)
{
    Dau*     thisPtr = (Dau*) getDataPtr();
}

//-----------------------------------------------------------------------------
void Dau::SignalCallback::onTerminate()
{
    Dau*     thisPtr = (Dau*) getDataPtr();

    ALOGD("onTerminate: flushing image processors");

    if (IMAGING_MODE_COLOR == mImagingMode)
    {
        thisPtr->mColorProcessor.flush();
    }

    if (IMAGING_MODE_M == mImagingMode)
    {
        thisPtr->mMModeProcessor.flush();
    }

    if (IMAGING_MODE_PW == mImagingMode)
    {
        thisPtr->mPwModeCommonProcessor.flush();
        thisPtr->mPwModeSpectralProcessor.flush();
        thisPtr->mPwModeAudioProcessor.flush();

        if (thisPtr->mDopplerPeakMeanProc != nullptr) {
            thisPtr->mDopplerPeakMeanProc->processAllFrames(false);
            thisPtr->mDopplerPeakMeanProc->flush();
        }
    }

    if (IMAGING_MODE_CW == mImagingMode)
    {
        thisPtr->mCwModeCommonProcessor.flush();
        thisPtr->mCwModeSpectralProcessor.flush();
        thisPtr->mCwModeAudioProcessor.flush();

        if (thisPtr->mDopplerPeakMeanProc != nullptr) {
            thisPtr->mDopplerPeakMeanProc->processAllFrames(false);
            thisPtr->mDopplerPeakMeanProc->flush();
        }
    }

    thisPtr->mBModeProcessor.flush();

    ALOGD("onTerminate: freeing cine clients");
    thisPtr->mCineBuffer.freeClients();

    ALOGD("onTerminate: exiting");
}

