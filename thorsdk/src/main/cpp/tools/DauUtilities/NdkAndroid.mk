LOCAL_PATH := $(call my-dir)

#------------------------------------------------------------------------------
# daupower
include $(CLEAR_VARS)

LOCAL_MODULE := daupower

LOCAL_SRC_FILES := daupower.cpp

LOCAL_C_INCLUDES := \
	$(LOCAL_PATH)/../../libThorCommon/include \
	$(LOCAL_PATH)/../../libThorHardware/include

LOCAL_STATIC_LIBRARIES := \
	libThorHardware \
	libThorCommon

LOCAL_LDLIBS := -llog
LOCAL_LDFLAGS += '-Wl,-rpath,/data/local/thortools,--enable-new-dtags'

include $(BUILD_EXECUTABLE)


#------------------------------------------------------------------------------
# dauread
include $(CLEAR_VARS)

LOCAL_MODULE := dauread

LOCAL_SRC_FILES := dauread.cpp

LOCAL_C_INCLUDES := \
	$(LOCAL_PATH)/../../libThorCommon/include \
	$(LOCAL_PATH)/../../libThorHardware/include

LOCAL_STATIC_LIBRARIES := \
	libThorHardware \
	libThorCommon

LOCAL_LDLIBS := -llog
LOCAL_LDFLAGS += '-Wl,-rpath,/data/local/thortools,--enable-new-dtags'

include $(BUILD_EXECUTABLE)


#------------------------------------------------------------------------------
# dauwrite
include $(CLEAR_VARS)

LOCAL_MODULE := dauwrite

LOCAL_SRC_FILES := dauwrite.cpp

LOCAL_C_INCLUDES := \
	$(LOCAL_PATH)/../../libThorCommon/include \
	$(LOCAL_PATH)/../../libThorHardware/include

LOCAL_STATIC_LIBRARIES := \
	libThorHardware \
	libThorCommon

LOCAL_LDLIBS := -llog
LOCAL_LDFLAGS += '-Wl,-rpath,/data/local/thortools,--enable-new-dtags'

include $(BUILD_EXECUTABLE)


#------------------------------------------------------------------------------
# dmainfo
include $(CLEAR_VARS)

LOCAL_MODULE := dmainfo

LOCAL_SRC_FILES := dmainfo.cpp

LOCAL_C_INCLUDES := \
	$(LOCAL_PATH)/../../libThorCommon/include \
	$(LOCAL_PATH)/../../libThorHardware/include

LOCAL_STATIC_LIBRARIES := \
	libThorHardware \
	libThorCommon

LOCAL_LDLIBS := -llog
LOCAL_LDFLAGS += '-Wl,-rpath,/data/local/thortools,--enable-new-dtags'

include $(BUILD_EXECUTABLE)


#------------------------------------------------------------------------------
# dmaread
include $(CLEAR_VARS)

LOCAL_MODULE := dmaread

LOCAL_SRC_FILES := dmaread.cpp

LOCAL_C_INCLUDES := \
	$(LOCAL_PATH)/../../libThorCommon/include \
	$(LOCAL_PATH)/../../libThorHardware/include

LOCAL_STATIC_LIBRARIES := \
	libThorHardware \
	libThorCommon

LOCAL_LDLIBS := -llog
LOCAL_LDFLAGS += '-Wl,-rpath,/data/local/thortools,--enable-new-dtags'

include $(BUILD_EXECUTABLE)


#------------------------------------------------------------------------------
# dmawrite
include $(CLEAR_VARS)

LOCAL_MODULE := dmawrite

LOCAL_SRC_FILES := dmawrite.cpp

LOCAL_C_INCLUDES := \
	$(LOCAL_PATH)/../../libThorCommon/include \
	$(LOCAL_PATH)/../../libThorHardware/include

LOCAL_STATIC_LIBRARIES := \
	libThorHardware \
	libThorCommon

LOCAL_LDLIBS := -llog
LOCAL_LDFLAGS += '-Wl,-rpath,/data/local/thortools,--enable-new-dtags'

include $(BUILD_EXECUTABLE)

#------------------------------------------------------------------------------
# fwupgrade
include $(CLEAR_VARS)

LOCAL_MODULE := fwupgrade

LOCAL_SRC_FILES := fwupgrade.cpp

LOCAL_C_INCLUDES := \
	$(LOCAL_PATH)/../../libThorCommon/include \
	$(LOCAL_PATH)/../../libThorHardware/include

LOCAL_STATIC_LIBRARIES := \
	libThorHardware \
	libThorCommon

LOCAL_LDLIBS := -llog
LOCAL_LDFLAGS += '-Wl,-rpath,/data/local/thortools,--enable-new-dtags'

include $(BUILD_EXECUTABLE)

#------------------------------------------------------------------------------
# dautemp
include $(CLEAR_VARS)

LOCAL_MODULE := dautemp

LOCAL_SRC_FILES := dautemp.cpp

LOCAL_C_INCLUDES := \
	$(LOCAL_PATH)/../../libThorCommon/include \
	$(LOCAL_PATH)/../../libThorHardware/include

LOCAL_STATIC_LIBRARIES := \
	libThorHardware \
	libThorCommon

LOCAL_LDLIBS := -llog
LOCAL_LDFLAGS += '-Wl,-rpath,/data/local/thortools,--enable-new-dtags'

include $(BUILD_EXECUTABLE)

#------------------------------------------------------------------------------
# factdatarw
include $(CLEAR_VARS)

LOCAL_MODULE := factdatarw

LOCAL_SRC_FILES := factdatarw.cpp

LOCAL_C_INCLUDES := \
	$(LOCAL_PATH)/../../libThorCommon/include \
	$(LOCAL_PATH)/../../libThorHardware/include

LOCAL_STATIC_LIBRARIES := \
	libThorHardware \
	libThorCommon

LOCAL_LDLIBS := -llog
LOCAL_LDFLAGS += '-Wl,-rpath,/data/local/thortools,--enable-new-dtags'

include $(BUILD_EXECUTABLE)

#------------------------------------------------------------------------------
# dauusbread
include $(CLEAR_VARS)

LOCAL_MODULE := dauusbread

LOCAL_SRC_FILES := dauusbread.cpp

LOCAL_C_INCLUDES := \
	$(LOCAL_PATH)/../../libThorCommon/include \
	$(LOCAL_PATH)/../../libThorHardware/include

LOCAL_STATIC_LIBRARIES := \
	libThorHardware \
	libThorCommon

LOCAL_LDLIBS := -llog
LOCAL_LDFLAGS += '-Wl,-rpath,/data/local/thortools,--enable-new-dtags'

include $(BUILD_EXECUTABLE)

#------------------------------------------------------------------------------
# dauusbwrite
include $(CLEAR_VARS)

LOCAL_MODULE := dauusbwrite

LOCAL_SRC_FILES := dauusbwrite.cpp

LOCAL_C_INCLUDES := \
	$(LOCAL_PATH)/../../libThorCommon/include \
	$(LOCAL_PATH)/../../libThorHardware/include

LOCAL_STATIC_LIBRARIES := \
	libThorHardware \
	libThorCommon

LOCAL_LDLIBS := -llog
LOCAL_LDFLAGS += '-Wl,-rpath,/data/local/thortools,--enable-new-dtags'

include $(BUILD_EXECUTABLE)

#------------------------------------------------------------------------------
# dauusbpower
include $(CLEAR_VARS)

LOCAL_MODULE := dauusbpower

LOCAL_SRC_FILES := dauusbpower.cpp

LOCAL_C_INCLUDES := \
	$(LOCAL_PATH)/../../libThorCommon/include \
	$(LOCAL_PATH)/../../libThorHardware/include

LOCAL_STATIC_LIBRARIES := \
	libThorHardware \
	libThorCommon

LOCAL_LDLIBS := -llog
LOCAL_LDFLAGS += '-Wl,-rpath,/data/local/thortools,--enable-new-dtags'

include $(BUILD_EXECUTABLE)

#------------------------------------------------------------------------------
# pcibist
include $(CLEAR_VARS)

LOCAL_MODULE := pcibist

LOCAL_SRC_FILES := pcibist.cpp

LOCAL_C_INCLUDES := \
	$(LOCAL_PATH)/../../libThorCommon/include \
	$(LOCAL_PATH)/../../libThorHardware/include

LOCAL_STATIC_LIBRARIES := \
	libThorHardware \
	libThorCommon

LOCAL_LDLIBS := -llog
LOCAL_LDFLAGS += '-Wl,-rpath,/data/local/thortools,--enable-new-dtags'

include $(BUILD_EXECUTABLE)

#------------------------------------------------------------------------------
# usbbist
include $(CLEAR_VARS)

LOCAL_MODULE := usbbist

LOCAL_SRC_FILES := usbbist.cpp

LOCAL_C_INCLUDES := \
	$(LOCAL_PATH)/../../libThorCommon/include \
	$(LOCAL_PATH)/../../libThorHardware/include

LOCAL_STATIC_LIBRARIES := \
	libThorHardware \
	libThorCommon

LOCAL_LDLIBS := -llog
LOCAL_LDFLAGS += '-Wl,-rpath,/data/local/thortools,--enable-new-dtags'

include $(BUILD_EXECUTABLE)
#------------------------------------------------------------------------------
# setfcs
include $(CLEAR_VARS)

LOCAL_MODULE := setfcs

LOCAL_SRC_FILES := setfcs.cpp

LOCAL_C_INCLUDES := \
	$(LOCAL_PATH)/../../libThorCommon/include \
	$(LOCAL_PATH)/../../libThorHardware/include

LOCAL_STATIC_LIBRARIES := \
	libThorHardware \
	libThorCommon

LOCAL_LDLIBS := -llog
LOCAL_LDFLAGS += '-Wl,-rpath,/data/local/thortools,--enable-new-dtags'

include $(BUILD_EXECUTABLE)

#------------------------------------------------------------------------------
# Thor Puck Test
include $(CLEAR_VARS)

LOCAL_MODULE := thorpucktest

LOCAL_SRC_FILES := ThorPuckTest.c

LOCAL_C_INCLUDES := \
	$(LOCAL_PATH)/../../libThorCommon/include \
	$(LOCAL_PATH)/../../libThorHardware/include

LOCAL_STATIC_LIBRARIES := \
	libThorHardware \
	libThorCommon

LOCAL_LDLIBS := -llog
LOCAL_LDFLAGS += '-Wl,-rpath,/data/local/thortools,--enable-new-dtags'

include $(BUILD_EXECUTABLE)

#------------------------------------------------------------------------------
# thorarraytest
include $(CLEAR_VARS)

LOCAL_MODULE := thorarraytest

LOCAL_SRC_FILES := ThorArrayTest.cpp \
                   $(LOCAL_PATH)/../../libUltrasound/UpsReader.cpp \
                   $(LOCAL_PATH)/../../libUltrasound/DBWrappers.cpp \
                   $(LOCAL_PATH)/../../libUltrasound/dau/DauArrayTest.cpp


LOCAL_C_INCLUDES := \
    $(LOCAL_PATH)/../../libUltrasound/include \
    $(LOCAL_PATH)/../../libUltrasound/include/dau \
	$(LOCAL_PATH)/../../libThorCommon/include \
	$(LOCAL_PATH)/../../libThorHardware/include \
	$(LOCAL_PATH)/../../prebuilts/SpeckleReduction/include \
	$(LOCAL_PATH)/../../prebuilts/opencv/include \
    $(LOCAL_PATH)/../../libUltrasound/include/Rendering \
    $(LOCAL_PATH)/../../libUltrasound/include/ImageProcessing

LOCAL_STATIC_LIBRARIES := \
	libThorHardware \
	libThorCommon

LOCAL_LDLIBS := -llog
LOCAL_LDFLAGS += '-Wl,-rpath,/data/local/thortools,--enable-new-dtags'

include $(BUILD_EXECUTABLE)


