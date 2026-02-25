#!/bin/bash
# Build all libs and tests
ndk-build
# Create dest directory if not already exists
adb shell "mkdir -p /data/local/tmp/thor/assets"
adb shell "mkdir -p /data/local/tmp/thor/results"
# Push libs/executables
adb push --sync libs/arm64-v8a /data/local/tmp/thor
# Push UPS db and test clips
adb push --sync assets/* /data/local/tmp/thor/assets
#adb shell "cd /data/local/tmp/thor; export LD_LIBRARY_PATH=arm64-v8a; arm64-v8a/persistencetest"
adb shell "cd /data/local/tmp/thor; export LD_LIBRARY_PATH=arm64-v8a; arm64-v8a/notchtest"
#adb shell "cd /data/local/tmp/thor; export LD_LIBRARY_PATH=arm64-v8a; arm64-v8a/iirfiltertest"
mkdir -p results
adb pull --sync "/data/local/tmp/thor/results" .