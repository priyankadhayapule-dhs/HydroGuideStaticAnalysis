LOCAL_PATH := $(call my-dir)

#------------------------------------------------------------------------------
# General setup for tests

# Host specific redirect to /dev/null (used in this file and ThorTest.mk)
ifeq ($(HOST_OS),windows)
host-devnull = > NUL
else
host-devnull = > /dev/null
endif

# Push Thor libraries (shared libs needed by tests)
THOR_LIBS := \
    Ultrasound \
    acui_specklereduction \
    opencv_imgproc \
    opencv_core \
    freetype \
    png \
    jpeg \
    symphony-cpu

PUSH_THOR_LIBS := $(addprefix push-,$(THOR_LIBS))
# Sometimes, the executable loses permissions (when host is windows?),
# so chmod the target after push
$(PUSH_THOR_LIBS): $(SHARED_LIBRARIES)
	@echo Push (sync)  : $(subst push-,,$@)
	$(hide) adb shell "mkdir -p /data/local/thortests"
	$(hide) adb push --sync $(call module-get-built,$(subst push-,,$@)) /data/local/thortests/ $(host-devnull)
	$(hide) adb shell "chmod 777 /data/local/thortests/$(notdir $(call module-get-built,$(subst push-,,$@)))"
.PHONY: $(PUSH_THOR_LIBS)
#------------------------------------------------------------------------------

# Add new tests below here, run using target `run-$(LOCAL_MODULE)`
# e.g. ndk-build run-thor-example-test

#------------------------------------------------------------------------------
# Example test
include $(CLEAR_VARS)
LOCAL_MODULE := thor-example-test
LOCAL_SRC_FILES := ThorExampleTest.cpp
include $(LOCAL_PATH)/ThorTest.mk
#------------------------------------------------------------------------------

#------------------------------------------------------------------------------
# Autolabel verification test
include $(CLEAR_VARS)
LOCAL_MODULE := autolabel-verify
LOCAL_SRC_FILES := AutolabelVerify.cpp
include $(LOCAL_PATH)/ThorTest.mk
#------------------------------------------------------------------------------

#------------------------------------------------------------------------------
# Guidance & Grading verification test
include $(CLEAR_VARS)
LOCAL_MODULE := guidance-verify
LOCAL_SRC_FILES := GuidanceVerify.cpp
include $(LOCAL_PATH)/ThorTest.mk
#------------------------------------------------------------------------------

#------------------------------------------------------------------------------
# ScanConverterCl Unit test
include $(CLEAR_VARS)
LOCAL_MODULE := scanconvertercl-unit-test
LOCAL_SRC_FILES := ScanConverterClUnitTest.cpp catch2/catch.cpp
include $(LOCAL_PATH)/ThorTest.mk
#------------------------------------------------------------------------------

#------------------------------------------------------------------------------
# Thor capnp serialization test
include $(CLEAR_VARS)
LOCAL_MODULE := thor-capnp-test
LOCAL_SRC_FILES := ThorCapnpTest.cpp
include $(LOCAL_PATH)/ThorTest.mk
#------------------------------------------------------------------------------
