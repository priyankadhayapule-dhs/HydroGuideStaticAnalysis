#!/usr/bin/env bash

# Build and push DauUtilities and SeqUtilities
echo "Building DauUtilities and SeqUtilities"
ndk-build

TARGET_DEST=/data/local/thortools/
echo "Pushing all built packages into $TARGET_DEST"
adb root
adb shell "mkdir -p $TARGET_DEST"
adb push libs/arm64-v8a/* "$TARGET_DEST"