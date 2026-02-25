LOCAL_PATH := $(call my-dir)

include $(LOCAL_PATH)/Sources.mk

#------------------------------------------------------------------------------
# libUltrasound.so
include $(CLEAR_VARS)

LOCAL_MODULE := Ultrasound

LOCAL_SRC_FILES := \
	$(sources)

LOCAL_EXPORT_C_INCLUDES := \
	$(LOCAL_PATH)/include \
	$(LOCAL_PATH)/include/dau \
	$(LOCAL_PATH)/include/ImageProcessing \
	$(LOCAL_PATH)/include/Rendering \
	$(LOCAL_PATH)/include/Rendering/imgui \
	$(LOCAL_PATH)/include/api \
	$(LOCAL_PATH)/include/AI \
	$(LOCAL_PATH)/include/AI/AutoPreset \
	$(LOCAL_PATH)/include/AI/Bladder \
	$(LOCAL_PATH)/Rendering/opengl \
	$(LOCAL_PATH)/../prebuilts/opencl20/include

LOCAL_C_INCLUDES := \
	$(LOCAL_EXPORT_C_INCLUDES)

LOCAL_STATIC_LIBRARIES := \
	libThorHardware \
	libThorCommon \
	libcapnp \
	libkj

LOCAL_SHARED_LIBRARIES := \
	libacui_specklereduction \
	libopencv_imgproc \
	libopencv_core \
	libopencv_imgcodecs \
	libfreetype \
	libturbojpeg \


LOCAL_LDLIBS := \
	-llog \
	-landroid \
	-lEGL \
	-lGLESv1_CM \
	-lGLESv2 \
	-lGLESv3 \
	-lz \
	-laaudio \
	-ljnigraphics \
	-lmediandk \



include $(BUILD_SHARED_LIBRARY)

#------------------------------------------------------------------------------
# libEmuUltrasound.so
#include $(CLEAR_VARS)
#
#LOCAL_MODULE := libEmuUltrasound
#
#LOCAL_SRC_FILES := $(emu_sources)
#
#LOCAL_C_INCLUDES := \
#	$(LOCAL_PATH)/../libThorCommon/include \
#	$(LOCAL_PATH)/../libThorHardware/include \
#	$(LOCAL_PATH)/../prebuilts/opencv/include \
#	$(LOCAL_PATH)/../prebuilts/opencl20/include \
#	$(LOCAL_PATH)/../prebuilts/SpeckleReduction/include \
#	$(LOCAL_PATH)/../prebuilts/snpe/include \
#	$(LOCAL_PATH)/include \
#	$(LOCAL_PATH)/include/dau \
#	$(LOCAL_PATH)/include/ImageProcessing \
#	$(LOCAL_PATH)/include/Rendering \
#	$(LOCAL_PATH)/Rendering/opengl \
#	$(LOCAL_PATH)/include/emulated
#
#LOCAL_STATIC_LIBRARIES := \
#	libThorHardware \
#	libThorCommon
#
#LOCAL_SHARED_LIBRARIES := \
#	libSNPE
#
#ifeq ($(TARGET_ARCH_ABI), armeabi-v7a)
#LOCAL_CFLAGS += -mfpu=neon
#endif
#LOCAL_LDLIBS := -llog -landroid -lEGL -lGLESv1_CM -lGLESv2 -lz -lOpenCL
#
#include $(BUILD_SHARED_LIBRARY)

