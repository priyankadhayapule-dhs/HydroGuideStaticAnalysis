//
// Copyright 2017 EchoNous Inc.
//
//

#pragma once

#include <android/asset_manager.h>
#include <ThorError.h>
#include <CineBuffer.h>
#include <CineViewer.h>
#include <CinePlayer.h>
#include <CineRecorder.h>
#include <UltrasoundEncoder.h>
#include <UpsReader.h>
#include <AIManager.h>
#include <libusb.h>
#include <UsbDauFwUpgrade.h>
#include <Filesystem.h>
#include <PciDauFwUpgrade.h>
#include <DauController.h>
#include <AutoPreset/AutoControlManager.h>
#define DEFAULT_FONT_FILE_NAME "HelveticaNeueCyr-Bold.ttf"
#define DISABLE_USB_CONFIGURATION   0
#define SET_ANDROID_CONFIGURATION   2

class UltrasoundManager
{
public: // Methods
    UltrasoundManager();
    ~UltrasoundManager();

    ThorStatus      open(void* javaEnv, void* javaContext, AAssetManager* assetManagerPtr, const char* appPath);
    
    // For debug/test purposes, open the UPS directly from the file system (no asset manager)
    ThorStatus      openDirect(const char* assetPath);
    void            close();

    void            setLanguage(const char *lang);

    CineBuffer&     getCineBuffer();
    CineViewer&     getCineViewer();
    CinePlayer&     getCinePlayer();
    CineRecorder&   getCineRecorder();

    int             bootApplicationImage(int fd);
    bool            isFwUpdateAvailableUsb(int fd, bool isAppFd);
    bool            applicationFwUpdateUsb();
    bool            setUsbConfig(int fd);

    bool            isFwUpdateAvailablePCIe();
    bool            applicationFwUpdatePCIe();
    int             versionCompare(std::string v1, std::string v2);
    std::string     getVersionStr(int maj, int min, int patch);

    UltrasoundEncoder& getUltrasoundEncoder();
    AutoControlManager& getAutoControlManager();

    AIManager&      getAIManager();
	
    const std::shared_ptr<UpsReader>&   getUpsReader();

    std::string     loadAssetString(const char* sourceFile);
    Filesystem*     getFilesystem();
    std::string     getAppPath();

    bool            setProbeInfo(ProbeInfo *probeData, int fd);
    void            readProbeInfoUsb(int fd,ProbeInfo *probeInfo);
    const char*     getDbName(uint32_t probeType);

private: // Methods
    bool            extractUps(std::string& upsPath, std::string& upsDb);
    bool            extractFont(std::string& fontPath);
    bool            extractFwUpgradeBinary(std::string& binaryPath);
    bool            extractBLFwUpgradeBinary(std::string &binaryPath);


private:
    const char* const DEFAULT_FONT_FILE = DEFAULT_FONT_FILE_NAME;

private: // Properties
    CineBuffer                  mCineBuffer;
    CineViewer                  mCineViewer;
    CinePlayer                  mCinePlayer;
    CineRecorder                mCineRecorder;
    AAssetManager*              mAssetManagerPtr;
    std::shared_ptr<UpsReader>  mUpsReaderSPtr;
    std::string                 mAppPath;

    UltrasoundEncoder           mUltrasoundEncoder;
    AutoControlManager          mAutoControlManager;
    AIManager                   mAIManager;
    Filesystem                  mFilesystem;
    DauContext                  mDauContext;
    PciDauFwUpgrade             mPciFwUpgrade;
    UsbDauFwUpgrade             mUsbFwUpgrade;
    FwInfo                      mFwInfo;
    bool                        mBootLoaderFwUpdate;
    bool                        mApplicationFwUpdate;
    bool                        mBootLoaderFwUpdateUsb;
    bool                        mApplicationFwUpdateUsb;
};
