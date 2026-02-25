APP_PLATFORM := android-27
APP_ABI := arm64-v8a
APP_STL := c++_shared
APP_CPPFLAGS += -std=c++17 -fexceptions -frtti
APP_BUILD_SCRIPT := $(call my-dir)/NdkAndroid.mk

NDK_TOOLCHAIN_VERSION := clang

