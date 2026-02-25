ROOT_PATH := $(call my-dir)/../..

# Build libraries
include $(ROOT_PATH)/libThorCommon/NdkAndroid.mk
include $(ROOT_PATH)/libThorHardware/NdkAndroid.mk
include $(ROOT_PATH)/prebuilts/opencv/NdkAndroid.mk
include $(ROOT_PATH)/prebuilts/SpeckleReduction/NdkAndroid.mk
include $(ROOT_PATH)/prebuilts/freetype/NdkAndroid.mk
include $(ROOT_PATH)/libUltrasound/NdkAndroid.mk


include $(ROOT_PATH)/prebuilts/png/NdkAndroid.mk
include $(ROOT_PATH)/prebuilts/jpeg/NdkAndroid.mk
include $(ROOT_PATH)/prebuilts/tiff/NdkAndroid.mk
include $(ROOT_PATH)/prebuilts/lzma/NdkAndroid.mk
include $(ROOT_PATH)/prebuilts/xml2/NdkAndroid.mk
include $(ROOT_PATH)/prebuilts/iconv/NdkAndroid.mk
include $(ROOT_PATH)/prebuilts/openssl/NdkAndroid.mk
include $(ROOT_PATH)/prebuilts/dcmtk/NdkAndroid.mk
include $(ROOT_PATH)/libDcmtkApp/NdkAndroid.mk
include $(ROOT_PATH)/prebuilts/capnproto/NdkAndroid.mk

# Build command-line utilities
ifneq ($(strip $(THOR_ENABLE_TOOLS)),)
include $(ROOT_PATH)/tools/DauUtilities/NdkAndroid.mk
include $(ROOT_PATH)/tools/SeqUtilities/NdkAndroid.mk
endif

# Tests
ifneq ($(strip $(THOR_ENABLE_TESTS)),)
include $(ROOT_PATH)/test/NdkAndroid.mk
endif
