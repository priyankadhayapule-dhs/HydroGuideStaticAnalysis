LOCAL_PATH := $(call my-dir)

include $(LOCAL_PATH)/Sources.mk

#------------------------------------------------------------------------------
# libThorCommon.a
include $(CLEAR_VARS)

LOCAL_MODULE := ThorCommon

LOCAL_SRC_FILES := $(sources)

LOCAL_EXPORT_C_INCLUDES := \
	$(LOCAL_PATH)/include \
	$(LOCAL_PATH)/include/jsoncpp

LOCAL_C_INCLUDES := \
	$(LOCAL_EXPORT_C_INCLUDES)

include $(BUILD_STATIC_LIBRARY)

