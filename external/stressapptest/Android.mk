LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_ADDITIONAL_DEPENDENCIES := $(LOCAL_PATH)/Android.mk

LOCAL_SRC_FILES := \
	src/main.cc \
	src/adler32memcpy.cc \
	src/disk_blocks.cc \
	src/error_diag.cc \
	src/finelock_queue.cc \
	src/logger.cc \
	src/os.cc \
	src/os_factory.cc \
	src/pattern.cc \
	src/queue.cc \
	src/sat.cc \
	src/sat_factory.cc \
	src/worker.cc

# just build a 32b version, even on 64b hosts
LOCAL_MULTILIB := 32
LOCAL_MODULE := stressapptest
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_UNSUPPORTED_TARGET_ARCH := mips x86

LOCAL_CFLAGS := -DHAVE_CONFIG_H -DANDROID -DNDEBUG -UDEBUG -DCHECKOPTS
LOCAL_CFLAGS += -Wall -Werror -Wno-unused-parameter -Wno-\#warnings

LOCAL_CPP_EXTENSION := .cc

include $(BUILD_EXECUTABLE)
