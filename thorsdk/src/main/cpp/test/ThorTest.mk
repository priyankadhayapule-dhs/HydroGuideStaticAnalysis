# Set all variables EXCEPT LOCAL_MODULE and LOCAL_SRC_FILES
LOCAL_CFLAGS += -g -Wall

LOCAL_STATIC_LIBRARIES += \
	libThorHardware \
	libThorCommon

LOCAL_SHARED_LIBRARIES += \
	libUltrasound \
	libSNPE \
	libacui_specklereduction \
	libopencv_imgproc \
	libopencv_core \
	libfreetype \
	libpng \
	libjpeg \
	libsymphony-cpu

ifeq ($(TARGET_ARCH_ABI), armeabi-v7a)
LOCAL_CFLAGS += -mfpu=neon
endif
LOCAL_LDLIBS := -llog -fuse-ld=gold

include $(BUILD_EXECUTABLE)

#-------------------------------------------------------------
# Custom targets to push and run executable
#-------------------------------------------------------------

# WARNING: when executing the target build commands, the LOCAL_* variables WILL be different
# To work around this problem, we use the special variables based on the target name/prereqs,
# and the $(subst) function to do some basic text manipulation
# There might be a better way to do this?

# Add custom target to push executable
# Sometimes, the executable loses permissions (when host is windows?),
# so chmod the target after push
push-$(LOCAL_MODULE): $(LOCAL_BUILT_MODULE)
	@echo Push (sync)  : $(subst push-,,$@)
	$(hide) adb shell "mkdir -p /data/local/thortests"
	$(hide) adb push --sync $< /data/local/thortests/ $(host-devnull)
	$(hide) adb shell "chmod 777 /data/local/thortests/$(subst push-,,$@)"
.PHONY: push-$(LOCAL_MODULE)

# Add custom target to run executable (depends on push all libraries as well)
run-$(LOCAL_MODULE): push-$(LOCAL_MODULE) $(PUSH_THOR_LIBS)
	@echo Run          : $(subst run-,,$@)
	$(hide) adb shell "cd /data/local/thortests && LD_LIBRARY_PATH=/data/local/thortests time /data/local/thortests/$(subst push-,,$<)"
.PHONY: run-$(LOCAL_MODULE)
