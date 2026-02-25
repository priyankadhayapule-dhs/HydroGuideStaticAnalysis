LOCAL_PATH := $(call my-dir)

include $(LOCAL_PATH)/Sources.mk

#------------------------------------------------------------------------------
# libThorHardware.a
include $(CLEAR_VARS)

LOCAL_MODULE := ThorHardware

LOCAL_SRC_FILES := $(sources)

LOCAL_EXPORT_C_INCLUDES := \
	$(LOCAL_PATH)/include/linux \
	$(LOCAL_PATH)/include/ion \
	$(LOCAL_PATH)/include/libusb \
	$(LOCAL_PATH)/include/libusb/os \
	$(LOCAL_PATH)/include/cypress \
	$(LOCAL_PATH)/include

LOCAL_C_INCLUDES := \
	$(LOCAL_PATH)/../libThorCommon/include \
	$(LOCAL_EXPORT_C_INCLUDES)


include $(BUILD_STATIC_LIBRARY)


