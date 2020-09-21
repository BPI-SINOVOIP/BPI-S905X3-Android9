### GENERATED, do not edit
### for changes, see genmakefiles.py
LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)
include $(LOCAL_PATH)/Android.v8common.mk
LOCAL_MODULE := v8mkpeephole
LOCAL_SRC_FILES := \
	src/interpreter/bytecode-operands.cc \
	src/interpreter/bytecodes.cc \
	src/interpreter/mkpeephole.cc
LOCAL_STATIC_LIBRARIES += libv8base liblog
LOCAL_LDLIBS_linux += -lrt
include $(BUILD_HOST_EXECUTABLE)
include $(CLEAR_VARS)
include $(LOCAL_PATH)/Android.v8common.mk
LOCAL_MODULE := v8peephole
LOCAL_MODULE_CLASS := STATIC_LIBRARIES
generated_sources := $(call local-generated-sources-dir)
PEEPHOLE_TOOL := $(HOST_OUT_EXECUTABLES)/v8mkpeephole
PEEPHOLE_FILE := $(generated_sources)/bytecode-peephole-table.cc
$(PEEPHOLE_FILE): PRIVATE_CUSTOM_TOOL = $(PEEPHOLE_TOOL) $(PEEPHOLE_FILE)
$(PEEPHOLE_FILE): $(PEEPHOLE_TOOL)
	$(transform-generated-source)
LOCAL_GENERATED_SOURCES += $(PEEPHOLE_FILE)
include $(BUILD_STATIC_LIBRARY)
