#include "AutoPreset/AutoControlManager.h"
#include <thread>

class MimicAutoController {

public:
    void attachTestAutoControls(const AutoControlManager& controlManager, bool mimicGain, bool mimicDepth, bool mimicPreset);
    void killMimicAuto();

private:

    std::atomic<bool> stopAutoControl {false};
    std::atomic<bool> stopAutoPreset {false};

    pthread_t autoGainThreadId;
    pthread_t autoDepthThreadId;
    pthread_t autoPresetThreadId;

    AutoControlManager autoControlManager;

    static void *startGainUpdateThread(void *);
    static void *startDepthUpdateThread(void *);
    static void *startPresetUpdateThread(void *);

    void autoUpdateGain();
    void autoUpdateDepth();
    void autoPreset();
};