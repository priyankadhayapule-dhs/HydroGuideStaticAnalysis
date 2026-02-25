LOCAL_PATH := $(call my-dir)

#------------------------------------------------------------------------------
# bseq
#include $(CLEAR_VARS)
#
#LOCAL_MODULE := bseq
#
#LOCAL_SRC_FILES := bseq.cpp
#
#LOCAL_C_INCLUDES := \
#	$(LOCAL_PATH)/../../libThorCommon/include \
#	$(LOCAL_PATH)/../../libThorHardware/include \
#	$(LOCAL_PATH)/../../libUltrasound/include \
#	$(LOCAL_PATH)/../../libUltrasound/include/dau \
#	$(LOCAL_PATH)/../../libUltrasound/include/ImageProcessing \
#	$(LOCAL_PATH)/../../libUltrasound/include/Rendering \
#	$(LOCAL_PATH)/../../prebuilts/opencv/include \
#	$(LOCAL_PATH)/../../prebuilts/SpeckleReduction/include
#
#LOCAL_STATIC_LIBRARIES := \
#	libThorHardware \
#	libThorCommon
#
#LOCAL_SHARED_LIBRARIES += \
#	libUltrasound
#
#ifeq ($(TARGET_ARCH_ABI), armeabi-v7a)
#LOCAL_CFLAGS += -mfpu=neon
#endif
#LOCAL_LDLIBS := -llog -fuse-ld=gold
#
#include $(BUILD_EXECUTABLE)

#------------------------------------------------------------------------------
# bluts
#include $(CLEAR_VARS)
#
#LOCAL_MODULE := bluts
#
#LOCAL_SRC_FILES := bluts.cpp
#
#LOCAL_C_INCLUDES := \
#	$(LOCAL_PATH)/../../libThorCommon/include \
#	$(LOCAL_PATH)/../../libThorHardware/include \
#	$(LOCAL_PATH)/../../libUltrasound/include \
#	$(LOCAL_PATH)/../../libUltrasound/include/dau \
#	$(LOCAL_PATH)/../../libUltrasound/include/ImageProcessing \
#	$(LOCAL_PATH)/../../libUltrasound/include/Rendering \
#	$(LOCAL_PATH)/../../prebuilts/opencv/include \
#	$(LOCAL_PATH)/../../prebuilts/SpeckleReduction/include
#
#LOCAL_STATIC_LIBRARIES := \
#	libThorHardware \
#	libThorCommon
#
#LOCAL_SHARED_LIBRARIES += \
#	libUltrasound
#
#ifeq ($(TARGET_ARCH_ABI), armeabi-v7a)
#LOCAL_CFLAGS += -mfpu=neon
#endif
#LOCAL_LDLIBS := -llog -fuse-ld=gold
#
#include $(BUILD_EXECUTABLE)


#------------------------------------------------------------------------------
# runic
#include $(CLEAR_VARS)
#
#LOCAL_MODULE := runic
#
#LOCAL_SRC_FILES := runic.cpp
#
#LOCAL_C_INCLUDES := \
#	$(LOCAL_PATH)/../../libThorCommon/include \
#	$(LOCAL_PATH)/../../libThorHardware/include \
#	$(LOCAL_PATH)/../../libUltrasound/include \
#	$(LOCAL_PATH)/../../libUltrasound/include/dau \
#	$(LOCAL_PATH)/../../libUltrasound/include/ImageProcessing \
#	$(LOCAL_PATH)/../../libUltrasound/include/Rendering \
#	$(LOCAL_PATH)/../../prebuilts/opencv/include \
#	$(LOCAL_PATH)/../../prebuilts/SpeckleReduction/include
#
#LOCAL_STATIC_LIBRARIES := \
#	libThorHardware \
#	libThorCommon
#
#LOCAL_SHARED_LIBRARIES += \
#	libUltrasound
#
#ifeq ($(TARGET_ARCH_ABI), armeabi-v7a)
#LOCAL_CFLAGS += -mfpu=neon
#endif
#LOCAL_LDLIBS := -llog -fuse-ld=gold
#
#include $(BUILD_EXECUTABLE)


#------------------------------------------------------------------------------
# fifoh
include $(CLEAR_VARS)

LOCAL_MODULE := fifoh

LOCAL_SRC_FILES := fifoh.cpp \
	$(LOCAL_PATH)/../../libThorHardware/PciDauMemory.cpp 

LOCAL_C_INCLUDES := \
	$(LOCAL_PATH)/../../libThorCommon/include \
	$(LOCAL_PATH)/../../libThorHardware/include \
	$(LOCAL_PATH)/../../libUltrasound/include \
	$(LOCAL_PATH)/../../libUltrasound/include/dau \
	$(LOCAL_PATH)/../../libUltrasound/include/ImageProcessing \
	$(LOCAL_PATH)/../../libUltrasound/include/Rendering \
	$(LOCAL_PATH)/../../prebuilts/opencv/include \
	$(LOCAL_PATH)/../../prebuilts/SpeckleReduction/include

LOCAL_STATIC_LIBRARIES := \
	libThorHardware \
	libThorCommon

ifeq ($(TARGET_ARCH_ABI), armeabi-v7a)
LOCAL_CFLAGS += -mfpu=neon
endif
LOCAL_LDFLAGS += '-Wl,-rpath,/data/local/thortools,--enable-new-dtags'
LOCAL_LDLIBS := -llog -fuse-ld=gold

include $(BUILD_EXECUTABLE)


#------------------------------------------------------------------------------
# applygain
#include $(CLEAR_VARS)
#
#LOCAL_MODULE := applygain
#
#LOCAL_SRC_FILES := applygain.cpp
#
#LOCAL_C_INCLUDES := \
#	$(LOCAL_PATH)/../../libThorCommon/include \
#	$(LOCAL_PATH)/../../libThorHardware/include \
#	$(LOCAL_PATH)/../../libUltrasound/include \
#	$(LOCAL_PATH)/../../libUltrasound/include/dau \
#	$(LOCAL_PATH)/../../libUltrasound/include/ImageProcessing \
#	$(LOCAL_PATH)/../../libUltrasound/include/Rendering \
#	$(LOCAL_PATH)/../../prebuilts/opencv/include \
#	$(LOCAL_PATH)/../../prebuilts/SpeckleReduction/include
#
#LOCAL_STATIC_LIBRARIES := \
#	libThorHardware \
#	libThorCommon
#
#LOCAL_SHARED_LIBRARIES += \
#	libUltrasound
#
#ifeq ($(TARGET_ARCH_ABI), armeabi-v7a)
#LOCAL_CFLAGS += -mfpu=neon
#endif
#LOCAL_LDLIBS := -llog -fuse-ld=gold
#
#include $(BUILD_EXECUTABLE)

