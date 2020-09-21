LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)
ifeq ($(HOST_OS),linux)
LOCAL_SANITIZE := integer
endif
LOCAL_MODULE := fec
LOCAL_SRC_FILES := main.cpp image.cpp
LOCAL_MODULE_TAGS := optional
LOCAL_STATIC_LIBRARIES := \
    libsparse \
    libz \
    libcrypto_utils \
    libcrypto \
    libfec \
    libfec_rs \
    libext4_utils \
    libsquashfs_utils
LOCAL_SHARED_LIBRARIES := libbase
LOCAL_CFLAGS += -Wall -Werror -O3
LOCAL_C_INCLUDES += external/fec
include $(BUILD_HOST_EXECUTABLE)
