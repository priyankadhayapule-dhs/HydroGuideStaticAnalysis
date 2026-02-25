#include "include/MimicAutoController.h"
#include <AutoControlManager.h>
#include <iostream>
#include <chrono>
#include <random>
#include <thread>
#include <pthread.h>

void MimicAutoController::attachTestAutoControls(const AutoControlManager& controlManager, bool mimicGain, bool mimicDepth, bool mimicPreset) {

    this->autoControlManager = controlManager;

    MimicAutoController::stopAutoPreset = false;
    MimicAutoController::stopAutoControl = false;

    if (mimicGain) {
        int gainThreadRet = pthread_create(&autoGainThreadId, nullptr, startGainUpdateThread, this);
        ALOGD("Gain thread created: %d", gainThreadRet);
    }

    if (mimicDepth) {
        int depthThreadRet = pthread_create(&autoDepthThreadId, nullptr, startDepthUpdateThread, this);
        ALOGD("Depth thread created: %d", depthThreadRet);
    }

    if (mimicPreset) {
        int presetThreadRet = pthread_create(&autoPresetThreadId, nullptr, startPresetUpdateThread,this);
        ALOGD("Preset thread created: %d", presetThreadRet);
    }
}

void MimicAutoController::killMimicAuto() {
    MimicAutoController::stopAutoControl = true;
    MimicAutoController::stopAutoPreset = true;
    ALOGD("kill the thread stop auto value %u", stopAutoControl.load());
}

void *MimicAutoController::startGainUpdateThread(void *thisPtr) {
    ((MimicAutoController *) thisPtr)->autoUpdateGain();
    return (nullptr);
}

void *MimicAutoController::startDepthUpdateThread(void *thisPtr) {
    ((MimicAutoController *) thisPtr)->autoUpdateDepth();
    return (nullptr);
}

void *MimicAutoController::startPresetUpdateThread(void *thisPtr) {
    ((MimicAutoController *) thisPtr)->autoPreset();
    return (nullptr);
}

void MimicAutoController::autoUpdateGain() {
    std::random_device rand;
    std::mt19937 gen(rand());
    std::uniform_int_distribution<> dis(-20, 20);

    while (!MimicAutoController::stopAutoControl.load()) {
        int random_number = dis(gen);
        autoControlManager.setAutoGain(random_number);
        ALOGD("Random %d and stopAutoControl %u", random_number, stopAutoControl.load());
        std::this_thread::sleep_for(std::chrono::seconds(15));
    }
}

void MimicAutoController::autoUpdateDepth() {
    std::random_device rand;
    std::mt19937 gen(rand());
    std::uniform_int_distribution<> dis(-20, 20);

    while (!MimicAutoController::stopAutoControl.load()) {
        int random_number = dis(gen);
        autoControlManager.setAutoDepth(random_number);
        std::this_thread::sleep_for(std::chrono::seconds(15));
    }
}

void MimicAutoController::autoPreset() {
    std::random_device rand;
    std::mt19937 gen(rand());
    std::uniform_int_distribution<> dis(0, 2);

    while (!MimicAutoController::stopAutoPreset.load()) {
        std::this_thread::sleep_for(std::chrono::seconds(30));
        int random_number = dis(gen);
        ALOGD("Auto preset organ to select %d", random_number);
        autoControlManager.setAutoOrgan(random_number);
    }
}
