LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE := memcpy-perf
LOCAL_CFLAGS += -g -Wall -Werror -Wno-missing-field-initializers -Wno-sign-compare -O3
LOCAL_SRC_FILES := memcpy-perf.cpp test-funcs.cpp
LOCAL_FORCE_STATIC_EXECUTABLE := true
LOCAL_CXX_STL := libc++_static
LOCAL_STATIC_LIBRARIES := libc
include $(BUILD_EXECUTABLE)
