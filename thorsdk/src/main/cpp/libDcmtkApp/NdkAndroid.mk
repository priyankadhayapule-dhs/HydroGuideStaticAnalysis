LOCAL_PATH := $(call my-dir)

include $(LOCAL_PATH)/Sources.mk

#------------------------------------------------------------------------------
# libDcmtkapp.so
include $(CLEAR_VARS)

LOCAL_MODULE := DcmtkApp

LOCAL_SRC_FILES := \
	$(sources)

LOCAL_EXPORT_C_INCLUDES := \
	$(LOCAL_PATH)/include \
    $(LOCAL_PATH)/../prebuilts/png/include \
    $(LOCAL_PATH)/../prebuilts/jpeg/include \
    $(LOCAL_PATH)/../prebuilts/tiff/include \
    $(LOCAL_PATH)/../prebuilts/lzma/include \
	$(LOCAL_PATH)/../prebuilts/xml2/include \
	$(LOCAL_PATH)/../prebuilts/openssl/include \
    $(LOCAL_PATH)/../prebuilts/iconv/include \

LOCAL_C_INCLUDES := \
    $(LOCAL_PATH)/../libThorCommon/include \
	$(LOCAL_EXPORT_C_INCLUDES) \

LOCAL_STATIC_LIBRARIES := \
	libThorCommon

LOCAL_SHARED_LIBRARIES := \
    libpng \
    libjpeg \
    libtiff \
    liblzma \
    libxml2 \
    libssl \
	libcrypto \
	libiconv \
	libcharset \
	libdcmtk

LOCAL_LDLIBS := \
	-llog \
	-landroid

include $(BUILD_SHARED_LIBRARY)



