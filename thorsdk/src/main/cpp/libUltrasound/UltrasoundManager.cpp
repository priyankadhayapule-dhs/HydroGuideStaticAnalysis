//
// Copyright 2017 EchoNous Inc.
//
//

#define LOG_TAG "UltrasoundManager"

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <ThorUtils.h>
#include <UltrasoundManager.h>
#include <PciDauFwUpgrade.h>


//-----------------------------------------------------------------------------
UltrasoundManager::UltrasoundManager() :
    mCineViewer(mCineBuffer, mAIManager),
    mCinePlayer(mCineBuffer),
    mCineRecorder(mCineBuffer),
    mAssetManagerPtr(nullptr),
    mUltrasoundEncoder(),
    mAutoControlManager(),
    mAIManager(this),
    mBootLoaderFwUpdate(false),
    mApplicationFwUpdate(false),
    mBootLoaderFwUpdateUsb(false),
    mApplicationFwUpdateUsb(false)
{
}

//-----------------------------------------------------------------------------
UltrasoundManager::~UltrasoundManager()
{
    ALOGD("%s", __func__);
    close();
}

//-----------------------------------------------------------------------------
ThorStatus UltrasoundManager::open(void* javaEnv,
                                   void *javaContext,
                                   AAssetManager* assetManagerPtr,
                                   const char* appPath)
{
    ThorStatus      retVal = THOR_ERROR;
    std::string     upsPath;
    std::string     fontPath;
    std::string     appBinaryPath;
    std::string     blBinaryPath;

    mAssetManagerPtr = assetManagerPtr;
    mAppPath = appPath;
    mCineBuffer.registerCallback(&mCineViewer);
    mCineBuffer.registerCallback(&mCineRecorder);

    retVal = mFilesystem.init(mAssetManagerPtr, appPath);
    if (IS_THOR_ERROR(retVal)) {
        LOGE("Failed to open asset reader");
        goto err_ret;
    }

    for (int index = 0;
         0x0 != UpsReader::ProbeInfoArray[index].probeType;
         index++)
    {
        std::string upsDb = UpsReader::ProbeInfoArray[index].dbName;

        if (!upsDb.empty())
        {
            upsPath = mAppPath + "/" + upsDb;

            if (!extractUps(upsPath, upsDb))
            {
                ALOGE("Unable to extract the UPS %s", upsDb.c_str());
                retVal = TER_IE_UPS_EXTRACT;
                goto err_ret;
            }
        }
    }

    appBinaryPath = mAppPath + "/" + PciDauFwUpgrade::DEFAULT_BIN_FILE;

    if (!extractFwUpgradeBinary(appBinaryPath)) {
        /*ALOGE("Unable to extract FwUpgrade binary file");
        retVal = THOR_ERROR;
        goto err_ret;*/
        ALOGI("FwUpgrade Binary File is not added in asset");
    }

    blBinaryPath = mAppPath + "/" + PciDauFwUpgrade::DEFAULT_BL_BIN_FILE;

    if (!extractBLFwUpgradeBinary(blBinaryPath)) {
        /*ALOGE("Unable to extract FwUpgrade binary file");
        retVal = THOR_ERROR;
        goto err_ret;*/
        ALOGI("BLFwUpgrade Binary File is not added in asset");
    }

    fontPath = mAppPath + "/" + DEFAULT_FONT_FILE;

    if (!extractFont(fontPath))
    {
        ALOGE("Unable to extract the font file");
        retVal = TER_IE_FONT_EXTRACT;
        goto err_ret;
    }

    mUpsReaderSPtr = std::make_shared<UpsReader>();

    retVal = mCinePlayer.init(mUpsReaderSPtr, appPath);
    if (IS_THOR_ERROR(retVal)) {
        LOGE("Failed to init CinePlayer: %08x", retVal);
        goto err_ret;
    }
    retVal = mUltrasoundEncoder.init(appPath);
    if (IS_THOR_ERROR(retVal)) {
        LOGE("Failed to init UltrasoundEncoder: %08x", retVal);
        goto err_ret;
    }
    // init AutoControlManager
    retVal = mAutoControlManager.init();
    if (IS_THOR_ERROR(retVal)) {
        ALOGE("Failed to init AutoControlManager: %08x", retVal);
        goto err_ret;
    }
    retVal = mAIManager.init(javaEnv, javaContext);
    if (IS_THOR_ERROR(retVal)) {
        LOGE("Failed to init AIManager: %08x", retVal);
        goto err_ret;
    }
    retVal = mCineViewer.getImguiRenderer().load(&mFilesystem);
    if (IS_THOR_ERROR(retVal)) {
        LOGE("Failed to load ImGuiRenderer: %08x", retVal);
        goto err_ret;
    }

    retVal = THOR_OK;

err_ret:
    return(retVal); 
}

//-----------------------------------------------------------------------------
ThorStatus UltrasoundManager::openDirect(const char *assetPath)
{
    ThorStatus      retVal = THOR_ERROR;
    const char      *sep;
    std::string     appPath;

    mCineBuffer.registerCallback(&mCineViewer);
    mCineBuffer.registerCallback(&mCineRecorder);

    mUpsReaderSPtr = std::make_shared<UpsReader>();

    mCinePlayer.init(mUpsReaderSPtr, assetPath);
    mUltrasoundEncoder.init(assetPath);
    mAutoControlManager.init();
    // Use the dir of upsPath as the appPath
    sep = strrchr(assetPath, '/');
    if (sep) {
        appPath = std::string(assetPath, sep);
    } else {
        // Didn't find a path separator, just use current directory as app path
        appPath = assetPath;
    }
    retVal = mFilesystem.init(mAssetManagerPtr, appPath.c_str());
    if (IS_THOR_ERROR(retVal)) {
        LOGE("Failed to open asset reader");
        goto err_ret;
    }

    retVal = THOR_OK;

err_ret:
    return(retVal); 
}


//-----------------------------------------------------------------------------
void UltrasoundManager::close()
{
    mCinePlayer.terminate();
    mCineBuffer.unregisterCallback(&mCineViewer);
    mCineBuffer.unregisterCallback(&mCineRecorder);
    mUltrasoundEncoder.terminate();
    mAutoControlManager.terminate();
    mAIManager.shutdown();

    if (nullptr != mUpsReaderSPtr)
    {
        mUpsReaderSPtr->close();
        mUpsReaderSPtr = nullptr;
    }
}

//-----------------------------------------------------------------------------
void UltrasoundManager::setLanguage(const char *lang)
{
    mAIManager.setLanguage(lang);
}

//-----------------------------------------------------------------------------
CineBuffer& UltrasoundManager::getCineBuffer()
{
    ALOGD("getCineBuffer - me: 0x%p,  my Cine Buffer: 0x%p",
          this, &mCineBuffer);
    return(mCineBuffer); 
}

//-----------------------------------------------------------------------------
CineViewer& UltrasoundManager::getCineViewer()
{
    return(mCineViewer); 
}

//-----------------------------------------------------------------------------
CinePlayer& UltrasoundManager::getCinePlayer()
{
    return(mCinePlayer); 
}

//-----------------------------------------------------------------------------
CineRecorder& UltrasoundManager::getCineRecorder()
{
    return(mCineRecorder); 
}

//-----------------------------------------------------------------------------
const std::shared_ptr<UpsReader>& UltrasoundManager::getUpsReader()
{
    return(mUpsReaderSPtr); 
}

//-----------------------------------------------------------------------------
UltrasoundEncoder &UltrasoundManager::getUltrasoundEncoder()
{
    return (mUltrasoundEncoder);
}

//-----------------------------------------------------------------------------
AutoControlManager &UltrasoundManager::getAutoControlManager()
{
    return (mAutoControlManager);
}

//-----------------------------------------------------------------------------
AIManager &UltrasoundManager::getAIManager()
{
    return (mAIManager);
}

//-----------------------------------------------------------------------------
std::string UltrasoundManager::getAppPath()
{
    return(mAppPath);
}

//-----------------------------------------------------------------------------
bool UltrasoundManager::extractUps(std::string& upsPath, std::string& upsDb)
{
    bool        retVal = false;
    struct stat statBuf;
    int         fd = -1;

    if (-1 == stat(upsPath.c_str(), &statBuf))
    {
        std::string     upsSource;

        // Extract the database from the assets and copy to filesystem
        upsSource = loadAssetString(upsDb.c_str());

        if (upsSource.empty())
        {
            ALOGE("Could not find UPS asset");
            goto err_ret;
        }

        fd = ::open(upsPath.c_str(), 
                    O_CREAT | O_WRONLY | O_TRUNC, S_IRWXU | S_IRWXG);
        if (-1 == fd)
        {
            ALOGE("Unable to create UPS database: errno = %d", errno);
            goto err_ret;
        }

        write(fd, upsSource.c_str(), upsSource.length());
        ALOGD("Extracted fresh UPS database: %s", upsPath.c_str());
        retVal = true;
    }
    else
    {
        // Database is already there.  Nothing to do
        ALOGD("Using existing UPS database");
        retVal = true;
    }

err_ret:
    if (-1 != fd)
    {
        ::close(fd);
    }

    return(retVal);
}

//-----------------------------------------------------------------------------
bool UltrasoundManager::extractFont(std::string& fontPath)
{
    bool        retVal = false;
    struct stat statBuf;
    int         fd = -1;

    if (-1 == stat(fontPath.c_str(), &statBuf))
    {
        std::string     fontSource;

        // Extract the front from the assets and copy to filesystem
        fontSource = loadAssetString(DEFAULT_FONT_FILE);

        fd = ::open(fontPath.c_str(),
                    O_CREAT | O_WRONLY | O_TRUNC, S_IRWXU | S_IRWXG);
        if (-1 == fd)
        {
            ALOGE("Unable to create Font file: errno = %d", errno);
            goto err_ret;
        }

        write(fd, fontSource.c_str(), fontSource.length());
        ALOGD("Font file has been copied");
        retVal = true;
    }
    else
    {
        // Font file is already there.  Nothing to do
        ALOGD("Using existing font file");
        retVal = true;
    }

    err_ret:
    if (-1 != fd)
    {
        ::close(fd);
    }

    return(retVal);
}

//-----------------------------------------------------------------------------
bool UltrasoundManager::extractFwUpgradeBinary(std::string &binaryPath) {
    bool retVal = false;
    struct stat statBuf;
    int fd = -1;

    stat(binaryPath.c_str(), &statBuf);
    std::string binSource;

    // Extract the binary from the assets and copy to filesystem
    binSource = loadAssetString(PciDauFwUpgrade::DEFAULT_BIN_FILE);

    if (binSource.empty())
        goto err_ret;

    fd = ::open(binaryPath.c_str(),
                O_CREAT | O_WRONLY | O_TRUNC, S_IRWXU | S_IRWXG);
    if (-1 == fd) {
        ALOGE("Unable to create binary file: errno = %d", errno);
        goto err_ret;
    }

    write(fd, binSource.c_str(), binSource.length());
    ALOGD("Extracted App fresh binary file");
    retVal = true;

    err_ret:
    if (-1 != fd) {
        ::close(fd);
    }
    return (retVal);
}

//-----------------------------------------------------------------------------
bool UltrasoundManager::extractBLFwUpgradeBinary(std::string &binaryPath) {
    bool retVal = false;
    struct stat statBuf;
    int fd = -1;

    stat(binaryPath.c_str(), &statBuf);
    std::string binSource;

    // Extract the binary from the assets and copy to filesystem
    binSource = loadAssetString(PciDauFwUpgrade::DEFAULT_BL_BIN_FILE);

    if (binSource.empty())
        goto err_ret;

    fd = ::open(binaryPath.c_str(),
                O_CREAT | O_WRONLY | O_TRUNC, S_IRWXU | S_IRWXG);
    if (-1 == fd) {
        ALOGE("Unable to create binary file: errno = %d", errno);
        goto err_ret;
    }

    write(fd, binSource.c_str(), binSource.length());
    ALOGD("Extracted BLE fresh binary file");
    retVal = true;

    err_ret:
    if (-1 != fd) {
        ::close(fd);
    }
    return (retVal);
}

//-----------------------------------------------------------------------------
std::string UltrasoundManager::loadAssetString(const char* sourceFile)
{
    std::string     assetString;
    AAsset*         assetPtr = nullptr;
    void*           assetBufferPtr = nullptr;
    off_t           assetLength;

    if (nullptr == mAssetManagerPtr)
    {
        ALOGE("The asset manager is null");
        goto err_ret;
    }

    assetPtr = AAssetManager_open(mAssetManagerPtr, sourceFile, AASSET_MODE_BUFFER);
    if (nullptr == assetPtr)
    {
        ALOGE("Unable to open asset file: %s", sourceFile);
        goto err_ret;
    }

    assetBufferPtr = (void*) AAsset_getBuffer(assetPtr);
    if (nullptr == assetBufferPtr)
    {
        ALOGE("Unable to read asset file: %s", sourceFile);
        goto err_ret;
    }

    assetLength = AAsset_getLength(assetPtr);
    assetString = std::string((const char*) assetBufferPtr,
                              (size_t) assetLength);
err_ret:
    if (nullptr != assetPtr)
    {
        AAsset_close(assetPtr);
    }
    return(assetString);
}

//-----------------------------------------------------------------------------
Filesystem* UltrasoundManager::getFilesystem()
{
    return &mFilesystem;
}

//-----------------------------------------------------------------------------
bool UltrasoundManager::isFwUpdateAvailablePCIe() {
    FILE*               appFp;
    FILE*               blFp;
    ImageHeader         appfileIh,blfileIh;
    DauPower            dauPower;
    bool                retVal = false;
    std::string         appBinPath,blBinPath;
    std::string         defaultPath = mAppPath;
    std::string         fileVersion;
    std::string         probeVersion;
    int                 verStatus;

    blBinPath = defaultPath + "/" + PciDauFwUpgrade::DEFAULT_BL_BIN_FILE;
    appBinPath = defaultPath + "/" + PciDauFwUpgrade::DEFAULT_BIN_FILE;

    blFp = fopen(blBinPath.c_str(), "rb");
    if (blFp != NULL) {
        ALOGI("File %s opened successfully",PciDauFwUpgrade::DEFAULT_BL_BIN_FILE);
    } else {
        ALOGE("Failed to open File %s", blBinPath.c_str());
        goto  err_ret;
    }

    appFp = fopen(appBinPath.c_str(), "rb");
    if (appFp != NULL) {
        ALOGI("File %s opened successfully",PciDauFwUpgrade::DEFAULT_BIN_FILE);
    } else {
        ALOGE("Failed to open File %s", appBinPath.c_str());
        goto  err_ret;
    }

    fread(&blfileIh, sizeof(ImageHeader), 1, blFp);
    ALOGD("Bootloader Firmware Info from File %d.%d.%d", blfileIh.appVersion.majorV, blfileIh.appVersion.minorV,
          blfileIh.appVersion.patchV);

    fread(&appfileIh, sizeof(ImageHeader), 1, appFp);
    ALOGD("Application Firmware Info from File %d.%d.%d", appfileIh.appVersion.majorV, appfileIh.appVersion.minorV,
          appfileIh.appVersion.patchV);

    fclose(blFp);
    fclose(appFp);

    if (!dauPower.powerOn())
    {
        if (dauPower.getPowerState() == DauPowerState::Reboot)
        {
            ALOGE("Unable to power the Dau on, a reboot is required.");
            goto err_ret;
        }
        else
        {
            ALOGE("Unable to power the Dau on");
            goto err_ret;
        }
    }
    usleep(1000*1000);

    if (!mPciFwUpgrade.init(&mDauContext)) {
        ALOGE("Unable to initialize PciDauFwUpgrade");
        goto err_ret;
    }

    if (!mPciFwUpgrade.getFwInfo(&mFwInfo)) {
        ALOGE("Unable to read probe firmware information");
        goto err_ret;
    }
    ALOGD("Bootloader Firmware Info %d.%d.%d", mFwInfo.bootLoaderVersion.majorV,
          mFwInfo.bootLoaderVersion.minorV, mFwInfo.bootLoaderVersion.patchV);

    fileVersion = getVersionStr(blfileIh.appVersion.majorV,
                                blfileIh.appVersion.minorV,
                                blfileIh.appVersion.patchV);

    probeVersion = getVersionStr(mFwInfo.bootLoaderVersion.majorV,
                                 mFwInfo.bootLoaderVersion.minorV,
                                 mFwInfo.bootLoaderVersion.patchV);

    verStatus = versionCompare(fileVersion,probeVersion);

    if(verStatus > 0)
    {
        ALOGI("Bootloader Firmware Update is available");
        mBootLoaderFwUpdate = true;
    }
    else
    {
        ALOGI("Bootloader Firmware is up-to-date");
        mBootLoaderFwUpdate = false;
    }

    if (mFwInfo.bootUpdateFlag1) {
        ALOGD("Probe Firmware1 Info %d.%d.%d", mFwInfo.app1Version.majorV,
              mFwInfo.app1Version.minorV, mFwInfo.app1Version.patchV);

        fileVersion = getVersionStr(appfileIh.appVersion.majorV,
                                    appfileIh.appVersion.minorV,
                                    appfileIh.appVersion.patchV);

        probeVersion = getVersionStr(mFwInfo.app1Version.majorV,
                                     mFwInfo.app1Version.minorV,
                                     mFwInfo.app1Version.patchV);

        verStatus = versionCompare(fileVersion,probeVersion);
        if(verStatus > 0)
        {
            ALOGI("App Firmware2 Update is available");
            mApplicationFwUpdate = true;
        }
        else
        {
            ALOGI("App Firmware1 is up-to-date");
            mApplicationFwUpdate = false;
        }
    } else if (mFwInfo.bootUpdateFlag2) {
        ALOGD("Probe Firmware2 Info %d.%d.%d", mFwInfo.app2Version.majorV,
              mFwInfo.app2Version.minorV, mFwInfo.app2Version.patchV);

        fileVersion = getVersionStr(appfileIh.appVersion.majorV,
                                    appfileIh.appVersion.minorV,
                                    appfileIh.appVersion.patchV);

        probeVersion = getVersionStr(mFwInfo.app2Version.majorV,
                                     mFwInfo.app2Version.minorV,
                                     mFwInfo.app2Version.patchV);

        verStatus = versionCompare(fileVersion,probeVersion);
        if(verStatus > 0)
        {
            ALOGI("App Firmware1 Update is available");
            mApplicationFwUpdate = true;
        }
        else
        {
            ALOGI("App Firmware2 is up-to-date");
            mApplicationFwUpdate = false;
        }
    } else {
        ALOGE("Failed to Read FwInfo Partition");
        goto err_ret;
    }

    if(mBootLoaderFwUpdate || mApplicationFwUpdate)
    {
        ALOGI("Update is available");
        retVal = true;
    }

    ALOGI("Update Val: %d",retVal);
    err_ret:
    if(!mPciFwUpgrade.deinit())
        ALOGE("Failed to deinit PciDauFwUpgrade");

    if(dauPower.getPowerState() == DauPowerState::On)
        dauPower.powerOff();

    return retVal;
}

//-----------------------------------------------------------------------------
bool UltrasoundManager::applicationFwUpdatePCIe() {
    FILE*               fp;
    ImageHeader*        ih = NULL;
    int                 fileSize = 0;
    unsigned char*      blFileBuf = NULL;
    unsigned char*      apFileBuf = NULL;
    uint32_t            flashAddress, crcLength, crcCheck;
    DauPower            dauPower;
    bool                retVal = false;
    std::string         appBinPath,blBinPath;
    std::string         defaultPath = mAppPath;

    blBinPath = defaultPath + "/" + PciDauFwUpgrade::DEFAULT_BL_BIN_FILE;
    appBinPath = defaultPath + "/" + PciDauFwUpgrade::DEFAULT_BIN_FILE;

    if(!mBootLoaderFwUpdate && !mApplicationFwUpdate)
    {
        ALOGI("Firmware are already up-to-date");
        goto err_ret;
    }

    if (!dauPower.powerOn())
    {
        if (dauPower.getPowerState() == DauPowerState::Reboot)
        {
            ALOGE("Unable to power the Dau on, a reboot is required.");
            goto err_ret;
        }
        else
        {
            ALOGE("Unable to power the Dau on");
            goto err_ret;
        }
    }
    usleep(1000*1000);

    if (!mPciFwUpgrade.init(&mDauContext)) {
        ALOGE("Unable to initialize PciDauFwUpgrade");
        goto err_ret;
    }

    if(mApplicationFwUpdate) {

        if (mFwInfo.bootUpdateFlag1 != mFwInfo.bootUpdateFlag2) {
            fp = fopen(appBinPath.c_str(), "rb");

            if (fp != NULL) {
                ALOGI("File %s opened successfully",PciDauFwUpgrade::DEFAULT_BIN_FILE);
            } else {
                ALOGE("Failed to open File %s",appBinPath.c_str());
                goto err_ret;
            }

            fseek(fp, APPLICATION_2_IMAGE_SIZE + sizeof(ImageHeader), SEEK_SET);
            fileSize = ftell(fp);
            apFileBuf = (unsigned char *) malloc(fileSize);
            if (apFileBuf == NULL)
            {
                ALOGE("App malloc failed");
                goto err_ret;
            }
            ALOGI("Application File Size: %d", fileSize);

            fseek(fp, 0L, SEEK_SET);
            fread(apFileBuf, fileSize, 1, (FILE *) fp);

            if (mFwInfo.bootUpdateFlag1 == INEXECUTION) {
                flashAddress = APPLICATION_2_IMAGE_START;
                ALOGI("Upgrade Application 2");
            } else if (mFwInfo.bootUpdateFlag2 == INEXECUTION) {
                flashAddress = APPLICATION_1_IMAGE_START;
                ALOGI("Upgrade Application 1");
            }
        } else {
            ALOGE("Probe Firmware maybe corrupted");
            goto err_ret;
        }
        fclose(fp);

        ih = (ImageHeader *) apFileBuf;

        ALOGI("Application MajV:%d MinV:%d PatchV:%d", ih->appVersion.majorV, ih->appVersion.minorV,
              ih->appVersion.patchV);
        ALOGI("Application Img Len:%d", ih->imageLength);

        crcLength = ih->imageLength + (sizeof(ImageHeader) - sizeof(uint32_t));
        crcCheck = mPciFwUpgrade.computeCRC(apFileBuf + sizeof(uint32_t), crcLength);
        ALOGI("Received CRC:0x%x", ih->CRC);
        ALOGI("Calculated CRC:0x%x", crcCheck);

        if (ih->CRC == crcCheck) {
            ALOGD("App CRC OK");
        } else {
            ALOGE("App CRC Fail");
            goto err_ret;
        }

        if (!mPciFwUpgrade.uploadImage((uint32_t *) apFileBuf, flashAddress, fileSize)) {
            ALOGE("Error in Upload image");
            goto err_ret;
        }

        ALOGD("Application Firmware Updated Successfully");
        ih = NULL;
        if(!mPciFwUpgrade.deinit())
            ALOGE("Failed to deinit PciDauFwUpgrade");

        if(dauPower.getPowerState() == DauPowerState::On)
            dauPower.powerOff();

        if(mBootLoaderFwUpdate) {
            usleep(2000 * 1000);
        }
    }

    if(mBootLoaderFwUpdate)
    {
        if (!dauPower.powerOn())
        {
            if (dauPower.getPowerState() == DauPowerState::Reboot)
            {
                ALOGE("Unable to power the Dau on, a reboot is required.");
                goto err_ret;
            }
            else
            {
                ALOGE("Unable to power the Dau on");
                goto err_ret;
            }
        }
        usleep(1000*1000);

        if (!mPciFwUpgrade.init(&mDauContext)) {
            ALOGE("Unable to initialize PciDauFwUpgrade");
            goto err_ret;
        }

        fp = fopen(blBinPath.c_str(), "rb");

        if (fp != NULL) {
            ALOGI("File %s opened successfully",PciDauFwUpgrade::DEFAULT_BL_BIN_FILE);
        } else {
            ALOGE("Failed to open File %s", blBinPath.c_str());
            goto  err_ret;
        }

        fseek(fp, EBL_IMAGE_SIZE + sizeof(ImageHeader), SEEK_SET);
        fileSize = ftell(fp);
        ALOGD("Bootloader File Size: %d\n",fileSize);

        blFileBuf = (unsigned char *) malloc(fileSize);
        if (blFileBuf == NULL)
        {
            ALOGE("BL malloc failed");
            goto err_ret;
        }
        fseek(fp, 0L, SEEK_SET);
        fread(blFileBuf, fileSize, 1, (FILE *) fp);

        ALOGI("Upgrade BootLoader\n");
        fclose(fp);

        ih = (ImageHeader *) blFileBuf;
        ALOGD("Bootloader Img Len:%d\n", ih->imageLength);

        crcLength = ih->imageLength + (sizeof(ImageHeader)- sizeof(uint32_t));
        crcCheck = mPciFwUpgrade.computeCRC(blFileBuf + sizeof(uint32_t), crcLength);
        ALOGI("BL Received CRC:0x%x\n", ih->CRC);
        ALOGI("BL Calculated CRC:0x%x\n", crcCheck);

        if (ih->CRC == crcCheck)
        {
            ALOGI("BL CRC OK\n");
        }
        else
        {
            ALOGE("BL CRC Fail\n");
            goto err_ret;
        }

        flashAddress = EBL_IMAGE_START;

        if(!mPciFwUpgrade.uploadImage((uint32_t *)blFileBuf, flashAddress, fileSize))
        {
            ALOGE("Error in Upload image\n");
            goto err_ret;
        }
        ALOGD("Bootloader Firmware Updated Successfully");
    }

    err_ret:
    ALOGD("retVal %d", retVal);
    mBootLoaderFwUpdate = false;
    mApplicationFwUpdate = false;

    if(!mPciFwUpgrade.deinit())
        ALOGE("Failed to de-init PciDauFwUpgrade");

    if(dauPower.getPowerState() == DauPowerState::On)
        dauPower.powerOff();

    if(blFileBuf != NULL)
    {
        ALOGD("Free blFileBuf");
        free(blFileBuf);
    }

    if(apFileBuf != NULL)
    {
        ALOGD("Free apFileBuf");
        free(apFileBuf);
    }

    return retVal;
}

//-----------------------------------------------------------------------------
const char* UltrasoundManager::getDbName(uint32_t probeType)
{
    return(mUpsReaderSPtr->getDbName(probeType));
}


//-----------------------------------------------------------------------------
void UltrasoundManager::readProbeInfoUsb(int fd,ProbeInfo *probeData)
{
    FactoryData         fData;
    std::string         majV;
    std::string         minV;
    std::string         patV;
    std::string         appVersion;
    mDauContext.usbFd = fd;
    mDauContext.isUsbContext = true;
    ALOGD("USB probe firmware information");

    probeData->probeType = 0x00000000;

    if (!mUsbFwUpgrade.init(&mDauContext))
    {
        ALOGE("Unable to intialize UsbDauFwUpgrade");
        goto err_ret;
    }
    if (!mUsbFwUpgrade.getFwInfo(&mFwInfo)) {
        ALOGE("Unable to read probe firmware information");
        mUsbFwUpgrade.deInit();
        goto err_ret;
    }
    if (!mUsbFwUpgrade.readFactData((uint32_t *)&fData,FACTORY_DATA_START, sizeof(FactoryData)))
    {
        ALOGE("Unable to read factory data");
        goto err_ret;
    }
    probeData->probeType = fData.probeType;
    if(fData.probeType != -1)
    {
        probeData->serialNo = fData.serialNum;
        probeData->part = fData.partNum;
    }
    else
    {
        ALOGE("probeType : 0x%x\n\r", fData.probeType);
        ALOGE("serialNum : %s\n\r", fData.serialNum);
        ALOGE("partNum : %s\n\r", fData.partNum);
        probeData->serialNo = "00000000";
        probeData->part = "00000000";
    }
    if(mFwInfo.bootUpdateFlag1 == 1)
    {
        ALOGD("Application1 v%d.%d.%d\n\r", mFwInfo.app1Version.majorV,mFwInfo.app1Version.minorV,mFwInfo.app1Version.patchV);
        ALOGD("Boot Flag 1 : 0x%x\n\r", mFwInfo.bootUpdateFlag1);
        majV = std::to_string(mFwInfo.app1Version.majorV);
        minV = std::to_string(mFwInfo.app1Version.minorV);
        patV = std::to_string(mFwInfo.app1Version.patchV);
        appVersion.append(majV);
        appVersion.append(1u,'.');
        appVersion.append(minV);
        appVersion.append(1u,'.');
        appVersion.append(patV);
    }
    else if(mFwInfo.bootUpdateFlag2 == 1)
    {
        ALOGD("Application2 v%d.%d.%d\n\r", mFwInfo.app2Version.majorV,mFwInfo.app2Version.minorV,mFwInfo.app2Version.patchV);
        ALOGD("Boot Flag 2 : 0x%x\n\r", mFwInfo.bootUpdateFlag2);
        majV = std::to_string(mFwInfo.app2Version.majorV);
        minV = std::to_string(mFwInfo.app2Version.minorV);
        patV = std::to_string(mFwInfo.app2Version.patchV);
        appVersion.append(majV);
        appVersion.append(1u,'.');
        appVersion.append(minV);
        appVersion.append(1u,'.');
        appVersion.append(patV);
    }
    else
    {
        ALOGE("Boot Flag 1 : 0x%x\n\r", mFwInfo.bootUpdateFlag1);
        ALOGE("Boot Flag 2 : 0x%x\n\r", mFwInfo.bootUpdateFlag2);
        appVersion = "0.0.0";
    }
    probeData->fwVersion = appVersion;
    ALOGD("Probe Type : 0x%X\n\r", probeData->probeType);
    ALOGD("Serial Number : %s\n\r", probeData->serialNo.c_str());
    ALOGD("part : %s\n\r", probeData->part.c_str());
    ALOGD("Firmware version : %s\n\r", probeData->fwVersion.c_str());
    err_ret:
    mUsbFwUpgrade.deInit();
}

int UltrasoundManager::versionCompare(std::string v1, std::string v2)
{
    // vnum stores each numeric
    // part of version
    int vnum1 = 0, vnum2 = 0;

    // loop until both string are
    // processed
    for (int i = 0, j = 0; (i < v1.length()
                            || j < v2.length());) {
        // storing numeric part of
        // version 1 in vnum1
        while (i < v1.length() && v1[i] != '.') {
            vnum1 = vnum1 * 10 + (v1[i] - '0');
            i++;
        }

        // storing numeric part of
        // version 2 in vnum2
        while (j < v2.length() && v2[j] != '.') {
            vnum2 = vnum2 * 10 + (v2[j] - '0');
            j++;
        }

        if (vnum1 > vnum2)
            return 1;
        if (vnum2 > vnum1)
            return -1;

        // if equal, reset variables and
        // go for next numeric part
        vnum1 = vnum2 = 0;
        i++;
        j++;
    }
    return 0;
}

std::string UltrasoundManager::getVersionStr(int maj, int min, int patch)
{
    std::string         majV;
    std::string         minV;
    std::string         patV;
    std::string         appVersion;

    majV = std::to_string(maj);
    minV = std::to_string(min);
    patV = std::to_string(patch);

    appVersion.append(majV);
    appVersion.append(1u,'.');
    appVersion.append(minV);
    appVersion.append(1u,'.');
    appVersion.append(patV);

    return appVersion;
}

//-----------------------------------------------------------------------------
bool UltrasoundManager::isFwUpdateAvailableUsb(int fd, bool isAppFd) {
    FILE*               appFp;
    FILE*               blFp;
    ImageHeader         appfileIh,blfileIh;
    std::string         appBinPath, blBinPath;
    std::string         fileVersion;
    std::string         probeVersion;
    bool                retVal = false;
    int                 verStatus;

    mDauContext.usbFd = fd;
    mDauContext.isUsbContext = true;
    std::string defaultPath = mAppPath;

    blBinPath = defaultPath + "/" + PciDauFwUpgrade::DEFAULT_BL_BIN_FILE;
    appBinPath = defaultPath + "/" + PciDauFwUpgrade::DEFAULT_BIN_FILE;

    blFp = fopen(blBinPath.c_str(), "rb");
    if (blFp != NULL) {
        ALOGI("File %s opened successfully",PciDauFwUpgrade::DEFAULT_BL_BIN_FILE);
    } else {
        ALOGE("Failed to open File %s", blBinPath.c_str());
        goto  err_ret;
    }

    appFp = fopen(appBinPath.c_str(), "rb");
    if (appFp != NULL) {
        ALOGI("File %s opened successfully",PciDauFwUpgrade::DEFAULT_BIN_FILE);
    } else {
        ALOGE("Failed to open File %s", appBinPath.c_str());
        goto  err_ret;
    }

    fread(&blfileIh, sizeof(ImageHeader), 1, blFp);
    ALOGD("Bootloader Firmware Info from File %d.%d.%d", blfileIh.appVersion.majorV, blfileIh.appVersion.minorV,
          blfileIh.appVersion.patchV);

    fread(&appfileIh, sizeof(ImageHeader), 1, appFp);
    ALOGD("Application Firmware Info from File %d.%d.%d", appfileIh.appVersion.majorV, appfileIh.appVersion.minorV,
          appfileIh.appVersion.patchV);

    fclose(blFp);
    fclose(appFp);

    if (!mUsbFwUpgrade.init(&mDauContext)) {
        ALOGE("Unable to initialize UsbDauFwUpgrade");
        mUsbFwUpgrade.deInit();
        goto err_ret;
    }

    if (!mUsbFwUpgrade.getFwInfo(&mFwInfo)) {
        ALOGE("Unable to read probe firmware information");
        mUsbFwUpgrade.deInit();
        goto err_ret;
    }

    ALOGD("Bootloader Firmware Info %d.%d.%d", mFwInfo.bootLoaderVersion.majorV,
          mFwInfo.bootLoaderVersion.minorV, mFwInfo.bootLoaderVersion.patchV);

    fileVersion = getVersionStr(blfileIh.appVersion.majorV,
                                blfileIh.appVersion.minorV,
                                blfileIh.appVersion.patchV);

    probeVersion = getVersionStr(mFwInfo.bootLoaderVersion.majorV,
                                 mFwInfo.bootLoaderVersion.minorV,
                                 mFwInfo.bootLoaderVersion.patchV);

    verStatus = versionCompare(fileVersion,probeVersion);

    if(verStatus > 0)
    {
        ALOGI("Bootloader Firmware Update is available");
        mBootLoaderFwUpdateUsb = true;
    }
    else
    {
        ALOGI("Bootloader Firmware is up-to-date");
        mBootLoaderFwUpdateUsb = false;
    }

    if (mFwInfo.bootUpdateFlag1) {
        ALOGD("Probe Firmware1 Info %d.%d.%d", mFwInfo.app1Version.majorV,
              mFwInfo.app1Version.minorV, mFwInfo.app1Version.patchV);

        fileVersion = getVersionStr(appfileIh.appVersion.majorV,
                                    appfileIh.appVersion.minorV,
                                    appfileIh.appVersion.patchV);

        probeVersion = getVersionStr(mFwInfo.app1Version.majorV,
                                     mFwInfo.app1Version.minorV,
                                     mFwInfo.app1Version.patchV);

        verStatus = versionCompare(fileVersion,probeVersion);

        if(verStatus > 0)
        {
            ALOGI("App Firmware2 Update is available");
            if(isAppFd){
                mUsbFwUpgrade.deInit();
            }
            mApplicationFwUpdateUsb = true;
        }
        else
        {
            ALOGI("App Firmware1 is up-to-date");
            if(isAppFd){
                mUsbFwUpgrade.deInit();
            }
            mApplicationFwUpdateUsb = false;
        }
    } else if (mFwInfo.bootUpdateFlag2) {
        ALOGD("Probe Firmware2 Info %d.%d.%d", mFwInfo.app2Version.majorV,
              mFwInfo.app2Version.minorV, mFwInfo.app2Version.patchV);
        fileVersion = getVersionStr(appfileIh.appVersion.majorV,
                                    appfileIh.appVersion.minorV,
                                    appfileIh.appVersion.patchV);

        probeVersion = getVersionStr(mFwInfo.app2Version.majorV,
                                     mFwInfo.app2Version.minorV,
                                     mFwInfo.app2Version.patchV);

        verStatus = versionCompare(fileVersion,probeVersion);

        if(verStatus > 0)
        {
            ALOGI("App Firmware1 Update is available");
            if(isAppFd){
                mUsbFwUpgrade.deInit();
            }
            mApplicationFwUpdateUsb = true;
        }
        else
        {
            ALOGI("App Firmware2 is up-to-date");
            if(isAppFd){
                mUsbFwUpgrade.deInit();
            }
            mApplicationFwUpdateUsb = false;
        }
    } else {
        ALOGE("Failed to Read FwInfo Partition");
        mUsbFwUpgrade.deInit();
        goto err_ret;
    }

    if(mBootLoaderFwUpdateUsb || mApplicationFwUpdateUsb)
    {
        ALOGI("Update is available");
        retVal = true;
    }

    err_ret:
    mUsbFwUpgrade.deInit();
    return retVal;
}

//-----------------------------------------------------------------------------
bool UltrasoundManager::applicationFwUpdateUsb() {
    FILE*               fp;
    ImageHeader*        ih = nullptr;
    int                 fileSize = 0;
    uint32_t            flashAddress, crcLength, crcCheck;
    bool                retVal = false;
    std::string         appBinPath,blBinPath;
    std::string         defaultPath = mAppPath;
    unsigned char*      blFileBuf = NULL;
    unsigned char*      apFileBuf = NULL;

    blBinPath = defaultPath + "/" + PciDauFwUpgrade::DEFAULT_BL_BIN_FILE;
    appBinPath = defaultPath + "/" + PciDauFwUpgrade::DEFAULT_BIN_FILE;

    if(!mBootLoaderFwUpdateUsb && !mApplicationFwUpdateUsb)
    {
        ALOGI("Firmware are already up-to-date");
        goto err_ret;
    }

    if (!mUsbFwUpgrade.init(&mDauContext)) {
        ALOGE("Unable to intialize UsbDauFwUpgrade");
        mUsbFwUpgrade.deInit();
        goto err_ret;
    }

    if(mApplicationFwUpdateUsb) {
        if (mFwInfo.bootUpdateFlag1 != mFwInfo.bootUpdateFlag2) {
            fp = fopen(appBinPath.c_str(), "rb");

            if (fp != NULL) {
                ALOGI("File opened successfully");
            } else {
                ALOGE("Failed to open File");
                goto err_ret;
            }

            fseek(fp, APPLICATION_2_IMAGE_SIZE + sizeof(ImageHeader), SEEK_SET);
            fileSize = ftell(fp);
            apFileBuf = (unsigned char *) malloc(fileSize);

            ALOGI("File Size: %d", fileSize);

            fseek(fp, 0L, SEEK_SET);
            fread(apFileBuf, fileSize, 1, (FILE *) fp);

            if (mFwInfo.bootUpdateFlag1 == INEXECUTION) {
                flashAddress = APPLICATION_2_IMAGE_START;
                ALOGI("Upgrade Application 2");
            } else if (mFwInfo.bootUpdateFlag2 == INEXECUTION) {
                flashAddress = APPLICATION_1_IMAGE_START;
                ALOGI("Upgrade Application 1");
            }
        } else {
            ALOGE("Probe Firmware maybe corrupted");
            goto err_ret;
        }

        fclose(fp);

        ih = (ImageHeader *) apFileBuf;

        ALOGI("MajV:%d MinV:%d PatchV:%d", ih->appVersion.majorV, ih->appVersion.minorV,
              ih->appVersion.patchV);
        ALOGI("Img Len:%d", ih->imageLength);

        uint32_t crcLength = ih->imageLength + (sizeof(ImageHeader) - sizeof(uint32_t));
        uint32_t crcCheck = mUsbFwUpgrade.computeCRC(apFileBuf + sizeof(uint32_t), crcLength);
        ALOGI("Received CRC:0x%x", ih->CRC);
        ALOGI("Calculated CRC:0x%x", crcCheck);

        if (ih->CRC == crcCheck) {
            ALOGD("CRC OK");
        } else {
            ALOGE("CRC Fail");
            goto err_ret;
        }

        if (mDauContext.isUsbContext) {
            if ((mFwInfo.bootLoaderVersion.majorV <= 1) &&
                (mFwInfo.bootLoaderVersion.minorV == 0) &&
                (mFwInfo.bootLoaderVersion.patchV <= 5)) {
                ALOGI("Old Firmware");
                crcLength = ih->imageLength + (sizeof(ImageHeader) - (sizeof(uint32_t) *
                                                                      2));  // Remove ih.imgTyp for older version
                crcCheck = mUsbFwUpgrade.computeCRC(apFileBuf + (sizeof(uint32_t) * 2), crcLength);
                ALOGI("Calculated CRC:0x%x", crcCheck);
                ih->CRC = crcCheck;
                ih->imgTyp = crcCheck;
                apFileBuf += sizeof(ih->CRC);
            }
            flashAddress = SE_FB_MEM_START_ADDR;
        }

        if (!mUsbFwUpgrade.uploadImage((uint32_t *) apFileBuf, flashAddress, fileSize)) {
            ALOGE("Error in Upload image");
            goto err_ret;
        }
        ALOGD("Application Firmware Updated Successfully");
        ih = NULL;
        if(mBootLoaderFwUpdateUsb)
            usleep(10000);

    }

    if(mBootLoaderFwUpdateUsb) {
        fp = fopen(blBinPath.c_str(), "rb");

        if (fp != NULL) {
            ALOGI("File %s opened successfully",PciDauFwUpgrade::DEFAULT_BL_BIN_FILE);
        } else {
            ALOGE("Failed to open File %s", blBinPath.c_str());
            goto  err_ret;
        }

        fseek(fp, EBL_IMAGE_SIZE + sizeof(ImageHeader), SEEK_SET);
        fileSize = ftell(fp);
        ALOGD("Bootloader File Size: %d\n",fileSize);

        blFileBuf = (unsigned char *) malloc(fileSize);
        if (blFileBuf == NULL)
        {
            ALOGE("BL malloc failed");
            goto err_ret;
        }
        fseek(fp, 0L, SEEK_SET);
        fread(blFileBuf, fileSize, 1, (FILE *) fp);

        ALOGI("Upgrade BootLoader\n");
        fclose(fp);

        ih = (ImageHeader *) blFileBuf;
        ALOGD("Bootloader Img Len:%d\n", ih->imageLength);

        crcLength = ih->imageLength + (sizeof(ImageHeader)- sizeof(uint32_t));
        crcCheck = mUsbFwUpgrade.computeCRC(blFileBuf + sizeof(uint32_t), crcLength);
        ALOGI("BL Received CRC:0x%x\n", ih->CRC);
        ALOGI("BL Calculated CRC:0x%x\n", crcCheck);

        if (ih->CRC == crcCheck)
        {
            ALOGI("BL CRC OK\n");
        }
        else
        {
            ALOGE("BL CRC Fail\n");
            goto err_ret;
        }

        if ((mFwInfo.bootLoaderVersion.majorV <= 1) &&
            (mFwInfo.bootLoaderVersion.minorV == 0) &&
            (mFwInfo.bootLoaderVersion.patchV <= 5)) {
            ALOGI("Bootloader Upgrade Old Firmware");
            crcLength = ih->imageLength + (sizeof(ImageHeader) - (sizeof(uint32_t) *
                                                                  2));  // Remove ih.imgTyp for older version
            crcCheck = mUsbFwUpgrade.computeCRC(apFileBuf + (sizeof(uint32_t) * 2), crcLength);
            ALOGI("Calculated CRC:0x%x", crcCheck);
            ih->CRC = crcCheck;
            ih->imgTyp = crcCheck;
            apFileBuf += sizeof(ih->CRC);
        }

        flashAddress = SE_FB_MEM_START_ADDR;

        if(!mUsbFwUpgrade.uploadImage((uint32_t *)blFileBuf, flashAddress, fileSize))
        {
            ALOGE("Error in Upload image\n");
            goto err_ret;
        }
        ALOGD("Bootloader Firmware Updated Successfully");

        ih = NULL;
    }

    retVal = true;
    mUsbFwUpgrade.rebootDevice();
    usleep(500000);
    err_ret:
    mBootLoaderFwUpdateUsb = false;
    mApplicationFwUpdateUsb = false;
    if(apFileBuf)
        free(apFileBuf);
    if(blFileBuf)
        free(blFileBuf);
    mUsbFwUpgrade.deInit();
    return retVal;
}

//-----------------------------------------------------------------------------
int UltrasoundManager::bootApplicationImage(int fd) {
    mUsbFwUpgrade.deInit();

    libusb_device_handle *usbHandlePtr = NULL;
    int rc = THOR_OK;
    UsbTlv tlv;
    const unsigned int USB_TIMEOUT = 100;   // 100 ms, should be plenty time

    ALOGD("File Descriptor Value:%d", fd);

    rc = libusb_init(nullptr);
    if (rc < 0) {
        ALOGE("libusb_init failed: %s", libusb_error_name(rc));
        rc = THOR_ERROR;
        goto err_ret;
    }

    // Open USB (Dau) device
    if (0 != libusb_wrap_fd(nullptr, fd, &usbHandlePtr)) {
        ALOGE("Unable to wrap libusb fd");
        rc = THOR_ERROR;
        goto err_ret;
    }

    libusb_claim_interface(usbHandlePtr, 0);

    tlv.tag = TLV_TAG_BOOT_APPLICATION_IMAGE;
    tlv.length = 0;
    tlv.values[0] = 0;

    libusb_control_transfer(usbHandlePtr,
                            0x40,
                            0x0,
                            0,
                            0,
                            (unsigned char *) &tlv,
                            sizeof(tlv),
                            USB_TIMEOUT);

    libusb_release_interface(usbHandlePtr, 0);
    libusb_close(usbHandlePtr);
    libusb_exit(nullptr);
    return rc;
    err_ret:
    return (rc);
}

bool UltrasoundManager::setUsbConfig(int fd)
{
    mUsbFwUpgrade.deInit();

    libusb_device_handle *usbHandlePtr = NULL;
    int rc = THOR_OK;

    ALOGD("setUsbConfig:: File Descriptor Value:%d", fd);

    rc = libusb_init(nullptr);
    if (rc < 0) {
        ALOGE("libusb_init failed: %s", libusb_error_name(rc));
        rc = THOR_ERROR;
        goto err_ret;
    }

    // Open USB (Dau) device
    if (0 != libusb_wrap_fd(nullptr, fd, &usbHandlePtr)) {
        ALOGE("Unable to wrap libusb fd");
        rc = THOR_ERROR;
        goto err_ret;
    }

    libusb_set_configuration(usbHandlePtr,DISABLE_USB_CONFIGURATION);
    libusb_set_configuration(usbHandlePtr,SET_ANDROID_CONFIGURATION);

    libusb_release_interface(usbHandlePtr, 0);
    libusb_close(usbHandlePtr);
    libusb_exit(nullptr);
    return rc;
    err_ret:
    return (rc);
}

//-----------------------------------------------------------------------------
bool UltrasoundManager::setProbeInfo(ProbeInfo *probeData, int fd){
    FactoryData         fData;
    mDauContext.usbFd = fd;
    mDauContext.isUsbContext = true;
    uint32_t factDatlen, wrfactDatlen;
    char  wrFactData[ sizeof(FactoryDataKey) + sizeof(FactoryData) ];
    FactoryDataKey  fDataKey;
    bool status = false;

    ALOGD("USB probe firmware information");
    if (!mUsbFwUpgrade.init(&mDauContext))
    {
        ALOGE("Unable to initialize UsbDauFwUpgrade");
        goto err_ret;
    }

    if (!mUsbFwUpgrade.readFactData((uint32_t *)&fData,FACTORY_DATA_START, sizeof(FactoryData)))
    {
        ALOGE("Unable to read factory data");
        goto err_ret;
    }

    ALOGD("USB old probe type : %x", fData.probeType);
    fData.probeType = probeData->probeType;
    ALOGD("USB new probe type : %x", fData.probeType);

    wrfactDatlen = sizeof(FactoryData) + sizeof(FactoryDataKey);
    memset(wrFactData, 0, wrfactDatlen);   // Set  with 0

    fDataKey.key0 = FACTORY_DATA_KEY0;
    fDataKey.key1 = FACTORY_DATA_KEY1;

    memcpy(wrFactData, &fDataKey, sizeof(FactoryDataKey));
    memcpy(wrFactData + sizeof(FactoryDataKey), &fData, sizeof(FactoryData));
    if (!mUsbFwUpgrade.writeFactData((uint32_t *)&wrFactData,SE_FB_MEM_START_ADDR, wrfactDatlen))
    {
        ALOGE("Unable to read factory data");
        goto err_ret;
    }

    if(probeData->probeType == fData.probeType){
        status = true;
    } else{
        status = false;
    }

    mUsbFwUpgrade.rebootDevice();

    err_ret:
    mUsbFwUpgrade.deInit();

    return status;
}