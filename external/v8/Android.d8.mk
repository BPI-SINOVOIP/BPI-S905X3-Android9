LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

include $(LOCAL_PATH)/Android.v8common.mk

LOCAL_MODULE := d8
LOCAL_MODULE_CLASS := EXECUTABLES

LOCAL_SRC_FILES := \
    src/d8.cc \
    src/d8-posix.cc

LOCAL_JS_D8_FILES := \
	$(LOCAL_PATH)/src/d8.js \
	$(LOCAL_PATH)/src/js/macros.py

generated_sources := $(call local-generated-sources-dir)
# Copy js2c.py to generated sources directory and invoke there to avoid
# generating jsmin.pyc in the source directory
JS2C_PY := $(generated_sources)/js2c.py $(generated_sources)/jsmin.py
$(JS2C_PY): $(generated_sources)/%.py : $(LOCAL_PATH)/tools/%.py | $(ACP)
	@echo "Copying $@"
	$(copy-file-to-target)

# Generate d8-js.cc
D8_GEN := $(generated_sources)/d8-js.cc
$(D8_GEN): SCRIPT := $(generated_sources)/js2c.py
$(D8_GEN): $(LOCAL_JS_D8_FILES) $(JS2C_PY)
	@echo "Generating d8-js.cc"
	@mkdir -p $(dir $@)
	python $(SCRIPT) $@ D8 $(LOCAL_JS_D8_FILES)
LOCAL_GENERATED_SOURCES += $(D8_GEN)

LOCAL_STATIC_LIBRARIES := libv8
LOCAL_SHARED_LIBRARIES := liblog libicuuc libicui18n

LOCAL_MODULE_TAGS := optional

LOCAL_CFLAGS += \
	-O0

LOCAL_C_INCLUDES := \
	$(LOCAL_PATH)/include

# Bug: http://b/31101212  WAR LLVM bug until next Clang update
LOCAL_CFLAGS_mips += -O2

LOCAL_MODULE_TARGET_ARCH_WARN := $(V8_SUPPORTED_ARCH)

include $(BUILD_EXECUTABLE)


